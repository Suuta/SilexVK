
#pragma once

#include "Rendering/RenderingAPI.h"
#include "Rendering/Vulkan/VulkanStructures.h"


namespace Silex
{
    class  VulkanContext;
    struct VulkanSurface;

    //=============================================
    // Vulkan API 実装
    //=============================================
    class VulkanAPI : public RenderingAPI
    {
        SL_CLASS(VulkanAPI, RenderingAPI)

    public:

        VulkanAPI(VulkanContext* context);
        ~VulkanAPI();

        bool Initialize() override;

        //--------------------------------------------------
        // コマンドキュー
        //--------------------------------------------------
        CommandQueueHandle* CreateCommandQueue(QueueID id, uint32 indexInFamily = 0) override;
        void DestroyCommandQueue(CommandQueueHandle* queue) override;
        QueueID QueryQueueID(QueueFamilyFlags queueFlag, SurfaceHandle* surface = nullptr) const override;
        bool SubmitQueue(CommandQueueHandle* queue, CommandBufferHandle* commandbuffer, FenceHandle* fence, SemaphoreHandle* present, SemaphoreHandle* render) override;

        //--------------------------------------------------
        // コマンドプール
        //--------------------------------------------------
        CommandPoolHandle* CreateCommandPool(QueueID id, CommandBufferType type = COMMAND_BUFFER_TYPE_PRIMARY) override;
        void DestroyCommandPool(CommandPoolHandle* pool) override;

        //--------------------------------------------------
        // コマンドバッファ
        //--------------------------------------------------
        CommandBufferHandle* CreateCommandBuffer(CommandPoolHandle* pool) override;
        void DestroyCommandBuffer(CommandBufferHandle* commandBuffer) override;
        bool BeginCommandBuffer(CommandBufferHandle* commandBuffer) override;
        bool EndCommandBuffer(CommandBufferHandle* commandBuffer) override;

        //--------------------------------------------------
        // セマフォ
        //--------------------------------------------------
        SemaphoreHandle* CreateSemaphore() override;
        void DestroySemaphore(SemaphoreHandle* semaphore) override;

        //--------------------------------------------------
        // フェンス
        //--------------------------------------------------
        FenceHandle* CreateFence() override;
        void DestroyFence(FenceHandle* fence) override;
        bool WaitFence(FenceHandle* fence) override;

        //--------------------------------------------------
        // スワップチェイン
        //--------------------------------------------------
        SwapChainHandle* CreateSwapChain(SurfaceHandle* surface, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) override;
        void DestroySwapChain(SwapChainHandle* swapchain) override;
        bool ResizeSwapChain(SwapChainHandle* swapchain, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) override;
        std::pair<FramebufferHandle*, TextureViewHandle*> GetCurrentBackBuffer(SwapChainHandle* swapchain, SemaphoreHandle* present) override;
        bool Present(CommandQueueHandle* queue, SwapChainHandle* swapchain, SemaphoreHandle* render) override;

        RenderPassHandle* GetSwapChainRenderPass(SwapChainHandle* swapchain) override;
        RenderingFormat GetSwapChainFormat(SwapChainHandle* swapchain) override;

        //--------------------------------------------------
        // バッファ
        //--------------------------------------------------
        BufferHandle* CreateBuffer(uint64 size, BufferUsageFlags usage, MemoryAllocationType memoryType) override;
        void DestroyBuffer(BufferHandle* buffer) override;
        void* MapBuffer(BufferHandle* buffer) override;
        void UnmapBuffer(BufferHandle* buffer) override;
        bool UpdateBufferData(BufferHandle* buffer, const void* data, uint64 dataByte) override;
        void* GetBufferMappedPointer(BufferHandle* buffer) override;

        //--------------------------------------------------
        // テクスチャ
        //--------------------------------------------------
        TextureHandle* CreateTexture(const TextureInfo& info) override;
        void DestroyTexture(TextureHandle* texture) override;

        //--------------------------------------------------
        // テクスチャビュー
        //--------------------------------------------------
        TextureViewHandle* CreateTextureView(TextureHandle* texture, const TextureViewInfo& info) override;
        void DestroyTextureView(TextureViewHandle* view) override;

        //--------------------------------------------------
        // サンプラ
        //--------------------------------------------------
        SamplerHandle* CreateSampler(const SamplerInfo& info) override;
        void DestroySampler(SamplerHandle* sampler) override;

        //--------------------------------------------------
        // フレームバッファ
        //--------------------------------------------------
        FramebufferHandle* CreateFramebuffer(RenderPassHandle* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height) override;
        void DestroyFramebuffer(FramebufferHandle* framebuffer) override;

        //--------------------------------------------------
        // レンダーパス
        //--------------------------------------------------
        RenderPassHandle* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies, uint32 numClearValue, RenderPassClearValue* clearValue) override;
        void DestroyRenderPass(RenderPassHandle* renderpass) override;
        
        //--------------------------------------------------
        // シェーダー
        //--------------------------------------------------
        ShaderHandle* CreateShader(const ShaderCompiledData& compiledData) override;
        void DestroyShader(ShaderHandle* shader) override;

        //--------------------------------------------------
        // デスクリプターセット
        //--------------------------------------------------
        DescriptorSetHandle* CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex) override;
        void UpdateDescriptorSet(DescriptorSetHandle* set, uint32 numdescriptors, DescriptorInfo* descriptors) override;
        void DestroyDescriptorSet(DescriptorSetHandle* descriptorset) override;

        //--------------------------------------------------
        // パイプライン
        //--------------------------------------------------
        PipelineHandle* CreateGraphicsPipeline(ShaderHandle* shader, PipelineStateInfo* info, RenderPassHandle* renderpass, uint32 renderSubpass = 0, PipelineDynamicStateFlags dynamicState = DYNAMIC_STATE_NONE) override;
        PipelineHandle* CreateComputePipeline(ShaderHandle* shader) override;
        void DestroyPipeline(PipelineHandle* pipeline) override;

        //--------------------------------------------------
        // コマンド
        //--------------------------------------------------
        void Cmd_PipelineBarrier(CommandBufferHandle* commandbuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrierInfo* memoryBarrier, uint32 numBufferBarrier, BufferBarrierInfo* bufferBarrier, uint32 numTextureBarrier, TextureBarrierInfo* textureBarrier) override;
        void Cmd_ClearBuffer(CommandBufferHandle* commandbuffer, BufferHandle* buffer, uint64 offset, uint64 size) override;
        void Cmd_CopyBuffer(CommandBufferHandle* commandbuffer, BufferHandle* srcBuffer, BufferHandle* dstBuffer, uint32 numRegion, BufferCopyRegion* regions) override;
        void Cmd_CopyTexture(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureCopyRegion* regions) override;
        void Cmd_ResolveTexture(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, uint32 srcLayer, uint32 srcMipmap, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 dstLayer, uint32 dstMipmap) override;
        void Cmd_ClearColorTexture(CommandBufferHandle* commandbuffer, TextureHandle* texture, TextureLayout textureLayout, const glm::vec4& color, const TextureSubresourceRange& subresources) override;
        void Cmd_CopyBufferToTexture(CommandBufferHandle* commandbuffer, BufferHandle* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions) override;
        void Cmd_CopyTextureToBuffer(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, BufferHandle* dstBuffer, uint32 numRegion, BufferTextureCopyRegion* regions) override;
        void Cmd_BlitTexture(CommandBufferHandle* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureBlitRegion* regions, SamplerFilter filter = SAMPLER_FILTER_LINEAR) override;

        void Cmd_PushConstants(CommandBufferHandle* commandbuffer, ShaderHandle* shader, const void* data, uint32 numData, uint32 offsetIndex = 0) override;
        void Cmd_BeginRenderPass(CommandBufferHandle* commandbuffer, RenderPassHandle* renderpass, FramebufferHandle* framebuffer, uint32 numView, TextureViewHandle** views, CommandBufferType commandBufferType = COMMAND_BUFFER_TYPE_PRIMARY) override;
        void Cmd_EndRenderPass(CommandBufferHandle* commandbuffer) override;
        void Cmd_NextRenderSubpass(CommandBufferHandle* commandbuffer, CommandBufferType commandBufferType) override;
        void Cmd_SetViewport(CommandBufferHandle* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void Cmd_SetScissor(CommandBufferHandle* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void Cmd_ClearAttachments(CommandBufferHandle* commandbuffer, uint32 numAttachmentClear, AttachmentClear** attachmentClears, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void Cmd_BindPipeline(CommandBufferHandle* commandbuffer, PipelineHandle* pipeline) override;
        void Cmd_BindDescriptorSet(CommandBufferHandle* commandbuffer, DescriptorSetHandle* descriptorset, uint32 setIndex) override;
        void Cmd_Draw(CommandBufferHandle* commandbuffer, uint32 vertexCount, uint32 instanceCount, uint32 baseVertex, uint32 firstInstance) override;
        void Cmd_DrawIndexed(CommandBufferHandle* commandbuffer, uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) override;
        void Cmd_BindVertexBuffers(CommandBufferHandle* commandbuffer, uint32 bindingCount, BufferHandle** buffers, uint64* offsets) override;
        void Cmd_BindVertexBuffer(CommandBufferHandle* commandbuffer, BufferHandle* buffer, uint64 offset) override;
        void Cmd_BindIndexBuffer(CommandBufferHandle* commandbuffer, BufferHandle* buffer, IndexBufferFormat format, uint64 offset) override;

        //--------------------------------------------------
        // MISC
        //--------------------------------------------------
        bool ImmidiateCommands(CommandQueueHandle* queue, CommandBufferHandle* commandBuffer, FenceHandle* fence, std::function<void(CommandBufferHandle*)>&& func) override;
        bool WaitDevice() override;


    private:

        // デスクリプタプールの管理
        VkDescriptorPool _FindOrCreateDescriptorPool(const VulkanDescriptorSet::PoolKey& key);
        void             _DecrementPoolRefCount(VkDescriptorPool pool, VulkanDescriptorSet::PoolKey& poolKey);

        // 利用可能サンプル数のチェック
        VkSampleCountFlagBits _CheckSupportedSampleCounts(TextureSamples samples);

        // スワップチェイン共通処理
        VulkanSwapChain*  _CreateSwapChain_Internal(VulkanSwapChain* swapchain, const VulkanSwapChain::Capability& cap);
        void              _DestroySwapChain_Internal(VulkanSwapChain* swapchain);
        VulkanRenderPass* _CreateSwapChainRenderPass(VkFormat format);
        bool              _QuerySwapChainCapability(VkSurfaceKHR surface, uint32 width, uint32 height, uint32 requestFramebufferCount, VkPresentModeKHR mode, VulkanSwapChain::Capability& out_cap);

    private:

        // デスクリプター型と個数では一意のハッシュ値を生成できないので、unordered_mapではなく、mapを採用
        std::map<VulkanDescriptorSet::PoolKey, std::unordered_map<VkDescriptorPool, uint32>> descriptorsetPools;

        // デバイス拡張機能関数
        PFN_vkCreateSwapchainKHR    CreateSwapchainKHR    = nullptr;
        PFN_vkDestroySwapchainKHR   DestroySwapchainKHR   = nullptr;
        PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR   AcquireNextImageKHR   = nullptr;
        PFN_vkQueuePresentKHR       QueuePresentKHR       = nullptr;

        // レンダリングコンテキスト
        VulkanContext* context = nullptr;

        // 論理デバイス
        VkDevice device = nullptr;

        // VMAアロケータ (VulkanMemoryAllocator: VkImage/VkBuffer に関るメモリ管理を代行)
        VmaAllocator allocator = nullptr;
    };
}
