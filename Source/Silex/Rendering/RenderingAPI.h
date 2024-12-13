
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    // A Comparison of Modern Graphics APIs
    // https://alain.xyz/blog/comparison-of-modern-graphics-apis


    struct ShaderCompiledData;

    class RenderingAPI : public Object
    {
        SL_CLASS(RenderingAPI, Object)

    public:

        RenderingAPI()  {};
        ~RenderingAPI() {};

    public:

        virtual bool Initialize() = 0;

        //--------------------------------------------------
        // コマンドキュー
        //--------------------------------------------------
        virtual CommandQueueHandle* CreateCommandQueue(QueueID id, uint32 indexInFamily = 0) = 0;
        virtual void DestroyCommandQueue(CommandQueueHandle* queue) = 0;
        virtual QueueID QueryQueueID(QueueFamilyFlags flag, SurfaceHandle* surface = nullptr) const = 0;
        virtual bool SubmitQueue(CommandQueueHandle* queue, CommandBufferHandle* commandbuffer, FenceHandle* fence, SemaphoreHandle* present, SemaphoreHandle* render) = 0;

        //--------------------------------------------------
        // コマンドプール
        //--------------------------------------------------
        virtual CommandPoolHandle* CreateCommandPool(QueueID id, CommandBufferType type = COMMAND_BUFFER_TYPE_PRIMARY) = 0;
        virtual void DestroyCommandPool(CommandPoolHandle* pool) = 0;

        //--------------------------------------------------
        // コマンドバッファ
        //--------------------------------------------------
        virtual CommandBufferHandle* CreateCommandBuffer(CommandPoolHandle* pool) = 0;
        virtual void DestroyCommandBuffer(CommandBufferHandle* commandBuffer) = 0;
        virtual bool BeginCommandBuffer(CommandBufferHandle* commandBuffer) = 0;
        virtual bool EndCommandBuffer(CommandBufferHandle* commandBuffer) = 0;

        //--------------------------------------------------
        // セマフォ
        //--------------------------------------------------
#ifdef CreateSemaphore
#undef CreateSemaphore // マクロを使わずに CreateSemaphoreA/W を使えばよい
#endif
        virtual SemaphoreHandle* CreateSemaphore() = 0;
        virtual void DestroySemaphore(SemaphoreHandle* semaphore) = 0;

        //--------------------------------------------------
        // フェンス
        //--------------------------------------------------
        virtual FenceHandle* CreateFence() = 0;
        virtual void DestroyFence(FenceHandle* fence) = 0;
        virtual bool WaitFence(FenceHandle* fence) = 0;

        //--------------------------------------------------
        // スワップチェイン
        //--------------------------------------------------
        virtual SwapChainHandle* CreateSwapChain(SurfaceHandle* surface, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) = 0;
        virtual void DestroySwapChain(SwapChainHandle* swapchain) = 0;
        virtual bool ResizeSwapChain(SwapChainHandle* swapchain, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) = 0;
        virtual std::pair<FramebufferHandle*, TextureViewHandle*> GetCurrentBackBuffer(SwapChainHandle* swapchain, SemaphoreHandle* present) = 0;
        virtual bool Present(CommandQueueHandle* queue, SwapChainHandle* swapchain, SemaphoreHandle* render) = 0;
        
        virtual RenderPassHandle* GetSwapChainRenderPass(SwapChainHandle* swapchain) = 0;
        virtual RenderingFormat GetSwapChainFormat(SwapChainHandle* swapchain) = 0;

        //--------------------------------------------------
        // バッファ
        //--------------------------------------------------
        virtual BufferHandle* CreateBuffer(uint64 size, BufferUsageFlags usage, MemoryAllocationType memoryType) = 0;
        virtual void DestroyBuffer(BufferHandle* buffer) = 0;
        virtual void* MapBuffer(BufferHandle* buffer) = 0;
        virtual void UnmapBuffer(BufferHandle* buffer) = 0;
        virtual bool UpdateBufferData(BufferHandle* buffer, const void* data, uint64 dataByte) = 0;
        virtual void* GetBufferMappedPointer(BufferHandle* buffer) = 0;

        //--------------------------------------------------
        // テクスチャ
        //--------------------------------------------------
        virtual TextureHandle* CreateTexture(const TextureInfo& info) = 0;
        virtual void DestroyTexture(TextureHandle* texture) = 0;

        //--------------------------------------------------
        // テクスチャビュー
        //--------------------------------------------------
        virtual TextureViewHandle* CreateTextureView(TextureHandle* texture, const TextureViewInfo& info) = 0;
        virtual void DestroyTextureView(TextureViewHandle* view) = 0;

        //--------------------------------------------------
        // サンプラ
        //--------------------------------------------------
        virtual SamplerHandle* CreateSampler(const SamplerInfo& info) = 0;
        virtual void DestroySampler(SamplerHandle* sampler) = 0;

        //--------------------------------------------------
        // フレームバッファ
        //--------------------------------------------------
        virtual FramebufferHandle* CreateFramebuffer(RenderPassHandle* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height) = 0;
        virtual void DestroyFramebuffer(FramebufferHandle* framebuffer) = 0;

        //--------------------------------------------------
        // レンダーパス
        //--------------------------------------------------
        virtual RenderPassHandle* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies, uint32 numClearValue, RenderPassClearValue* clearValue) = 0;
        virtual void DestroyRenderPass(RenderPassHandle* renderpass) = 0;

        //--------------------------------------------------
        // シェーダー
        //--------------------------------------------------
        virtual ShaderHandle* CreateShader(const ShaderCompiledData& compiledData) = 0;
        virtual void DestroyShader(ShaderHandle* shader) = 0;

        //--------------------------------------------------
        // デスクリプターセット
        //--------------------------------------------------
        virtual DescriptorSetHandle* CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex) = 0;
        virtual void UpdateDescriptorSet(DescriptorSetHandle* set, uint32 numdescriptors, DescriptorInfo* descriptors) = 0;
        virtual void DestroyDescriptorSet(DescriptorSetHandle* descriptorset) = 0;

        //--------------------------------------------------
        // パイプライン
        //--------------------------------------------------
        virtual PipelineHandle* CreateGraphicsPipeline(ShaderHandle* shader, PipelineStateInfo* info, RenderPassHandle* renderpass, uint32 renderSubpass = 0, PipelineDynamicStateFlags dynamicState = DYNAMIC_STATE_NONE) = 0;
        virtual PipelineHandle* CreateComputePipeline(ShaderHandle* shader) = 0;
        virtual void DestroyPipeline(PipelineHandle* pipeline) = 0;

        //--------------------------------------------------
        // コマンド
        //--------------------------------------------------
        virtual void Cmd_PipelineBarrier(CommandBufferHandle* commandbuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrierInfo* memoryBarrier, uint32 numBufferBarrier, BufferBarrierInfo* bufferBarrier, uint32 numTextureBarrier, TextureBarrierInfo* textureBarrier) = 0;
        virtual void Cmd_ClearBuffer(CommandBufferHandle* commandbuffer, BufferHandle* buffer, uint64 offset, uint64 size) = 0;
        virtual void Cmd_CopyBuffer(CommandBufferHandle* commandbuffer, BufferHandle* srcBuffer, BufferHandle* dstBuffer, uint32 numRegion, BufferCopyRegion* regions) = 0;
        virtual void Cmd_CopyTexture(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureCopyRegion* regions) = 0;
        virtual void Cmd_ResolveTexture(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, uint32 srcLayer, uint32 srcMipmap, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 dstLayer, uint32 dstMipmap) = 0;
        virtual void Cmd_ClearColorTexture(CommandBufferHandle* commandbuffer, TextureHandle* texture, TextureLayout textureLayout, const glm::vec4& color, const TextureSubresourceRange& subresources) = 0;
        virtual void Cmd_CopyBufferToTexture(CommandBufferHandle* commandbuffer, BufferHandle* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions) = 0;
        virtual void Cmd_CopyTextureToBuffer(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, BufferHandle* dstBuffer, uint32 numRegion, BufferTextureCopyRegion* regions) = 0;
        virtual void Cmd_BlitTexture(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureBlitRegion* regions, SamplerFilter filter = SAMPLER_FILTER_LINEAR) = 0;

        virtual void Cmd_PushConstants(CommandBufferHandle* commandbuffer, ShaderHandle* shader, const void* data, uint32 numData, uint32 offsetIndex = 0) = 0;
        virtual void Cmd_BeginRenderPass(CommandBufferHandle* commandbuffer, RenderPassHandle* renderpass, FramebufferHandle* framebuffer, uint32 numView, TextureViewHandle** views, CommandBufferType commandBufferType = COMMAND_BUFFER_TYPE_PRIMARY) = 0;
        virtual void Cmd_EndRenderPass(CommandBufferHandle* commandbuffer) = 0;
        virtual void Cmd_NextRenderSubpass(CommandBufferHandle* commandbuffer, CommandBufferType commandBufferType) = 0;
        virtual void Cmd_SetViewport(CommandBufferHandle* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void Cmd_SetScissor(CommandBufferHandle* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void Cmd_ClearAttachments(CommandBufferHandle* commandbuffer, uint32 numAttachmentClear, AttachmentClear** attachmentClears, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void Cmd_BindPipeline(CommandBufferHandle* commandbuffer, PipelineHandle* pipeline) = 0;
        virtual void Cmd_BindDescriptorSet(CommandBufferHandle* commandbuffer, DescriptorSetHandle* descriptorset, uint32 setIndex) = 0;
        virtual void Cmd_Draw(CommandBufferHandle* commandbuffer, uint32 vertexCount, uint32 instanceCount, uint32 baseVertex, uint32 firstInstance) = 0;
        virtual void Cmd_DrawIndexed(CommandBufferHandle* commandbuffer, uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) = 0;
        virtual void Cmd_BindVertexBuffers(CommandBufferHandle* commandbuffer, uint32 bindingCount, BufferHandle** buffers, uint64* offsets) = 0;
        virtual void Cmd_BindVertexBuffer(CommandBufferHandle* commandbuffer, BufferHandle* buffer, uint64 offset) = 0;
        virtual void Cmd_BindIndexBuffer(CommandBufferHandle* commandbuffer, BufferHandle* buffer, IndexBufferFormat format, uint64 offset) = 0;

        //--------------------------------------------------
        // MISC
        //--------------------------------------------------
        virtual bool ImmidiateCommands(CommandQueueHandle* queue, CommandBufferHandle* commandBuffer, FenceHandle* fence, std::function<void(CommandBufferHandle*)>&& func) = 0;
        virtual bool WaitDevice() = 0;
    };
}
