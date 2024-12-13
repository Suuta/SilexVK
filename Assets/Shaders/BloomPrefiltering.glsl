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
    float threshold;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;


// ブルームにしきい値を適応
//https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/

vec3 Prefilter (vec3 color, float threshold)
{
    float brightness   = max(color.r, max(color.g, color.b));
    float contribution = max(0.0, brightness - threshold);
    contribution /= max(brightness, 0.0001);

    return color * contribution;
}

void main()
{
    vec3  color        = texture(srcTexture,  uv).rgb;
    float brightness   = max(color.r, max(color.g, color.b));
    float contribution = max(0.0, brightness - threshold);
    contribution /= max(brightness, 0.0001);

    color *= contribution;


    pixel = vec4(color, 1.0);
}
