
#pragma once

#include "Core/Core.h"
#include "ImGui/GUI.h"


namespace Silex
{
    class VulkanGUI : public GUI
    {
        SL_CLASS(VulkanGUI, GUI)

    public:

        VulkanGUI();
        ~VulkanGUI();

        void Init(RenderingContext* context) override;
        void BeginFrame()                    override;
        void Render()                        override;
        void EndFrame()                      override;

    private:

        class VulkanContext* vulkanContext  = nullptr;
        VkDescriptorPool     descriptorPool = nullptr;
    };
}
