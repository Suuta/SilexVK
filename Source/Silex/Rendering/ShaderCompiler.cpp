
#include "PCH.h"
#include "Rendering/ShaderCompiler.h"

//==========================================================================
// NOTE:
// glslang + spirv_tool のコンパイルでは、静的ライブラリが複雑 + サイズが大きすぎる
// 特に "SPIRV-Tools-optd.lib" が 300MB あり、100MB制限で github にアップできない
// ⇒ 共有ライブラリでビルドした shaderc に移行
//==========================================================================
#define SHADERC 1
#if !SHADERC
    #include <glslang/Public/ResourceLimits.h>
    #include <glslang/SPIRV/GlslangToSpv.h>
    #include <glslang/Public/ShaderLang.h>
#else
    #include <shaderc/shaderc.hpp>
#endif

#include <spirv_cross/spirv_glsl.hpp>



namespace Silex
{
    //======================================
    // シェーダーコンパイラのインスタンス
    //======================================
    static ShaderCompiler shaderCompiler;



    static bool ReadString(std::string& output, const std::string& filepath)
    {
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            output.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&output[0], output.size());
        }
        else
        {
            SL_LOG_ERROR("ファイルの読み込みに失敗しました: {}", filepath.c_str());
            return false;
        }

        in.close();
        return true;
    }

    static bool ReadSpvBinary(std::vector<uint32>& output, const std::string& filepath)
    {
        FILE* f = std::fopen(filepath.c_str(), "rb");
        if (f)
        {
            std::fseek(f, 0, SEEK_END);
            uint64 size = std::ftell(f);
            std::fseek(f, 0, SEEK_SET);

            output = std::vector<uint32>(size / sizeof(uint32));
            std::fread(output.data(), sizeof(uint32), output.size(), f);
        }
        else
        {
            SL_LOG_ERROR("ファイルの読み込みに失敗しました: {}", filepath.c_str());
            return false;
        }

        std::fclose(f);
        return true;
    }

    static bool WriteSpvBinary(const std::vector<uint32>& writeData, const std::string& filepath)
    {
        FILE* f = std::fopen(filepath.c_str(), "wb");
        if (f)
        {
            std::fwrite(writeData.data(), sizeof(uint32), writeData.size(), f);
        }
        else
        {
            SL_LOG_ERROR("ファイルの読み込みに失敗しました: {}", filepath.c_str());
            return false;
        }

        std::fclose(f);
        return true;
    }

    static bool IsFileCachedRecently(const std::filesystem::path& file, const std::filesystem::path& cacheFile)
    {
        auto fts = std::filesystem::last_write_time(file);
        auto cts = std::filesystem::last_write_time(cacheFile);

        auto fsec = std::chrono::duration_cast<std::chrono::seconds>(fts.time_since_epoch());
        auto csec = std::chrono::duration_cast<std::chrono::seconds>(cts.time_since_epoch());

        // コンパイルに成功した場合にキャッシュが更新されるので、キャッシュのタイムスタンプが最近であれば
        // 最新のコンパイル済みキャッシュと判断できる
        return csec > fsec;
    }


    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:

        // shadeerc が #include "header.h" のようなインクルードディレクティブを検出した時に呼び出される
        // そのファイルを読み込む方法と、読み込んで返すデータを定義する
        shaderc_include_result* GetInclude(const char* requestedSource, shaderc_include_type type, const char* requestingSource, size_t includeDepth) override 
        {
            std::filesystem::path readSource = std::filesystem::path(requestingSource).parent_path() / requestedSource;
            std::string includePath = readSource.string();

            std::replace(includePath.begin(), includePath.end(), '\\', '/');

            std::string content;
            bool res = ReadString(content, includePath);

            shaderc_include_result* result = new shaderc_include_result;
            result->source_name        = _strdup(includePath.c_str());
            result->source_name_length = includePath.length();
            result->content            = _strdup(content.c_str());
            result->content_length     = content.length();
            result->user_data          = nullptr;

            return result;
        }

        // GetInclude で確保したメモリを解放するコード
        void ReleaseInclude(shaderc_include_result* include_result) override
        {
            if (include_result)
            {
                free((void*)include_result->source_name);
                free((void*)include_result->content);
                delete include_result;
            }
        }
    };


    static const char* ToEntryPoint(ShaderStage stage)
    {
        //========================================================================
        // shaderc における GLSLコンパイラでは、エントリーポイントは "main" であると想定され
        // エントリーポイント指定は HLSL 専用の機能になっている
        //========================================================================
        SL_ASSERT(false);


        switch (stage)
        {
            case Silex::SHADER_STAGE_VERTEX_BIT:                 return "vsmain";
            case Silex::SHADER_STAGE_TESSELATION_CONTROL_BIT:    return "tcsmain"; // hull
            case Silex::SHADER_STAGE_TESSELATION_EVALUATION_BIT: return "tesmain"; // domain
            case Silex::SHADER_STAGE_GEOMETRY_BIT:               return "gsmain";
            case Silex::SHADER_STAGE_FRAGMENT_BIT:               return "fsmain";
            case Silex::SHADER_STAGE_COMPUTE_BIT:                return "csmain";
        }

        return "";
    }

    static ShaderStage ToShaderStage(const std::string& type)
    {
        if (type == "VERTEX")   return SHADER_STAGE_VERTEX_BIT;
        if (type == "FRAGMENT") return SHADER_STAGE_FRAGMENT_BIT;
        if (type == "GEOMETRY") return SHADER_STAGE_GEOMETRY_BIT;
        if (type == "COMPUTE")  return SHADER_STAGE_COMPUTE_BIT;

        return SHADER_STAGE_ALL;
    }

    static const char* ToStageString(const ShaderStage stage)
    {
        if (stage & SHADER_STAGE_VERTEX_BIT)   return "VERTEX";
        if (stage & SHADER_STAGE_FRAGMENT_BIT) return "FRAGMENT";
        if (stage & SHADER_STAGE_GEOMETRY_BIT) return "GEOMETRY";
        if (stage & SHADER_STAGE_COMPUTE_BIT)  return "COMPUTE";

        return "SHADER_STAGE_ALL";
    }

    static const char* ToExtention(const ShaderStage stage)
    {
        if (stage & SHADER_STAGE_VERTEX_BIT)   return ".vs";
        if (stage & SHADER_STAGE_FRAGMENT_BIT) return ".fs";
        if (stage & SHADER_STAGE_GEOMETRY_BIT) return ".gs";
        if (stage & SHADER_STAGE_COMPUTE_BIT)  return ".cs";

        return ".undefine";
    }

    static ShaderDataType ToShaderDataType(const spirv_cross::SPIRType& type)
    {
        switch (type.basetype)
        {
            case spirv_cross::SPIRType::Boolean:  return SHADER_DATA_TYPE_BOOL;
            case spirv_cross::SPIRType::Int:
                if (type.vecsize == 1)            return SHADER_DATA_TYPE_INT;
                if (type.vecsize == 2)            return SHADER_DATA_TYPE_IVEC2;
                if (type.vecsize == 3)            return SHADER_DATA_TYPE_IVEC3;
                if (type.vecsize == 4)            return SHADER_DATA_TYPE_IVEC4;

            case spirv_cross::SPIRType::UInt:     return SHADER_DATA_TYPE_UINT;
            case spirv_cross::SPIRType::Float:
                if (type.columns == 3)            return SHADER_DATA_TYPE_MAT3;
                if (type.columns == 4)            return SHADER_DATA_TYPE_MAT4;

                if (type.vecsize == 1)            return SHADER_DATA_TYPE_FLOAT;
                if (type.vecsize == 2)            return SHADER_DATA_TYPE_VEC2;
                if (type.vecsize == 3)            return SHADER_DATA_TYPE_VEC3;
                if (type.vecsize == 4)            return SHADER_DATA_TYPE_VEC4;
        }

        return SHADER_DATA_TYPE_NONE;
    }


#if SHADERC
    static shaderc_shader_kind ToShaderC(const ShaderStage stage)
    {
        switch (stage)
        {
            case SHADER_STAGE_VERTEX_BIT:   return shaderc_vertex_shader;
            case SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
            case SHADER_STAGE_COMPUTE_BIT:  return shaderc_compute_shader;
            case SHADER_STAGE_GEOMETRY_BIT: return shaderc_geometry_shader;
        }

        return {};
    }
#else
    static EShLanguage ToGlslang(const ShaderStage stage)
    {
        switch (stage)
        {
            case SHADER_STAGE_VERTEX_BIT:    return EShLangVertex;
            case SHADER_STAGE_FRAGMENT_BIT:  return EShLangFragment;
            case SHADER_STAGE_COMPUTE_BIT:   return EShLangCompute;
            case SHADER_STAGE_GEOMETRY_BIT:  return EShLangGeometry;
        }

        return {};
    }
#endif

    using _SetIndex = uint32;
    using _Binding  = uint32;

    // ステージ間共有バッファ保存用変数
    static std::unordered_map<_SetIndex, std::unordered_map<_Binding, ShaderBuffer>> ExistUniformBuffers;
    static std::unordered_map<_SetIndex, std::unordered_map<_Binding, ShaderBuffer>> ExistStorageBuffers;
    static bool                                                                      IsExistPushConstant;
    static ShaderReflectionData                                                      ReflectionData;

    static const char* ShaderCacheDirectory = "Assets/Shaders/Cache/";


    ShaderCompiler* ShaderCompiler::Get()
    {
        return instance;
    }

    bool ShaderCompiler::Compile(const std::string& filePath, ShaderCompiledData& out_compiledData)
    {
        bool result = false;

        // キャッシュファイルが無ければ生成
        if (!std::filesystem::exists(ShaderCacheDirectory))
        {
            std::filesystem::create_directory(ShaderCacheDirectory);
        }

        // ファイル読み込み
        std::string rawSource;
        result = ReadString(rawSource, filePath);
        SL_CHECK(!result, false);

        // コンパイル前処理として、ステージごとに分割する
        std::unordered_map<ShaderStage, std::string> parsedRawSources;
        parsedRawSources = _SplitStages(rawSource);

        SL_LOG_TRACE("**************************************************");
        SL_LOG_TRACE("Compile: {}", filePath.c_str());
        SL_LOG_TRACE("**************************************************");

        // ステージごとにコンパイル
        std::unordered_map<ShaderStage, std::vector<uint32>> spirvBinaries;
        for (const auto& [stage, source] : parsedRawSources)
        {
            std::filesystem::path file = filePath;
            std::string cachepath = ShaderCacheDirectory + file.stem().string() + ToExtention(stage) + ".bin";

            // キャッシュファイルが存在し、タイムスタンプが最新であれば使用する
            if (std::filesystem::exists(cachepath) && IsFileCachedRecently(file, cachepath))
            {
                ReadSpvBinary(spirvBinaries[stage], cachepath);
            }
            else
            {
                std::string error = _CompileStage(stage, source, spirvBinaries[stage], filePath);
                if (!error.empty())
                {
                    SL_LOG_ERROR("ShaderCompile: {}", error.c_str());
                    return false;
                }
            }

            // バイナリファイル書き込み（キャッシュ）
            WriteSpvBinary(spirvBinaries[stage], cachepath);
        }

        out_compiledData.reflection.descriptorSets.clear();
        out_compiledData.reflection.pushConstantRanges.clear();
        out_compiledData.reflection.pushConstants.clear();
        out_compiledData.reflection.resources.clear();
        out_compiledData.shaderBinaries.clear();

        ExistUniformBuffers.clear();
        ExistStorageBuffers.clear();
        IsExistPushConstant = false;

        // ステージごとにリフレクション
        for (const auto& [stage, binary] : spirvBinaries)
        {
            _ReflectStage(stage, binary);
        }

        SL_LOG_TRACE("**************************************************");

        // コンパイル結果
        out_compiledData.reflection     = ReflectionData;
        out_compiledData.shaderBinaries = spirvBinaries;

        // リフレクションデータリセット
        ReflectionData.descriptorSets.clear();
        ReflectionData.pushConstantRanges.clear();
        ReflectionData.pushConstants.clear();
        ReflectionData.resources.clear();

        ExistUniformBuffers.clear();
        ExistStorageBuffers.clear();
        IsExistPushConstant = false;

        return true;
    }

    std::unordered_map<ShaderStage, std::string> ShaderCompiler::_SplitStages(const std::string& source)
    {
        std::unordered_map<ShaderStage, std::string> shaderSources;
        std::string type;

        uint64 keywordLength = strlen("#pragma");
        uint64 pos           = source.find("#pragma", 0);
        uint64 lastPos       = 0;

        while (pos != std::string::npos)
        {
            uint64 eol = source.find_first_of("\r\n", pos);
            SL_ASSERT(eol != std::string::npos, "シンタックスエラー: シェーダーステージ指定が見つかりません");

            uint64 begin = pos + keywordLength + 1;
            uint64 end   = source.find_first_of(" \r\n", begin);
            type = source.substr(begin, end - begin);

            SL_ASSERT(type == "VERTEX" || type == "FRAGMENT" || type == "GEOMETRY" || type == "COMPUTE", "無効なシェーダーステージです");

            uint64 nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find("#pragma", nextLinePos);
            
            // #pragma XXXX を取り除いたソースを追加
            shaderSources[ToShaderStage(type)] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos? source.size() - 1 : nextLinePos));
            

            // ステージ間の '{' '}' を取り除く
            // シェーダーステージ間を {} で囲うようにしていたが、シンタックスハイライトが有効にならないので採用しないようにする。
            // そもそも、他のIDEでは全てエラーになってしまうので、Intellij IDEA (glsl拡張有効) に限っての話ですが... 
            #if 0
            {
                size_t startPos = shaderSources[ToShaderStage(type)].find('{');

                if (startPos != std::string::npos)
                    shaderSources[ToShaderStage(type)].erase(startPos, 1);

                size_t endPos = shaderSources[ToShaderStage(type)].rfind('}');

                if (endPos != std::string::npos)
                    shaderSources[ToShaderStage(type)].erase(endPos, 1);
            }
            #endif
        }

        return shaderSources;
    }

    std::string ShaderCompiler::_CompileStage(ShaderStage stage, const std::string& source, std::vector<uint32>& out_putSpirv, const std::string& filepath)
    {
#if SHADERC

        shaderc::Compiler compiler;

        // コンパイル: SPIR-V バイナリ形式へコンパイル
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetWarningsAsErrors();
        options.SetIncluder(std::make_unique<ShaderIncluder>());

        
        const shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(source, ToShaderC(stage), filepath.c_str(), options);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            // エラーあり
            return compileResult.GetErrorMessage();
        }

        // SPIR-V は 4バイトアラインメント
        out_putSpirv = std::vector<uint32>(compileResult.begin(), compileResult.end());

        // エラ-なし
        return {};
#else
        glslang::InitializeProcess();
        EShLanguage glslangStage = ToGlslang(stage);
        EShMessages messages     = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

        const char* sourceStrings[1];
        sourceStrings[0] = source.data();

        glslang::TShader shader(glslangStage);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
        shader.setStrings(sourceStrings, 1);

        bool result = shader.parse(GetDefaultResources(), 100, false, messages);
        if (result)
        {
            return shader.getInfoLog();
        }
        
        glslang::TProgram program;
        program.addShader(&shader);

        result = program.link(messages);
        if (result)
        {
            return shader.getInfoLog();
        }

        glslang::GlslangToSpv(*program.getIntermediate(glslangStage), out_putSpirv);
        glslang::FinalizeProcess();
        return {};
#endif
    }

    void ShaderCompiler::_ReflectStage(ShaderStage stage, const std::vector<uint32>& spirv)
    {
        SL_LOG_TRACE("・{}", ToStageString(stage));

        // リソースデータ取得
        spirv_cross::Compiler compiler(spirv);
        const spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        // ユニフォームバッファ
        for (const spirv_cross::Resource& resource : resources.uniform_buffers)
        {
            const auto activeBuffers = compiler.get_active_buffer_ranges(resource.id);

            if (activeBuffers.size())
            {
                const auto& name           = resource.name;
                const auto& bufferType     = compiler.get_type(resource.base_type_id);
                const uint32 memberCount   = bufferType.member_types.size();
                const uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
                const uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                const uint32 size          = compiler.get_declared_struct_size(bufferType);

                if (descriptorSet >= ReflectionData.descriptorSets.size())
                    ReflectionData.descriptorSets.resize(descriptorSet + 1);

                ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
                if (ExistUniformBuffers[descriptorSet].find(binding) == ExistUniformBuffers[descriptorSet].end())
                {
                    ShaderBuffer uniformBuffer;
                    uniformBuffer.setIndex     = descriptorSet;
                    uniformBuffer.bindingPoint = binding;
                    uniformBuffer.size         = size;
                    uniformBuffer.name         = name;
                    uniformBuffer.stage        = SHADER_STAGE_ALL;

                    ExistUniformBuffers.at(descriptorSet)[binding] = uniformBuffer;
                }
                else
                {
                    ShaderBuffer& uniformBuffer = ExistUniformBuffers.at(descriptorSet).at(binding);
                    if (size > uniformBuffer.size)
                    {
                        uniformBuffer.size = size;
                    }
                }

                shaderDescriptorSet.uniformBuffers[binding] = ExistUniformBuffers.at(descriptorSet).at(binding);

                SL_LOG_TRACE("  (set: {}, bind: {}) uniform {}", descriptorSet, binding, name);
            }
        }

        // ストレージバッファ
        for (const spirv_cross::Resource& resource : resources.storage_buffers)
        {
            const auto activeBuffers = compiler.get_active_buffer_ranges(resource.id);

            if (activeBuffers.size())
            {
                const auto& name           = resource.name;
                const auto& bufferType     = compiler.get_type(resource.base_type_id);
                const uint32 memberCount   = bufferType.member_types.size();
                const uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
                const uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                const uint32 size          = compiler.get_declared_struct_size(bufferType);

                if (descriptorSet >= ReflectionData.descriptorSets.size())
                    ReflectionData.descriptorSets.resize(descriptorSet + 1);

                ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
                if (ExistStorageBuffers[descriptorSet].find(binding) == ExistStorageBuffers[descriptorSet].end())
                {
                    ShaderBuffer storageBuffer;
                    storageBuffer.setIndex     = descriptorSet;
                    storageBuffer.bindingPoint = binding;
                    storageBuffer.size         = size;
                    storageBuffer.name         = name;
                    storageBuffer.stage        = SHADER_STAGE_ALL;

                    ExistStorageBuffers.at(descriptorSet)[binding] = storageBuffer;
                }
                else
                {
                    ShaderBuffer& storageBuffer = ExistStorageBuffers.at(descriptorSet).at(binding);
                    if (size > storageBuffer.size)
                    {
                        storageBuffer.size = size;
                    }
                }

                shaderDescriptorSet.storageBuffers[binding] = ExistStorageBuffers.at(descriptorSet).at(binding);

                SL_LOG_TRACE("  (set: {}, bind: {}) buffer {}", descriptorSet, binding, name);
            }
        }

        // イメージサンプラー
        for (const auto& resource : resources.sampled_images)
        {
            const auto& name     = resource.name;
            const auto& baseType = compiler.get_type(resource.base_type_id);
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = baseType.image.dim;
            uint32 arraySize     = 1;

            if (!type.array.empty())
                arraySize = type.array[0];

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderImage& imageSampler = ReflectionData.descriptorSets[descriptorSet].imageSamplers[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.stage        = stage;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name]; 
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) sampled_image {}", descriptorSet, binding, name);
        }

        // イメージ
        for (const auto& resource : resources.separate_images)
        {
            const auto& name     = resource.name;
            const auto& baseType = compiler.get_type(resource.base_type_id);
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = baseType.image.dim;
            uint32 arraySize     = type.array[0];

            if (arraySize == 0)
                arraySize = 1;

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
            auto& imageSampler        = shaderDescriptorSet.separateTextures[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.stage        = stage;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name];
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) image {}", descriptorSet, binding, name);
        }

        // サンプラー
        for (const auto& resource : resources.separate_samplers)
        {
            const auto& name     = resource.name;
            const auto& baseType = compiler.get_type(resource.base_type_id);
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = baseType.image.dim;
            uint32 arraySize     = type.array[0];

            if (arraySize == 0)
                arraySize = 1;

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
            auto& imageSampler = shaderDescriptorSet.separateSamplers[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.stage        = stage;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name];
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) sampler {}", descriptorSet, binding, name);
        }

        // ストレージイメージ
        for (const auto& resource : resources.storage_images)
        {
            const auto& name     = resource.name;
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = type.image.dim;
            uint32 arraySize     = type.array[0];

            if (arraySize == 0)
                arraySize = 1;

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
            auto& imageSampler = shaderDescriptorSet.storageImages[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;
            imageSampler.stage        = stage;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name];
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) storage_image {}", descriptorSet, binding, name);
        }

        // プッシュ定数
        for (const auto& resource : resources.push_constant_buffers)
        {
            const auto& name       = resource.name;
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32 bufferSize      = compiler.get_declared_struct_size(bufferType);
            uint32 memberCount     = bufferType.member_types.size();
            uint32 bufferOffset    = 0;

            //if (ReflectionData.pushConstantRanges.size())
            //     bufferOffset = ReflectionData.pushConstantRanges.back().offset + ReflectionData.pushConstantRanges.back().size;

            // auto& pushConstantRange  = ReflectionData.pushConstantRanges.emplace_back();

            //================================================================================================================
            // 全シェーダーステージ間で同じレイアウトのプッシュ定数が定義されていることを前提とし、1つの定数レンジで SHADER_STAGE_ALL とする
            // つまり、（ループで1つのデータを更新するので）一番最後にプッシュ定数宣言したステージのデータが全ステージで共有される
            //================================================================================================================
            if (!IsExistPushConstant)
            {
                PushConstantRange& range = ReflectionData.pushConstantRanges.emplace_back();
                range.stage  = SHADER_STAGE_ALL;
                range.size   = bufferSize;
                range.offset = bufferOffset;

                IsExistPushConstant = true;
            }

            //================================================================================================================
            // 以下、メンバー情報のデバッグ情報のため、実際には使用しない（※ステージ間で異なるレイアウトのリフレクションの取得を試みたが、出来なかった）
            //================================================================================================================
            {
                ShaderPushConstant& buffer = ReflectionData.pushConstants[name];
                buffer.name = name;
                buffer.size = bufferSize - bufferOffset;

                SL_LOG_TRACE("  (push_constant) {}", name);

                for (uint32 i = 0; i < memberCount; i++)
                {
                    const auto& memberName = compiler.get_member_name(bufferType.self, i);
                    auto& type             = compiler.get_type(bufferType.member_types[i]);
                    uint32 size            = compiler.get_declared_struct_member_size(bufferType, i);
                    uint32 offset          = compiler.type_struct_member_offset(bufferType, i) - bufferOffset;

                    std::string uniformName = std::format("{}.{}", name, memberName);

                    PushConstantMember& pushConstantMember = buffer.members[uniformName];
                    pushConstantMember.name   = uniformName;
                    pushConstantMember.type   = ToShaderDataType(type);
                    pushConstantMember.offset = offset;
                    pushConstantMember.size   = size;

                    SL_LOG_TRACE("      offset({}), size({}): {}", offset, size, uniformName);
                }
            }
            //================================================================================================================
        }
    }
}
