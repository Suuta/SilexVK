
#include "PCH.h"

#include "Core/Window.h"
#include "Core/Engine.h"
#include "Asset/TextureReader.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderingUtility.h"
#include "Rendering/Mesh.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>
#include <cmath>


namespace Silex
{
    Renderer* Renderer::Get()
    {
        return instance;
    }

    Renderer::Renderer()
    {
        instance = this;
    }

    Renderer::~Renderer()
    {
        api->WaitDevice();

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            _DestroyPendingResources(i);

            api->DestroyCommandBuffer(frameData[i].commandBuffer);
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].presentSemaphore);
            api->DestroySemaphore(frameData[i].renderSemaphore);
            api->DestroyFence(frameData[i].fence);

            sldelete(frameData[i].pendingResources);
        }

        api->DestroyCommandBuffer(immidiateContext.commandBuffer);
        api->DestroyCommandPool(immidiateContext.commandPool);
        api->DestroyFence(immidiateContext.fence);
        api->DestroyCommandQueue(graphicsQueue);

        context->DestroyRendringAPI(api);

        instance = nullptr;
    }

    bool Renderer::Initialize(RenderingContext* renderingContext, uint32 framesInFlight, uint32 numSwapchainBuffer)
    {
        context                 = renderingContext;
        numFramesInFlight       = framesInFlight;
        numSwapchainFrameBuffer = numSwapchainBuffer;

        // レンダーAPI実装クラスを生成
        api = context->CreateRendringAPI();
        SL_CHECK(!api->Initialize(), false);
       
        // グラフィックスをサポートするキューファミリを取得
        graphicsQueueID = api->QueryQueueID(QUEUE_FAMILY_GRAPHICS_BIT, Window::Get()->GetSurface());
        SL_CHECK(graphicsQueueID == RENDER_INVALID_ID, false);

        // コマンドキュー生成
        graphicsQueue = api->CreateCommandQueue(graphicsQueueID);
        SL_CHECK(!graphicsQueue, false);
      
        // フレームデータ生成
        frameData.resize(numFramesInFlight);
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            frameData[i].pendingResources = slnew(PendingDestroyResourceQueue);

            // コマンドプール生成
            frameData[i].commandPool = api->CreateCommandPool(graphicsQueueID);
            SL_CHECK(!frameData[i].commandPool, false);

            // コマンドバッファ生成
            frameData[i].commandBuffer = api->CreateCommandBuffer(frameData[i].commandPool);
            SL_CHECK(!frameData[i].commandBuffer, false);

            // セマフォ生成
            frameData[i].presentSemaphore = api->CreateSemaphore();
            SL_CHECK(!frameData[i].presentSemaphore, false);

            frameData[i].renderSemaphore = api->CreateSemaphore();
            SL_CHECK(!frameData[i].renderSemaphore, false);

            // フェンス生成
            frameData[i].fence = api->CreateFence();
            SL_CHECK(!frameData[i].fence, false);
        }

        // 即時コマンドデータ
        immidiateContext.commandPool = api->CreateCommandPool(graphicsQueueID);
        SL_CHECK(!immidiateContext.commandPool, false);

        immidiateContext.commandBuffer = api->CreateCommandBuffer(immidiateContext.commandPool);
        SL_CHECK(!immidiateContext.commandBuffer, false);

        immidiateContext.fence = api->CreateFence();
        SL_CHECK(!immidiateContext.fence, false);

        return true;
    }

    bool Renderer::BeginFrame()
    {
        bool result = false;
        FrameData& frame = frameData[frameIndex];

        // GPU 待機
        if (frameData[frameIndex].waitingSignal)
        {
            SL_SCOPE_PROFILE("Renderer::BeginFrame")

            result = api->WaitFence(frame.fence);
            SL_CHECK(!result, false);

            frameData[frameIndex].waitingSignal = false;
        }

        // 削除キュー実行
        _DestroyPendingResources(frameIndex);

        // 描画先スワップチェインバッファを取得
        auto [fb, view] = api->GetCurrentBackBuffer(Window::Get()->GetSwapChain(), frame.presentSemaphore);
        currentSwapchainFramebuffer = fb;
        currentSwapchainView        = view;

        return true;
    }

    bool Renderer::EndFrame()
    {
        bool result = false;

        FrameData& frame = frameData[frameIndex];
        result = api->EndCommandBuffer(frame.commandBuffer);

        result = api->SubmitQueue(graphicsQueue, frame.commandBuffer, frame.fence, frame.presentSemaphore, frame.renderSemaphore);
        frameData[frameIndex].waitingSignal = true;

        return result;
    }

    Texture2D* Renderer::CreateTextureFromMemory(const uint8* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // RGBA8_UNORM フォーマットテクスチャ
        TextureHandle* gpuTexture = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, RENDERING_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, genMipmap, TEXTURE_USAGE_COPY_DST_BIT);
        _SubmitTextureData(gpuTexture, width, height, genMipmap, pixelData, dataSize);

        Texture2D* texture = slnew(Texture2D, numFramesInFlight);
        texture->SetHandle(gpuTexture, 0);

        return texture;
    }

    Texture2D* Renderer::CreateTextureFromMemory(const float* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // RGBA16_SFLOAT フォーマットテクスチャ
        TextureHandle* gpuTexture = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height, 1, 1, genMipmap, TEXTURE_USAGE_COPY_DST_BIT);
        _SubmitTextureData(gpuTexture, width, height, genMipmap, pixelData, dataSize);

        Texture2D* texture = slnew(Texture2D, numFramesInFlight);
        texture->SetHandle(gpuTexture, 0);

        return texture;
    }

    Texture2D* Renderer::CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        Texture2D* texture = slnew(Texture2D, numFramesInFlight);
        TextureHandle* h = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, format, width, height, 1, 1, genMipmap, additionalFlags);
        texture->SetHandle(h, 0);

        return texture;
    }

    Texture2DArray* Renderer::CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        Texture2DArray* texture = slnew(Texture2DArray, numFramesInFlight);
        TextureHandle* h = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D_ARRAY, format, width, height, 1, array, genMipmap, additionalFlags);
        texture->SetHandle(h, 0);

        return texture;
    }

    TextureCube* Renderer::CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        TextureCube* texture = slnew(TextureCube, numFramesInFlight);
        TextureHandle* h = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_CUBE, format, width, height, 1, 6, genMipmap, additionalFlags);
        texture->SetHandle(h, 0);

        return texture;
    }

    void Renderer::DestroyTexture(Texture* texture)
    {
        FrameData& frame = frameData[frameIndex];

        TextureHandle* h = texture->GetHandle();
        frame.pendingResources->texture.push_back(h);

        sldelete(texture);
    }

    TextureView* Renderer::CreateTextureView(Texture* texture, TextureType type, TextureAspectFlags aspect, uint32 baseArrayLayer, uint32 numArrayLayer, uint32 baseMipLevel, uint32 numMipLevel)
    {
        TextureViewInfo viewInfo = {};
        viewInfo.type                      = type;
        viewInfo.subresource.aspect        = aspect;
        viewInfo.subresource.baseLayer     = baseArrayLayer;
        viewInfo.subresource.layerCount    = numArrayLayer;
        viewInfo.subresource.baseMipLevel  = baseMipLevel;
        viewInfo.subresource.mipLevelCount = numMipLevel;

        TextureView* view    = slnew(TextureView, numFramesInFlight);
        TextureViewHandle* h = api->CreateTextureView(texture->GetHandle(), viewInfo);
        view->SetHandle(h, 0);

        return view;
    }

    void Renderer::DestroyTextureView(TextureView* view)
    {
        FrameData& frame = frameData[frameIndex];

        TextureViewHandle* h = view->GetHandle();
        frame.pendingResources->textureView.push_back(h);

        sldelete(view);
    }

    Sampler* Renderer::CreateSampler(SamplerFilter filter, SamplerRepeatMode mode, bool enableCompare, CompareOperator compareOp)
    {
        SamplerInfo samplerInfo = {};
        samplerInfo.magFilter     = filter;
        samplerInfo.minFilter     = filter;
        samplerInfo.mipFilter     = filter;
        samplerInfo.repeatU       = mode;
        samplerInfo.repeatV       = mode;
        samplerInfo.repeatW       = mode;
        samplerInfo.enableCompare = enableCompare;
        samplerInfo.compareOp     = compareOp;

        Sampler* sampler = slnew(Sampler, numFramesInFlight);
        SamplerHandle* h = api->CreateSampler(samplerInfo);
        sampler->SetHandle(h, 0);

        return sampler;
    }

    void Renderer::DestroySampler(Sampler* sampler)
    {
        FrameData& frame = frameData[frameIndex];

        SamplerHandle* h = sampler->GetHandle();
        frame.pendingResources->sampler.push_back(h);

        sldelete(sampler);
    }

    TextureHandle* Renderer::_CreateTexture(TextureDimension dimension, TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        int32 usage = TEXTURE_USAGE_SAMPLING_BIT;
        usage |= RenderingUtility::IsDepthFormat(format)? TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
        usage |= genMipmap? TEXTURE_USAGE_COPY_SRC_BIT | TEXTURE_USAGE_COPY_DST_BIT : 0;
        usage |= additionalFlags;

        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        TextureInfo texformat = {};
        texformat.format    = format;
        texformat.width     = width;
        texformat.height    = height;
        texformat.dimension = dimension;
        texformat.type      = type;
        texformat.usageBits = usage;
        texformat.samples   = TEXTURE_SAMPLES_1;
        texformat.array     = array;
        texformat.depth     = depth;
        texformat.mipLevels = genMipmap? mipmaps.size() : 1;

        return api->CreateTexture(texformat);
    }

    void Renderer::_SubmitTextureData(TextureHandle* texture, uint32 width, uint32 height, bool genMipmap, const void* pixelData, uint64 dataSize)
    {
        // ステージングバッファ
        BufferHandle* staging = api->CreateBuffer(dataSize, BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);

        // ステージングにテクスチャデータをコピー
        void* mappedPtr = api->MapBuffer(staging);
        std::memcpy(mappedPtr, pixelData, dataSize);
        api->UnmapBuffer(staging);

        // コピーコマンド
        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            TextureSubresourceRange range = {};
            range.aspect = TEXTURE_ASPECT_COLOR_BIT;

            TextureBarrierInfo info = {};
            info.texture      = texture;
            info.subresources = range;
            info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout    = TEXTURE_LAYOUT_UNDEFINED;
            info.newLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;

            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            TextureSubresource subresource = {};
            subresource.aspect     = TEXTURE_ASPECT_COLOR_BIT;

            BufferTextureCopyRegion region = {};
            region.bufferOffset        = 0;
            region.textureOffset       = { 0, 0, 0 };
            region.textureRegionSize   = { width, height, 1 };
            region.textureSubresources = subresource;

            api->Cmd_CopyBufferToTexture(cmd, staging, texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            if (genMipmap)
            {
                // ミップマップ生成
                _GenerateMipmaps(cmd, texture, width, height, 1, 1, TEXTURE_ASPECT_COLOR_BIT);

                // シェーダーリードに移行 (ミップマップ生成時のコピーでコピーソースに移行するため、コピーソース -> シェーダーリード)
                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            }
            else
            {
                // シェーダーリードに移行 (バッファからの転送でレイアウト変更がないため、コピー先 -> シェーダーリード)
                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            }
        });

        // ステージング破棄
        api->DestroyBuffer(staging);
    }
    
    void Renderer::_GenerateMipmaps(CommandBufferHandle* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect)
    {
        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        // コピー回数は (ミップ数 - 1)
        for (uint32 mipLevel = 0; mipLevel < mipmaps.size() - 1; mipLevel++)
        {
            // バリア
            TextureBarrierInfo info = {};
            info.texture                    = texture;
            info.subresources.aspect        = aspect;
            info.subresources.baseMipLevel  = mipLevel;
            info.subresources.mipLevelCount = 1;
            info.subresources.baseLayer     = 0;
            info.subresources.layerCount    = array;
            info.srcAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT | BARRIER_ACCESS_MEMORY_READ_BIT;
            info.oldLayout                  = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
            info.newLayout                  = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            // Blit
            Extent srcExtent = mipmaps[mipLevel + 0];
            Extent dstExtent = mipmaps[mipLevel + 1];

            //------------------------------------------------------------------------------------------------------------------------------------------
            // NOTE:
            // srcImageが VK_IMAGE_TYPE_1D または VK_IMAGE_TYPE_2D 型の場合、pRegionsの各要素について、srcOffsets[0].zは0、srcOffsets[1].z は1でなければなりません。
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdBlitImage.html#VUID-vkCmdBlitImage-srcImage-00247
            //------------------------------------------------------------------------------------------------------------------------------------------

            TextureBlitRegion region = {};
            region.srcOffset[0] = { 0, 0, 0 };
            region.dstOffset[0] = { 0, 0, 0 };
            region.srcOffset[1] = srcExtent;
            region.dstOffset[1] = dstExtent;

            region.srcSubresources.aspect     = aspect;
            region.srcSubresources.mipLevel   = mipLevel;
            region.srcSubresources.baseLayer  = 0;
            region.srcSubresources.layerCount = array;

            region.dstSubresources.aspect     = aspect;
            region.dstSubresources.mipLevel   = mipLevel + 1;
            region.dstSubresources.baseLayer  = 0;
            region.dstSubresources.layerCount = array;

            api->Cmd_BlitTexture(cmd, texture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, SAMPLER_FILTER_LINEAR);
        }

        //------------------------------------------------------------------------
        // 最後のミップレベルはコピー先としか利用せず、コピーソースレイアウトに移行していない
        // 後で実行されるレイアウト移行時に、ミップレベル間でレイアウトの不一致があると不便なので
        // 最後のミップレベルのレイアウトも [コピー先 → コピーソース] に移行させておく
        //------------------------------------------------------------------------
        TextureSubresourceRange range = {};
        range.aspect        = TEXTURE_ASPECT_COLOR_BIT;
        range.baseMipLevel  = mipmaps.size() - 1;
        range.mipLevelCount = 1;

        TextureBarrierInfo info = {};
        info.texture      = texture;
        info.subresources = range;
        info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
        info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT | BARRIER_ACCESS_MEMORY_READ_BIT;
        info.oldLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
        info.newLayout    = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        api->Cmd_PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
    }


    Buffer* Renderer::CreateBuffer(void* data, uint64 size)
    {
        Buffer* buffer = slnew(Buffer, numFramesInFlight);

        void* mapped = nullptr;
        BufferHandle* h = _CreateAndMapBuffer(0, data, size, &mapped);
        buffer->SetHandle(h, 0);
        //buffer->mappedPtr = mapped;

        return buffer;
    }

    UniformBuffer* Renderer::CreateUniformBuffer(void* data, uint64 size)
    {
        UniformBuffer* buffer = slnew(UniformBuffer, numFramesInFlight);
        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            void* mapped = nullptr;
            BufferHandle* h = _CreateAndMapBuffer(BUFFER_USAGE_UNIFORM_BIT, data, size, &mapped);
            buffer->SetHandle(h, i);
            //buffer->mappedPtr = mapped;
        }
        
        return buffer;
    }

    BufferHandle* Renderer::CreateStorageBuffer(void* data, uint64 size)
    {
        //StorageBuffer* buffer = slnew(StorageBuffer, numFramesInFlight);
        //for (uint32 i = 0; i < numFramesInFlight; i++)
        //{
        //    void* mapped = nullptr;
        //    BufferHandle* h = _CreateAndMapBuffer(BUFFER_USAGE_STORAGE_BIT, data, size, &mapped);
        //    buffer->handle[i] = h;
        //    buffer->mappedPtr = mapped;
        //}

        return _CreateAndMapBuffer(BUFFER_USAGE_STORAGE_BIT, data, size, nullptr);
    }

    VertexBuffer* Renderer::CreateVertexBuffer(void* data, uint64 size)
    {
        VertexBuffer* buffer = slnew(VertexBuffer, numFramesInFlight);
        BufferHandle* h = _CreateAndSubmitBufferData(BUFFER_USAGE_VERTEX_BIT, data, size);
        buffer->SetHandle(h, 0);

        return buffer;
    }

    IndexBuffer* Renderer::CreateIndexBuffer(void* data, uint64 size)
    {
        IndexBuffer* buffer = slnew(IndexBuffer, numFramesInFlight);
        BufferHandle* h = _CreateAndSubmitBufferData(BUFFER_USAGE_INDEX_BIT, data, size);
        buffer->SetHandle(h, 0);

        return buffer;
    }

    void* Renderer::GetMappedPointer(BufferHandle* buffer)
    {
        return api->GetBufferMappedPointer(buffer);
    }

    void Renderer::DestroyBuffer(Buffer* buffer)
    {
        FrameData& frame = frameData[frameIndex];

        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            BufferHandle* handle = buffer->GetHandle(i);
            if (handle)
            {
                api->UnmapBuffer(handle);
                frame.pendingResources->buffer.push_back(handle);
            }
        }

        sldelete(buffer);
    }

    bool Renderer::UpdateBufferData(BufferHandle* buffer, const void* data, uint32 dataByte)
    {
        return api->UpdateBufferData(buffer, data, dataByte);
    }

    BufferHandle* Renderer::_CreateAndMapBuffer(BufferUsageFlags type, const void* data, uint64 dataSize, void** outMappedPtr)
    {
        BufferHandle* buffer  = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        void* mappedPtr = api->MapBuffer(buffer);

        if (data != nullptr)
            std::memcpy(mappedPtr, data, dataSize);

        if (outMappedPtr != nullptr)
            *outMappedPtr = mappedPtr;

        return buffer;
    }

    BufferHandle* Renderer::_CreateAndSubmitBufferData(BufferUsageFlags type, const void* data, uint64 dataSize)
    {
        BufferHandle* buffer  = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_GPU);
        BufferHandle* staging = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        
        void* mapped = api->MapBuffer(staging);
        std::memcpy(mapped, data, dataSize);
        api->UnmapBuffer(staging);

        ImmidiateExcute([&](CommandBufferHandle* cmd)
        {
            BufferCopyRegion region = {};
            region.size = dataSize;

            api->Cmd_CopyBuffer(cmd, staging, buffer, 1, &region);
        });

        api->DestroyBuffer(staging);

        return buffer;
    }



    FramebufferHandle* Renderer::CreateFramebuffer(RenderPassHandle* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height)
    {
        return api->CreateFramebuffer(renderpass, numTexture, textures, width, height);
    }

    void Renderer::DestroyFramebuffer(FramebufferHandle* framebuffer)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources->framebuffer.push_back(framebuffer);
    }


    DescriptorSet* Renderer::CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex)
    {
        DescriptorSet* set = slnew(DescriptorSet, numFramesInFlight);
        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            DescriptorSetHandle* h = api->CreateDescriptorSet(shader, setIndex);
            set->SetHandle(h, i);
        }

        return set;
    }

    void Renderer::UpdateDescriptorSet(DescriptorSetHandle* set, DescriptorSetInfo& setInfo)
    {
        api->UpdateDescriptorSet(set, setInfo.infos.size(), setInfo.infos.data());
    }

    void Renderer::DestroyDescriptorSet(DescriptorSet* set)
    {
        FrameData& frame = frameData[frameIndex];

        for (uint32 i = 0; i < numFramesInFlight; i++)
        {
            DescriptorSetHandle* h = set->GetHandle(i);
            frame.pendingResources->descriptorset.push_back(h);
        }

        sldelete(set);
    }



    SwapChainHandle* Renderer::CreateSwapChain(SurfaceHandle* surface, uint32 width, uint32 height, VSyncMode mode)
    {
        return api->CreateSwapChain(surface, width, height, numSwapchainFrameBuffer, mode);
    }

    void Renderer::DestoySwapChain(SwapChainHandle* swapchain)
    {
        api->DestroySwapChain(swapchain);
    }

    bool Renderer::ResizeSwapChain(SwapChainHandle* swapchain, uint32 width, uint32 height, VSyncMode mode)
    {
        return api->ResizeSwapChain(swapchain, width, height, numSwapchainFrameBuffer, mode);
    }

    bool Renderer::Present()
    {
        SwapChainHandle* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame = frameData[frameIndex];

        bool result = api->Present(graphicsQueue, swapchain, frame.renderSemaphore);
        frameIndex = (frameIndex + 1) % 2;

        return result;
    }

    void Renderer::BeginSwapChainPass()
    {
        swapchainPass = api->GetSwapChainRenderPass(Window::Get()->GetSwapChain());

        FrameData& frame = frameData[frameIndex];
        api->Cmd_BeginRenderPass(frame.commandBuffer, swapchainPass, currentSwapchainFramebuffer, 1, &currentSwapchainView);
    }

    void Renderer::EndSwapChainPass()
    {
        FrameData& frame = frameData[frameIndex];
        api->Cmd_EndRenderPass(frame.commandBuffer);
    }

    void Renderer::ImmidiateExcute(std::function<void(CommandBufferHandle*)>&& func)
    {
        api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, std::move(func));
    }

    void Renderer::_DestroyPendingResources(uint32 frame)
    {
        FrameData& f = frameData[frame];

        for (BufferHandle* buffer : f.pendingResources->buffer)
        {
            api->DestroyBuffer(buffer);
        }

        f.pendingResources->buffer.clear();

        for (TextureHandle* texture : f.pendingResources->texture)
        {
            api->DestroyTexture(texture);
        }

        f.pendingResources->texture.clear();

        for (TextureViewHandle* view : f.pendingResources->textureView)
        {
            api->DestroyTextureView(view);
        }

        f.pendingResources->textureView.clear();

        for (SamplerHandle* sampler : f.pendingResources->sampler)
        {
            api->DestroySampler(sampler);
        }

        f.pendingResources->sampler.clear();

        for (DescriptorSetHandle* set : f.pendingResources->descriptorset)
        {
            api->DestroyDescriptorSet(set);
        }

        f.pendingResources->descriptorset.clear();

        for (FramebufferHandle* framebuffer : f.pendingResources->framebuffer)
        {
            api->DestroyFramebuffer(framebuffer);
        }

        f.pendingResources->framebuffer.clear();

        for (ShaderHandle* shader : f.pendingResources->shader)
        {
            api->DestroyShader(shader);
        }

        f.pendingResources->shader.clear();

        for (PipelineHandle* pipeline : f.pendingResources->pipeline)
        {
            api->DestroyPipeline(pipeline);
        }

        f.pendingResources->pipeline.clear();
    }

    const DeviceInfo& Renderer::GetDeviceInfo() const
    {
        return context->GetDeviceInfo();
    }

    RenderingContext* Renderer::GetContext() const
    {
        return context;
    }

    RenderingAPI* Renderer::GetAPI() const
    {
        return api;
    }

    const FrameData& Renderer::GetFrameData() const
    {
        return frameData[frameIndex];
    }

    uint32 Renderer::GetCurrentFrameIndex() const
    {
        return frameIndex;
    }

    uint32 Renderer::GetFrameCountInFlight() const
    {
        return numFramesInFlight;
    }

    CommandQueueHandle* Renderer::GetGraphicsCommandQueue() const
    {
        return graphicsQueue;
    }

    QueueID Renderer::GetGraphicsQueueID() const
    {
        return graphicsQueueID;
    }
}
