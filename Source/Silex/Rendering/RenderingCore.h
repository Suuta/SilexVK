
#pragma once
#include "Core/Core.h"

namespace Silex
{
    //================================================
    // 定数
    //================================================
    enum : int32  { RENDER_INVALID_ID = 0x7FFFFFFF }; // INT_MAX
    enum : uint32 { RENDER_AUTO_ID    = 0xFFFFFFFF }; // UINT32_MAX

    //================================================
    // ハンドル
    //================================================
    using QueueID = uint32;

    SL_DECLARE_HANDLE(SurfaceHandle);
    SL_DECLARE_HANDLE(CommandQueueHandle);
    SL_DECLARE_HANDLE(CommandPoolHandle);
    SL_DECLARE_HANDLE(CommandBufferHandle);
    SL_DECLARE_HANDLE(FenceHandle);
    SL_DECLARE_HANDLE(SemaphoreHandle);
    SL_DECLARE_HANDLE(SwapChainHandle);
    SL_DECLARE_HANDLE(RenderPassHandle);
    SL_DECLARE_HANDLE(BufferHandle);
    SL_DECLARE_HANDLE(PipelineHandle);
    SL_DECLARE_HANDLE(SamplerHandle);
    SL_DECLARE_HANDLE(DescriptorSetHandle);
    SL_DECLARE_HANDLE(FramebufferHandle);
    SL_DECLARE_HANDLE(TextureViewHandle);
    SL_DECLARE_HANDLE(TextureHandle);
    SL_DECLARE_HANDLE(ShaderHandle);


    //================================================
    // ビューポート
    //================================================
    enum VSyncMode
    {
        VSYNC_MODE_DISABLED, // VK_PRESENT_MODE_IMMEDIATE_KHR
        VSYNC_MODE_MAILBOX,  // VK_PRESENT_MODE_MAILBOX_KHR
        VSYNC_MODE_ENABLED,  // VK_PRESENT_MODE_FIFO_KHR
        VSYNC_MODE_ADAPTIVE, // VK_PRESENT_MODE_FIFO_RELAXED_KHR
    };

    struct Rect
    {
        uint32 x      = 0;
        uint32 y      = 0;
        uint32 width  = 0;
        uint32 height = 0;
    };

    struct Extent
    {
        uint32 width  = 1;
        uint32 height = 1;
        uint32 depth  = 1;
    };

    //================================================
    // デバイス
    //================================================
    //https://pcisig.com/membership/member-companies?combine=&order=field_vendor_id&sort=asc&page=1
    enum DeviceVendor
    {
        DEVICE_VENDOR_UNKNOWN   = 0x0000,
        DEVICE_VENDOR_AMD       = 0x1022,
        DEVICE_VENDOR_NVIDIA    = 0x10DE,
        DEVICE_VENDOR_ARM       = 0x13B5,
        DEVICE_VENDOR_INTEL     = 0x8086,
      //DEVICE_VENDOR_APPLE     = 0x106B,
      //DEVICE_VENDOR_QUALCOMM  = 0x17CB,
    };

    enum DeviceType
    {
        DEVICE_TYPE_UNKNOW,
        DEVICE_TYPE_INTEGRATED_GPU,
        DEVICE_TYPE_DISCRETE_GPU,
        DEVICE_TYPE_VIRTUAL_GPU,
        DEVICE_TYPE_CPU,

        DEVICE_TYPE_MAX,
    };

    struct DeviceInfo
    {
        std::string  name   = "Unknown";
        DeviceVendor vendor = DEVICE_VENDOR_UNKNOWN;
        DeviceType   type   = DEVICE_TYPE_UNKNOW;
    };

    //================================================
    // フォーマット
    //================================================
    enum RenderingFormat
    {
        RENDERING_FORMAT_UNDEFINE,
        RENDERING_FORMAT_R4G4_UNORM_PACK8,
        RENDERING_FORMAT_R4G4B4A4_UNORM_PACK16,
        RENDERING_FORMAT_B4G4R4A4_UNORM_PACK16,
        RENDERING_FORMAT_R5G6B5_UNORM_PACK16,
        RENDERING_FORMAT_B5G6R5_UNORM_PACK16,
        RENDERING_FORMAT_R5G5B5A1_UNORM_PACK16,
        RENDERING_FORMAT_B5G5R5A1_UNORM_PACK16,
        RENDERING_FORMAT_A1R5G5B5_UNORM_PACK16,
        RENDERING_FORMAT_R8_UNORM,
        RENDERING_FORMAT_R8_SNORM,
        RENDERING_FORMAT_R8_USCALED,
        RENDERING_FORMAT_R8_SSCALED,
        RENDERING_FORMAT_R8_UINT,
        RENDERING_FORMAT_R8_SINT,
        RENDERING_FORMAT_R8_SRGB,
        RENDERING_FORMAT_R8G8_UNORM,
        RENDERING_FORMAT_R8G8_SNORM,
        RENDERING_FORMAT_R8G8_USCALED,
        RENDERING_FORMAT_R8G8_SSCALED,
        RENDERING_FORMAT_R8G8_UINT,
        RENDERING_FORMAT_R8G8_SINT,
        RENDERING_FORMAT_R8G8_SRGB,
        RENDERING_FORMAT_R8G8B8_UNORM,
        RENDERING_FORMAT_R8G8B8_SNORM,
        RENDERING_FORMAT_R8G8B8_USCALED,
        RENDERING_FORMAT_R8G8B8_SSCALED,
        RENDERING_FORMAT_R8G8B8_UINT,
        RENDERING_FORMAT_R8G8B8_SINT,
        RENDERING_FORMAT_R8G8B8_SRGB,
        RENDERING_FORMAT_B8G8R8_UNORM,
        RENDERING_FORMAT_B8G8R8_SNORM,
        RENDERING_FORMAT_B8G8R8_USCALED,
        RENDERING_FORMAT_B8G8R8_SSCALED,
        RENDERING_FORMAT_B8G8R8_UINT,
        RENDERING_FORMAT_B8G8R8_SINT,
        RENDERING_FORMAT_B8G8R8_SRGB,
        RENDERING_FORMAT_R8G8B8A8_UNORM,
        RENDERING_FORMAT_R8G8B8A8_SNORM,
        RENDERING_FORMAT_R8G8B8A8_USCALED,
        RENDERING_FORMAT_R8G8B8A8_SSCALED,
        RENDERING_FORMAT_R8G8B8A8_UINT,
        RENDERING_FORMAT_R8G8B8A8_SINT,
        RENDERING_FORMAT_R8G8B8A8_SRGB,
        RENDERING_FORMAT_B8G8R8A8_UNORM,
        RENDERING_FORMAT_B8G8R8A8_SNORM,
        RENDERING_FORMAT_B8G8R8A8_USCALED,
        RENDERING_FORMAT_B8G8R8A8_SSCALED,
        RENDERING_FORMAT_B8G8R8A8_UINT,
        RENDERING_FORMAT_B8G8R8A8_SINT,
        RENDERING_FORMAT_B8G8R8A8_SRGB,
        RENDERING_FORMAT_A8B8G8R8_UNORM_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SNORM_PACK32,
        RENDERING_FORMAT_A8B8G8R8_USCALED_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SSCALED_PACK32,
        RENDERING_FORMAT_A8B8G8R8_UINT_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SINT_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SRGB_PACK32,
        RENDERING_FORMAT_A2R10G10B10_UNORM_PACK32,
        RENDERING_FORMAT_A2R10G10B10_SNORM_PACK32,
        RENDERING_FORMAT_A2R10G10B10_USCALED_PACK32,
        RENDERING_FORMAT_A2R10G10B10_SSCALED_PACK32,
        RENDERING_FORMAT_A2R10G10B10_UINT_PACK32,
        RENDERING_FORMAT_A2R10G10B10_SINT_PACK32,
        RENDERING_FORMAT_A2B10G10R10_UNORM_PACK32,
        RENDERING_FORMAT_A2B10G10R10_SNORM_PACK32,
        RENDERING_FORMAT_A2B10G10R10_USCALED_PACK32,
        RENDERING_FORMAT_A2B10G10R10_SSCALED_PACK32,
        RENDERING_FORMAT_A2B10G10R10_UINT_PACK32,
        RENDERING_FORMAT_A2B10G10R10_SINT_PACK32,
        RENDERING_FORMAT_R16_UNORM,
        RENDERING_FORMAT_R16_SNORM,
        RENDERING_FORMAT_R16_USCALED,
        RENDERING_FORMAT_R16_SSCALED,
        RENDERING_FORMAT_R16_UINT,
        RENDERING_FORMAT_R16_SINT,
        RENDERING_FORMAT_R16_SFLOAT,
        RENDERING_FORMAT_R16G16_UNORM,
        RENDERING_FORMAT_R16G16_SNORM,
        RENDERING_FORMAT_R16G16_USCALED,
        RENDERING_FORMAT_R16G16_SSCALED,
        RENDERING_FORMAT_R16G16_UINT,
        RENDERING_FORMAT_R16G16_SINT,
        RENDERING_FORMAT_R16G16_SFLOAT,
        RENDERING_FORMAT_R16G16B16_UNORM,
        RENDERING_FORMAT_R16G16B16_SNORM,
        RENDERING_FORMAT_R16G16B16_USCALED,
        RENDERING_FORMAT_R16G16B16_SSCALED,
        RENDERING_FORMAT_R16G16B16_UINT,
        RENDERING_FORMAT_R16G16B16_SINT,
        RENDERING_FORMAT_R16G16B16_SFLOAT,
        RENDERING_FORMAT_R16G16B16A16_UNORM,
        RENDERING_FORMAT_R16G16B16A16_SNORM,
        RENDERING_FORMAT_R16G16B16A16_USCALED,
        RENDERING_FORMAT_R16G16B16A16_SSCALED,
        RENDERING_FORMAT_R16G16B16A16_UINT,
        RENDERING_FORMAT_R16G16B16A16_SINT,
        RENDERING_FORMAT_R16G16B16A16_SFLOAT,
        RENDERING_FORMAT_R32_UINT,
        RENDERING_FORMAT_R32_SINT,
        RENDERING_FORMAT_R32_SFLOAT,
        RENDERING_FORMAT_R32G32_UINT,
        RENDERING_FORMAT_R32G32_SINT,
        RENDERING_FORMAT_R32G32_SFLOAT,
        RENDERING_FORMAT_R32G32B32_UINT,
        RENDERING_FORMAT_R32G32B32_SINT,
        RENDERING_FORMAT_R32G32B32_SFLOAT,
        RENDERING_FORMAT_R32G32B32A32_UINT,
        RENDERING_FORMAT_R32G32B32A32_SINT,
        RENDERING_FORMAT_R32G32B32A32_SFLOAT,
        RENDERING_FORMAT_R64_UINT,
        RENDERING_FORMAT_R64_SINT,
        RENDERING_FORMAT_R64_SFLOAT,
        RENDERING_FORMAT_R64G64_UINT,
        RENDERING_FORMAT_R64G64_SINT,
        RENDERING_FORMAT_R64G64_SFLOAT,
        RENDERING_FORMAT_R64G64B64_UINT,
        RENDERING_FORMAT_R64G64B64_SINT,
        RENDERING_FORMAT_R64G64B64_SFLOAT,
        RENDERING_FORMAT_R64G64B64A64_UINT,
        RENDERING_FORMAT_R64G64B64A64_SINT,
        RENDERING_FORMAT_R64G64B64A64_SFLOAT,
        RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32,
        RENDERING_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        RENDERING_FORMAT_D16_UNORM,
        RENDERING_FORMAT_X8_D24_UNORM_PACK32,
        RENDERING_FORMAT_D32_SFLOAT,
        RENDERING_FORMAT_S8_UINT,
        RENDERING_FORMAT_D16_UNORM_S8_UINT,
        RENDERING_FORMAT_D24_UNORM_S8_UINT,
        RENDERING_FORMAT_D32_SFLOAT_S8_UINT,
        RENDERING_FORMAT_BC1_RGB_UNORM_BLOCK,
        RENDERING_FORMAT_BC1_RGB_SRGB_BLOCK,
        RENDERING_FORMAT_BC1_RGBA_UNORM_BLOCK,
        RENDERING_FORMAT_BC1_RGBA_SRGB_BLOCK,
        RENDERING_FORMAT_BC2_UNORM_BLOCK,
        RENDERING_FORMAT_BC2_SRGB_BLOCK,
        RENDERING_FORMAT_BC3_UNORM_BLOCK,
        RENDERING_FORMAT_BC3_SRGB_BLOCK,
        RENDERING_FORMAT_BC4_UNORM_BLOCK,
        RENDERING_FORMAT_BC4_SNORM_BLOCK,
        RENDERING_FORMAT_BC5_UNORM_BLOCK,
        RENDERING_FORMAT_BC5_SNORM_BLOCK,
        RENDERING_FORMAT_BC6H_UFLOAT_BLOCK,
        RENDERING_FORMAT_BC6H_SFLOAT_BLOCK,
        RENDERING_FORMAT_BC7_UNORM_BLOCK,
        RENDERING_FORMAT_BC7_SRGB_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        RENDERING_FORMAT_EAC_R11_UNORM_BLOCK,
        RENDERING_FORMAT_EAC_R11_SNORM_BLOCK,
        RENDERING_FORMAT_EAC_R11G11_UNORM_BLOCK,
        RENDERING_FORMAT_EAC_R11G11_SNORM_BLOCK,
        RENDERING_FORMAT_ASTC_4x4_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_4x4_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_5x4_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_5x4_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_5x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_5x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_6x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_6x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_6x6_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_6x6_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_8x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_8x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_8x6_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_8x6_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_8x8_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_8x8_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x6_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x6_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x8_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x8_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x10_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x10_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_12x10_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_12x10_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_12x12_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_12x12_SRGB_BLOCK,
        RENDERING_FORMAT_G8B8G8R8_422_UNORM,
        RENDERING_FORMAT_B8G8R8G8_422_UNORM,
        RENDERING_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        RENDERING_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        RENDERING_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
        RENDERING_FORMAT_G8_B8R8_2PLANE_422_UNORM,
        RENDERING_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
        RENDERING_FORMAT_R10X6_UNORM_PACK16,
        RENDERING_FORMAT_R10X6G10X6_UNORM_2PACK16,
        RENDERING_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        RENDERING_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
        RENDERING_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
        RENDERING_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        RENDERING_FORMAT_R12X4_UNORM_PACK16,
        RENDERING_FORMAT_R12X4G12X4_UNORM_2PACK16,
        RENDERING_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
        RENDERING_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
        RENDERING_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
        RENDERING_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        RENDERING_FORMAT_G16B16G16R16_422_UNORM,
        RENDERING_FORMAT_B16G16R16G16_422_UNORM,
        RENDERING_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
        RENDERING_FORMAT_G16_B16R16_2PLANE_420_UNORM,
        RENDERING_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
        RENDERING_FORMAT_G16_B16R16_2PLANE_422_UNORM,
        RENDERING_FORMAT_G16_B16_R16_3PLANE_444_UNORM,

        RENDERING_FORMAT_MAX,
    };

    //================================================
    // 比較関数
    //================================================
    enum CompareOperator
    {
        COMPARE_OP_NEVER,
        COMPARE_OP_LESS,
        COMPARE_OP_EQUAL,
        COMPARE_OP_LESS_OR_EQUAL,
        COMPARE_OP_GREATER,
        COMPARE_OP_NOT_EQUAL,
        COMPARE_OP_GREATER_OR_EQUAL,
        COMPARE_OP_ALWAYS,

        COMPARE_OP_MAX
    };

    //================================================
    // メモリ
    //================================================
    enum MemoryAllocationType
    {
        MEMORY_ALLOCATION_TYPE_CPU,
        MEMORY_ALLOCATION_TYPE_GPU,

        MEMORY_ALLOCATION_TYPE_MAX,
    };

    //================================================
    // キューファミリ
    //================================================
    enum QueueFamilyBits
    {
        QUEUE_FAMILY_GRAPHICS_BIT = SL_BIT(0),
        QUEUE_FAMILY_COMPUTE_BIT  = SL_BIT(1),
        QUEUE_FAMILY_TRANSFER_BIT = SL_BIT(2),
    };
    using QueueFamilyFlags = uint32;

    //================================================
    // コマンドバッファ
    //================================================
    enum CommandBufferType
    {
        COMMAND_BUFFER_TYPE_PRIMARY,
        COMMAND_BUFFER_TYPE_SECONDARY,

        COMMAND_BUFFER_TYPE_MAX,
    };

    //=================================================
    // バッファ
    //=================================================
    enum BufferUsageBits
    {
        BUFFER_USAGE_TRANSFER_SRC_BIT         = SL_BIT(0),
        BUFFER_USAGE_TRANSFER_DST_BIT         = SL_BIT(1),
        BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = SL_BIT(2),
        BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = SL_BIT(3),
        BUFFER_USAGE_UNIFORM_BIT              = SL_BIT(4),
        BUFFER_USAGE_STORAGE_BIT              = SL_BIT(5),
        BUFFER_USAGE_INDEX_BIT                = SL_BIT(6),
        BUFFER_USAGE_VERTEX_BIT               = SL_BIT(7),
        BUFFER_USAGE_INDIRECT_BIT             = SL_BIT(8),
    };
    using BufferUsageFlags = uint32;

    //=================================================
    // テクスチャ
    //=================================================
    enum TextureDimension
    {
        TEXTURE_DIMENSION_1D,
        TEXTURE_DIMENSION_2D,
        TEXTURE_DIMENSION_3D,

        TEXTURE_DIMENSION_MAX,
    };

    enum TextureType
    {
        TEXTURE_TYPE_1D,
        TEXTURE_TYPE_2D,
        TEXTURE_TYPE_3D,
        TEXTURE_TYPE_CUBE,
        TEXTURE_TYPE_1D_ARRAY,
        TEXTURE_TYPE_2D_ARRAY,
        TEXTURE_TYPE_CUBE_ARRAY,

        TEXTURE_TYPE_MAX,
    };

    enum TextureSamples
    {
        TEXTURE_SAMPLES_1,
        TEXTURE_SAMPLES_2,
        TEXTURE_SAMPLES_4,
        TEXTURE_SAMPLES_8,
        TEXTURE_SAMPLES_16,
        TEXTURE_SAMPLES_32,
        TEXTURE_SAMPLES_64,

        TEXTURE_SAMPLES_MAX,
    };

    enum TextureLayout
    {
        TEXTURE_LAYOUT_UNDEFINED,
        TEXTURE_LAYOUT_GENERAL,
        TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL,

        TEXTURE_LAYOUT_PRESENT_SRC = 1000001002,
    };

    enum TextureAspectBits
    {
        TEXTURE_ASPECT_COLOR_BIT   = SL_BIT(0),
        TEXTURE_ASPECT_DEPTH_BIT   = SL_BIT(1),
        TEXTURE_ASPECT_STENCIL_BIT = SL_BIT(2),
    };
    using TextureAspectFlags = uint32;

    enum TextureUsageBits
    {
        TEXTURE_USAGE_COPY_SRC_BIT                 = SL_BIT(0),
        TEXTURE_USAGE_COPY_DST_BIT                 = SL_BIT(1),
        TEXTURE_USAGE_SAMPLING_BIT                 = SL_BIT(2),
        TEXTURE_USAGE_STORAGE_BIT                  = SL_BIT(3),
        TEXTURE_USAGE_COLOR_ATTACHMENT_BIT         = SL_BIT(4),
        TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = SL_BIT(5),
        TEXTURE_USAGE_TRANSIENT_ATTACHMENT_BIT     = SL_BIT(6),
        TEXTURE_USAGE_INPUT_ATTACHMENT_BIT         = SL_BIT(7),
        TEXTURE_USAGE_CPU_READ_BIT                 = SL_BIT(8),
    };
    using TextureUsageFlags = uint32;


    // コピー・Blit 系のサブリソース指定
    struct TextureSubresource
    {
        // 残念ながら VK_REMAINING_ARRAY_LAYERS (UINT32_MAX)は指定できず、必ず対象イメージのレイヤー数を指定する必要がある
        // VK_REMAINING_ARRAY_LAYERS を指定するには maintenance5 拡張を有効にする必要がある
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMaintenance5FeaturesKHR.html

        TextureAspectFlags aspect     = TEXTURE_ASPECT_COLOR_BIT;
        uint32             mipLevel   = 0;
        uint32             baseLayer  = 0;
        uint32             layerCount = 1;
    };

    //===========================================================================
    // ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ 
    //---------------------------------------------------------------------------
    // TODO: 類似構造体の削除
    //---------------------------------------------------------------------------
    // ミップレベルが複数かどうかの違いしかないが、VulkanAPI で区別されているので現状は分けておく
    // 分かり辛いのでこの構造体は統合する予定、Vulkan にはこのような類似した構造体が存在するので
    // 可能な限り修正したい...
    //---------------------------------------------------------------------------
    // ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ 
    //===========================================================================

    // ビュー・バリアのサブリソース範囲指定
    struct TextureSubresourceRange
    {
        TextureAspectFlags aspect        = TEXTURE_ASPECT_COLOR_BIT;
        uint32             baseMipLevel  = 0;
        uint32             mipLevelCount = RENDER_AUTO_ID; //VK_REMAINING_ARRAY_LAYERS
        uint32             baseLayer     = 0;
        uint32             layerCount    = RENDER_AUTO_ID; // VK_REMAINING_MIP_LEVELS
    };

    struct TextureInfo
    {
        RenderingFormat   format    = RENDERING_FORMAT_UNDEFINE;
        uint32            width     = 1;
        uint32            height    = 1;
        uint32            depth     = 1;
        uint32            array     = 1;
        uint32            mipLevels = 1;
        TextureDimension  dimension = TEXTURE_DIMENSION_2D;
        TextureType       type      = TEXTURE_TYPE_2D;
        TextureSamples    samples   = TEXTURE_SAMPLES_1;
        TextureUsageFlags usageBits = 0;
    };

    struct TextureViewInfo
    {
        TextureType             type        = TEXTURE_TYPE_2D;
        TextureSubresourceRange subresource = {};
    };

    //================================================
    // サンプラー
    //================================================
    enum SamplerFilter
    {
        SAMPLER_FILTER_NEAREST,
        SAMPLER_FILTER_LINEAR,

        SAMPLER_FILTER_MAX,
    };

    enum SamplerRepeatMode
    {
        SAMPLER_REPEAT_MODE_REPEAT,
        SAMPLER_REPEAT_MODE_MIRRORED_REPEAT,
        SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE,
        SAMPLER_REPEAT_MODE_CLAMP_TO_BORDER,
        SAMPLER_REPEAT_MODE_MIRROR_CLAMP_TO_EDGE,

        SAMPLER_REPEAT_MODE_MAX
    };

    enum SamplerBorderColor
    {
        SAMPLER_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        SAMPLER_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        SAMPLER_BORDER_COLOR_INT_OPAQUE_BLACK,
        SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        SAMPLER_BORDER_COLOR_INT_OPAQUE_WHITE,

        SAMPLER_BORDER_COLOR_MAX
    };

    struct SamplerInfo
    {
        SamplerFilter      magFilter     = SAMPLER_FILTER_LINEAR;
        SamplerFilter      minFilter     = SAMPLER_FILTER_LINEAR;
        SamplerFilter      mipFilter     = SAMPLER_FILTER_LINEAR;
        SamplerRepeatMode  repeatU       = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        SamplerRepeatMode  repeatV       = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        SamplerRepeatMode  repeatW       = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        float              lodBias       = 0.0f;
        bool               useAnisotropy = false;
        float              anisotropyMax = 0.0f;
        bool               enableCompare = false;
        CompareOperator    compareOp     = COMPARE_OP_ALWAYS;
        float              minLod        = 0.0f;
        float              maxLod        = 0.0f;
        SamplerBorderColor borderColor   = SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        bool               unnormalized  = false;
    };

    //================================================
    // バリア
    //================================================
    enum PipelineStageBits
    {
        PIPELINE_STAGE_TOP_OF_PIPE_BIT                    = SL_BIT(0),
        PIPELINE_STAGE_DRAW_INDIRECT_BIT                  = SL_BIT(1),
        PIPELINE_STAGE_VERTEX_INPUT_BIT                   = SL_BIT(2),
        PIPELINE_STAGE_VERTEX_SHADER_BIT                  = SL_BIT(3),
        PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT    = SL_BIT(4),
        PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = SL_BIT(5),
        PIPELINE_STAGE_GEOMETRY_SHADER_BIT                = SL_BIT(6),
        PIPELINE_STAGE_FRAGMENT_SHADER_BIT                = SL_BIT(7),
        PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT           = SL_BIT(8),
        PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT            = SL_BIT(9),
        PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT        = SL_BIT(10),
        PIPELINE_STAGE_COMPUTE_SHADER_BIT                 = SL_BIT(11),
        PIPELINE_STAGE_TRANSFER_BIT                       = SL_BIT(12),
        PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT                 = SL_BIT(13),
        PIPELINE_STAGE_HOST_BIT                           = SL_BIT(14),
        PIPELINE_STAGE_ALL_GRAPHICS_BIT                   = SL_BIT(15),
        PIPELINE_STAGE_ALL_COMMANDS_BIT                   = SL_BIT(16),
    };
    using PipelineStageFlags = uint32;

    enum BarrierAccessBits
    {
        BARRIER_ACCESS_INDIRECT_COMMAND_READ_BIT          = SL_BIT(0),
        BARRIER_ACCESS_INDEX_READ_BIT                     = SL_BIT(1),
        BARRIER_ACCESS_VERTEX_ATTRIBUTE_READ_BIT          = SL_BIT(2),
        BARRIER_ACCESS_UNIFORM_READ_BIT                   = SL_BIT(3),
        BARRIER_ACCESS_INPUT_ATTACHMENT_READ_BIT          = SL_BIT(4),
        BARRIER_ACCESS_SHADER_READ_BIT                    = SL_BIT(5),
        BARRIER_ACCESS_SHADER_WRITE_BIT                   = SL_BIT(6),
        BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT          = SL_BIT(7),
        BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         = SL_BIT(8),
        BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  = SL_BIT(9),
        BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = SL_BIT(10),
        BARRIER_ACCESS_TRANSFER_READ_BIT                  = SL_BIT(11),
        BARRIER_ACCESS_TRANSFER_WRITE_BIT                 = SL_BIT(12),
        BARRIER_ACCESS_HOST_READ_BIT                      = SL_BIT(13),
        BARRIER_ACCESS_HOST_WRITE_BIT                     = SL_BIT(14),
        BARRIER_ACCESS_MEMORY_READ_BIT                    = SL_BIT(15),
        BARRIER_ACCESS_MEMORY_WRITE_BIT                   = SL_BIT(16),
    };
    using BarrierAccessFlags = uint32;

    struct MemoryBarrierInfo
    {
        BarrierAccessFlags srcAccess;
        BarrierAccessFlags dstAccess;
    };

    struct BufferBarrierInfo
    {
        BufferHandle*            buffer;
        BarrierAccessFlags srcAccess;
        BarrierAccessFlags dstAccess;
        uint64             offset;
        uint64             size;
    };

    struct TextureBarrierInfo
    {
        TextureHandle*          texture;
        BarrierAccessFlags      srcAccess;
        BarrierAccessFlags      dstAccess;
        TextureLayout           oldLayout = TEXTURE_LAYOUT_UNDEFINED;
        TextureLayout           newLayout = TEXTURE_LAYOUT_UNDEFINED;
        TextureSubresourceRange subresources;
    };

    //================================================
    // レンダーパス
    //================================================
    enum AttachmentLoadOp
    {
        ATTACHMENT_LOAD_OP_LOAD,
        ATTACHMENT_LOAD_OP_CLEAR,
        ATTACHMENT_LOAD_OP_DONT_CARE,

        ATTACHMENT_LOAD_OP_MAX,
    };

    enum AttachmentStoreOp
    {
        ATTACHMENT_STORE_OP_STORE,
        ATTACHMENT_STORE_OP_DONT_CARE,

        ATTACHMENT_STORE_OP_MAX,
    };

    struct Attachment
    {
        RenderingFormat   format         = RENDERING_FORMAT_MAX;
        TextureSamples    samples        = TEXTURE_SAMPLES_MAX;
        AttachmentLoadOp  loadOp         = ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentStoreOp storeOp        = ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentLoadOp  stencilLoadOp  = ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentStoreOp stencilStoreOp = ATTACHMENT_STORE_OP_DONT_CARE;
        TextureLayout     initialLayout  = TEXTURE_LAYOUT_UNDEFINED;
        TextureLayout     finalLayout    = TEXTURE_LAYOUT_UNDEFINED;
    };

    struct AttachmentReference
    {
        uint32        attachment = RENDER_INVALID_ID;
        TextureLayout layout     = TEXTURE_LAYOUT_UNDEFINED;
    };

    struct Subpass
    {
        std::vector<AttachmentReference> inputReferences;
        std::vector<AttachmentReference> colorReferences;
        AttachmentReference              depthstencilReference;
        AttachmentReference              resolveReferences;
        std::vector<uint32>              preserveAttachments;
    };

    struct SubpassDependency
    {
        uint32             srcSubpass = UINT32_MAX; // VK_SUBPASS_EXTERNAL
        uint32             dstSubpass = 0;
        PipelineStageFlags srcStages;
        PipelineStageFlags dstStages;
        BarrierAccessFlags srcAccess;
        BarrierAccessFlags dstAccess;
    };

    //================================================
    // デスクリプタ
    //================================================
    enum DescriptorType
    {
        DESCRIPTOR_TYPE_SAMPLER,                // VK_DESCRIPTOR_TYPE_SAMPLER
        DESCRIPTOR_TYPE_IMAGE_SAMPLER,          // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        DESCRIPTOR_TYPE_IMAGE,                  // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        DESCRIPTOR_TYPE_STORAGE_IMAGE,          // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        DESCRIPTOR_TYPE_UNIFORM_TEXTURE_BUFFER, // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
        DESCRIPTOR_TYPE_STORAGE_TEXTURE_BUFFER, // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        DESCRIPTOR_TYPE_STORAGE_BUFFER,         // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
        DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
        DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT

        DESCRIPTOR_TYPE_MAX
    };

    struct DescriptorHandle
    {
        BufferHandle*      buffer    = nullptr;
        TextureViewHandle* imageView = nullptr;
        SamplerHandle*     sampler   = nullptr;
    };

    //=========================================================
    // "handle" はそのデスクリプターへのオブジェクトハンドルを指定する
    // 実装は未定だが、バインドレス実装によって、可変長サイズハンドルが必要
    // になった場合は、配列化にするとともに APIレイヤーの実装も変更する
    //=========================================================
    struct DescriptorInfo
    {
        DescriptorType   type    = DESCRIPTOR_TYPE_MAX;
        uint32           binding = 0;
        DescriptorHandle handles;
    };

    struct DescriptorSetInfo
    {
        std::vector<DescriptorInfo> infos;

        void BindTexture(uint32 binding, DescriptorType type, TextureViewHandle* view, SamplerHandle* sampler)
        {
            if (infos.size() < binding + 1)
                infos.resize(binding + 1);

            DescriptorInfo& info = infos[binding];
            info.binding = binding;
            info.type    = type;
            info.handles = DescriptorHandle{ nullptr, view, sampler };
        }

        void BindBuffer(uint32 binding, DescriptorType type, BufferHandle* buffer)
        {
            if (infos.size() < binding + 1)
                infos.resize(binding + 1);

            DescriptorInfo& info = infos[binding];
            info.binding = binding;
            info.type    = type;
            info.handles = DescriptorHandle{ buffer, nullptr, nullptr };
        }
    };

    //================================================
    // シェーダー
    //================================================
    enum ShaderStage
    {
        SHADER_STAGE_VERTEX_BIT                 = SL_BIT(0),
        SHADER_STAGE_TESSELATION_CONTROL_BIT    = SL_BIT(1),
        SHADER_STAGE_TESSELATION_EVALUATION_BIT = SL_BIT(2),
        SHADER_STAGE_GEOMETRY_BIT               = SL_BIT(3),
        SHADER_STAGE_FRAGMENT_BIT               = SL_BIT(4),
        SHADER_STAGE_COMPUTE_BIT                = SL_BIT(5),

        SHADER_STAGE_ALL                        = RENDER_INVALID_ID,
    };
    using ShaderStageFlags = uint32;

    //================================================
    // パイプライン
    //================================================
    enum PrimitiveTopology
    {
        PRIMITIVE_TOPOLOGY_POINTS,                        // VK_PRIMITIVE_TOPOLOGY_POINT_LIST                    = 0,
        PRIMITIVE_TOPOLOGY_LINES,                         // VK_PRIMITIVE_TOPOLOGY_LINE_LIST                     = 1,
        PRIMITIVE_TOPOLOGY_LINESTRIPS,                    // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP                    = 2,
        PRIMITIVE_TOPOLOGY_TRIANGLES,                     // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST                 = 3,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIPS,               // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP                = 4,
        PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,                  // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN                  = 5,
        PRIMITIVE_TOPOLOGY_LINES_WITH_ADJACENCY,          // VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY      = 6,
        PRIMITIVE_TOPOLOGY_LINESTRIPS_WITH_ADJACENCY,     // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY     = 7,
        PRIMITIVE_TOPOLOGY_TRIANGLES_WITH_ADJACENCY,      // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY  = 8,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIPS_WITH_AJACENCY, // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
        PRIMITIVE_TOPOLOGY_TESSELATION_PATCH,             // VK_PRIMITIVE_TOPOLOGY_PATCH_LIST                    = 10,

        RENDER_PRIMITIVE_MAX                              // VK_PRIMITIVE_TOPOLOGY_MAX_ENUM = 0x7FFFFFFF
    };

    enum PolygonCullMode
    {
        POLYGON_CULL_DISABLED,
        POLYGON_CULL_FRONT,
        POLYGON_CULL_BACK,

        POLYGON_CULL_MAX
    };

    enum PolygonFrontFace
    {
        POLYGON_FRONT_FACE_CLOCKWISE,
        POLYGON_FRONT_FACE_COUNTER_CLOCKWISE,

        POLYGON_FRONT_FACE_MAX,
    };

    enum StencilOperation
    {
        STENCIL_OP_KEEP,
        STENCIL_OP_ZERO,
        STENCIL_OP_REPLACE,
        STENCIL_OP_INCREMENT_AND_CLAMP,
        STENCIL_OP_DECREMENT_AND_CLAMP,
        STENCIL_OP_INVERT,
        STENCIL_OP_INCREMENT_AND_WRAP,
        STENCIL_OP_DECREMENT_AND_WRAP,

        STENCIL_OP_MAX
    };

    enum LogicOperation
    {
        LOGIC_OP_CLEAR,
        LOGIC_OP_AND,
        LOGIC_OP_AND_REVERSE,
        LOGIC_OP_COPY,
        LOGIC_OP_AND_INVERTED,
        LOGIC_OP_NO_OP,
        LOGIC_OP_XOR,
        LOGIC_OP_OR,
        LOGIC_OP_NOR,
        LOGIC_OP_EQUIVALENT,
        LOGIC_OP_INVERT,
        LOGIC_OP_OR_REVERSE,
        LOGIC_OP_COPY_INVERTED,
        LOGIC_OP_OR_INVERTED,
        LOGIC_OP_NAND,
        LOGIC_OP_SET,

        LOGIC_OP_MAX
    };

    enum BlendFactor
    {
        BLEND_FACTOR_ZERO,
        BLEND_FACTOR_ONE,
        BLEND_FACTOR_SRC_COLOR,
        BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        BLEND_FACTOR_DST_COLOR,
        BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        BLEND_FACTOR_SRC_ALPHA,
        BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        BLEND_FACTOR_DST_ALPHA,
        BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        BLEND_FACTOR_CONSTANT_COLOR,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        BLEND_FACTOR_CONSTANT_ALPHA,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        BLEND_FACTOR_SRC_ALPHA_SATURATE,
        BLEND_FACTOR_SRC1_COLOR,
        BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        BLEND_FACTOR_SRC1_ALPHA,
        BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,

        BLEND_FACTOR_MAX
    };

    enum BlendOperation
    {
        BLEND_OP_ADD,
        BLEND_OP_SUBTRACT,
        BLEND_OP_REVERSE_SUBTRACT,
        BLEND_OP_MINIMUM,
        BLEND_OP_MAXIMUM,

        BLEND_OP_MAX
    };

    enum VertexBufferFormat
    {
        VERTEX_BUFFER_FORMAT_R32          = RENDERING_FORMAT_R32_SFLOAT,
        VERTEX_BUFFER_FORMAT_R32G32       = RENDERING_FORMAT_R32G32_SFLOAT,
        VERTEX_BUFFER_FORMAT_R32G32B32    = RENDERING_FORMAT_R32G32B32_SFLOAT,
        VERTEX_BUFFER_FORMAT_R32G32B32A32 = RENDERING_FORMAT_R32G32B32A32_SFLOAT,
    };

    enum IndexBufferFormat
    {
        INDEX_BUFFER_FORMAT_UINT16,
        INDEX_BUFFER_FORMAT_UINT32,
    };

    enum VertexFrequency
    {
        VERTEX_FREQUENCY_VERTEX,
        VERTEX_FREQUENCY_INSTANCE,
    };

    struct InputAttribute
    {
        uint32             location = 0;
        uint32             offset   = 0;
        VertexBufferFormat format   = VERTEX_BUFFER_FORMAT_R32G32B32;
    };

    struct InputLayout
    {
        uint32                      binding   = 0;
        uint32                      stride    = 0;
        VertexFrequency             frequency = VERTEX_FREQUENCY_VERTEX;
        std::vector<InputAttribute> attributes;

        void Binding(uint32 bindIndex, VertexFrequency rate = VERTEX_FREQUENCY_VERTEX)
        {
            binding   = bindIndex;
            frequency = rate;
        }

        void Attribute(uint32 location, VertexBufferFormat format)
        {
            attributes.push_back({ location, stride, format });

            switch (format)
            {
                case VERTEX_BUFFER_FORMAT_R32:          stride += 4;  break;
                case VERTEX_BUFFER_FORMAT_R32G32:       stride += 8;  break;
                case VERTEX_BUFFER_FORMAT_R32G32B32:    stride += 12; break;
                case VERTEX_BUFFER_FORMAT_R32G32B32A32: stride += 16; break;
            }
        }
    };

    struct PipelineInputLayout
    {
        uint32       numLayout;
        InputLayout* layouts = nullptr;
    };

    struct PipelineInputAssemblyState
    {
        PrimitiveTopology topology               = PRIMITIVE_TOPOLOGY_TRIANGLES;
        bool              primitiveRestartEnable = false;
    };

    struct PipelineRasterizationState
    {
        bool             enableDepthClamp        = false;
        bool             discardPrimitives       = false;
        bool             wireframe               = false;
        PolygonCullMode  cullMode                = POLYGON_CULL_BACK;
        PolygonFrontFace frontFace               = POLYGON_FRONT_FACE_COUNTER_CLOCKWISE;
        bool             depthBiasEnabled        = false;
        float            depthBiasConstantFactor = 0.0f;
        float            depthBiasClamp          = 0.0f;
        float            depthBiasSlopeFactor    = 0.0f;
        float            lineWidth               = 1.0f;
        uint32           patchControlPoints      = 1;
    };

    struct PipelineMultisampleState
    {
        TextureSamples      sampleCount           = TEXTURE_SAMPLES_1;
        bool                enableSampleShading   = false;
        float               minSampleShading      = 0.0f;
        std::vector<uint32> sampleMask            = {};
        bool                enableAlphaToCoverage = false;
        bool                enableAlphaToOne      = false;
    };

    struct PipelineDepthStencilState
    {
        // 深度
        bool            enableDepthTest  = true;
        bool            enableDepthWrite = true;
        bool            enableDepthRange = false;
        CompareOperator depthCompareOp   = COMPARE_OP_LESS;
        float           depthRangeMin    = 0.0f;
        float           depthRangeMax    = 1.0f;

        // ステンシル
        struct StencilOperationState
        {
            StencilOperation pass        = STENCIL_OP_ZERO;
            StencilOperation fail        = STENCIL_OP_KEEP;
            StencilOperation depthFail   = STENCIL_OP_KEEP;
            CompareOperator  compare     = COMPARE_OP_ALWAYS;
            uint32           compareMask = 255;
            uint32           writeMask   = 255;
            uint32           reference   = 0;
        };

        bool                  enableStencil = false;
        StencilOperationState frontOp       = {};
        StencilOperationState backOp        = {};
    };

    struct PipelineColorBlendState
    {
        uint32 numAttachment = 1;
        bool   enableBlend   = false;


        struct Attachment
        {
            bool           enableBlend         = false;
            BlendFactor    srcColorBlendFactor = BLEND_FACTOR_ZERO;
            BlendFactor    srcAlphaBlendFactor = BLEND_FACTOR_ZERO;
            BlendFactor    dstColorBlendFactor = BLEND_FACTOR_ZERO;
            BlendFactor    dstAlphaBlendFactor = BLEND_FACTOR_ZERO;
            BlendOperation colorBlendOp        = BLEND_OP_ADD;
            BlendOperation alphaBlendOp        = BLEND_OP_ADD;
            bool           write_r             = true;
            bool           write_g             = true;
            bool           write_b             = true;
            bool           write_a             = true;
        };

        void BlendAttachments(uint32 numAttachment)
        {
            attachments.resize(numAttachment);
        }

        void Disable(uint32 index)
        {
            attachments[index] = Attachment();
        }

        void Enable(uint32 index)
        {
            Attachment ba = {};
            ba.enableBlend         = true;
            ba.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
            ba.srcAlphaBlendFactor = BLEND_FACTOR_SRC_ALPHA;
            ba.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            ba.dstAlphaBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

            attachments[index] = ba;
        }

        bool                    enableLogicOp = false;
        LogicOperation          logicOp       = LOGIC_OP_CLEAR;
        std::vector<Attachment> attachments   = {};
        glm::vec4               blendConstant = {};
    };

    enum PipelineDynamicStateBits
    {
        // ビューポート・シザーは常に動的ステートにしたいのでここには含めない

        DYNAMIC_STATE_NONE                 = 0,
        DYNAMIC_STATE_LINE_WIDTH           = SL_BIT(0),
        DYNAMIC_STATE_DEPTH_BIAS           = SL_BIT(1),
        DYNAMIC_STATE_BLEND_CONSTANTS      = SL_BIT(2),
        DYNAMIC_STATE_DEPTH_BOUNDS         = SL_BIT(3),
        DYNAMIC_STATE_STENCIL_COMPARE_MASK = SL_BIT(4),
        DYNAMIC_STATE_STENCIL_WRITE_MASK   = SL_BIT(5),
        DYNAMIC_STATE_STENCIL_REFERENCE    = SL_BIT(6),

        DYNAMIC_STATE_MAX = 7,
    };
    using PipelineDynamicStateFlags = uint32;


    struct PipelineStateInfo
    {
        PipelineInputLayout        inputLayout   = {};
        PipelineInputAssemblyState inputAssembly = {};
        PipelineRasterizationState rasterize     = {};
        PipelineMultisampleState   multisample   = {};
        PipelineDepthStencilState  depthStencil  = {};
        PipelineColorBlendState    blend         = {};
        PipelineDynamicStateBits   dynamic       = {};
    };

    class PipelineStateInfoBuilder
    {
    public:

        PipelineStateInfo Value()
        {
            return info;
        }

        PipelineStateInfoBuilder& InputAssembly(PrimitiveTopology topology)
        {
            info.inputAssembly.topology = topology;
            return *this;
        }

        PipelineStateInfoBuilder& InputLayout(uint32 numLayout, InputLayout* input)
        {
            info.inputLayout.layouts   = input;
            info.inputLayout.numLayout = numLayout;
            return *this;
        }

        PipelineStateInfoBuilder& Rasterizer(PolygonCullMode mode, PolygonFrontFace face, bool wireframe = false, float lineWidth = 1.0)
        {
            info.rasterize.cullMode  = mode;
            info.rasterize.frontFace = face;
            info.rasterize.wireframe = wireframe;
            info.rasterize.lineWidth = lineWidth;
            return *this;
        }

        PipelineStateInfoBuilder& RasterizerDepthBias(bool depthBiasEnable, float bias, float slope = 0.0f, bool depthBiasClamp = false, float biasClamp = 0.0f)
        {
            info.rasterize.depthBiasEnabled        = depthBiasEnable;
            info.rasterize.depthBiasConstantFactor = bias;
            info.rasterize.depthBiasSlopeFactor    = slope;
            info.rasterize.depthBiasClamp          = biasClamp;
            info.rasterize.enableDepthClamp        = depthBiasClamp;

            return *this;
        }

        PipelineStateInfoBuilder& Multisample()
        {
            return *this;
        }

        PipelineStateInfoBuilder& Depth(bool test, bool write, CompareOperator comp = COMPARE_OP_LESS)
        {
            info.depthStencil.enableDepthTest  = test;
            info.depthStencil.enableDepthWrite = write;
            info.depthStencil.depthCompareOp   = comp;
            return *this;
        }

        PipelineStateInfoBuilder& Stencil(bool enable, uint32 value = 0, CompareOperator comp = COMPARE_OP_ALWAYS, StencilOperation pass = STENCIL_OP_REPLACE, StencilOperation fail = STENCIL_OP_KEEP)
        {
            info.depthStencil.enableStencil     = enable;
            info.depthStencil.frontOp.reference = value;
            info.depthStencil.frontOp.compare   = comp;
            info.depthStencil.frontOp.pass      = pass;
            info.depthStencil.frontOp.fail      = fail;
            info.depthStencil.backOp.compare    = comp;
            info.depthStencil.backOp.pass       = pass;
            info.depthStencil.backOp.fail       = fail;
            return *this;
        }

        PipelineStateInfoBuilder& Blend(bool enable, uint32 numColorAttachment)
        {
            info.blend.attachments.resize(numColorAttachment);

            for (uint32 i = 0; i < numColorAttachment; i++)
            {
                if (enable)
                {
                    PipelineColorBlendState::Attachment ba = {};
                    ba.enableBlend         = true;
                    ba.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
                    ba.srcAlphaBlendFactor = BLEND_FACTOR_SRC_ALPHA;
                    ba.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    ba.dstAlphaBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

                    info.blend.attachments[i] = ba;
                }
                else
                {
                    info.blend.attachments[i] = {};
                }
            }

            return *this;
        }

    private:

        PipelineStateInfo info;
    };


    //================================================
    // コマンド
    //================================================
    union RenderPassClearValue
    {
        void SetFloat(float r, float g, float b, float a)
        {
            color._float[0] = r;
            color._float[1] = g;
            color._float[2] = b;
            color._float[3] = a;
        }

        void SetInt(int32 r, int32 g, int32 b, int32 a)
        {
            color._int[0] = r;
            color._int[1] = g;
            color._int[2] = b;
            color._int[3] = a;
        }

        void SetUint(uint32 r, uint32 g, uint32 b, uint32 a)
        {
            color._uint[0] = r;
            color._uint[1] = g;
            color._uint[2] = b;
            color._uint[3] = a;
        }

        void SetDepthStencil(float depthVal, uint32 stencilVal)
        {
            depth   = depthVal;
            stencil = stencilVal;
        }

        union
        {
            float  _float[4];
            int32  _int[4];
            uint32 _uint[4];
        } color;

        struct
        {
            float  depth;
            uint32 stencil;
        };
    };

    struct AttachmentClear
    {
        TextureAspectBits    aspect;
        uint32               colorAttachment = 0xffffffff;
        RenderPassClearValue value;
    };

    struct BufferCopyRegion
    {
        uint64 srcOffset = 0;
        uint64 dstOffset = 0;
        uint64 size      = 0;
    };

    struct TextureCopyRegion
    {
        TextureSubresource srcSubresources;
        Extent             srcOffset;
        TextureSubresource dstSubresources;
        Extent             dstOffset;
        Extent             size;
    };

    struct BufferTextureCopyRegion
    {
        uint64             bufferOffset;
        TextureSubresource textureSubresources;
        Extent             textureOffset;
        Extent             textureRegionSize;
    };

    struct TextureBlitRegion
    {
        TextureSubresource srcSubresources;
        Extent             srcOffset[2];
        TextureSubresource dstSubresources;
        Extent             dstOffset[2];
    };
}
