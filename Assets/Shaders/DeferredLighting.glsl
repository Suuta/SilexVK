//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout(location = 0) out vec2 outUV;

void main()
{
    // 三角形でフルスクリーン描画
    // https://stackoverflow.com/questions/2588875/whats-the-best-way-to-draw-a-fullscreen-quad-in-opengl-3-2

    const vec2 triangle[3] =
    {
        vec2(-1.0,  1.0), // 左上
        vec2(-1.0, -3.0), // 左下
        vec2( 3.0,  1.0), // 右上
    };

    vec4 pos = vec4(triangle[gl_VertexIndex], 0.0, 1.0);
    vec2 uv  = (0.5 * pos.xy) + vec2(0.5);

    // uv 反転
    uv.y = 1.0 - uv.y;

    outUV       = uv;
    gl_Position = pos;
}


//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450

layout(location = 0) in  vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D            sceneColor;
layout(set = 0, binding = 1) uniform sampler2D            sceneNormal;
layout(set = 0, binding = 2) uniform sampler2D            sceneEmission;
layout(set = 0, binding = 3) uniform sampler2D            sceneDepth;
layout(set = 0, binding = 4) uniform samplerCube          irradianceMap;
layout(set = 0, binding = 5) uniform samplerCube          prefilterMap;
layout(set = 0, binding = 6) uniform sampler2D            brdfMap;
layout(set = 0, binding = 7) uniform sampler2DArrayShadow cascadeshadowMap;

layout(set = 0, binding = 8) uniform Scene
{
    vec4  lightDir;
    vec4  lightColor;
    vec4  cameraPosition; // xyz: pos w: far
    mat4  view;
    mat4  invViewProjection;
} u_scene;


layout(set = 0, binding = 9) uniform Cascade
{
    vec4 cascadePlaneDistances[4];
};

layout(set = 0, binding = 10) uniform ShadowData
{
    mat4 lightSpaceMatrices[4];
};


//---------------------------------------------------------------------------
// 定数
//---------------------------------------------------------------------------
const int   cascadeCount    = 4;
const float shadowDepthBias = 0.01;
const float PI              = 3.14159265359;
const float EPSILON         = 0.00001;
const float iblIntencity    = 1.0;


//---------------------------------------------------------------------------
// 深度バイアス
//---------------------------------------------------------------------------
float ShadowDepthBias(int currentLayer, vec3 normal, float farPlane)
{
    float bias     = max(shadowDepthBias * (1.0 - dot(normal, normalize(u_scene.lightDir.xyz))), shadowDepthBias);
    float mask     = 1.0 - abs(sign(currentLayer - cascadeCount));
    bias *= mix(1.0 / (cascadePlaneDistances[currentLayer].x * 0.5), 1.0 / (farPlane * 0.5), mask);

    return bias;
}

//---------------------------------------------------------------------------
// ソフトシャドウでサンプリングするピクセルオフセット
//---------------------------------------------------------------------------
float ShadowSampleOffset(vec3 shadowMapCoords, vec2 offsetPos, vec2 texelSize, int layer, float currentDepth, float bias)
{
    vec2  offset = vec2(offsetPos.x, offsetPos.y) * texelSize;
    float depth  = texture(cascadeshadowMap, vec4(shadowMapCoords.xy + offset, layer, currentDepth - bias));
    return depth;
}

//---------------------------------------------------------------------------
// ピクセルのシャドウカラーの決定
//---------------------------------------------------------------------------
float ShadowSampling(float bias, int layer, vec3 shadowMapCoords, float lightSpaceDepth)
{
    // ピクセルあたりのテクセルサイズ
    vec2  texelSize   = 1.0 / vec2(textureSize(cascadeshadowMap, 0));
    float shadowColor = 0.0;

    // ソフトシャドウ (5 x 5 PCF)
    for (float x = -2.0; x <= 2.0; x += 1.0)
    {
        for (float y = -2.0; y <= 2.0; y += 1.0)
        {
            shadowColor += ShadowSampleOffset(shadowMapCoords, vec2(x, y), texelSize, layer, lightSpaceDepth, bias);
        }
    }
    shadowColor /= 25;


    // ソフトシャドウ なし
    //float depth = texture(cascadeshadowMap, vec4(shadowMapCoords.xy, layer, shadowMapCoords.z));
    //shadowColor = step(lightSpaceDepth - bias, depth);

    return shadowColor;
}

//---------------------------------------------------------------------------
// ディレクショナルライト
//---------------------------------------------------------------------------
float DirectionalLightShadow(vec3 fragPosWorldSpace, vec3 normal, out int currentLayer)
{
    // カメラ空間からの距離からカスケードレイヤー選択
    vec4 fragPosViewSpace = u_scene.view * vec4(fragPosWorldSpace, 1.0);
    float fragPosDistance = abs(fragPosViewSpace.z);

    currentLayer = cascadeCount - 1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (fragPosDistance < cascadePlaneDistances[i].x)
        {
            currentLayer = i;
            break;
        }
    }

    // ライト空間変換
    vec4 fragPosLightSpace = lightSpaceMatrices[currentLayer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords        = fragPosLightSpace.xyz / fragPosLightSpace.w;

    //-------------------------------------------------------------
    // 負のビューポート使用につき、y軸反転の必要あり、復元の時は
    // vulkan側の処理が行われないので手動で反転させる
    //-------------------------------------------------------------
    projCoords.y = -projCoords.y;

    // Z座標は（OpenGLをのぞいて） 0~1 のままなので xy のみ適応
    //projCoords  = projCoords    * 0.5 + 0.5; // OpenGL
    projCoords.xy = projCoords.xy * 0.5 + 0.5; // Vulkan

    // ライト空間での深度値 (視錐台外であれば影を落とさない)
    float lightSpaceDepth = projCoords.z;
    if (lightSpaceDepth > 1.0)
    {
        return 1.0;
    }

    // 深度値バイアス
    float bias = ShadowDepthBias(currentLayer, normal, u_scene.cameraPosition.w);

    // シャドウサンプリング
    return ShadowSampling(bias, currentLayer, projCoords, lightSpaceDepth);
}



//---------------------------------------------------------------------------
// 深度値 から ワールド座標 を計算
//---------------------------------------------------------------------------
vec3 ConstructWorldPosition(vec2 texcoord, float depthFromDepthBuffer, mat4 inverceProjectionView)
{
    // テクスチャ座標 と 深度値を使って NDC座標系[xy: -1~1] に変換 (zはそのまま)  ※openGLは z: -1~1 に変換する必要あり
    vec4 clipSpace = vec4(texcoord * vec2(2.0) - vec2(1.0), depthFromDepthBuffer, 1.0);

    //-------------------------------------------------------------
    // 負のビューポート使用につき、y軸反転の必要あり、復元の時は
    // vulkan側の処理が行われないので手動で反転させる
    //-------------------------------------------------------------
    clipSpace.y = -clipSpace.y;

    // ワールドに変換
    vec4 position = inverceProjectionView * clipSpace;

    // 透視除算
    return vec3(position.xyz / position.w);
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

//---------------------------------------------------------------------------
// フォンシェーディング (テスト用、実際には使用しない)
//---------------------------------------------------------------------------
vec3 BlinnPhong()
{
    vec3  ALBEDO   = texture(sceneColor,    inTexCoord).rgb;
    vec3  NORMAL   = texture(sceneNormal,   inTexCoord).rgb;
    vec3  EMISSION = texture(sceneEmission, inTexCoord).rgb;
    float DEPTH    = texture(sceneDepth,    inTexCoord).r;

    // 深度値から復元
    vec3 WORLD = ConstructWorldPosition(inTexCoord, DEPTH, u_scene.invViewProjection);

    // ノーマルを -1~1に戻す
    vec3 N = vec3(NORMAL * 2.0) - vec3(1.0);

    // 環境ベースカラー
    vec3 ambient = u_scene.lightColor.rgb * 0.01;

    // 拡散反射光
    vec3  L        = normalize(u_scene.lightDir.xyz);
    vec3  diffuse  = max(dot(N, L), 0.0) * u_scene.lightColor.rgb;

    // 鏡面反射
    vec3 V        = normalize(u_scene.cameraPosition.xyz - WORLD);
    vec3 H        = normalize(L + V);
    vec3 specular = pow(max(dot(N, H), 0.0), 64.0) * u_scene.lightColor.rgb;

    // シャドウ (Directional Light)
    int   currentLayer;
    float shadowColor = smoothstep(0.0, 1.0, DirectionalLightShadow(WORLD, N, currentLayer));

    // 最終コンポーネント
    vec3 ambientComponent  = ambient  * ALBEDO;
    vec3 diffuseComponent  = diffuse  * ALBEDO * shadowColor;
    vec3 specularComponent = specular * ALBEDO * shadowColor;
    vec3 emissionComponent = EMISSION;
    vec3 color             = ambientComponent + diffuseComponent + specularComponent + emissionComponent;

    // デバッグ: カスケード表示
    //float sc = float(showCascade);
    //color *= mix(vec3(1.0), CascadeColor(currentLayer), 1);

    return color;
}





// ラフネスから、反射の中間ベクトルとの整列具合を求める
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

// ラフネスから、反射の遮断率を求める（表面の陰影）
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

// 角度による表面反射率を求める
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

//---------------------------------------------------------------------------
// PBR シェーディング
//---------------------------------------------------------------------------
vec3 BRDF()
{
    // GBufferからシーン情報を復元
    vec4  ALBEDO   = texture(sceneColor,    inTexCoord);
    vec4  NORMAL   = texture(sceneNormal,   inTexCoord);
    vec4  EMISSION = texture(sceneEmission, inTexCoord);
    float DEPTH    = texture(sceneDepth,    inTexCoord).r;

    // 深度値から復元
    vec3 WORLD = ConstructWorldPosition(inTexCoord, DEPTH, u_scene.invViewProjection);

    // ノーマルを -1~1に戻す
    vec3 constructN = vec3(NORMAL.xyz * 2.0) - vec3(1.0);

    vec3  worldPos  = WORLD;      // ピクセル座標（ワールド空間）
    vec3  albedo    = ALBEDO.rgb; // ベースカラー（アルベド / 拡散反射率）
    float roughness = ALBEDO.a;   // ラフネス
    vec3  normal    = constructN; // 法線
    float metallic  = NORMAL.a;   // メタリック

    vec3 N = normalize(normal);                                // 法線ベクトル
    vec3 V = normalize(u_scene.cameraPosition.xyz - worldPos); // ビューベクトル
    vec3 R = reflect(-V, N);                                   // 反射ベクトル
    vec3 L = normalize(u_scene.lightDir.xyz);                  // ライトベクトル
    vec3 H = normalize(V + L);                                 // ビューベクトルとライトベクトルとのハーフベクトル

    // 誘電体は基本反射率(0.04)があり、メタリックパラメータによって線形補間
    // 金属表面は拡散反射が無く、アルベドを反射率として使用できる
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);


    //=======================================
    // ライト強度
    //=======================================
    float attenuation = 1.0; // （ディレクショナルライトなので、減衰はなし）
    vec3  radiance    = u_scene.lightColor.rgb * attenuation;

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
    vec3 diffuse     = albedo * irradiance;

    // 鏡面反射
    const float MAX_REFLECTION_LOD = 5.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf             = texture(brdfMap, vec2(max(dot(N, V), 0.0), roughness)).rg;

    specular              =  (F * brdf.x + brdf.y) * prefilteredColor;

    //========================================
    // シャドウ (Directional Light)
    //========================================
    int   currentLayer;
    float shadow      = DirectionalLightShadow(worldPos, N, currentLayer);
    float shadowColor = smoothstep(0.0, 1.0, shadow);

    //========================================
    // 最終カラー
    //========================================
    // TODO: AO マップ（実装は DeferredPrimitive で GBuffer に書き込む）
    float AO = 1.0;

    vec3 ambient = (kD * diffuse + specular) * AO;
    vec3 color   = (ambient * iblIntencity) + Lout * shadowColor + EMISSION.rgb;

    // デバッグ: カスケード表示
    //float sc = float(showCascade);
    //color *= mix(vec3(1.0), CascadeColor(currentLayer), sc);

    return color;
    //return shadow + (0.00001 * color);
}




//---------------------------------------------------------------------------
// エントリー
//---------------------------------------------------------------------------
void main()
{   
    // サンプリング関数
    // int   : isampler2D + texelFetch
    // float : sampler2D  + texture

    // 整数型のサンプリングは、テクスチャ座標ではなくピクセルを指定
    // int shadingModel = texelFetch(idMap, ivec2(fsi.TexCoords * textureSize(idMap, 0)), 0).r;

    vec3 color = BRDF();
    outColor = vec4(color, 1.0);
}
