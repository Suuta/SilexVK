
#include "PCH.h"

#include "Core/Engine.h"
#include "Rendering/RenderingStructures.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/Vulkan/VulkanStructures.h"
#include "Rendering/Vulkan/VulkanContext.h"
#include "ImGui/Vulkan/VulkanGUI.h"

#include <imgui/imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_dx12.h>


namespace Silex
{
    static VkDescriptorPool CreatePool(VkDevice device)
    {
        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000},
        };

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // vkFreeDescriptorSets が使用可能になる
        poolInfo.maxSets       = 1000;
        poolInfo.poolSizeCount = std::size(poolSizes);
        poolInfo.pPoolSizes    = poolSizes;

        VkDescriptorPool pool = nullptr;
        VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
        SL_CHECK_VKRESULT(result, nullptr);

        return pool;
    }


    // Vulkan validation error with SDK 1.3.275
    // https://github.com/ocornut/imgui/issues/7236

    // ImGui リポジトリを更新 => ver 1.91.0

    //==================================================================================
    // ImGui側の変更により 初回の NewFrame関数でフォントのデスクリプターリソース確認を行うようになった
    // また、コマンドバッファ等の転送リソース等も外部で良いする必要もなくなった
    //==================================================================================
#if 0
    static void UploadFontTexture(VkDevice device, VkQueue queue, uint32 queueFamily)
    {
        VkResult result;

        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = queueFamily;

        VkCommandPool commandPool = nullptr;
        result = vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool        = commandPool;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = nullptr;
        result = vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        {
            result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
            SL_CHECK_VKRESULT(result, SL_DONT_USE);

            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

            result = vkEndCommandBuffer(commandBuffer);
            SL_CHECK_VKRESULT(result, SL_DONT_USE);
        }

        VkSubmitInfo submit = {};
        submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers    = &commandBuffer;

        result = vkQueueSubmit(queue, 1, &submit, nullptr);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        result = vkQueueWaitIdle(queue);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        vkDestroyCommandPool(device, commandPool, nullptr);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
#endif


    VulkanGUI::VulkanGUI()
    {
    }

    VulkanGUI::~VulkanGUI()
    {
        vkDeviceWaitIdle(vulkanContext->GetDevice());

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        vkDestroyDescriptorPool(vulkanContext->GetDevice(), descriptorPool, nullptr);
    }

    void VulkanGUI::Init(RenderingContext* context)
    {
        vulkanContext = (VulkanContext*)context;

        GLFWwindow*         glfw      = Window::Get()->GetGLFWWindow();
        VulkanSwapChain*    swapchain = (VulkanSwapChain*)Window::Get()->GetSwapChain();
        VulkanCommandQueue* queue     = (VulkanCommandQueue*)Renderer::Get()->GetGraphicsCommandQueue();

        Super::Init(vulkanContext);

        // デスクリプタープール生成
        descriptorPool = CreatePool(vulkanContext->GetDevice());

        // VulknaImGui 初期化
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance              = vulkanContext->GetInstance();
        initInfo.PhysicalDevice        = vulkanContext->GetPhysicalDevice();
        initInfo.Device                = vulkanContext->GetDevice();
        initInfo.Queue                 = queue->queue;
        initInfo.QueueFamily           = queue->family;
        initInfo.DescriptorPool        = descriptorPool;
        initInfo.Subpass               = 0;
        initInfo.MinImageCount         = 2;
        initInfo.ImageCount            = 3;
        initInfo.MSAASamples           = VK_SAMPLE_COUNT_1_BIT;
        initInfo.RenderPass            = swapchain->renderpass->renderpass;
        initInfo.UseDynamicRendering   = false;
        initInfo.PipelineCache         = nullptr;
        initInfo.Allocator             = nullptr;
        initInfo.CheckVkResultFn       = nullptr;

        ImGui_ImplGlfw_InitForVulkan(glfw, true);
        ImGui_ImplVulkan_Init(&initInfo);

        ImGui_ImplVulkan_CreateFontsTexture();
    }

    void VulkanGUI::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void VulkanGUI::Render()
    {
        const FrameData& frame = Renderer::Get()->GetFrameData();

        Renderer::Get()->BeginSwapChainPass();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VulkanCast(frame.commandBuffer)->commandBuffer);
        Renderer::Get()->EndSwapChainPass();
    }

    void VulkanGUI::EndFrame()
    {
    }


#if SL_RENDERER_VULKAN

    //==========================================================================================
    // OpenGL　実装だったので、TextureID を渡すことを期待している
    // Vulkan 実装では、デスクリプターセットを渡す必要がある
    // また、ImGuiは 以下のように "イメージサンプラー x 1" レイアウトの デスクリプターセットを要求する
    //==========================================================================================
    // imgui_impl_vulkan.cpp [1026]
    //==========================================================================================
    // VkDescriptorSetLayoutBinding binding[1] = {};
    // binding[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // binding[0].descriptorCount = 1;
    // binding[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    // 
    // VkDescriptorSetLayoutCreateInfo info = {};
    // info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // info.bindingCount = 1;
    // info.pBindings    = binding;
    // 
    // vkCreateDescriptorSetLayout(v->Device, &info, v->Allocator, &bd->DescriptorSetLayout);
    //==========================================================================================

    void GUI::Image(DescriptorSet* set, float width, float height)
    {
        DescriptorSetHandle* h = set->GetHandle();

        VulkanDescriptorSet* descriptorset = VulkanCast(h);
        ImGui::Image(descriptorset->descriptorSet, { width, height });
    }

    void GUI::ImageButton(DescriptorSetHandle* set, float width, float height, uint32 framePadding)
    {
        VulkanDescriptorSet* descriptorset = (VulkanDescriptorSet*)set;
        ImGui::ImageButton(descriptorset->descriptorSet, { width, height }, {0, 0}, {1, 1}, framePadding);
    }

#endif

}
