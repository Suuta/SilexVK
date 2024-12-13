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
            vec3 uv = inPos[i];
            uv.y *= -1.0;

            outPosAsUV  = uv;
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

layout (location = 0) in  vec3 LocalPos;
layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 1) uniform samplerCube environmentCubeMap;


void main()
{
    const float PI = 3.14159265359;

    vec3 N = normalize(LocalPos);
    vec3 irradiance = vec3(0.0);

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples   = 0.0;

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec     = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environmentCubeMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    FragColor  = vec4(irradiance * 0.5, 1.0);
}
