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
layout (location = 1) out int  outInstanceIndex;

void main()
{
    outPosAsUV       = inPos;
    outInstanceIndex = gl_InstanceIndex;
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

layout (location = 0) in vec3     inPos[];
layout (location = 1) in flat int inInstanceIndex[];

layout (location = 0) out vec3 outPosAsUV;
layout (location = 1) out int  outInstanceIndex;

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

    outInstanceIndex = inInstanceIndex[0];
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450

layout (location = 0) in  vec3      inLocalPos;
layout (location = 1) in  flat int  inInstanceIndex;
layout (location = 0) out vec4      outColor;

layout (set = 0, binding = 1) uniform samplerCube environmentCubeMap;


float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    const float PI = 3.14159265359;

    float a  = roughness * roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    const float PI = 3.14159265359;
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

void main()
{
    const float PI = 3.14159265359;
    vec3 N = normalize(inLocalPos);
    
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 4096u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    // GL : gl_InstanceID
    // VK : gl_InstanceIndex

    // firstInstance で 呼び分け
    float ml = inInstanceIndex / 4.0;


    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, ml);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            float D   = DistributionGGX(N, H, ml);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float resolution = 512.0;
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = ml == 0.0? 0.0 : 0.5 * log2(saSample / saTexel);

            prefilteredColor += textureLod(environmentCubeMap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    outColor = vec4(prefilteredColor, 1.0);
}