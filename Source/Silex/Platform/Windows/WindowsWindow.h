
#pragma once
#include "Core/Window.h"
#include "Rendering/RenderingCore.h"


namespace Silex
{
    struct WindowsWindowHandle
    {
        HWND      windowHandle   = nullptr;
        HINSTANCE instanceHandle = nullptr;
    };


    class WindowsWindow : public Window
    {
        SL_CLASS(WindowsWindow, Window)

    public:

        WindowsWindow(const char* title, uint32 width, uint32 height);
        ~WindowsWindow();

        bool Initialize()    override;
        void ProcessMessage()override;

        // ウィンドウステート
        void Maximize() override;
        void Minimize() override;
        void Restore()  override;
        void Show()     override;
        void Hide()     override;

        // Get / Set
        void             SetTitle(const std::string& title) override;
        const char*      GetTitle()                         const override;
        glm::ivec2       GetSize()                          const override;
        glm::ivec2       GetWindowPos()                     const override;
        void*            GetPlatformHandle()                const override;
        GLFWwindow*      GetGLFWWindow()                    const override;
        WindowData*      GetWindowData()                    const override;
        SurfaceHandle*   GetSurface()                       const override;
        SwapChainHandle* GetSwapChain()                     const override;

        // レンダーコンテキスト
        bool SetupWindowContext(RenderingContext* context)   override;
        void CleanupWindowContext(RenderingContext* context) override;

        // ウィンドウイベント
        void OnWindowClose(WindowCloseEvent& e)                 override;
        void OnWindowResize(WindowResizeEvent& e)               override;
        void OnKeyPressed(KeyPressedEvent& e)                   override;
        void OnKeyReleased(KeyReleasedEvent& e)                 override;
        void OnMouseButtonPressed(MouseButtonPressedEvent& e)   override;
        void OnMouseButtonReleased(MouseButtonReleasedEvent& e) override;
        void OnMouseScroll(MouseScrollEvent& e)                 override;
        void OnMouseMove(MouseMoveEvent& e)                     override;
        void OnWindowMove(WindowMoveEvent& e)                   override;

    private:

        SurfaceHandle*   renderingSurface = nullptr;
        SwapChainHandle* swapchain        = nullptr;

        // ウィンドウデータ
        GLFWwindow* window = nullptr;

        // Windows 固有ハンドル
        WindowsWindowHandle handle;
    };
}

