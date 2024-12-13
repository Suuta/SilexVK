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
    ivec2 srcResolution;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

// 参照
// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

// |a| |b| |c|
// | |j| |k| |
// |d| |e| |f|
// | |l| |m| |
// |g| |h| |i|

void main()
{
    vec2 srcTexelSize = 1.0 / srcResolution;
    vec3 color        = vec3(1.0);
    float x           = srcTexelSize.x;
    float y           = srcTexelSize.y;

    vec3 a = texture(srcTexture, vec2(uv.x - 2 * x, uv.y + 2 * y)).rgb;
    vec3 b = texture(srcTexture, vec2(uv.x,         uv.y + 2 * y)).rgb;
    vec3 c = texture(srcTexture, vec2(uv.x + 2 * x, uv.y + 2 * y)).rgb;

    vec3 d = texture(srcTexture, vec2(uv.x - 2 * x, uv.y        )).rgb;
    vec3 e = texture(srcTexture, vec2(uv.x,         uv.y        )).rgb;
    vec3 f = texture(srcTexture, vec2(uv.x + 2 * x, uv.y        )).rgb;

    vec3 g = texture(srcTexture, vec2(uv.x - 2 * x, uv.y - 2 * y)).rgb;
    vec3 h = texture(srcTexture, vec2(uv.x,         uv.y - 2 * y)).rgb;
    vec3 i = texture(srcTexture, vec2(uv.x + 2 * x, uv.y - 2 * y)).rgb;

    vec3 j = texture(srcTexture, vec2(uv.x - x,     uv.y + y    )).rgb;
    vec3 k = texture(srcTexture, vec2(uv.x + x,     uv.y + y    )).rgb;
    vec3 l = texture(srcTexture, vec2(uv.x - x,     uv.y - y    )).rgb;
    vec3 m = texture(srcTexture, vec2(uv.x + x,     uv.y - y    )).rgb;

    color =   e * 0.125;
    color += (a + c + g + i) * 0.03125;
    color += (b + d + f + h) * 0.0625;
    color += (j + k + l + m) * 0.125;

    float EPSILON = 0.0001;
    color = max(color, EPSILON);

    pixel = vec4(color, 1.0);
}