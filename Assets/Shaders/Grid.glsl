
// How to make an infinite grid.
// https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/

#pragma VERTEX
#version 450

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;
layout(location = 2) out vec3 nearFarHeight;
layout(location = 3) out mat4 fragView;
layout(location = 7) out mat4 fragProj;

layout(set = 0, binding = 0) uniform CameraData
{
    mat4 view;
    mat4 proj;
    vec4 pos;
} cameraView;

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection)
{
    mat4 viewInv          = inverse(view);
    mat4 projInv          = inverse(projection);
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);

    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main()
{
    vec3 gridPlane[6] =
    {
        vec3(-1.0,  1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3( 1.0,  1.0, 0.0),
        vec3( 1.0,  1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3( 1.0, -1.0, 0.0),
    };

    vec3 p      = gridPlane[gl_VertexIndex];
    nearPoint   = UnprojectPoint(p.x, p.y, 0.0, cameraView.view, cameraView.proj).xyz;
    farPoint    = UnprojectPoint(p.x, p.y, 1.0, cameraView.view, cameraView.proj).xyz;
    gl_Position = vec4(p, 1.0);

    fragView  = cameraView.view;
    fragProj  = cameraView.proj;
    nearFarHeight.x = 0.1;
    nearFarHeight.y = cameraView.pos.w;
    nearFarHeight.z = cameraView.pos.y;
}



#pragma FRAGMENT
#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;
layout(location = 2) in vec3 nearFarHeight;
layout(location = 3) in mat4 fragView;
layout(location = 7) in mat4 fragProj;


float computeDepth(vec3 pos)
{
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

float computeLinearDepth(vec3 pos, float near, float far)
{
    vec4   clip_space_pos   = fragProj * fragView * vec4(pos.xyz, 1.0);
    float  clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0;                   // put back between -1 and 1
    float  linearDepth      = (2.0 * near * far) / (far + near - clip_space_depth * (far - near)); // get linear value between 0.01 and 100
    return linearDepth / far;
}

vec4 grid(vec3 fragPos3D, float scale, float fadingFactor)
{
    vec2  coord      = fragPos3D.xz * scale;
    vec2  derivative = fwidth(coord);
    vec2  grid       = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line       = min(grid.x, grid.y);
    float minimumz   = min(derivative.y, 1);
    float minimumx   = min(derivative.x, 1);

    float a = 1.0 - min(line, 1.0);
    vec4 color = vec4(0.2, 0.2, 0.2, a * fadingFactor);

    return color;
}

void main()
{
    // 視点が水平線と平行な場合に、1.0以上の値が代入されてしまう
    // RGBA16_SFLOAT(シーンテクスチャ) フォーマットのアタッチメントに1.0 以上の値が代入されないように
    float t        = min(-nearPoint.y / (farPoint.y - nearPoint.y), 1.0);
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    float linearDepth = computeLinearDepth(fragPos3D, nearFarHeight.x, nearFarHeight.y);
    float depth       = computeDepth(fragPos3D);
    float fading      = max(0, 0.3 - linearDepth);
    gl_FragDepth      = depth;

    float cameraY = nearFarHeight.z;
    float lod_0   = max(0.0, 1.0 - abs(cameraY) /  40.0);
    float lod_1   = max(0.0, 1.0 - abs(cameraY) / 160.0);


    // t > 0 の場合は不透明度 = 1、それ以外の場合は不透明度 = 0
    vec4 color = (grid(fragPos3D, 0.01, 1.0) + grid(fragPos3D, 0.1, lod_1) + grid(fragPos3D, 1.0, lod_0)) * float(t > 0);
    color.a *= fading;


    outColor = color;
}
