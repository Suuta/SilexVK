
#pragma once

#include "Rendering/Vulkan/VulkanContext.h"
#include <vulkan/vulkan_win32.h>


namespace Silex
{
    class WindowsVulkanContext final : public VulkanContext
    {
        SL_CLASS(WindowsVulkanContext, VulkanContext)

    public:

        WindowsVulkanContext(void* platformHandle);
        ~WindowsVulkanContext();

        // 初期化
        bool Initialize(bool enableValidation) override final;

        // サーフェース
        const char* GetPlatformSurfaceExtensionName() override final;
        SurfaceHandle*    CreateSurface()                   override final;
        void        DestroySurface(SurfaceHandle* surface)  override final;

    private:

        PFN_vkCreateWin32SurfaceKHR  CreateWin32SurfaceKHR = nullptr;
        PFN_vkDestroySurfaceKHR      DestroySurfaceKHR     = nullptr;

    private:

        HINSTANCE instanceHandle;
        HWND      windowHandle;
    };
}
