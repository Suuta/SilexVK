//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

layout (location = 0) out vec3 outPosAsUV;

void main()
{
    // 頂点データのローカル座標がUV座標になる
    outPosAsUV = inPos;
}


//===================================================================================
// ジオメトリシェーダ
//===================================================================================
#pragma GEOMETRY
#version 450

layout (set = 0, binding = 0) uniform Transform
{
    mat4 cubeView[6];
    mat4 cubeProj;
};

layout (location = 0) in  vec3 inPos[];
layout (location = 0) out vec3 outPosAsUV;

layout(triangles                        ) in;
layout(triangle_strip, max_vertices = 18) out;

void main()
{
    for (int face = 0; face < 6; face++)
    {
        gl_Layer = face;

        for (int i = 0; i < 3; i++)
        {
            outPosAsUV  = inPos[i];
            gl_Position = cubeProj * cubeView[face] * vec4(inPos[i], 1.0);
            EmitVertex();
        }

        EndPrimitive();
    }
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450

layout (location = 0) in  vec3 inPosAsUV;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D equirectangularMap;


vec2 SampleSphericalMap(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);

    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv    = SampleSphericalMap(normalize(inPosAsUV));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    outColor = vec4(vec3(color), 1.0);
}