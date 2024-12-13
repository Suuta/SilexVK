
#include "PCH.h"
#include "Rendering/Vulkan/VulkanContext.h"
#include "Rendering/Vulkan/VulkanAPI.h"
#include "ImGui/Vulkan/VulkanGUI.h"


namespace Silex
{
    const char* VkResultToString(VkResult result)
    {
        switch (result)
        {
            default: return "";

            case VK_SUCCESS:                        return "VK_SUCCESS";
            case VK_NOT_READY:                      return "VK_NOT_READY";
            case VK_TIMEOUT:                        return "VK_TIMEOUT";
            case VK_EVENT_SET:                      return "VK_EVENT_SET";
            case VK_EVENT_RESET:                    return "VK_EVENT_RESET";
            case VK_INCOMPLETE:                     return "VK_INCOMPLETE";
            case VK_ERROR_OUT_OF_HOST_MEMORY:       return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:     return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED:    return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST:              return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED:        return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT:        return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT:    return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT:      return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER:      return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS:         return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED:     return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_SURFACE_LOST_KHR:         return "VK_ERROR_SURFACE_LOST_KHR";
            case VK_SUBOPTIMAL_KHR:                 return "VK_SUBOPTIMAL_KHR";
            case VK_ERROR_OUT_OF_DATE_KHR:          return "VK_ERROR_OUT_OF_DATE_KHR";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VK_ERROR_VALIDATION_FAILED_EXT:    return "VK_ERROR_VALIDATION_FAILED_EXT";

            case VK_RESULT_MAX_ENUM:                return "VK_RESULT_MAX_ENUM";
        }
    }


    static VkBool32 DebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData)
    {
        const char* message = pCallbackData->pMessage;

        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: SL_LOG_TRACE("{}", message); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    SL_LOG_INFO("{}",  message); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: SL_LOG_WARN("{}",  message); break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   SL_LOG_ERROR("{}", message); break;

            default: break;
        }

        return VK_FALSE;
    }


    VulkanContext::VulkanContext()
    {
    }

    VulkanContext::~VulkanContext()
    {
        if (enableValidationLayer)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }

    bool VulkanContext::Initialize(bool enableValidation)
    {
        enableValidationLayer = enableValidation;
        uint32 instanceVersion = VK_API_VERSION_1_0;

        // ドライバーの Vulkan バージョンを取得(ver 1.1 から有効であり、関数のロードが失敗した場合 ver 1.0)
        auto vkEnumerateInstanceVersion_PFN = GET_VULKAN_INSTANCE_PROC(nullptr, vkEnumerateInstanceVersion);
        if (vkEnumerateInstanceVersion_PFN)
        {
            if (vkEnumerateInstanceVersion_PFN(&instanceVersion) == VK_SUCCESS)
            {
                uint32 major = VK_VERSION_MAJOR(instanceVersion);
                uint32 minor = VK_VERSION_MINOR(instanceVersion);
                uint32 patch = VK_VERSION_PATCH(instanceVersion);

                SL_LOG_INFO("Vulkan Instance Version: {}.{}.{}", major, minor, patch);
            }
            else
            {
                SL_LOG_ERROR("原因不明のエラーです");
            }
        }

        // 要求インスタンス拡張機能
        requestInstanceExtensions.insert(VK_KHR_SURFACE_EXTENSION_NAME);
        requestInstanceExtensions.insert(GetPlatformSurfaceExtensionName());

        // バリデーションが有効な場合
        if (enableValidationLayer)
        {
            requestInstanceExtensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // インスタンス拡張機能のクエリ
        uint32 instanceExtensionCount = 0;
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);

        std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
        result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());

        // 要求機能が有効かどうかを確認
        for (const VkExtensionProperties& extension : instanceExtensions)
        {
            const auto& itr = requestInstanceExtensions.find(extension.extensionName);
            if (itr != requestInstanceExtensions.end())
            {
                enableInstanceExtensions.push_back(itr->c_str());
            }
        }
        
        // バリデーションが有効な場合
        if (enableValidationLayer)
        {
            requestLayers.insert(VK_LAYER_KHRONOS_VALIDATION_NAME);
        }

        // インスタンスレイヤーのクエリ
        uint32 instanceLayerCount = 0;
        result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);

        std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
        result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

        // 要求レイヤーが有効かどうかを確認
        for (const VkLayerProperties& property : instanceLayers)
        {
            const auto& itr = requestLayers.find(property.layerName);
            if (itr != requestLayers.end())
            {
                enableLayers.push_back(itr->c_str());
            }
        }

        // デバック出力拡張機能の情報（バリデーションレイヤーのメッセージ出力もこの機能が行う）
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
        debugMessengerInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerInfo.pNext           = nullptr;
        debugMessengerInfo.flags           = 0;
        debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerInfo.pfnUserCallback = DebugMessengerCallback;
        debugMessengerInfo.pUserData       = this;


        // 1.0 を除き、GPU側がサポートしている限りは、指定バージョンに関係なくGPUバージョンレベルの機能が利用可能
        // つまり、物理デバイスのバージョンではなく、開発の最低保証バージョンを指定した方が良い？
        uint32 apiVersion = instanceVersion == VK_API_VERSION_1_0? VK_API_VERSION_1_0 : VK_API_VERSION_1_2;

        // アプリ情報
        VkApplicationInfo appInfo = {};
        appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Silex";
        appInfo.pEngineName      = "Silex";
        appInfo.engineVersion    = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion       = apiVersion;

        // インスタンス情報
        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo        = &appInfo;
        instanceInfo.enabledExtensionCount   = enableInstanceExtensions.size();
        instanceInfo.ppEnabledExtensionNames = enableInstanceExtensions.data();
        instanceInfo.enabledLayerCount       = enableLayers.size();
        instanceInfo.ppEnabledLayerNames     = enableLayers.data();
        instanceInfo.pNext                   = &debugMessengerInfo;

        result = vkCreateInstance(&instanceInfo, nullptr, &instance);
        SL_CHECK_VKRESULT(result, false);

        if (enableValidationLayer)
        {
            // デバッグメッセンジャーの関数ポインタを取得
            CreateDebugUtilsMessengerEXT  = GET_VULKAN_INSTANCE_PROC(instance, vkCreateDebugUtilsMessengerEXT);
            DestroyDebugUtilsMessengerEXT = GET_VULKAN_INSTANCE_PROC(instance, vkDestroyDebugUtilsMessengerEXT);
            SL_CHECK(!CreateDebugUtilsMessengerEXT, false);
            SL_CHECK(!DestroyDebugUtilsMessengerEXT, false);

            // デバッグメッセンジャー生成
            result = CreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger);
            SL_CHECK_VKRESULT(result, false);
        }

        // 物理デバイスの列挙
        uint32 physicalDeviceCount = 0;
        result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        // 物理デバイスの選択
        for (const auto& pd : physicalDevices)
        {
            VkPhysicalDeviceProperties property;
            vkGetPhysicalDeviceProperties(pd, &property);
            
            VkPhysicalDeviceFeatures feature;
            vkGetPhysicalDeviceFeatures(pd, &feature);

            // 外部GPUでジオメトリシェーダをサポートしているデバイスのみを選択
            if (property.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && feature.geometryShader)
            {
                deviceInfo.name   = property.deviceName;
                deviceInfo.vendor = (DeviceVendor)property.vendorID;
                deviceInfo.type   = (DeviceType)property.deviceType;

                physicalDevice = pd;
                break;
            }
        }

        if (physicalDevice == nullptr)
        {
            SL_ASSERT("適切なGPUが見つかりませんでした");
            return false;
        }

        // キューファミリーの取得
        uint32 queueFamilyPropertiesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, nullptr);

        queueFamilyProperties.resize(queueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());

        // 要求デバイス拡張
        requestDeviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        //requestDeviceExtensions.insert(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);   // レンダーパス
        //requestDeviceExtensions.insert(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME); // GPUアドレス取得
        //requestDeviceExtensions.insert(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);   // デスクリプター配列にインデックス参照
        //requestDeviceExtensions.insert(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);     // 動的レンダリング（レンダーパス不要）
        //requestDeviceExtensions.insert(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);       // デスクリプターの更新タイミング
        //requestDeviceExtensions.insert(VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME); // レンダーパス開始時にイメージの指定を延期

        // デバイス拡張機能のクエリ
        uint32 deviceExtensionCount = 0;
        result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);

        std::vector<VkExtensionProperties> deviceExtension(deviceExtensionCount);
        result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtension.data());

        for (const VkExtensionProperties& property : deviceExtension)
        {
            const auto& itr = requestDeviceExtensions.find(property.extensionName);
            if (itr != requestDeviceExtensions.end())
            {
                enableDeviceExtensions.push_back(itr->c_str());
            }
        }

        // 拡張機能関数のロード
        extensionFunctions.GetPhysicalDeviceSurfaceSupportKHR = GET_VULKAN_INSTANCE_PROC(instance, vkGetPhysicalDeviceSurfaceSupportKHR);
        SL_CHECK(!extensionFunctions.GetPhysicalDeviceSurfaceSupportKHR, false);

        extensionFunctions.GetPhysicalDeviceSurfaceCapabilitiesKHR = GET_VULKAN_INSTANCE_PROC(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        SL_CHECK(!extensionFunctions.GetPhysicalDeviceSurfaceCapabilitiesKHR, false);

        extensionFunctions.GetPhysicalDeviceSurfaceFormatsKHR = GET_VULKAN_INSTANCE_PROC(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
        SL_CHECK(!extensionFunctions.GetPhysicalDeviceSurfaceFormatsKHR, false);

        extensionFunctions.GetPhysicalDeviceSurfacePresentModesKHR = GET_VULKAN_INSTANCE_PROC(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
        SL_CHECK(!extensionFunctions.GetPhysicalDeviceSurfacePresentModesKHR, false);


        return true;
    }

    RenderingAPI* VulkanContext::CreateRendringAPI()
    {
        return slnew(VulkanAPI, this);
    }

    void VulkanContext::DestroyRendringAPI(RenderingAPI* api)
    {
        sldelete(api);
    }

    const DeviceInfo& VulkanContext::GetDeviceInfo() const
    {
        return deviceInfo;
    }

    bool VulkanContext::QueueHasPresent(SurfaceHandle* surface, uint32 queueIndex) const
    {
        VkSurfaceKHR vkSurface = VulkanCast(surface)->surface;
        VkBool32 supported = false;

        VkResult result = extensionFunctions.GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueIndex, vkSurface, &supported);
        SL_CHECK_VKRESULT(result, false);

        return supported;
    }

    //bool D3D12Context::QueueHasPresent(Surface* surface, uint32 queueIndex) const
    //{
    //    return true;
    //}

    const std::vector<VkQueueFamilyProperties>& VulkanContext::GetQueueFamilyProperties() const
    {
        return queueFamilyProperties;
    }

    const std::vector<const char*>& VulkanContext::GetEnabledInstanceExtensions() const
    {
        return enableInstanceExtensions;
    }

    const std::vector<const char*>& VulkanContext::GetEnabledDeviceExtensions() const
    {
        return enableDeviceExtensions;
    }

    VkPhysicalDevice VulkanContext::GetPhysicalDevice() const
    {
        return physicalDevice;
    }

    VkDevice VulkanContext::GetDevice() const
    {
        SL_ASSERT(device != nullptr, "device は VulkanAPI で初期化された後から使用可能です");
        return device;
    }

    VkInstance VulkanContext::GetInstance() const
    {
        return instance;
    }

    const VulkanContext::ExtensionFunctions& VulkanContext::GetExtensionFunctions() const
    {
        return extensionFunctions;
    }
}
