
#pragma once
#include "Rendering/RenderingCore.h"


namespace Silex
{
    enum ShaderDataType
    {
        SHADER_DATA_TYPE_NONE,
        SHADER_DATA_TYPE_BOOL,
        SHADER_DATA_TYPE_INT,
        SHADER_DATA_TYPE_UINT,
        SHADER_DATA_TYPE_FLOAT,
        SHADER_DATA_TYPE_VEC2,
        SHADER_DATA_TYPE_VEC3,
        SHADER_DATA_TYPE_VEC4,
        SHADER_DATA_TYPE_MAT3,
        SHADER_DATA_TYPE_MAT4,
        SHADER_DATA_TYPE_IVEC2,
        SHADER_DATA_TYPE_IVEC3,
        SHADER_DATA_TYPE_IVEC4,

        SHADER_DATA_TYPE_MAX,
    };

    //------------------------------------
    // プッシュ定数
    //------------------------------------
    struct PushConstantMember
    {
        ShaderDataType type;
        std::string    name;
        uint32         size;
        uint32         offset;
    };

    struct ShaderPushConstant
    {
        std::string name;
        uint32      size;

        std::unordered_map<std::string, PushConstantMember> members;
    };

    struct PushConstantRange
    {
        ShaderStage stage  = SHADER_STAGE_ALL;
        uint32      offset = 0;
        uint32      size   = 0;
    };

    //------------------------------------
    // シェーダーバッファ
    //------------------------------------
    struct ShaderBuffer
    {
        ShaderStage stage;
        std::string name;
        uint32      size;
        uint32      setIndex;
        uint32      bindingPoint;
    };

    //------------------------------------
    // シェーダーイメージ
    //------------------------------------
    struct ShaderImage
    {
        ShaderStage stage;
        std::string name;
        uint32      arraySize;
        uint32      dimension;
        uint32      setIndex;
        uint32      bindingPoint;
    };

    //------------------------------------
    // リソース
    //------------------------------------
    struct ShaderResourceDeclaration
    {
        std::string name;
        uint32      setIndex;
        uint32      registerIndex;
        uint32      count;
    };

    //------------------------------------
    // シェーダーデスクリプター
    //------------------------------------
    struct ShaderDescriptorSet
    {
        std::unordered_map<uint32, ShaderBuffer> uniformBuffers;
        std::unordered_map<uint32, ShaderBuffer> storageBuffers;
        std::unordered_map<uint32, ShaderImage>  imageSamplers;
        std::unordered_map<uint32, ShaderImage>  storageImages;
        std::unordered_map<uint32, ShaderImage>  separateTextures;
        std::unordered_map<uint32, ShaderImage>  separateSamplers;

        //std::unordered_map<uint32, ShaderImage>  inputAttachment;
    };

    //------------------------------------
    // リフレクションデータ
    //------------------------------------
    struct ShaderReflectionData
    {
        std::vector<ShaderDescriptorSet> descriptorSets;
        std::vector<PushConstantRange>   pushConstantRanges;


        // シェーダーコンパイル時のコンソール出力に使用中
        // 現状は使用していないが、データ情報が必要になった場合(エディターでの表示)に使用できる
        std::unordered_map<std::string, ShaderPushConstant>        pushConstants;
        std::unordered_map<std::string, ShaderResourceDeclaration> resources;
    };

    //------------------------------------
    // コンパイルデータ
    //------------------------------------
    struct ShaderCompiledData
    {
        ShaderReflectionData                                 reflection;
        std::unordered_map<ShaderStage, std::vector<uint32>> shaderBinaries;
    };


    //------------------------------------
    // シェーダーコンパイラ
    //------------------------------------
    class ShaderCompiler : public Object
    {
        SL_CLASS(ShaderCompiler, Object)

    public:

        ShaderCompiler()  { instance = this;    }
        ~ShaderCompiler() { instance = nullptr; }

        static ShaderCompiler* Get();

    public:

        // コンパイル
        bool Compile(const std::string& filePath, ShaderCompiledData& out_compiledData);

    private:

        // コンパイル前処理
        std::unordered_map<ShaderStage, std::string> _SplitStages(const std::string& source);

        // コンパイル(spirvバイナリ)を生成
        std::string _CompileStage(ShaderStage stage, const std::string& source, std::vector<uint32>& out_putSpirv, const std::string& filepath);

        // コンパイル(spirvバイナリ)を生成
        void _ReflectStage(ShaderStage stage, const std::vector<uint32>& spirv);

    private:

        static inline ShaderCompiler* instance = nullptr;
    };
}

