//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out VS_OUT
{
    vec2 TexCoords;
    mat4 ViewMatrix;
} vso;

uniform mat4 view;
uniform mat4 model;

void main()
{
    vso.TexCoords  = aTexCoords;
    vso.ViewMatrix = view;

    gl_Position = vec4(aPos, 1.0);
}


//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450

layout(location = 0) out vec4 PixelColor;

in VS_OUT
{
    vec2 TexCoords;
    mat4 ViewMatrix;
} fsi;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[4];
};

//----- カスケード -----
uniform float cascadePlaneDistances[4];
uniform int   cascadeCount;
uniform float farPlane;
uniform float shadowDepthBias;

//----- シャドウマップ -----
uniform sampler2DArrayShadow cascadeshadowMap;

//----- 環境マップ -----
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;

//----- GBuffer -----
uniform sampler2D  albedoMap;
uniform sampler2D  positionMap;
uniform sampler2D  normalMap;
uniform sampler2D  emissionMap;
uniform isampler2D idMap;

//----- ライト -----
uniform vec3  lightDir;
uniform vec3  lightColor;
uniform float iblIntencity;

//----- カメラ -----
uniform vec3 camPos;

//----- コンフィグ -----
uniform bool enableSoftShadow;
uniform bool showCascade;

//----- 定数 -----
const float PI      = 3.14159265359;
const float EPSILON = 0.0001;


// stepであそぼう！～stepはシェーダーで一番楽しい関数。たぶん。～
// https://sayachang-bot.hateblo.jp/entry/2019/12/05/211518

// 「Shaderでif文を使ったら遅い」は正しくない
// https://qiita.com/up-hash/items/e932ccae110d687e225f

// スレッド固有変数の if分岐 は遅い。また、[Flatten][branch] 属性によってもことなる
// しかし、定数バッファはどのスレッドでも一定の値であり、実行は阻害されない
// ? 演算子は [Flatten] であり、常に両辺が評価されてしまう
// GLSL には明示的に指定する機能がない...（if が branch か flatten に評価されるかもコンパイラ次第）


float ShadowDepthBias(int currentLayer, vec3 normal)
{
    float bias = max(shadowDepthBias * (1.0 - dot(normal, normalize(lightDir))), shadowDepthBias);

    //bias *= (currentLayer == cascadeCount)
    //    ? 1.0 / (farPlane * 0.5)
    //    : 1.0 / (cascadePlaneDistances[currentLayer] * 0.5);

    float mask = 1.0 - abs(sign(currentLayer - cascadeCount));
    bias *= mix(1.0 / (cascadePlaneDistances[currentLayer] * 0.5), 1.0 / (farPlane * 0.5), mask);

    return bias;
}

float ShadowSampleOffset(vec3 shadowMapCoords, vec2 offsetPos, vec2 texelSize, int layer, float currentDepth, float bias)
{
    vec2 offset = vec2(offsetPos.x, offsetPos.y) * texelSize;
    float depth = texture(cascadeshadowMap, vec4(shadowMapCoords.xy + offset, layer, currentDepth - bias));
    return depth;
}

float ShadowSampling(float bias, int layer, vec3 shadowMapCoords, float lightSpaceDepth)
{
    // ピクセルあたりのテクセルサイズ
    vec2  texelSize   = 1.0 / vec2(textureSize(cascadeshadowMap, 0));
    float shadowColor = 0.0;

    // 5 x 5 PCF
    if (enableSoftShadow)
    {
        for(float x = -2.0; x <= 2.0; x += 1.0)
        {
            for(float y = -2.0; y <= 2.0; y += 1.0)
            {
                shadowColor += ShadowSampleOffset(shadowMapCoords, vec2(x, y), texelSize, layer, lightSpaceDepth, bias);
            }
        }

        shadowColor /= 25;
    }
    else
    {
        float depth = texture(cascadeshadowMap, vec4(shadowMapCoords.xy, layer, shadowMapCoords.z));
        shadowColor = step(lightSpaceDepth - bias, depth);
    }

    return shadowColor;
}

float DirectionalLightShadow(vec3 fragPosWorldSpace, vec3 normal, out int currentLayer)
{
    // 距離からレイヤー選択
    vec4 fragPosViewSpace = fsi.ViewMatrix * vec4(fragPosWorldSpace, 1.0);
    float fragPosDistance = abs(fragPosViewSpace.z);

    currentLayer = cascadeCount - 1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (fragPosDistance < cascadePlaneDistances[i])
        {
            currentLayer = i;
            break;
        }
    }

    // ライト空間変換
    vec4 fragPosLightSpace = lightSpaceMatrices[currentLayer] * vec4(fragPosWorldSpace, 1.0);

    // プロジェクション変換
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;

    // ライト空間での深度値 (視錐台外であれば影を落とさない)
    float lightSpaceDepth = projCoords.z;
    if (lightSpaceDepth > 1.0)
    {
        return 1.0;
    }


    // 深度値バイアス
    float bias = ShadowDepthBias(currentLayer, normal);

    // シャドウサンプリング
    return ShadowSampling(bias, currentLayer, projCoords, lightSpaceDepth);
}


//======================================================
// ラフネスから、反射の中間ベクトルとの整列具合を求める
//======================================================
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

//======================================================
// ラフネスから、反射の遮断率を求める（表面の陰影）
//======================================================
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

//======================================================
// 角度による表面反射率を求める
//======================================================
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


vec3 CascadeColor(int layer)
{
    switch(layer)
    {
        case  0: return vec3(1.00, 0.25, 0.25);
        case  1: return vec3(0.25, 1.00, 0.25);
        case  2: return vec3(0.25, 0.25, 1.00);
        case  3: return vec3(0.25, 0.25, 0.25);
    }
}


vec3 BRDF()
{
    // GBufferからシーン情報を復元
    vec4 ALBEDO    = texture(albedoMap,   fsi.TexCoords);
    vec4 WORLD_POS = texture(positionMap, fsi.TexCoords);
    vec4 NORMAL    = texture(normalMap,   fsi.TexCoords);
    vec4 EMISSION  = texture(emissionMap, fsi.TexCoords);

    vec3  albedo    = ALBEDO.rgb;    // ベースカラー（アルベド / 拡散反射率）
    vec3  worldPos  = WORLD_POS.rgb; // ピクセル座標（ワールド空間）
    vec3  normal    = NORMAL.rgb;    // 法線
    float metallic  = EMISSION.a;    // メタリック
    float roughness = NORMAL.a;      // ラフネス

    vec3 N = normalize(normal);            // 法線ベクトル
    vec3 V = normalize(camPos - worldPos); // ビューベクトル
    vec3 R = reflect(-V, N);               // 反射ベクトル
    vec3 L = normalize(lightDir);          // ライトベクトル
    vec3 H = normalize(V + L);             // ビューベクトルとライトベクトルとのハーフベクトル

    // 誘電体は基本反射率(0.04)があり、メタリックパラメータによって線形補間
    // 金属表面は拡散反射が無く、アルベドを反射率として使用できる
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    //=======================================
    // ライト強度
    //=======================================
    // 放射輝度? ライトの強さ（ディレクショナルライトなので、減衰はなし）
    float attenuation = 1.0;
    vec3  radiance    = lightColor * attenuation;

    //============================================
    // Cook-Torrance BRDF
    //============================================
    // 拡散反射 ... Phong の拡散反射と同じ
    float NdotL = max(dot(N, L), 0.0);

    // 鏡面反射 ... DFG / 4(ωo⋅n)(ωi⋅n)
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3  numerator   = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    vec3  specular    = numerator / denominator;

    // エネルギー節約のため、拡散光と鏡面反射光は 1.0 を超えることはできない (サーフェスが光を発しない限り)
    // この関係を維持するには、拡散成分 (kD) が 1.0 - kS(F) に等しくなければならないらしい
    vec3 kD = vec3(1.0) - F;

    // kD に逆金属性を掛けて、非金属のみが拡散光を持つようにする (純粋な金属には拡散光がない)
    kD *= 1.0 - metallic;

    // 既に BRDF にフレネル (kS) を乗算しているので、再度 kS を乗算しない
    vec3 Lout = (kD * albedo / PI + specular) * radiance * NdotL;

    //========================================
    // 環境光（IBL）
    //========================================
    // 拡散反射
    vec3 irradiance  = texture(irradianceMap, N).rgb;
    vec3 diffuse     = irradiance * albedo;

    // 鏡面反射
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf             = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    specular              = prefilteredColor * (F * brdf.x + brdf.y);

    //========================================
    // シャドウ (Directional Light)
    //========================================
    int   currentLayer;
    float shadow      = DirectionalLightShadow(worldPos, normal, currentLayer);
    float shadowColor = smoothstep(0.0, 1.0, shadow);

    //========================================
    // 最終カラー
    //========================================
    float AO = 1.0;
    vec3 ambient = (kD * diffuse + specular) * AO;
    vec3 color   = (ambient * iblIntencity) + Lout * shadowColor + EMISSION.rgb;

    // デバッグ: カスケード表示
    float sc = float(showCascade);
    color *= mix(vec3(1.0), CascadeColor(currentLayer), sc);

    return color;
}


// フォンシェーディング
vec3 BlinnPhong()
{
    // 未使用（パラメータ統一のため）
    float irr    = texture(irradianceMap, vec3(1.0)).r;
    float pre    = textureLod(prefilterMap, vec3(1.0), 1.0).r;
    float brdf   = texture(brdfLUT, vec2(1.0)).r;
    float NO_USE = irr + pre + brdf;

    vec3 ALBEDO    = texture(albedoMap,   fsi.TexCoords).rgb;
    vec3 WORLD_POS = texture(positionMap, fsi.TexCoords).rgb;
    vec3 N         = texture(normalMap,   fsi.TexCoords).rgb;
    vec3 EMISSION  = texture(emissionMap, fsi.TexCoords).rgb;

    // 環境光 (PBRシェーダー側では輝度を10倍した値を1.0として扱っているため)
    vec3 ambient = (lightColor / 10.0) * (iblIntencity / 10.0);

    // 拡散反射光
    vec3  L        = normalize(lightDir);
    vec3  diffuse  = max(dot(N, L), 0.0) * (lightColor / 10.0);

    // 鏡面反射
    vec3 V        = normalize(camPos - WORLD_POS);
    vec3 H        = normalize(L + V);
    vec3 specular = pow(max(dot(N, H), 0.0), 64.0) * (lightColor / 10.0);

    // シャドウ (Directional Light)
    int   currentLayer;
    float shadowColor = smoothstep(0.0, 1.0, DirectionalLightShadow(WORLD_POS, N, currentLayer));

    // 最終コンポーネント
    vec3 ambientComponent  = ambient  * ALBEDO;
    vec3 diffuseComponent  = diffuse  * ALBEDO * shadowColor;
    vec3 specularComponent = specular * ALBEDO * shadowColor;
    vec3 emissionComponent = EMISSION;
    vec3 color             = ambientComponent + diffuseComponent + specularComponent + emissionComponent;

    // デバッグ: カスケード表示
    float sc = float(showCascade);
    color *= mix(vec3(1.0), CascadeColor(currentLayer), sc);

    return color + (NO_USE * EPSILON);
}


vec3 SelectShader(int id)
{
    if (id == 0) return BlinnPhong();
    if (id == 1) return BRDF();
}


void main()
{   
    // サンプリング関数
    // int   : isampler2D + texelFetch
    // float : sampler2D  + texture

    // テクスチャ座標ではなくピクセルを指定
    int id     = texelFetch(idMap, ivec2(fsi.TexCoords * textureSize(idMap, 0)), 0).r;
    vec3 color = SelectShader(id);

    PixelColor = vec4(color, 1.0);
}

