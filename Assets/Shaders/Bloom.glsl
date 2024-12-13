//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout (location = 0) out vec2 outUV;

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
    vec2 uv = (0.5 * pos.xy) + vec2(0.5);

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

layout (location = 0) in  vec2 uv;
layout (location = 0) out vec4 pixel;

layout (push_constant) uniform Constant
{
    float intencity;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;
layout (set = 0, binding = 1) uniform sampler2D bloomBlur;


void main()
{
    vec3 color = vec3(1.0);

    // ブルームテクスチャサンプリング
    vec3 hdrColor   = texture(srcTexture, uv).rgb;
    vec3 bloomColor = texture(bloomBlur,  uv).rgb;

    // 0.0 ~ 1.0 で線形補間
    color = mix(hdrColor, bloomColor, intencity);

    // ブルームが適応されなかったピクセルに対しても元の色が適応されるようにする
    color = max(color, hdrColor);

    pixel = vec4(color, 1.0);
}
