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
    float filterRadius;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;



// 参照
// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

void main()
{
    float x = filterRadius;
    float y = filterRadius;

    // a - b - c
    // d - e - f
    // g - h - i
    vec3 a = texture(srcTexture, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 b = texture(srcTexture, vec2(uv.x,     uv.y + y)).rgb;
    vec3 c = texture(srcTexture, vec2(uv.x + x, uv.y + y)).rgb;

    vec3 d = texture(srcTexture, vec2(uv.x - x, uv.y    )).rgb;
    vec3 e = texture(srcTexture, vec2(uv.x,     uv.y    )).rgb;
    vec3 f = texture(srcTexture, vec2(uv.x + x, uv.y    )).rgb;

    vec3 g = texture(srcTexture, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 h = texture(srcTexture, vec2(uv.x,     uv.y - y)).rgb;
    vec3 i = texture(srcTexture, vec2(uv.x + x, uv.y - y)).rgb;

    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |

    vec3 color = vec3(1.0);

    color =  e * 4.0;
    color += (b + d + f + h) * 2.0;
    color += (a + c + g + i);
    color *= 1.0 / 16.0;

    pixel = vec4(color, 1.0);
}