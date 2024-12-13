//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;


//uniform mat4 model;
//uniform int  instanceOffset;

//struct InstanceParameter
//{
//    mat4  transformMatrix;
//    mat4  normalMatrix;
//    ivec4 pixelID;
//};

// インスタンスバッファ
//layout (std430, binding = 0) buffer InstanceParameterStorage
//{
//    InstanceParameter parameter[];
//;


layout (set = 0, binding = 0) uniform Transform
{
    mat4 world;
};


void main()
{
    // SSBOを使う場合
    //mat4 worldMatrix = parameter[instanceOffset + gl_InstanceID].transformMatrix;
    //gl_Position = worldMatrix * vec4(inPos, 1.0);

    // 頂点インスタンスを使った場合
    //gl_Position = aTransform0 * vec4(aPos, 1.0);
    //gl_Position = model * vec4(aPos, 1.0);

    gl_Position = world * vec4(inPos, 1.0);
}


//===================================================================================
// ジオメトリシェーダ
//===================================================================================
#pragma GEOMETRY
#version 450

// シェーダー呼び出し回数
layout(triangles,      invocations  = 4) in;
layout(triangle_strip, max_vertices = 3) out;

layout(set = 0, binding = 1) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[4];
};


void main()
{
    for (int i = 0; i < 3; ++i)
    {
        // gl_in[i] 三角形の各頂点に対して各カスケードの変換行列を適応
        gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        
        // カスケード Texture2DArray の書き込み先レイヤーを指定
        gl_Layer = gl_InvocationID;
    
        // プリミティブを構築する頂点に設定する
        EmitVertex();
    }
    
    // 次のプリミティブに移行
    EndPrimitive();
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450
    
void main()
{
    //-------------------------------------------------------------------------------
    // フラグメントシェーダーが内部で実行する処理
    // gl_FragDepth = gl_FragCoord.z;
    //-------------------------------------------------------------------------------
    // 本来は ピクセルシェーダー ⇒ 深度 の順で実行されるが、余剰ピクセルの計算が発生しないように
    // 深度 => ピクセルシェーダー の順で実行されるように最適化される。
    // しかし、手動で深度値に変更を加えると、深度テスト前にフラグメントシェーダーが（本来の順序で）
    // 実行されるようになってしまう。 このほかに "discard" 命令もこの対象になる
    //--------------------------------------------------------------------------------
}
