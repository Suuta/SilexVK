
#include "PCH.h"
#include "Scene/SceneRenderer.h"
#include "Core/Window.h"
#include "Core/Engine.h"
#include "Asset/TextureReader.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingUtility.h"


namespace Silex
{
    namespace UBO
    {
        struct PrifilterParam
        {
            float roughness;
        };

        struct Transform
        {
            glm::mat4 world = glm::mat4(1.0f);
            glm::mat4 view = glm::mat4(1.0f);
            glm::mat4 projection = glm::mat4(1.0f);
        };

        struct ShadowTransformData
        {
            glm::mat4 world = glm::mat4(1.0f);
        };

        struct LightSpaceTransformData
        {
            glm::mat4 cascade[4];
        };

        struct CascadeData
        {
            glm::vec4 cascadePlaneDistances[4];
        };

        struct GridData
        {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec4 pos;
        };

        struct Light
        {
            glm::vec4 directionalLight;
            glm::vec4 cameraPos;
        };

        struct EquirectangularData
        {
            glm::mat4 view[6];
            glm::mat4 projection;
        };

        struct MaterialUBO
        {
            glm::vec3 albedo;
            float     metallic;
            glm::vec3 emission;
            float     roughness;
            glm::vec2 textureTiling;
        };

        struct SceneUBO
        {
            glm::vec4 lightDir;
            glm::vec4 lightColor;
            glm::vec4 cameraPosition;
            glm::mat4 view;
            glm::mat4 invViewProjection;
        };

        struct EnvironmentUBO
        {
            glm::mat4 view;
            glm::mat4 projection;
        };

        struct CompositUBO
        {
            float exposure;
        };

        struct BloomPrefilterData
        {
            float threshold;
        };

        struct BloomDownSamplingData
        {
            glm::vec2 sourceResolution;
        };

        struct BloomUpSamplingData
        {
            float filterRadius;
        };

        struct BloomData
        {
            float intencity;
        };
    }

    SceneRenderer::SceneRenderer()
    {
        api = Renderer::Get()->GetAPI();
    }

    SceneRenderer::~SceneRenderer()
    {
        api = nullptr;
    }


    void SceneRenderer::SetSkyLight(const SkyLightComponent& data)
    {
        skyLight = data;
    }

    void SceneRenderer::SetDirectionalLight(const DirectionalLightComponent& data)
    {
        directionalLight   = data;
        shouldRenderShadow = true;
    }

    void SceneRenderer::SetPostProcess(const PostProcessComponent& data)
    {
        postProcess       = data;
        enablePostProcess = true;
    }

    void SceneRenderer::AddMeshDrawList(const MeshDrawData& data)
    {
        meshDrawList.emplace_back(data);
        stats.numRenderMesh++;
    }

    void SceneRenderer::Initialize()
    {
        _InitializePasses();
    }

    void SceneRenderer::Finalize()
    {
        _FinalizeePasses();
    }

    DescriptorSet* SceneRenderer::GetSceneFinalOutput()
    {
        return imageSet;
    }

    SceneRenderStats SceneRenderer::GetRenderStats()
    {
        return stats;
    }

    void SceneRenderer::_InitializePasses()
    {
        auto size = Window::Get()->GetSize();

        cubeMesh   = MeshFactory::Cube();
        sponzaMesh = MeshFactory::Sponza();

        defaultLayout.Binding(0);
        defaultLayout.Attribute(0, VERTEX_BUFFER_FORMAT_R32G32B32);
        defaultLayout.Attribute(1, VERTEX_BUFFER_FORMAT_R32G32B32);
        defaultLayout.Attribute(2, VERTEX_BUFFER_FORMAT_R32G32);
        defaultLayout.Attribute(3, VERTEX_BUFFER_FORMAT_R32G32B32);
        defaultLayout.Attribute(4, VERTEX_BUFFER_FORMAT_R32G32B32);

        linearSampler = Renderer::Get()->CreateSampler(SAMPLER_FILTER_LINEAR, SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);
        shadowSampler = Renderer::Get()->CreateSampler(SAMPLER_FILTER_LINEAR, SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE, true, COMPARE_OP_LESS_OR_EQUAL);

        TextureReader reader;
        byte* pixels;

        pixels = reader.Read("Assets/Textures/default.png");
        defaultTexture     = Renderer::Get()->CreateTextureFromMemory(pixels, reader.data.byteSize, reader.data.width, reader.data.height, true);
        defaultTextureView = Renderer::Get()->CreateTextureView(defaultTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        reader.Unload(pixels);

        // ID リードバック
        pixelIDBuffer = Renderer::Get()->CreateBuffer(nullptr, sizeof(int32));

        // IBL
        _PrepareIBL("Assets/Textures/cloud.png");

        // シャドウマップ
        shadow = slnew(ShadowData);
        _PrepareShadowBuffer();

        // Gバッファ
        gbuffer = slnew(GBufferData);
        _PrepareGBuffer(size.x, size.y);

        // ライティングバッファ
        lighting = slnew(LightingData);
        _PrepareLightingBuffer(size.x, size.y);

        // 環境マップ
        environment = slnew(EnvironmentData);
        _PrepareEnvironmentBuffer(size.x, size.y);

        // ブルーム
        bloom = slnew(BloomData);
        _PrepareBloomBuffer(size.x, size.y);

        {
            // グリッド
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .Depth(true, false)
                .Blend(true, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Grid.glsl", compiledData);
            gridShader   = api->CreateShader(compiledData);
            gridPipeline = api->CreateGraphicsPipeline(gridShader, &pipelineInfo, environment->pass);
            gridUBO      = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::GridData));

            gridSet = Renderer::Get()->CreateDescriptorSet(gridShader, 0);
            gridSet->SetResource(0, gridUBO);
            gridSet->Flush();
        }

        {
            // コンポジット
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp = ATTACHMENT_STORE_OP_STORE;
            color.samples = TEXTURE_SAMPLES_1;
            color.format = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);

            RenderPassClearValue clear = {};
            clear.SetFloat(0, 0, 0, 1);

            compositePass    = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);
            compositeTexture = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, size.x, size.y);

            auto hcomposite      = compositeTexture->GetHandle();
            compositeTextureView = Renderer::Get()->CreateTextureView(compositeTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            compositeFB          = Renderer::Get()->CreateFramebuffer(compositePass, 1, &hcomposite, size.x, size.y);

            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Composit.glsl", compiledData);
            compositeShader = api->CreateShader(compiledData);
            compositePipeline = api->CreateGraphicsPipeline(compositeShader, &pipelineInfo, compositePass);

            compositeSet = Renderer::Get()->CreateDescriptorSet(compositeShader, 0);
            compositeSet->SetResource(0, bloom->bloomView, linearSampler);
            compositeSet->Flush();
        }

        // ImGui ビューポート用 デスクリプターセット
        imageSet = Renderer::Get()->CreateDescriptorSet(compositeShader, 0);
        imageSet->SetResource(0, compositeTextureView, linearSampler);
        imageSet->Flush();
    }

    void SceneRenderer::_FinalizeePasses()
    {
        api->WaitDevice();

        sldelete(cubeMesh);
        sldelete(sponzaMesh);

        _CleanupIBL();
        _CleanupShadowBuffer();
        _CleanupGBuffer();
        _CleanupLightingBuffer();
        _CleanupEnvironmentBuffer();
        _CleanupBloomBuffer();

        sldelete(shadow);
        sldelete(gbuffer);
        sldelete(lighting);
        sldelete(environment);
        sldelete(bloom);

        Renderer::Get()->DestroyBuffer(gridUBO);
        Renderer::Get()->DestroyBuffer(pixelIDBuffer);

        Renderer::Get()->DestroyTexture(defaultTexture);
        Renderer::Get()->DestroyTexture(compositeTexture);

        Renderer::Get()->DestroyTextureView(defaultTextureView);
        Renderer::Get()->DestroyTextureView(compositeTextureView);
        api->DestroyShader(gridShader);
        api->DestroyShader(compositeShader);

        Renderer::Get()->DestroyDescriptorSet(gridSet);
        Renderer::Get()->DestroyDescriptorSet(compositeSet);
        Renderer::Get()->DestroyDescriptorSet(imageSet);

        api->DestroyPipeline(gridPipeline);
        api->DestroyPipeline(compositePipeline);
        api->DestroyRenderPass(compositePass);
        api->DestroyFramebuffer(compositeFB);
        Renderer::Get()->DestroySampler(linearSampler);
        Renderer::Get()->DestroySampler(shadowSampler);
    }

    void SceneRenderer::_PrepareIBL(const char* environmentTexturePath)
    {
        TextureReader reader;
        byte* pixels;

        pixels = reader.Read(environmentTexturePath);
        envTexture     = Renderer::Get()->CreateTextureFromMemory(pixels, reader.data.byteSize, reader.data.width, reader.data.height, true);
        envTextureView = Renderer::Get()->CreateTextureView(envTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);

        // 環境マップ変換 レンダーパス
        Attachment color = {};
        color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
        color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        color.loadOp        = ATTACHMENT_LOAD_OP_DONT_CARE;
        color.storeOp       = ATTACHMENT_STORE_OP_STORE;
        color.samples       = TEXTURE_SAMPLES_1;
        color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

        AttachmentReference colorRef = {};
        colorRef.attachment = 0;
        colorRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        Subpass subpass = {};
        subpass.colorReferences.push_back(colorRef);

        RenderPassClearValue clear;
        clear.SetFloat(0, 0, 0, 1);
        IBLProcessPass = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);

        // シェーダー
        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/EquirectangularToCubeMap.glsl", compiledData);
        equirectangularShader = api->CreateShader(compiledData);

        // キューブマップ
        const uint32 envResolution = 2048;
        cubemapTexture     = Renderer::Get()->CreateTextureCube(RENDERING_FORMAT_R8G8B8A8_UNORM, envResolution, envResolution, true);
        cubemapTextureView = Renderer::Get()->CreateTextureView(cubemapTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT);

        // 一時ビュー (キューブマップがミップレベルをもつため、コピー操作時に単一ミップを対象としたビューが別途必要)
        TextureView* captureView = Renderer::Get()->CreateTextureView(cubemapTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT, 0, 6, 0, 1);

        // キューブマップ UBO
        UBO::EquirectangularData data;
        data.projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1.0f);
        data.view[0] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        data.view[1] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        data.view[2] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        data.view[3] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        data.view[4] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        data.view[5] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        equirectangularUBO = Renderer::Get()->CreateUniformBuffer(&data, sizeof(UBO::EquirectangularData));

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        equirectangularPipeline = api->CreateGraphicsPipeline(equirectangularShader, &pipelineInfo, IBLProcessPass);

        // デスクリプタ
        equirectangularSet = Renderer::Get()->CreateDescriptorSet(equirectangularShader, 0);
        equirectangularSet->SetResource(0, equirectangularUBO);
        equirectangularSet->SetResource(1, envTextureView, linearSampler);
        equirectangularSet->Flush();

        auto hcubemap = cubemapTexture->GetHandle();
        IBLProcessFB = Renderer::Get()->CreateFramebuffer(IBLProcessPass, 1, &hcubemap, envResolution, envResolution);

        // キューブマップ書き込み
        Renderer::Get()->ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            // イメージレイアウトを read_only に移行させる
            TextureBarrierInfo info = {};
            info.texture = cubemapTexture->GetHandle();
            info.subresources = {};
            info.srcAccess = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout = TEXTURE_LAYOUT_UNDEFINED;
            info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            //-------------------------------------------------------------------------------------------------------------------------
            // ↑このレイアウト移行を実行しない場合、shader_read_only に移行していなので、リソースの利用時に バリデーションエラーが発生する
            // レンダーパスとして mip[0] のみを使用した場合はその例外となる状態が発生した。（mip[0]を書き込みに使用し、残りのレベルは Blitでコピー）
            //-------------------------------------------------------------------------------------------------------------------------
            // 移行しない場合、イメージのすべてのサブリソースが undefine 状態であり、その状態で書き込み後のレイアウト遷移では shader_read_only を期待している
            // ので、すべてのミップレベルでレイアウトに起因するバリデーションエラーが発生するはずだが、mip[0] のみ エラーが発生しなかった。
            // また、レイアウトを mip[1~N] のように mip[0] を除いて移行したとしても、エラーは発生しなかった。
            // 
            // つまり、mip[0] が暗黙的に shader_read_only に移行しているのか、どこかで移行しているのを見逃しているかもしれないし
            // ドライバーが暗黙的に移行させているのかもしれない。（AMDなら検証できるのかも？）
            //-------------------------------------------------------------------------------------------------------------------------


            MeshSource* ms = cubeMesh->GetMeshSource();

            api->Cmd_SetViewport(cmd, 0, 0, envResolution, envResolution);
            api->Cmd_SetScissor(cmd, 0, 0, envResolution, envResolution);

            auto* view = captureView->GetHandle();
            api->Cmd_BeginRenderPass(cmd, IBLProcessPass, IBLProcessFB, 1, &view);
            api->Cmd_BindPipeline(cmd, equirectangularPipeline);
            api->Cmd_BindDescriptorSet(cmd, equirectangularSet->GetHandle(Renderer::Get()->GetCurrentFrameIndex()), 0);
            api->Cmd_BindVertexBuffer(cmd, ms->GetVertexBuffer()->GetHandle(), 0);
            api->Cmd_BindIndexBuffer(cmd, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);
            api->Cmd_DrawIndexed(cmd, ms->GetIndexCount(), 1, 0, 0, 0);
            api->Cmd_EndRenderPass(cmd);

            info = {};
            info.texture = cubemapTexture->GetHandle();
            info.subresources = {};
            info.srcAccess = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.newLayout = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            Renderer::Get()->_GenerateMipmaps(cmd, cubemapTexture->GetHandle(), envResolution, envResolution, 1, 6, TEXTURE_ASPECT_COLOR_BIT);

            info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
        });

        // キャプチャ用テンポラリビュー破棄
        Renderer::Get()->DestroyTextureView(captureView);

        _CreateIrradiance();
        _CreatePrefilter();
        _CreateBRDF();
    }

    void SceneRenderer::_CreateIrradiance()
    {
        const uint32 irradianceResolution = 32;

        // シェーダー
        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/Irradiance.glsl", compiledData);
        irradianceShader = api->CreateShader(compiledData);

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        irradiancePipeline = api->CreateGraphicsPipeline(irradianceShader, &pipelineInfo, IBLProcessPass);

        // キューブマップ
        irradianceTexture     = Renderer::Get()->CreateTextureCube(RENDERING_FORMAT_R8G8B8A8_UNORM, irradianceResolution, irradianceResolution, false);
        irradianceTextureView = Renderer::Get()->CreateTextureView(irradianceTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT);

        // バッファを再利用するため、リサイズ
        api->DestroyFramebuffer(IBLProcessFB);
        auto hirradiance = irradianceTexture->GetHandle();
        IBLProcessFB = api->CreateFramebuffer(IBLProcessPass, 1, &hirradiance, irradianceResolution, irradianceResolution);

        // デスクリプタ
        irradianceSet = Renderer::Get()->CreateDescriptorSet(irradianceShader, 0);
        irradianceSet->SetResource(0, equirectangularUBO);
        irradianceSet->SetResource(1, cubemapTextureView, linearSampler);
        irradianceSet->Flush();

        // キューブマップ書き込み
        Renderer::Get()->ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            MeshSource* ms = cubeMesh->GetMeshSource();

            api->Cmd_SetViewport(cmd, 0, 0, irradianceResolution, irradianceResolution);
            api->Cmd_SetScissor(cmd, 0, 0, irradianceResolution, irradianceResolution);

            auto* view = irradianceTextureView->GetHandle();
            api->Cmd_BeginRenderPass(cmd, IBLProcessPass, IBLProcessFB, 1, &view);
            api->Cmd_BindPipeline(cmd, irradiancePipeline);
            api->Cmd_BindDescriptorSet(cmd, irradianceSet->GetHandle(Renderer::Get()->GetCurrentFrameIndex()), 0);
            api->Cmd_BindVertexBuffer(cmd, ms->GetVertexBuffer()->GetHandle(), 0);
            api->Cmd_BindIndexBuffer(cmd, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);
            api->Cmd_DrawIndexed(cmd, ms->GetIndexCount(), 1, 0, 0, 0);
            api->Cmd_EndRenderPass(cmd);
        });
    }

    void SceneRenderer::_CreatePrefilter()
    {
        const uint32 prefilterResolution = 256;
        const uint32 prefilterMipCount = 5;
        const auto   miplevels = RenderingUtility::CalculateMipmap(prefilterResolution, prefilterResolution);

        // シェーダー
        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/Prefilter.glsl", compiledData);
        prefilterShader = api->CreateShader(compiledData);

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        prefilterPipeline = api->CreateGraphicsPipeline(prefilterShader, &pipelineInfo, IBLProcessPass);

        // キューブマップ
        prefilterTexture = Renderer::Get()->CreateTextureCube(RENDERING_FORMAT_R8G8B8A8_UNORM, prefilterResolution, prefilterResolution, true);

        // 使用するミップレベル分だけ (5個[0 ~ 4])
        prefilterTextureView = Renderer::Get()->CreateTextureView(prefilterTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT, 0, 6, 0, prefilterMipCount);
        auto hprefilter = prefilterTexture->GetHandle();

        // 生成用ビュー（ミップレベル分用意）
        std::array<TextureView*, prefilterMipCount> prefilterViews;
        for (uint32 i = 0; i < prefilterMipCount; i++)
        {
            prefilterViews[i] = Renderer::Get()->CreateTextureView(prefilterTexture, TEXTURE_TYPE_CUBE, TEXTURE_ASPECT_COLOR_BIT, 0, 6, i, 1);
        }

        // 生成用フレームバッファ（ミップレベル分用意）
        std::array<FramebufferHandle*, prefilterMipCount> prefilterFBs;
        for (uint32 i = 0; i < prefilterMipCount; i++)
        {
            prefilterFBs[i] = api->CreateFramebuffer(IBLProcessPass, 1, &hprefilter, miplevels[i].width, miplevels[i].height);
        }

        // デスクリプタ
        prefilterSet = Renderer::Get()->CreateDescriptorSet(prefilterShader, 0);
        prefilterSet->SetResource(0, equirectangularUBO);
        prefilterSet->SetResource(1, cubemapTextureView, linearSampler);
        prefilterSet->Flush();

        // キューブマップ書き込み
        Renderer::Get()->ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            MeshSource* ms = cubeMesh->GetMeshSource();
            api->Cmd_BindVertexBuffer(cmd, ms->GetVertexBuffer()->GetHandle(), 0);
            api->Cmd_BindIndexBuffer(cmd, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);

            for (uint32 i = 0; i < prefilterMipCount; i++)
            {
                api->Cmd_SetViewport(cmd, 0, 0, miplevels[i].width, miplevels[i].height);
                api->Cmd_SetScissor(cmd, 0, 0, miplevels[i].width, miplevels[i].height);

                auto* view = prefilterViews[i]->GetHandle();
                api->Cmd_BeginRenderPass(cmd, IBLProcessPass, prefilterFBs[i], 1, &view);
                api->Cmd_BindPipeline(cmd, prefilterPipeline);
                api->Cmd_BindDescriptorSet(cmd, prefilterSet->GetHandle(Renderer::Get()->GetCurrentFrameIndex()), 0);

                //-------------------------------------------------------
                // firstInstance でミップレベルを指定（シェーダー内でラフネス計算）
                //-------------------------------------------------------
                api->Cmd_DrawIndexed(cmd, ms->GetIndexCount(), 1, 0, 0, i);
                api->Cmd_EndRenderPass(cmd);
            }

            //==========================================================================================
            // ミップ 0~4 は 'SHADER_READ_ONLY' (移行済み) なので、oldLayout は 'UNDEFINED' ではないが
            // 現状は 再度 0~ALL に対して 'UNDEFINED' -> 'SHADER_READ_ONLY' を実行しても 検証エラーが出ない
            // Vulkan の仕様なのか、NVIDIA だから問題ないのかは 現状不明
            //==========================================================================================
            //TextureBarrierInfo info = {};
            //info.texture      = prefilterTexture;
            //info.subresources = {};
            //info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            //info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            //info.oldLayout    = TEXTURE_LAYOUT_UNDEFINED;
            //info.newLayout    = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            //api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
        });

        // テンポラリオブジェクト破棄
        for (uint32 i = 0; i < prefilterMipCount; i++)
        {
            api->DestroyFramebuffer(prefilterFBs[i]);
            Renderer::Get()->DestroyTextureView(prefilterViews[i]);
        }
    }

    void SceneRenderer::_CreateBRDF()
    {
        const uint32 brdfResolution = 512;

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/IBL/BRDF.glsl", compiledData);
        brdflutShader = api->CreateShader(compiledData);

        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        brdflutPipeline = api->CreateGraphicsPipeline(brdflutShader, &pipelineInfo, IBLProcessPass);
        brdflutTexture  = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, brdfResolution, brdfResolution, false);

        auto hbrdf = brdflutTexture->GetHandle();
        brdflutTextureView = Renderer::Get()->CreateTextureView(brdflutTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);

        // バッファを再利用するため、リサイズ
        api->DestroyFramebuffer(IBLProcessFB);
        IBLProcessFB = api->CreateFramebuffer(IBLProcessPass, 1, &hbrdf, brdfResolution, brdfResolution);

        // キューブマップ書き込み
        Renderer::Get()->ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            MeshSource* ms = cubeMesh->GetMeshSource();

            api->Cmd_SetViewport(cmd, 0, 0, brdfResolution, brdfResolution);
            api->Cmd_SetScissor(cmd, 0, 0, brdfResolution, brdfResolution);

            auto* view = brdflutTextureView->GetHandle();
            api->Cmd_BeginRenderPass(cmd, IBLProcessPass, IBLProcessFB, 1, &view);
            api->Cmd_BindPipeline(cmd, brdflutPipeline);
            api->Cmd_Draw(cmd, 3, 1, 0, 0);
            api->Cmd_EndRenderPass(cmd);
        });
    }

    void SceneRenderer::_PrepareShadowBuffer()
    {
        Attachment depth = {};
        depth.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
        depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depth.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
        depth.samples       = TEXTURE_SAMPLES_1;
        depth.format        = RENDERING_FORMAT_D32_SFLOAT;

        AttachmentReference depthRef = {};
        depthRef.attachment = 0;
        depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        Subpass subpass = {};
        subpass.depthstencilReference = depthRef;

        SubpassDependency dep = {};
        dep.srcSubpass = RENDER_AUTO_ID;
        dep.dstSubpass = 0;
        dep.srcStages  = PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstStages  = PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccess  = BARRIER_ACCESS_SHADER_READ_BIT;
        dep.dstAccess  = BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        RenderPassClearValue clear;
        clear.SetDepthStencil(1.0f, 0);
        shadow->pass  = api->CreateRenderPass(1, &depth, 1, &subpass, 1, &dep, 1, &clear);
        shadow->depth = Renderer::Get()->CreateTexture2DArray(RENDERING_FORMAT_D32_SFLOAT, shadowMapResolution, shadowMapResolution, 4);

        auto hdepth = shadow->depth->GetHandle();
        shadow->depthView   = Renderer::Get()->CreateTextureView(shadow->depth, TEXTURE_TYPE_2D_ARRAY, TEXTURE_ASPECT_DEPTH_BIT);
        shadow->framebuffer = Renderer::Get()->CreateFramebuffer(shadow->pass, 1, &hdepth, shadowMapResolution, shadowMapResolution);

        shadow->transformUBO      = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::ShadowTransformData));
        shadow->lightTransformUBO = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::LightSpaceTransformData));
        shadow->cascadeUBO        = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::CascadeData));

        // パイプライン
        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .InputLayout(1, &defaultLayout)
            .Depth(true, true)
            .RasterizerDepthBias(true, 1.0, 2.0)
            .Blend(false, 1)
            .Value();

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/DirectionalLight.glsl", compiledData);
        shadow->shader   = api->CreateShader(compiledData);
        shadow->pipeline = api->CreateGraphicsPipeline(shadow->shader, &pipelineInfo, shadow->pass);

        // デスクリプター
        shadow->set = Renderer::Get()->CreateDescriptorSet(shadow->shader, 0);
        shadow->set->SetResource(0, shadow->transformUBO);
        shadow->set->SetResource(1, shadow->lightTransformUBO);
        shadow->set->Flush();
    }

    void SceneRenderer::_CleanupShadowBuffer()
    {
        Renderer::Get()->DestroyBuffer(shadow->transformUBO);
        Renderer::Get()->DestroyBuffer(shadow->lightTransformUBO);
        Renderer::Get()->DestroyBuffer(shadow->cascadeUBO);
        Renderer::Get()->DestroyTexture(shadow->depth);

        Renderer::Get()->DestroyTextureView(shadow->depthView);

        api->DestroyShader(shadow->shader);
        api->DestroyPipeline(shadow->pipeline);
        Renderer::Get()->DestroyDescriptorSet(shadow->set);
        api->DestroyRenderPass(shadow->pass);
        api->DestroyFramebuffer(shadow->framebuffer);
    }

    void SceneRenderer::_CalculateLightSapceMatrices(glm::vec3 directionalLightDir, Camera* camera, std::array<glm::mat4, 4>& out_result)
    {
        auto nearP = camera->GetNearPlane();
        auto farP = camera->GetFarPlane();

        out_result[0] = (_GetLightSpaceMatrix(directionalLightDir, camera, nearP, shadowCascadeLevels[0]));
        out_result[1] = (_GetLightSpaceMatrix(directionalLightDir, camera, shadowCascadeLevels[0], shadowCascadeLevels[1]));
        out_result[2] = (_GetLightSpaceMatrix(directionalLightDir, camera, shadowCascadeLevels[1], shadowCascadeLevels[2]));
        out_result[3] = (_GetLightSpaceMatrix(directionalLightDir, camera, shadowCascadeLevels[2], farP));
    }

    glm::mat4 SceneRenderer::_GetLightSpaceMatrix(glm::vec3 directionalLightDir, Camera* camera, const float nearPlane, const float farPlane)
    {
        glm::mat4 proj = glm::perspective(glm::radians(camera->GetFOV()), (float)sceneViewportSize.x / (float)sceneViewportSize.y, nearPlane, farPlane);

        std::array<glm::vec4, 8> corners;
        _GetFrustumCornersWorldSpace(proj * camera->GetViewMatrix(), corners);

        glm::vec3 center = glm::vec3(0, 0, 0);
        for (const auto& v : corners)
        {
            center += glm::vec3(v);
        }

        center /= corners.size();

        const auto lightView = glm::lookAt(center + glm::normalize(directionalLightDir), center, glm::vec3(0.0f, 1.0f, 0.0f));
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();

        for (const auto& v : corners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        constexpr float zMult = 10.0f;

        if (minZ < 0) minZ *= zMult;
        else          minZ /= zMult;

        if (maxZ < 0) maxZ /= zMult;
        else          maxZ *= zMult;

        const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        return lightProjection * lightView;
    }

    void SceneRenderer::_GetFrustumCornersWorldSpace(const glm::mat4& projview, std::array<glm::vec4, 8>& out_result)
    {
        const glm::mat4 inv = glm::inverse(projview);
        uint32 index = 0;

        for (uint32 x = 0; x < 2; ++x)
        {
            for (uint32 y = 0; y < 2; ++y)
            {
                for (uint32 z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    out_result[index] = (pt / pt.w);
                    index++;
                }
            }
        }
    }

    void SceneRenderer::_PrepareGBuffer(uint32 width, uint32 height)
    {
        gbuffer->albedo   = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, width, height);
        gbuffer->normal   = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, width, height);
        gbuffer->emission = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32, width, height);
        gbuffer->id       = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R32_SINT, width, height, false, TEXTURE_USAGE_COPY_SRC_BIT);
        gbuffer->depth    = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_D32_SFLOAT, width, height);

        gbuffer->albedoView   = Renderer::Get()->CreateTextureView(gbuffer->albedo, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->normalView   = Renderer::Get()->CreateTextureView(gbuffer->normal, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->emissionView = Renderer::Get()->CreateTextureView(gbuffer->emission, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->idView       = Renderer::Get()->CreateTextureView(gbuffer->id, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->depthView    = Renderer::Get()->CreateTextureView(gbuffer->depth, TEXTURE_TYPE_2D, TEXTURE_ASPECT_DEPTH_BIT);

        RenderPassClearValue clearvalues[5] = {};
        Attachment           attachments[5] = {};
        Subpass              subpass = {};

        {
            // ベースカラー
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(colorRef);
            attachments[0] = color;
            clearvalues[0].SetFloat(0, 0, 0, 0);

            // ノーマル
            Attachment normal = {};
            normal.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            normal.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normal.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            normal.storeOp       = ATTACHMENT_STORE_OP_STORE;
            normal.samples       = TEXTURE_SAMPLES_1;
            normal.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference normalRef = {};
            normalRef.attachment = 1;
            normalRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(normalRef);
            attachments[1] = normal;
            clearvalues[1].SetFloat(0, 0, 0, 1);

            // エミッション
            Attachment emission = {};
            emission.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            emission.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            emission.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            emission.storeOp       = ATTACHMENT_STORE_OP_STORE;
            emission.samples       = TEXTURE_SAMPLES_1;
            emission.format        = RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32;

            AttachmentReference emissionRef = {};
            emissionRef.attachment = 2;
            emissionRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(emissionRef);
            attachments[2] = emission;
            clearvalues[2].SetFloat(0, 0, 0, 1);

            // ID
            Attachment id = {};
            id.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            id.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            id.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            id.storeOp       = ATTACHMENT_STORE_OP_STORE;
            id.samples       = TEXTURE_SAMPLES_1;
            id.format        = RENDERING_FORMAT_R32_SINT;

            AttachmentReference idRef = {};
            idRef.attachment = 3;
            idRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(idRef);
            attachments[3] = id;
            clearvalues[3].SetInt(10, 0, 0, 1);

            // 深度
            Attachment depth = {};
            depth.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
            depth.samples       = TEXTURE_SAMPLES_1;
            depth.format        = RENDERING_FORMAT_D32_SFLOAT;

            AttachmentReference depthRef = {};
            depthRef.attachment = 4;
            depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            subpass.depthstencilReference = depthRef;
            attachments[4] = depth;
            clearvalues[4].SetDepthStencil(1.0f, 0);

            gbuffer->pass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr, std::size(clearvalues), clearvalues);
        }

        {
            TextureHandle* attachments[] = {
                gbuffer->albedo->GetHandle(),
                gbuffer->normal->GetHandle(),
                gbuffer->emission->GetHandle(),
                gbuffer->id->GetHandle(),
                gbuffer->depth->GetHandle(),
            };

            gbuffer->framebuffer = Renderer::Get()->CreateFramebuffer(gbuffer->pass, std::size(attachments), attachments, width, height);
        }

        {
            // Gバッファ―
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &defaultLayout)
                .Depth(true, true, COMPARE_OP_LESS)
                .Blend(false, 4)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/DeferredPrimitive.glsl", compiledData);
            gbuffer->shader   = api->CreateShader(compiledData);
            gbuffer->pipeline = api->CreateGraphicsPipeline(gbuffer->shader, &pipelineInfo, gbuffer->pass);
        }

        // ユニフォームバッファ
        gbuffer->transformUBO = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::Transform));
        gbuffer->materialUBO  = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::MaterialUBO));

        // トランスフォーム
        gbuffer->transformSet = Renderer::Get()->CreateDescriptorSet(gbuffer->shader, 0);
        gbuffer->transformSet->SetResource(0, gbuffer->transformUBO);
        gbuffer->transformSet->Flush();

        // マテリアル
        gbuffer->materialSet = Renderer::Get()->CreateDescriptorSet(gbuffer->shader, 1);
        gbuffer->materialSet->SetResource(0, gbuffer->materialUBO);
        gbuffer->materialSet->SetResource(1, defaultTextureView, linearSampler);
        gbuffer->materialSet->Flush();
    }

    void SceneRenderer::_PrepareLightingBuffer(uint32 width, uint32 height)
    {
        // 初期レイアウトとサブパスレイアウトの違い
        //https://community.khronos.org/t/initial-layout-in-vkattachmentdescription/109850/5

        // パス
        Attachment color = {};
        color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
        color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp       = ATTACHMENT_STORE_OP_STORE;
        color.samples       = TEXTURE_SAMPLES_1;
        color.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

        AttachmentReference colorRef = {};
        colorRef.attachment = 0;
        colorRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        Subpass subpass = {};
        subpass.colorReferences.push_back(colorRef);

        RenderPassClearValue clear;
        clear.SetFloat(0, 0, 0, 1);

        lighting->pass  = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);
        lighting->color = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);

        auto hcolor = lighting->color->GetHandle();
        lighting->view        = Renderer::Get()->CreateTextureView(lighting->color, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        lighting->framebuffer = Renderer::Get()->CreateFramebuffer(lighting->pass, 1, &hcolor, width, height);

        // パイプライン
        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .Depth(false, false)
            .Blend(false, 1)
            .Value();

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/DeferredLighting.glsl", compiledData);
        lighting->shader   = api->CreateShader(compiledData);
        lighting->pipeline = api->CreateGraphicsPipeline(lighting->shader, &pipelineInfo, lighting->pass);
        lighting->sceneUBO = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::SceneUBO));

        // セット
        lighting->set = Renderer::Get()->CreateDescriptorSet(lighting->shader, 0);
        lighting->set->SetResource( 0, gbuffer->albedoView, linearSampler);
        lighting->set->SetResource( 1, gbuffer->normalView, linearSampler);
        lighting->set->SetResource( 2, gbuffer->emissionView, linearSampler);
        lighting->set->SetResource( 3, gbuffer->depthView, linearSampler);
        lighting->set->SetResource( 4, irradianceTextureView, linearSampler);
        lighting->set->SetResource( 5, prefilterTextureView, linearSampler);
        lighting->set->SetResource( 6, brdflutTextureView, linearSampler);
        lighting->set->SetResource( 7, shadow->depthView, shadowSampler);
        lighting->set->SetResource( 8, lighting->sceneUBO);
        lighting->set->SetResource( 9, shadow->cascadeUBO);
        lighting->set->SetResource(10, shadow->lightTransformUBO);
        lighting->set->Flush();
    }

    void SceneRenderer::_PrepareEnvironmentBuffer(uint32 width, uint32 height)
    {
        {
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_LOAD;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Attachment depth = {};
            depth.initialLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.loadOp        = ATTACHMENT_LOAD_OP_LOAD;
            depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
            depth.samples       = TEXTURE_SAMPLES_1;
            depth.format        = RENDERING_FORMAT_D32_SFLOAT;

            AttachmentReference depthRef = {};
            depthRef.attachment = 1;
            depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);
            subpass.depthstencilReference = depthRef;

            RenderPassClearValue clear[2] = {};
            clear[0].SetFloat(0, 0, 0, 1);
            clear[1].SetDepthStencil(1, 0);

            Attachment attachments[] = { color, depth };
            environment->pass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr, 2, clear);

            TextureHandle* textures[] = { lighting->color->GetHandle(), gbuffer->depth->GetHandle() };
            environment->framebuffer = Renderer::Get()->CreateFramebuffer(environment->pass, std::size(textures), textures, width, height);
        }

        {
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &defaultLayout)
                .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
                .Depth(true, false, COMPARE_OP_LESS_OR_EQUAL)
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Environment.glsl", compiledData);
            environment->shader   = api->CreateShader(compiledData);
            environment->pipeline = api->CreateGraphicsPipeline(environment->shader, &pipelineInfo, environment->pass);
            environment->ubo      = Renderer::Get()->CreateUniformBuffer(nullptr, sizeof(UBO::EnvironmentUBO));

            environment->set = Renderer::Get()->CreateDescriptorSet(environment->shader, 0);
            environment->set->SetResource(0, environment->ubo);
            environment->set->SetResource(1, cubemapTextureView, linearSampler);
            environment->set->Flush();
        }
    }

    void SceneRenderer::_CleanupIBL()
    {
        Renderer::Get()->DestroyTexture(envTexture);
        Renderer::Get()->DestroyTextureView(envTextureView);

        api->DestroyFramebuffer(IBLProcessFB);
        api->DestroyRenderPass(IBLProcessPass);
        Renderer::Get()->DestroyBuffer(equirectangularUBO);

        api->DestroyPipeline(equirectangularPipeline);
        api->DestroyShader(equirectangularShader);
        Renderer::Get()->DestroyDescriptorSet(equirectangularSet);
        Renderer::Get()->DestroyTexture(cubemapTexture);
        Renderer::Get()->DestroyTextureView(cubemapTextureView);

        api->DestroyPipeline(irradiancePipeline);
        api->DestroyShader(irradianceShader);
        Renderer::Get()->DestroyDescriptorSet(irradianceSet);
        Renderer::Get()->DestroyTexture(irradianceTexture);
        Renderer::Get()->DestroyTextureView(irradianceTextureView);

        api->DestroyPipeline(prefilterPipeline);
        api->DestroyShader(prefilterShader);
        Renderer::Get()->DestroyDescriptorSet(prefilterSet);
        Renderer::Get()->DestroyTexture(prefilterTexture);
        Renderer::Get()->DestroyTextureView(prefilterTextureView);

        api->DestroyPipeline(brdflutPipeline);
        api->DestroyShader(brdflutShader);
        Renderer::Get()->DestroyTexture(brdflutTexture);
        Renderer::Get()->DestroyTextureView(brdflutTextureView);
    }

    void SceneRenderer::_CleanupGBuffer()
    {
        api->DestroyShader(gbuffer->shader);
        api->DestroyPipeline(gbuffer->pipeline);
        api->DestroyRenderPass(gbuffer->pass);
        api->DestroyFramebuffer(gbuffer->framebuffer);

        Renderer::Get()->DestroyTexture(gbuffer->albedo);
        Renderer::Get()->DestroyTexture(gbuffer->normal);
        Renderer::Get()->DestroyTexture(gbuffer->emission);
        Renderer::Get()->DestroyTexture(gbuffer->id);
        Renderer::Get()->DestroyTexture(gbuffer->depth);

        Renderer::Get()->DestroyTextureView(gbuffer->albedoView);
        Renderer::Get()->DestroyTextureView(gbuffer->normalView);
        Renderer::Get()->DestroyTextureView(gbuffer->emissionView);
        Renderer::Get()->DestroyTextureView(gbuffer->idView);
        Renderer::Get()->DestroyTextureView(gbuffer->depthView);

        Renderer::Get()->DestroyDescriptorSet(gbuffer->transformSet);
        Renderer::Get()->DestroyDescriptorSet(gbuffer->materialSet);

        Renderer::Get()->DestroyBuffer(gbuffer->materialUBO);
        Renderer::Get()->DestroyBuffer(gbuffer->transformUBO);
    }

    void SceneRenderer::_CleanupLightingBuffer()
    {
        api->DestroyShader(lighting->shader);
        api->DestroyPipeline(lighting->pipeline);
        api->DestroyRenderPass(lighting->pass);
        api->DestroyFramebuffer(lighting->framebuffer);

        Renderer::Get()->DestroyTextureView(lighting->view);
        Renderer::Get()->DestroyTexture(lighting->color);
        Renderer::Get()->DestroyDescriptorSet(lighting->set);
        Renderer::Get()->DestroyBuffer(lighting->sceneUBO);
    }

    void SceneRenderer::_CleanupEnvironmentBuffer()
    {
        api->DestroyShader(environment->shader);
        api->DestroyPipeline(environment->pipeline);
        api->DestroyRenderPass(environment->pass);
        api->DestroyFramebuffer(environment->framebuffer);

        Renderer::Get()->DestroyDescriptorSet(environment->set);
        Renderer::Get()->DestroyBuffer(environment->ubo);
    }

    void SceneRenderer::_ResizeGBuffer(uint32 width, uint32 height)
    {
        Renderer::Get()->DestroyTexture(gbuffer->albedo);
        Renderer::Get()->DestroyTexture(gbuffer->normal);
        Renderer::Get()->DestroyTexture(gbuffer->emission);
        Renderer::Get()->DestroyTexture(gbuffer->id);
        Renderer::Get()->DestroyTexture(gbuffer->depth);

        Renderer::Get()->DestroyTextureView(gbuffer->albedoView);
        Renderer::Get()->DestroyTextureView(gbuffer->normalView);
        Renderer::Get()->DestroyTextureView(gbuffer->emissionView);
        Renderer::Get()->DestroyTextureView(gbuffer->idView);
        Renderer::Get()->DestroyTextureView(gbuffer->depthView);

        Renderer::Get()->DestroyFramebuffer(gbuffer->framebuffer);

        gbuffer->albedo   = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, width, height);
        gbuffer->normal   = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, width, height);
        gbuffer->emission = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32, width, height);
        gbuffer->id       = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R32_SINT, width, height, false, TEXTURE_USAGE_COPY_SRC_BIT);
        gbuffer->depth    = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_D32_SFLOAT, width, height);

        gbuffer->albedoView   = Renderer::Get()->CreateTextureView(gbuffer->albedo, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->normalView   = Renderer::Get()->CreateTextureView(gbuffer->normal, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->emissionView = Renderer::Get()->CreateTextureView(gbuffer->emission, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->idView       = Renderer::Get()->CreateTextureView(gbuffer->id, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        gbuffer->depthView    = Renderer::Get()->CreateTextureView(gbuffer->depth, TEXTURE_TYPE_2D, TEXTURE_ASPECT_DEPTH_BIT);

        TextureHandle* textures[] = {
            gbuffer->albedo->GetHandle(),
            gbuffer->normal->GetHandle(),
            gbuffer->emission->GetHandle(),
            gbuffer->id->GetHandle(),
            gbuffer->depth->GetHandle(),
        };

        gbuffer->framebuffer = Renderer::Get()->CreateFramebuffer(gbuffer->pass, std::size(textures), textures, width, height);
    }

    void SceneRenderer::_ResizeLightingBuffer(uint32 width, uint32 height)
    {
        Renderer::Get()->DestroyTexture(lighting->color);
        Renderer::Get()->DestroyTextureView(lighting->view);
        Renderer::Get()->DestroyFramebuffer(lighting->framebuffer);

        lighting->color = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);

        auto hcolor = lighting->color->GetHandle();
        lighting->view        = Renderer::Get()->CreateTextureView(lighting->color, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        lighting->framebuffer = Renderer::Get()->CreateFramebuffer(lighting->pass, 1, &hcolor, width, height);

        Renderer::Get()->DestroyDescriptorSet(lighting->set);
        lighting->set = Renderer::Get()->CreateDescriptorSet(lighting->shader, 0);
        lighting->set->SetResource( 0, gbuffer->albedoView, linearSampler);
        lighting->set->SetResource( 1, gbuffer->normalView, linearSampler);
        lighting->set->SetResource( 2, gbuffer->emissionView, linearSampler);
        lighting->set->SetResource( 3, gbuffer->depthView, linearSampler);
        lighting->set->SetResource( 4, irradianceTextureView, linearSampler);
        lighting->set->SetResource( 5, prefilterTextureView, linearSampler);
        lighting->set->SetResource( 6, brdflutTextureView, linearSampler);
        lighting->set->SetResource( 7, shadow->depthView, shadowSampler);
        lighting->set->SetResource( 8, lighting->sceneUBO);
        lighting->set->SetResource( 9, shadow->cascadeUBO);
        lighting->set->SetResource(10, shadow->lightTransformUBO);
        lighting->set->Flush();
    }

    void SceneRenderer::_ResizeEnvironmentBuffer(uint32 width, uint32 height)
    {
        Renderer::Get()->DestroyFramebuffer(environment->framebuffer);

        TextureHandle* textures[] = { lighting->color->GetHandle(), gbuffer->depth->GetHandle() };
        environment->framebuffer = Renderer::Get()->CreateFramebuffer(environment->pass, std::size(textures), textures, width, height);
    }

    std::vector<Extent> SceneRenderer::_CalculateBlomSampling(uint32 width, uint32 height)
    {
        // 解像度に変化し、上限で6回サンプリングする
        auto miplevels = RenderingUtility::CalculateMipmap(width, height);
        if (miplevels.size() - 1 > bloom->numDefaultSampling + 1)
        {
            miplevels.resize(bloom->numDefaultSampling + 1);
        }

        miplevels.erase(miplevels.begin());
        return miplevels;
    }

    void SceneRenderer::_PrepareBloomBuffer(uint32 width, uint32 height)
    {
        {
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            color.finalLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.storeOp = ATTACHMENT_STORE_OP_STORE;
            color.samples = TEXTURE_SAMPLES_1;
            color.format = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);

            SubpassDependency dep = {};
            dep.srcSubpass = RENDER_AUTO_ID;
            dep.dstSubpass = 0;
            dep.srcStages = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dep.dstStages = PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dep.srcAccess = BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dep.dstAccess = BARRIER_ACCESS_SHADER_READ_BIT;

            RenderPassClearValue clear;
            clear.SetFloat(0, 0, 0, 1);

            // レンダーパス
            bloom->pass = api->CreateRenderPass(1, &color, 1, &subpass, 1, &dep, 1, &clear);
        }

        bloom->resolutions.clear();
        bloom->resolutions = _CalculateBlomSampling(width, height);
        bloom->sampling.resize(bloom->resolutions.size());
        bloom->samplingView.resize(bloom->resolutions.size());
        bloom->samplingFB.resize(bloom->resolutions.size());
        bloom->downSamplingSet.resize(bloom->resolutions.size());
        bloom->upSamplingSet.resize(bloom->resolutions.size() - 1);

        // サンプリングイメージ
        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            const Extent extent = bloom->resolutions[i];
            bloom->sampling[i]  = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, extent.width, extent.height);

            auto hsampling = bloom->sampling[i]->GetHandle();
            bloom->samplingView[i] = Renderer::Get()->CreateTextureView(bloom->sampling[i], TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            bloom->samplingFB[i]   = Renderer::Get()->CreateFramebuffer(bloom->pass, 1, &hsampling, extent.width, extent.height);
        }

        // ブルーム プリフィルター/ブレンド イメージ
        bloom->prefilter     = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        bloom->prefilterView = Renderer::Get()->CreateTextureView(bloom->prefilter, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        bloom->bloom         = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        bloom->bloomView     = Renderer::Get()->CreateTextureView(bloom->bloom, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);

        // フレームバッファ
        auto hbloom = bloom->bloom->GetHandle();
        bloom->bloomFB = Renderer::Get()->CreateFramebuffer(bloom->pass, 1, &hbloom, width, height);


        // パイプライン
        ShaderCompiledData compiledData;
        PipelineStateInfoBuilder builder;

        PipelineStateInfo pipelineInfo = builder
            .Depth(false, false)
            .Blend(false, 1) // ブレンド 無し
            .Value();

        PipelineStateInfo pipelineInfoUpSampling = builder
            .Depth(false, false)
            .Blend(true, 1) // ブレンド 有り
            .Value();

        ShaderCompiler::Get()->Compile("Assets/Shaders/Bloom.glsl", compiledData);
        bloom->bloomShader   = api->CreateShader(compiledData);
        bloom->bloomPipeline = api->CreateGraphicsPipeline(bloom->bloomShader, &pipelineInfo, bloom->pass);

        ShaderCompiler::Get()->Compile("Assets/Shaders/BloomPrefiltering.glsl", compiledData);
        bloom->prefilterShader   = api->CreateShader(compiledData);
        bloom->prefilterPipeline = api->CreateGraphicsPipeline(bloom->prefilterShader, &pipelineInfo, bloom->pass);

        ShaderCompiler::Get()->Compile("Assets/Shaders/BloomDownSampling.glsl", compiledData);
        bloom->downSamplingShader   = api->CreateShader(compiledData);
        bloom->downSamplingPipeline = api->CreateGraphicsPipeline(bloom->downSamplingShader, &pipelineInfo, bloom->pass);

        ShaderCompiler::Get()->Compile("Assets/Shaders/BloomUpSampling.glsl", compiledData);
        bloom->upSamplingShader   = api->CreateShader(compiledData);
        bloom->upSamplingPipeline = api->CreateGraphicsPipeline(bloom->upSamplingShader, &pipelineInfoUpSampling, bloom->pass);


        // プリフィルター
        bloom->prefilterSet = Renderer::Get()->CreateDescriptorSet(bloom->prefilterShader, 0);
        bloom->prefilterSet->SetResource(0, lighting->view, linearSampler);
        bloom->prefilterSet->Flush();

        // ダウンサンプリング (source)  - (target)
        // prefilter - sample[0]
        // sample[0] - sample[1]
        // sample[1] - sample[2]
        // sample[2] - sample[3]
        // sample[3] - sample[4]
        // sample[4] - sample[5]
        bloom->downSamplingSet[0] = Renderer::Get()->CreateDescriptorSet(bloom->downSamplingShader, 0);
        bloom->downSamplingSet[0]->SetResource(0, bloom->prefilterView, linearSampler);
        bloom->downSamplingSet[0]->Flush();

        for (uint32 i = 1; i < bloom->downSamplingSet.size(); i++)
        {
            bloom->downSamplingSet[i] = Renderer::Get()->CreateDescriptorSet(bloom->downSamplingShader, 0);
            bloom->downSamplingSet[i]->SetResource(0, bloom->samplingView[i - 1], linearSampler);
            bloom->downSamplingSet[i]->Flush();
        }

        // アップサンプリング (source)  - (target)
        // sample[5] - sample[4]
        // sample[4] - sample[3]
        // sample[3] - sample[2]
        // sample[2] - sample[1]
        // sample[1] - sample[0]
        uint32 upSamplingIndex = bloom->upSamplingSet.size();
        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            bloom->upSamplingSet[i] = Renderer::Get()->CreateDescriptorSet(bloom->upSamplingShader, 0);
            bloom->upSamplingSet[i]->SetResource(0, bloom->samplingView[upSamplingIndex], linearSampler);
            bloom->upSamplingSet[i]->Flush();

            upSamplingIndex--;
        }

        // ブルームコンポジット
        bloom->bloomSet = Renderer::Get()->CreateDescriptorSet(bloom->bloomShader, 0);
        bloom->bloomSet->SetResource(0, lighting->view, linearSampler);
        bloom->bloomSet->SetResource(1, bloom->samplingView[0], linearSampler);
        bloom->bloomSet->Flush();
    }

    void SceneRenderer::_ResizeBloomBuffer(uint32 width, uint32 height)
    {
        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            Renderer::Get()->DestroyTexture(bloom->sampling[i]);
            Renderer::Get()->DestroyTextureView(bloom->samplingView[i]);
            Renderer::Get()->DestroyFramebuffer(bloom->samplingFB[i]);
        }

        Renderer::Get()->DestroyFramebuffer(bloom->bloomFB);
        Renderer::Get()->DestroyTexture(bloom->prefilter);
        Renderer::Get()->DestroyTextureView(bloom->prefilterView);
        Renderer::Get()->DestroyTexture(bloom->bloom);
        Renderer::Get()->DestroyTextureView(bloom->bloomView);

        Renderer::Get()->DestroyDescriptorSet(bloom->prefilterSet);
        Renderer::Get()->DestroyDescriptorSet(bloom->bloomSet);

        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            Renderer::Get()->DestroyDescriptorSet(bloom->upSamplingSet[i]);
        }

        for (uint32 i = 0; i < bloom->downSamplingSet.size(); i++)
        {
            Renderer::Get()->DestroyDescriptorSet(bloom->downSamplingSet[i]);
        }

        bloom->resolutions.clear();
        bloom->resolutions = _CalculateBlomSampling(width, height);
        bloom->sampling.resize(bloom->resolutions.size());
        bloom->samplingView.resize(bloom->resolutions.size());
        bloom->samplingFB.resize(bloom->resolutions.size());
        bloom->downSamplingSet.resize(bloom->resolutions.size());
        bloom->upSamplingSet.resize(bloom->resolutions.size() - 1);

        // イメージ
        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            const Extent extent = bloom->resolutions[i];
            bloom->sampling[i]  = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, extent.width, extent.height);

            auto hsample = bloom->sampling[i]->GetHandle();
            bloom->samplingView[i] = Renderer::Get()->CreateTextureView(bloom->sampling[i], TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            bloom->samplingFB[i]   = Renderer::Get()->CreateFramebuffer(bloom->pass, 1, &hsample, extent.width, extent.height);
        }

        bloom->prefilter     = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        bloom->prefilterView = Renderer::Get()->CreateTextureView(bloom->prefilter, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        bloom->bloom         = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);

        auto hbloom = bloom->bloom->GetHandle();
        bloom->bloomView = Renderer::Get()->CreateTextureView(bloom->bloom, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
        bloom->bloomFB   = Renderer::Get()->CreateFramebuffer(bloom->pass, 1, &hbloom, width, height);

        // プリフィルター
        bloom->prefilterSet = Renderer::Get()->CreateDescriptorSet(bloom->prefilterShader, 0);
        bloom->prefilterSet->SetResource(0, lighting->view, linearSampler);
        bloom->prefilterSet->Flush();

        // ダウンサンプリング (source)  - (target)
        // prefilter - sample[0]
        // sample[0] - sample[1]
        // sample[1] - sample[2]
        // sample[2] - sample[3]
        // sample[3] - sample[4]
        // sample[4] - sample[5]
        bloom->downSamplingSet[0] = Renderer::Get()->CreateDescriptorSet(bloom->downSamplingShader, 0);
        bloom->downSamplingSet[0]->SetResource(0, bloom->prefilterView, linearSampler);
        bloom->downSamplingSet[0]->Flush();

        for (uint32 i = 1; i < bloom->downSamplingSet.size(); i++)
        {
            bloom->downSamplingSet[i] = Renderer::Get()->CreateDescriptorSet(bloom->downSamplingShader, 0);
            bloom->downSamplingSet[i]->SetResource(0, bloom->samplingView[i - 1], linearSampler);
            bloom->downSamplingSet[i]->Flush();
        }

        // アップサンプリング (source)  - (target)
        // sample[5] - sample[4]
        // sample[4] - sample[3]
        // sample[3] - sample[2]
        // sample[2] - sample[1]
        // sample[1] - sample[0]
        uint32 upSamplingIndex = bloom->upSamplingSet.size();
        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            bloom->upSamplingSet[i] = Renderer::Get()->CreateDescriptorSet(bloom->upSamplingShader, 0);
            bloom->upSamplingSet[i]->SetResource(0, bloom->samplingView[upSamplingIndex], linearSampler);
            bloom->upSamplingSet[i]->Flush();

            upSamplingIndex--;
        }

        bloom->bloomSet = Renderer::Get()->CreateDescriptorSet(bloom->bloomShader, 0);
        bloom->bloomSet->SetResource(0, lighting->view, linearSampler);
        bloom->bloomSet->SetResource(1, bloom->samplingView[0], linearSampler);
        bloom->bloomSet->Flush();
    }

    void SceneRenderer::_CleanupBloomBuffer()
    {
        api->DestroyRenderPass(bloom->pass);

        for (uint32 i = 0; i < bloom->resolutions.size(); i++)
        {
            api->DestroyFramebuffer(bloom->samplingFB[i]);
            Renderer::Get()->DestroyTextureView(bloom->samplingView[i]);
            Renderer::Get()->DestroyTexture(bloom->sampling[i]);
        }

        for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
        {
            Renderer::Get()->DestroyDescriptorSet(bloom->upSamplingSet[i]);
        }

        for (uint32 i = 0; i < bloom->downSamplingSet.size(); i++)
        {
            Renderer::Get()->DestroyDescriptorSet(bloom->downSamplingSet[i]);
        }

        Renderer::Get()->DestroyTexture(bloom->bloom);
        Renderer::Get()->DestroyTexture(bloom->prefilter);

        Renderer::Get()->DestroyTextureView(bloom->bloomView);
        Renderer::Get()->DestroyTextureView(bloom->prefilterView);
        api->DestroyFramebuffer(bloom->bloomFB);

        api->DestroyShader(bloom->bloomShader);
        api->DestroyPipeline(bloom->bloomPipeline);
        api->DestroyShader(bloom->prefilterShader);
        api->DestroyPipeline(bloom->prefilterPipeline);
        api->DestroyShader(bloom->downSamplingShader);
        api->DestroyPipeline(bloom->downSamplingPipeline);
        api->DestroyShader(bloom->upSamplingShader);
        api->DestroyPipeline(bloom->upSamplingPipeline);

        Renderer::Get()->DestroyDescriptorSet(bloom->bloomSet);
        Renderer::Get()->DestroyDescriptorSet(bloom->prefilterSet);
    }

    int32 SceneRenderer::ReadEntityIDFromPixel(uint32 x, uint32 y)
    {
        Renderer::Get()->ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            TextureBarrierInfo info = {};
            info.texture      = gbuffer->id->GetHandle();
            info.subresources = {};
            info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout    = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.newLayout    = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            BufferTextureCopyRegion region = {};
            region.bufferOffset = 0;
            region.textureSubresources = {};
            region.textureRegionSize   = { 1, 1, 1 };
            region.textureOffset       = { x, y, 0 };

            api->Cmd_CopyTextureToBuffer(cmd, gbuffer->id->GetHandle(), TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, pixelIDBuffer->GetHandle(0), 1, &region);

            info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
        });

        int32* outID = (int32*)pixelIDBuffer->GetMappedPointer();
        return *outID;
    }

    void SceneRenderer::ResizeFramebuffer(uint32 width, uint32 height)
    {
        if (width == 0 || height == 0)
            return;

        sceneViewportSize = { width, height };

        _ResizeGBuffer(width, height);
        _ResizeLightingBuffer(width, height);
        _ResizeEnvironmentBuffer(width, height);
        _ResizeBloomBuffer(width, height);

        {
            Renderer::Get()->DestroyFramebuffer(compositeFB);
            Renderer::Get()->DestroyTexture(compositeTexture);
            Renderer::Get()->DestroyTextureView(compositeTextureView);

            compositeTexture = Renderer::Get()->CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, width, height);

            auto hcomposite = compositeTexture->GetHandle();
            compositeTextureView = Renderer::Get()->CreateTextureView(compositeTexture, TEXTURE_TYPE_2D, TEXTURE_ASPECT_COLOR_BIT);
            compositeFB          = Renderer::Get()->CreateFramebuffer(compositePass, 1, &hcomposite, width, height);

            Renderer::Get()->DestroyDescriptorSet(compositeSet);
            compositeSet = Renderer::Get()->CreateDescriptorSet(compositeShader, 0);
            compositeSet->SetResource(0, bloom->bloomView, linearSampler);
            compositeSet->Flush();
        }

        {
            Renderer::Get()->DestroyDescriptorSet(imageSet);
            imageSet = Renderer::Get()->CreateDescriptorSet(compositeShader, 0);
            imageSet->SetResource(0, compositeTextureView, linearSampler);
            imageSet->Flush();
        }
    }

    void SceneRenderer::_UpdateUniformBuffer()
    {
        Camera* camera = sceneCamera;

        {
            UBO::ShadowTransformData shadowData;
            shadowData.world = glm::mat4(1.0);

            shadow->transformUBO->SetData(&shadowData, sizeof(UBO::ShadowTransformData));
        }

        {
            UBO::Transform sceneData;
            sceneData.projection = camera->GetProjectionMatrix();
            sceneData.view       = camera->GetViewMatrix();
            sceneData.world      = glm::mat4(1.0);

            gbuffer->transformUBO->SetData(&sceneData, sizeof(UBO::Transform));
        }

        {
            UBO::MaterialUBO matData = {};
            matData.albedo        = glm::vec3(1.0);
            matData.emission      = glm::vec3(0.0);
            matData.metallic      = 0.0f;
            matData.roughness     = 0.5f;
            matData.textureTiling = glm::vec2(1.0, 1.0);

            gbuffer->materialUBO->SetData(&matData, sizeof(UBO::MaterialUBO));
        }

        {
            UBO::GridData gridData;
            gridData.projection = camera->GetProjectionMatrix();
            gridData.view       = camera->GetViewMatrix();
            gridData.pos        = glm::vec4(camera->GetPosition(), camera->GetFarPlane());

            gridUBO->SetData(&gridData, sizeof(UBO::GridData));
        }

        {
            // ライト
            glm::vec3 sceneLightDir = { 0.5, 0.7, 0.1 };

            std::array<glm::mat4, 4> outLightMatrices;
            _CalculateLightSapceMatrices(sceneLightDir, camera, outLightMatrices);

            UBO::LightSpaceTransformData lightData = {};
            lightData.cascade[0] = outLightMatrices[0];
            lightData.cascade[1] = outLightMatrices[1];
            lightData.cascade[2] = outLightMatrices[2];
            lightData.cascade[3] = outLightMatrices[3];
            shadow->lightTransformUBO->SetData(&lightData, sizeof(UBO::LightSpaceTransformData));

            UBO::SceneUBO sceneUBO = {};
            sceneUBO.lightColor        = glm::vec4(1.0, 1.0, 1.0, 1.0);
            sceneUBO.lightDir          = glm::vec4(sceneLightDir, 1.0);
            sceneUBO.cameraPosition    = glm::vec4(camera->GetPosition(), camera->GetFarPlane());
            sceneUBO.view              = camera->GetViewMatrix();
            sceneUBO.invViewProjection = glm::inverse(camera->GetProjectionMatrix() * camera->GetViewMatrix());
            lighting->sceneUBO->SetData(&sceneUBO, sizeof(UBO::SceneUBO));

            UBO::CascadeData cascadeData;
            cascadeData.cascadePlaneDistances[0].x = shadowCascadeLevels[0];
            cascadeData.cascadePlaneDistances[1].x = shadowCascadeLevels[1];
            cascadeData.cascadePlaneDistances[2].x = shadowCascadeLevels[2];
            cascadeData.cascadePlaneDistances[3].x = shadowCascadeLevels[3];
            shadow->cascadeUBO->SetData(&cascadeData, sizeof(UBO::CascadeData));
        }

        {
            UBO::EnvironmentUBO environmentUBO = {};
            environmentUBO.view       = camera->GetViewMatrix();
            environmentUBO.projection = camera->GetProjectionMatrix();
            environment->ubo->SetData(&environmentUBO, sizeof(UBO::EnvironmentUBO));
        }
    }

    void SceneRenderer::Reset(Scene* scene, Camera* camera)
    {
        // シーン情報をセット
        renderScene = scene;
        sceneCamera = camera;

        // 統計をリセット
        stats.numRenderMesh       = 0;
        stats.numGeometryDrawCall = 0;
        stats.numShadowDrawCall   = 0;

        // ステートをリセット
        shouldRenderShadow = false;

        // ポストプロセスのクリア
        postProcess = {};

        // ライトのリセット
        skyLight.enableIBL = false;
        skyLight.renderSky = false;
        directionalLight   = {};

        // 描画リストリセット
        meshDrawList.clear();

        // シャドウインスタンスデータクリア
        //shadowDrawData.clear();
        //ShadowParameterData.clear();

        // メッシュインスタンスデータクリア
        //meshDrawData.clear();
        //meshParameterData.clear();
    }

    void SceneRenderer::Render()
    {
        _UpdateUniformBuffer();
        _ExcutePasses();
    }

    void SceneRenderer::_ExcutePasses()
    {
        const FrameData& frame   = Renderer::Get()->GetFrameData();
        const uint32 frameIndex  = Renderer::Get()->GetCurrentFrameIndex();
        const auto& viewportSize = sceneViewportSize;

        // コマンドバッファ開始
        api->BeginCommandBuffer(frame.commandBuffer);

        //===================================================================================================
        // memcopy mapped buffer in-between command buffer calls?
        // https://www.reddit.com/r/vulkan/comments/110ygxu/memcopy_mapped_buffer_inbetween_command_buffer/
        //---------------------------------------------------------------------------------------------------
        // UBO のマルチバッファリング化が実装されるまで、パイプラインバリアで同期するようにする
        // ただし、マルチバッファリングを使用しない限り、即時書き込みのUBOの同期をとる方法はないので、現状効果はない
        //===================================================================================================

        // HOST_COHERENT フラグ: 明示的にフラッシュしなくても、コマンドバッファが転送された時点でGPUと同期される
        // メモリバリア         : GPU側が読み取るタイミングの動機

        // マルチバッファリングを実装できれば、前フレームの実行がフェンスで完了している状態でUBOの更新が可能なので、この待機は不必要となる
        // MemoryBarrierInfo mb = {};
        // mb.srcAccess = BARRIER_ACCESS_HOST_WRITE_BIT;
        // mb.dstAccess = BARRIER_ACCESS_UNIFORM_READ_BIT;
        // api->PipelineBarrier(frame.commandBuffer, PIPELINE_STAGE_HOST_BIT, PIPELINE_STAGE_VERTEX_SHADER_BIT, 1, &mb, 0, nullptr, 0, nullptr);


        // シャドウパス
        if (1)
        {
            api->Cmd_SetViewport(frame.commandBuffer, 0, 0, shadowMapResolution, shadowMapResolution);
            api->Cmd_SetScissor(frame.commandBuffer, 0, 0, shadowMapResolution, shadowMapResolution);

            auto* view = shadow->depthView->GetHandle();
            api->Cmd_BeginRenderPass(frame.commandBuffer, shadow->pass, shadow->framebuffer, 1, &view);

            api->Cmd_BindPipeline(frame.commandBuffer, shadow->pipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, shadow->set->GetHandle(frameIndex), 0);

            // スポンザ
            for (MeshSource* source : sponzaMesh->GetMeshSources())
            {
                BufferHandle* vb  = source->GetVertexBuffer()->GetHandle();
                BufferHandle* ib  = source->GetIndexBuffer()->GetHandle();
                uint32 indexCount = source->GetIndexCount();
                api->Cmd_BindVertexBuffer(frame.commandBuffer, vb, 0);
                api->Cmd_BindIndexBuffer(frame.commandBuffer, ib, INDEX_BUFFER_FORMAT_UINT32, 0);
                api->Cmd_DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
            }

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        // メッシュパス
        if (1)
        {
            api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
            api->Cmd_SetScissor(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);

            TextureViewHandle* views[] = {
                gbuffer->albedoView->GetHandle(),
                gbuffer->normalView->GetHandle(),
                gbuffer->emissionView->GetHandle(),
                gbuffer->idView->GetHandle(),
                gbuffer->depthView->GetHandle(),
            };

            api->Cmd_BeginRenderPass(frame.commandBuffer, gbuffer->pass, gbuffer->framebuffer, 5, views);
            api->Cmd_BindPipeline(frame.commandBuffer, gbuffer->pipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, gbuffer->transformSet->GetHandle(frameIndex), 0);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, gbuffer->materialSet->GetHandle(frameIndex), 1);

            //========================================================================
            // TODO: バインドレスとインスタンシング描画のためのストレージバッファの設計までに...
            //------------------------------------------------------------------------
            // 設計が完了するまでは毎度ドローコールすることになるので、ワールド行列はプッシュ定数で
            // 渡すように変更する。現在は定数バッファで渡しているので、シーンで一律なカメラの
            // ビュー・プロジェクション行列とは分離させる
            // 
            // ※本来は描画パス全体で使用する共通パラメータ(View/Projection)として1回だけバインド
            //========================================================================

            // スポンザ
            for (MeshSource* source : sponzaMesh->GetMeshSources())
            {
                BufferHandle* vb = source->GetVertexBuffer()->GetHandle();
                BufferHandle* ib = source->GetIndexBuffer()->GetHandle();
                uint32 indexCount = source->GetIndexCount();
                api->Cmd_BindVertexBuffer(frame.commandBuffer, vb, 0);
                api->Cmd_BindIndexBuffer(frame.commandBuffer, ib, INDEX_BUFFER_FORMAT_UINT32, 0);
                api->Cmd_DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
            }

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        // ライティングパス
        if (1)
        {
            auto* view = lighting->view->GetHandle();
            api->Cmd_BeginRenderPass(frame.commandBuffer, lighting->pass, lighting->framebuffer, 1, &view);

            api->Cmd_BindPipeline(frame.commandBuffer, lighting->pipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, lighting->set->GetHandle(frameIndex), 0);
            api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        // スカイパス
        if (1)
        {
            TextureViewHandle* views[] = { lighting->view->GetHandle(), gbuffer->depthView->GetHandle() };
            api->Cmd_BeginRenderPass(frame.commandBuffer, environment->pass, environment->framebuffer, 2, views);

            if (1) // スカイ
            {
                api->Cmd_BindPipeline(frame.commandBuffer, environment->pipeline);
                api->Cmd_BindDescriptorSet(frame.commandBuffer, environment->set->GetHandle(frameIndex), 0);

                MeshSource* ms = cubeMesh->GetMeshSource();
                api->Cmd_BindVertexBuffer(frame.commandBuffer, ms->GetVertexBuffer()->GetHandle(), 0);
                api->Cmd_BindIndexBuffer(frame.commandBuffer, ms->GetIndexBuffer()->GetHandle(), INDEX_BUFFER_FORMAT_UINT32, 0);
                api->Cmd_DrawIndexed(frame.commandBuffer, ms->GetIndexCount(), 1, 0, 0, 0);
            }

            if (1) // グリッド
            {
                api->Cmd_BindPipeline(frame.commandBuffer, gridPipeline);
                api->Cmd_BindDescriptorSet(frame.commandBuffer, gridSet->GetHandle(frameIndex), 0);
                api->Cmd_Draw(frame.commandBuffer, 6, 1, 0, 0);
            }

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }

        // ブルーム
        if (1)
        {
            // プリフィルタリング
            if (1)
            {
                api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
                api->Cmd_SetScissor(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);

                auto* view = bloom->prefilterView->GetHandle();
                api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->bloomFB, 1, &view);
                api->Cmd_BindPipeline(frame.commandBuffer, bloom->prefilterPipeline);

                float threshold = 10.0f;
                api->Cmd_PushConstants(frame.commandBuffer, bloom->prefilterShader, &threshold, 1);
                api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->prefilterSet->GetHandle(frameIndex), 0);
                api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                api->Cmd_EndRenderPass(frame.commandBuffer);
            }

            // ダウンサンプリング
            if (1)
            {
                for (uint32 i = 0; i < bloom->downSamplingSet.size(); i++)
                {
                    auto* view = bloom->samplingView[i]->GetHandle();
                    api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->samplingFB[i], 1, &view);
                    api->Cmd_BindPipeline(frame.commandBuffer, bloom->downSamplingPipeline);

                    api->Cmd_SetViewport(frame.commandBuffer, 0, 0, bloom->resolutions[i].width, bloom->resolutions[i].height);
                    api->Cmd_SetScissor(frame.commandBuffer, 0, 0, bloom->resolutions[i].width, bloom->resolutions[i].height);

                    glm::ivec2 srcResolution = i == 0 ? sceneViewportSize : glm::ivec2(bloom->resolutions[i - 1].width, bloom->resolutions[i - 1].height);
                    api->Cmd_PushConstants(frame.commandBuffer, bloom->downSamplingShader, &srcResolution[0], sizeof(srcResolution) / sizeof(uint32));

                    api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->downSamplingSet[i]->GetHandle(frameIndex), 0);
                    api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                    api->Cmd_EndRenderPass(frame.commandBuffer);
                }
            }

            // アップサンプリング
            if (1)
            {
                uint32 upSamplingIndex = bloom->upSamplingSet.size();
                for (uint32 i = 0; i < bloom->upSamplingSet.size(); i++)
                {
                    auto* view = bloom->samplingView[upSamplingIndex - 1]->GetHandle();
                    api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->samplingFB[upSamplingIndex - 1], 1, &view);
                    api->Cmd_BindPipeline(frame.commandBuffer, bloom->upSamplingPipeline);

                    api->Cmd_SetViewport(frame.commandBuffer, 0, 0, bloom->resolutions[upSamplingIndex - 1].width, bloom->resolutions[upSamplingIndex - 1].height);
                    api->Cmd_SetScissor(frame.commandBuffer, 0, 0, bloom->resolutions[upSamplingIndex - 1].width, bloom->resolutions[upSamplingIndex - 1].height);

                    float filterRadius = 0.01f;
                    api->Cmd_PushConstants(frame.commandBuffer, bloom->upSamplingShader, &filterRadius, 1);

                    api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->upSamplingSet[i]->GetHandle(frameIndex), 0);
                    api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                    api->Cmd_EndRenderPass(frame.commandBuffer);
                    upSamplingIndex--;
                }
            }

            // マージ
            if (1)
            {
                auto* view = bloom->bloomView->GetHandle();
                api->Cmd_BeginRenderPass(frame.commandBuffer, bloom->pass, bloom->bloomFB, 1, &view);
                api->Cmd_BindPipeline(frame.commandBuffer, bloom->bloomPipeline);

                api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
                api->Cmd_SetScissor(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);

                float intencity = 0.1f;
                api->Cmd_PushConstants(frame.commandBuffer, bloom->bloomShader, &intencity, 1);

                api->Cmd_BindDescriptorSet(frame.commandBuffer, bloom->bloomSet->GetHandle(frameIndex), 0);
                api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

                api->Cmd_EndRenderPass(frame.commandBuffer);
            }
        }

        // コンポジットパス
        if (1)
        {
            auto* view = compositeTextureView->GetHandle();
            api->Cmd_BeginRenderPass(frame.commandBuffer, compositePass, compositeFB, 1, &view);

            api->Cmd_SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
            api->Cmd_SetScissor(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);

            api->Cmd_BindPipeline(frame.commandBuffer, compositePipeline);
            api->Cmd_BindDescriptorSet(frame.commandBuffer, compositeSet->GetHandle(frameIndex), 0);
            api->Cmd_Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->Cmd_EndRenderPass(frame.commandBuffer);
        }
    }

}
