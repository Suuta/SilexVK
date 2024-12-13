
#pragma once

#include "Rendering/RenderingCore.h"
#include "Rendering/ShaderCompiler.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


namespace Silex
{
    struct VulkanBuffer;
    struct VulkanTexture;
    struct VulkanTextureView;
    struct VulkanSampler;
    struct VulkanShader;
    struct VulkanFramebuffer;
    struct VulkanCommandQueue;
    struct VulkanCommandBuffer;
    struct VulkanCommandPool;
    struct VulkanFence;
    struct VulkanSemaphore;
    struct VulkanDescriptorSet;
    struct VulkanPipeline;
    struct VulkanRenderPass;
    struct VulkanSurface;
    struct VulkanSwapChain;

    template<class T> struct VulkanTypeTraits {};
    template<> struct VulkanTypeTraits<BufferHandle>        { using Internal = VulkanBuffer;        };
    template<> struct VulkanTypeTraits<TextureHandle>       { using Internal = VulkanTexture;       };
    template<> struct VulkanTypeTraits<TextureViewHandle>   { using Internal = VulkanTextureView;   };
    template<> struct VulkanTypeTraits<SamplerHandle>       { using Internal = VulkanSampler;       };
    template<> struct VulkanTypeTraits<ShaderHandle>        { using Internal = VulkanShader;        };
    template<> struct VulkanTypeTraits<FramebufferHandle>   { using Internal = VulkanFramebuffer;   };
    template<> struct VulkanTypeTraits<CommandQueueHandle>  { using Internal = VulkanCommandQueue;  };
    template<> struct VulkanTypeTraits<CommandBufferHandle> { using Internal = VulkanCommandBuffer; };
    template<> struct VulkanTypeTraits<CommandPoolHandle>   { using Internal = VulkanCommandPool;   };
    template<> struct VulkanTypeTraits<FenceHandle>         { using Internal = VulkanFence;         };
    template<> struct VulkanTypeTraits<SemaphoreHandle>     { using Internal = VulkanSemaphore;     };
    template<> struct VulkanTypeTraits<DescriptorSetHandle> { using Internal = VulkanDescriptorSet; };
    template<> struct VulkanTypeTraits<PipelineHandle>      { using Internal = VulkanPipeline;      };
    template<> struct VulkanTypeTraits<RenderPassHandle>    { using Internal = VulkanRenderPass;    };
    template<> struct VulkanTypeTraits<SurfaceHandle>       { using Internal = VulkanSurface;       };
    template<> struct VulkanTypeTraits<SwapChainHandle>     { using Internal = VulkanSwapChain;     };

    //=============================================
    // Vulkan 型キャスト
    //---------------------------------------------
    // 抽象型からAPI型への 型安全キャスト
    //=============================================
    template<typename T>
    SL_FORCEINLINE typename VulkanTypeTraits<T>::Internal* VulkanCast(T* type)
    {
        return static_cast<typename VulkanTypeTraits<T>::Internal*>(type);
    }

    template<typename T>
    SL_FORCEINLINE typename VulkanTypeTraits<T>::Internal** VulkanCast(T** type)
    {
        return reinterpret_cast<typename VulkanTypeTraits<T>::Internal**>(type);
    }



    //=============================================
    // Vulkan 構造体
    //=============================================

    // サーフェース
    struct VulkanSurface : public SurfaceHandle
    {
        VkSurfaceKHR surface = nullptr;
    };

    // コマンドキュー
    struct VulkanCommandQueue : public CommandQueueHandle
    {
        VkQueue queue  = nullptr;
        uint32  family = RENDER_INVALID_ID;
        uint32  index  = RENDER_INVALID_ID;
    };

    // コマンドプール
    struct VulkanCommandPool : public CommandPoolHandle
    {
        VkCommandPool     commandPool = nullptr;
        CommandBufferType type = COMMAND_BUFFER_TYPE_PRIMARY;
    };

    // コマンドバッファ
    struct VulkanCommandBuffer : public CommandBufferHandle
    {
        VkCommandBuffer commandBuffer = nullptr;
    };

    // セマフォ
    struct VulkanSemaphore : public SemaphoreHandle
    {
        VkSemaphore semaphore = nullptr;
    };

    // フェンス
    struct VulkanFence : public FenceHandle
    {
        VkFence fence = nullptr;
    };

    // レンダーパス
    struct VulkanRenderPass : public RenderPassHandle
    {
        VkRenderPass              renderpass = nullptr;
        std::vector<VkClearValue> clearValue = {};
    };

    // フレームバッファ
    struct VulkanFramebuffer : public FramebufferHandle
    {
        VkFramebuffer framebuffer = nullptr;
        Rect          rect        = {};
    };

    // スワップチェイン
    struct VulkanSwapChain : public SwapChainHandle
    { 
        // スワップチェイン情報クエリ
        struct Capability
        {
            uint32                        minImageCount;
            VkFormat                      format;
            VkColorSpaceKHR               colorspace;
            VkExtent2D                    extent;
            VkSurfaceTransformFlagBitsKHR transform;
            VkCompositeAlphaFlagBitsKHR   compositeAlpha;
            VkPresentModeKHR              presentMode;
        };

        VkSwapchainKHR    swapchain  = nullptr;
        VulkanSurface*    surface    = nullptr;
        VulkanRenderPass* renderpass = nullptr;
        Capability        capability = {};

        uint32                          imageIndex   = 0;
        std::vector<FramebufferHandle*> framebuffers = {};
        std::vector<TextureHandle*>     textures     = {};
        std::vector<TextureViewHandle*> views        = {};

        VkSemaphore present = nullptr;
        VkSemaphore render  = nullptr;
    };

    // バッファ
    struct VulkanBuffer : public BufferHandle
    {
        VkBuffer      buffer           = nullptr;
        VkBufferView  view             = nullptr;
        uint64        size             = 0;
        VmaAllocation allocationHandle = nullptr;

        bool  mapped  = false;
        void* pointer = nullptr;
    };

    // テクスチャ
    struct VulkanTexture : public TextureHandle
    {
        VkImage            image       = nullptr;
        VkFormat           format      = {};
        VkImageUsageFlags  usageflags  = {};
        VkImageCreateFlags createFlags = {};
        VkExtent3D         extent      = {};
        uint32             arrayLayers = 1;
        uint32             mipLevels   = 1;

        VmaAllocation allocationHandle = nullptr;
    };

    // テクスチャビュー
    struct VulkanTextureView : public TextureViewHandle
    {
        VkImageSubresourceRange subresource = {};
        VkImageView             view        = nullptr;
    };

    // サンプラー
    struct VulkanSampler : public SamplerHandle
    {
        VkSampler sampler = nullptr;
    };

    // シェーダー
    struct VulkanShader : public ShaderHandle
    {
        std::vector<VkPipelineShaderStageCreateInfo> stageInfos           = {};
        std::vector<VkDescriptorSetLayout>           descriptorsetLayouts = {};
        VkPipelineLayout                             pipelineLayout       = nullptr;
        ShaderReflectionData*                        reflection           = nullptr;
    };

    // デスクリプターセット
    struct VulkanDescriptorSet : public DescriptorSetHandle
    {
        VkDescriptorSet  descriptorSet  = nullptr;
        VkDescriptorPool descriptorPool = nullptr;
        VkPipelineLayout pipelineLayout = nullptr;

        std::vector<VkWriteDescriptorSet> writes;

        // プール検索キー
        struct PoolKey
        {
            uint16 descriptorTypeCounts[DESCRIPTOR_TYPE_MAX] = {};
            bool operator<(const PoolKey& other) const
            {
                return memcmp(descriptorTypeCounts, other.descriptorTypeCounts, sizeof(descriptorTypeCounts)) < 0;
            }
        };

        PoolKey poolKey;
    };

    // パイプライン
    struct VulkanPipeline : public PipelineHandle
    {
        VkPipeline pipeline = nullptr;
    };
}
