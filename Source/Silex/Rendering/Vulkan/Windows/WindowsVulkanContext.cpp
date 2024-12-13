
#include "PCH.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Rendering/Vulkan/Windows/WindowsVulkanContext.h"
#include "Rendering/Vulkan/VulkanStructures.h"

namespace Silex
{
    WindowsVulkanContext::WindowsVulkanContext(void* platformHandle)
    {
        WindowsWindowHandle* handle = (WindowsWindowHandle*)platformHandle;
        instanceHandle = handle->instanceHandle;
        windowHandle   = handle->windowHandle;
    }

    WindowsVulkanContext::~WindowsVulkanContext()
    {
    }

    bool WindowsVulkanContext::Initialize(bool enableValidation)
    {
        bool result = false;

        result = Super::Initialize(enableValidation);
        SL_CHECK(!result, false);

        CreateWin32SurfaceKHR = GET_VULKAN_INSTANCE_PROC(Super::instance, vkCreateWin32SurfaceKHR);
        SL_CHECK(!CreateWin32SurfaceKHR, false);

        DestroySurfaceKHR = GET_VULKAN_INSTANCE_PROC(Super::instance, vkDestroySurfaceKHR);
        SL_CHECK(!DestroySurfaceKHR, false);

        return result;
    }

    const char* WindowsVulkanContext::GetPlatformSurfaceExtensionName()
    {
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }

    SurfaceHandle* WindowsVulkanContext::CreateSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = instanceHandle;
        createInfo.hwnd      = windowHandle;

        VkSurfaceKHR vkSurface = nullptr;
        VkResult result = CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &vkSurface);
        SL_CHECK_VKRESULT(result, nullptr);

        VulkanSurface* surface = slnew(VulkanSurface);
        surface->surface = vkSurface;

        return surface;
    }

    void WindowsVulkanContext::DestroySurface(SurfaceHandle* surface)
    {
        if (surface)
        {
            VulkanSurface* vkSurface = (VulkanSurface*)surface;
            DestroySurfaceKHR(instance, vkSurface->surface, nullptr);

            sldelete(vkSurface);
        }
    }
}
