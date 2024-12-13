
#include "PCH.h"

#include "Core/Timer.h"
#include "Core/OS.h"
#include "Asset/TextureReader.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Rendering/RenderingCore.h"
#include "Rendering/Renderer.h"
#include "Rendering/Vulkan/Windows/WindowsVulkanContext.h"

#ifndef GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <glfw/glfw3.h>
#include <GLFW/glfw3native.h>



namespace Silex
{
    namespace Callback
    {
        static void OnWindowSize(GLFWwindow* window, int32 width, int32 height);
        static void OnWindowClose(GLFWwindow* window);
        static void OnKey(GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 mods);
        static void OnMouseButton(GLFWwindow* window, int32 button, int32 action, int32 mods);
        static void OnScroll(GLFWwindow* window, double xOffset, double yOffset);
        static void OnCursorPos(GLFWwindow* window, double x, double y);
        static void OnWindowMoved(GLFWwindow* window, int32 x, int32 y);
    }

    WindowsWindow::WindowsWindow(const char* title, uint32 width, uint32 height)
    {
        data = slnew(WindowData);
        data->width  = width;
        data->height = height;
        data->title  = title;
    }

    WindowsWindow::~WindowsWindow()
    {
        sldelete(data);
        glfwDestroyWindow(window);
    }

    bool WindowsWindow::Initialize()
    {
        SL_LOG_TRACE("WindowsWindow::Initialize");

        glfwWindowHint(GLFW_VISIBLE, FALSE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan / D3D は GLFW_NO_API を指定する

        //====================================================================
        // TODO: 抽象化しておきながら <glfw> を使用しているので WindowsAPI 置き換える
        //====================================================================
        window = glfwCreateWindow((int32)data->width, (int32)data->height, data->title.c_str(), nullptr, nullptr);
        SL_CHECK(window == nullptr, false);

        handle.windowHandle   = glfwGetWin32Window(window);
        handle.instanceHandle = GetModuleHandleW(nullptr);

        // リサイズはレンダラー生成後はレンダラーに依存するので、初期化前にリサイズしておく
        glfwMaximizeWindow(window);

        // ウィンドウに紐づけるデータを設定 (※ glfwGetWindowUserPointerで取り出し)
        glfwSetWindowUserPointer(window, this);

        // コールバック登録         
        glfwSetWindowSizeCallback(window,  Callback::OnWindowSize);  // リサイズ
        glfwSetWindowCloseCallback(window, Callback::OnWindowClose); // ウィンドウクローズ
        glfwSetKeyCallback(window,         Callback::OnKey);         // キー入力
        glfwSetMouseButtonCallback(window, Callback::OnMouseButton); // マウスボタン入力
        glfwSetScrollCallback(window,      Callback::OnScroll);      // マウススクロール
        glfwSetCursorPosCallback(window,   Callback::OnCursorPos);   // マウス位置
        glfwSetWindowPosCallback(window,   Callback::OnWindowMoved); // ウィンドウ位置


        // ウィンドウサイズ同期（既に最大化で変化しているので、同期が必要）
        glfwGetWindowSize(window, (int*)&data->width, (int*)&data->height);

        // アイコン設定
        GLFWimage icon;
        TextureReader reader;
        icon.pixels = reader.Read("Assets/Editor/Logo.png");
        icon.width  = reader.data.width;
        icon.height = reader.data.height;
        glfwSetWindowIcon(window, 1, &icon);

        return true;
    }

    bool WindowsWindow::SetupWindowContext(RenderingContext* context)
    {
        // サーフェース生成
        renderingSurface = context->CreateSurface();
        SL_CHECK(!renderingSurface, false);

        // スワップチェイン生成
        swapchain = Renderer::Get()->CreateSwapChain(renderingSurface, data->width, data->height, data->vsync);
        SL_CHECK(!swapchain, false);

        return true;
    }

    void WindowsWindow::CleanupWindowContext(RenderingContext* context)
    {
        // スワップチェイン破棄
        Renderer::Get()->DestoySwapChain(swapchain);

        // サーフェース破棄
        context->DestroySurface(renderingSurface);
    }

    glm::ivec2 WindowsWindow::GetSize() const
    {
        return { data->width, data->height };
    }

    glm::ivec2 WindowsWindow::GetWindowPos() const
    {
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        return { x, y };
    }

    void WindowsWindow::ProcessMessage()
    {
        glfwPollEvents();
    }

    void WindowsWindow::Maximize()
    {
        glfwMaximizeWindow(window);
    }

    void WindowsWindow::Minimize()
    {
        glfwIconifyWindow(window);
    }

    void WindowsWindow::Restore()
    {
        glfwRestoreWindow(window);
    }

    void WindowsWindow::Show()
    {
        glfwShowWindow(window);
    }

    void WindowsWindow::Hide()
    {
        glfwHideWindow(window);
    }

    const char* WindowsWindow::GetTitle() const
    {
        return data->title.c_str();
    }

    void WindowsWindow::SetTitle(const std::string& title)
    {
        data->title = title;
        glfwSetWindowTitle(window, data->title.c_str());
    }

    GLFWwindow* WindowsWindow::GetGLFWWindow() const
    {
        return window;
    }

    void* WindowsWindow::GetPlatformHandle() const
    {
        return (void*)&handle;
    }

    WindowData* WindowsWindow::GetWindowData() const
    {
        return data;
    }

    SurfaceHandle* WindowsWindow::GetSurface() const
    {
        return renderingSurface;
    }

    SwapChainHandle* WindowsWindow::GetSwapChain() const
    {
        return swapchain;
    }



    void WindowsWindow::OnWindowClose(WindowCloseEvent& e)
    {
    }

    void WindowsWindow::OnWindowResize(WindowResizeEvent& e)
    {
        Renderer::Get()->ResizeSwapChain(swapchain, data->width, data->height, data->vsync);
    }

    void WindowsWindow::OnKeyPressed(KeyPressedEvent& e)
    {
    }

    void WindowsWindow::OnKeyReleased(KeyReleasedEvent& e)
    {
    }

    void WindowsWindow::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
    }

    void WindowsWindow::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
    {
    }

    void WindowsWindow::OnMouseScroll(MouseScrollEvent& e)
    {
    }

    void WindowsWindow::OnMouseMove(MouseMoveEvent& e)
    {
    }

    void WindowsWindow::OnWindowMove(WindowMoveEvent& e)
    {
    }





    namespace Callback
    {
        static void OnWindowClose(GLFWwindow* window)
        {
            WindowCloseEvent event;

            WindowsWindow* data = ((WindowsWindow*)glfwGetWindowUserPointer(window));
            data->OnWindowClose(event);
            data->GetWindowData()->WindowCloseEvent.Execute(event);
        }

        static void OnWindowSize(GLFWwindow* window, int width, int height)
        {
            WindowResizeEvent event((uint32)width, (uint32)height);

            WindowsWindow* data = ((WindowsWindow*)glfwGetWindowUserPointer(window));
            data->GetWindowData()->width  = width;
            data->GetWindowData()->height = height;

            data->OnWindowResize(event);
            data->GetWindowData()->WindowResizeEvent.Execute(event);
        }

        static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowsWindow* data = ((WindowsWindow*)glfwGetWindowUserPointer(window));

            switch (action)
            {
                case GLFW_PRESS:
                {
                    Input::ProcessKey((Keys)key, true);

                    KeyPressedEvent event((Keys)key);
                    data->OnKeyPressed(event);
                    data->GetWindowData()->KeyPressedEvent.Execute(event);

                    break;
                }
                case GLFW_RELEASE:
                {
                    Input::ProcessKey((Keys)key, false);

                    KeyReleasedEvent event((Keys)key);
                    data->OnKeyReleased(event);
                    data->GetWindowData()->KeyReleasedEvent.Execute(event);
                    break;
                }

                default: break;
            }
        }

        static void OnMouseButton(GLFWwindow* window, int button, int action, int mods)
        {
            WindowsWindow* data = ((WindowsWindow*)glfwGetWindowUserPointer(window));

            switch (action)
            {
                case GLFW_PRESS:
                {
                    Input::ProcessButton((Mouse)button, true);

                    MouseButtonPressedEvent event(button);
                    data->OnMouseButtonPressed(event);
                    data->GetWindowData()->MouseButtonPressedEvent.Execute(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    Input::ProcessButton((Mouse)button, false);

                    MouseButtonReleasedEvent event(button);
                    data->OnMouseButtonReleased(event);
                    data->GetWindowData()->MouseButtonReleasedEvent.Execute(event);
                    break;
                }

                default: break;
            }
        }

        static void OnScroll(GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowsWindow* data = ((WindowsWindow*)glfwGetWindowUserPointer(window));

            MouseScrollEvent event((float)xOffset, (float)yOffset);
            data->OnMouseScroll(event);
            data->GetWindowData()->MouseScrollEvent.Execute(event);
        }

        static void OnCursorPos(GLFWwindow* window, double x, double y)
        {
            WindowsWindow* data = ((WindowsWindow*)glfwGetWindowUserPointer(window));

            Input::ProcessMove((int16)x, (int16)y);

            MouseMoveEvent event((float)x, (float)y);
            data->OnMouseMove(event);
            data->GetWindowData()->MouseMoveEvent.Execute(event);
        }

        void OnWindowMoved(GLFWwindow* window, int32 x, int32 y)
        {
            WindowsWindow* data = ((WindowsWindow*)glfwGetWindowUserPointer(window));

            WindowMoveEvent event(x, y);
            data->OnWindowMove(event);
            data->GetWindowData()->WindowMoveEvent.Execute(event);
        }
    }
}
