#pragma once

#include "Core/Event.h"
#include "Rendering/RenderingCore.h"


struct GLFWwindow;

namespace Silex
{
    #define SL_DECLARE_WINDOW_EVENT_FN(eventName)                      \
    template <typename T, typename F>                                  \
    void Bind##eventName(T* instance, F memberFunc)                    \
    {                                                                  \
        static_assert(Traits::IsSame<F, void(T::*)(eventName&)>());    \
        data->eventName.Bind(SL_BIND_MEMBER_FN(instance, memberFunc)); \
    }                                                                  \
    template <typename T>                                              \
    void Bind##eventName(T&& func)                                     \
    {                                                                  \
        static_assert(Traits::IsSame<T, void(*)(eventName&)>());       \
        data->eventName.Bind(Traits::Forward<T>(func));                \
    }

    class Window;
    class RenderingContext;

    using WindowCreateFunction = Window* (*)(const char* title, uint32 width, uint32 height);

    SL_DECLARE_DELEGATE(WindowCloseDelegate,         void, WindowCloseEvent&)
    SL_DECLARE_DELEGATE(WindowResizeDelegate,        void, WindowResizeEvent&)
    SL_DECLARE_DELEGATE(KeyPressedDelegate,          void, KeyPressedEvent&)
    SL_DECLARE_DELEGATE(KeyReleasedDelegate,         void, KeyReleasedEvent&)
    SL_DECLARE_DELEGATE(MouseButtonPressedDelegate,  void, MouseButtonPressedEvent&)
    SL_DECLARE_DELEGATE(MouseButtonReleasedDelegate, void, MouseButtonReleasedEvent&)
    SL_DECLARE_DELEGATE(MouseScrollDelegate,         void, MouseScrollEvent&)
    SL_DECLARE_DELEGATE(MouseMoveDelegate,           void, MouseMoveEvent&)
    SL_DECLARE_DELEGATE(WindowMoveDelegate,          void, WindowMoveEvent&)


    // ウィンドウデータ
    struct WindowData
    {
        std::string title  = {};
        uint32      width  = 0;
        uint32      height = 0;
        VSyncMode   vsync  = VSYNC_MODE_DISABLED;

        ~WindowData()
        {
            WindowCloseEvent.Unbind();
            WindowResizeEvent.Unbind();
            KeyPressedEvent.Unbind();
            KeyReleasedEvent.Unbind();
            MouseButtonPressedEvent.Unbind();
            MouseButtonReleasedEvent.Unbind();
            MouseScrollEvent.Unbind();
            MouseMoveEvent.Unbind();
        }

        WindowCloseDelegate         WindowCloseEvent;
        WindowResizeDelegate        WindowResizeEvent;
        KeyPressedDelegate          KeyPressedEvent;
        KeyReleasedDelegate         KeyReleasedEvent;
        MouseButtonPressedDelegate  MouseButtonPressedEvent;
        MouseButtonReleasedDelegate MouseButtonReleasedEvent;
        MouseScrollDelegate         MouseScrollEvent;
        MouseMoveDelegate           MouseMoveEvent;
        WindowMoveDelegate          WindowMoveEvent;
    };

    // ウィンドウインターフェース
    class Window : public Object
    {
        SL_CLASS(Window, Object)

    public:

        static Window* Create(const char* title, uint32 width, uint32 height)
        {
            return createFunction(title, width, height);
        }

        static Window* Get()
        {
            return instance;
        }

        static void RegisterCreateFunction(WindowCreateFunction createFunc)
        {
            createFunction = createFunc;
        }

    public:

        Window()  { instance = this;    }
        ~Window() { instance = nullptr; }

        virtual bool Initialize() = 0;

        // ウィンドウメッセージ
        virtual void ProcessMessage() = 0;

        // ウィンドウサイズ
        virtual glm::ivec2 GetSize()      const = 0;
        virtual glm::ivec2 GetWindowPos() const = 0;

        // ウィンドウ属性
        virtual void Maximize() = 0;
        virtual void Minimize() = 0;
        virtual void Restore()  = 0;
        virtual void Show()     = 0;
        virtual void Hide()     = 0;

        // タイトル
        virtual const char* GetTitle() const                   = 0;
        virtual void        SetTitle(const std::string& title) = 0;

        // ウィンドウデータ
        virtual GLFWwindow*      GetGLFWWindow()     const = 0;
        virtual WindowData*      GetWindowData()     const = 0;
        virtual SurfaceHandle*   GetSurface()        const = 0;
        virtual SwapChainHandle* GetSwapChain()      const = 0;
        virtual void*            GetPlatformHandle() const = 0;

        // レンダリングコンテキスト
        virtual bool SetupWindowContext(RenderingContext* context)   = 0;
        virtual void CleanupWindowContext(RenderingContext* context) = 0;

        // ウィンドウイベント
        virtual void OnWindowClose(WindowCloseEvent& e)                 = 0;
        virtual void OnWindowResize(WindowResizeEvent& e)               = 0;
        virtual void OnKeyPressed(KeyPressedEvent& e)                   = 0;
        virtual void OnKeyReleased(KeyReleasedEvent& e)                 = 0;
        virtual void OnMouseButtonPressed(MouseButtonPressedEvent& e)   = 0;
        virtual void OnMouseButtonReleased(MouseButtonReleasedEvent& e) = 0;
        virtual void OnMouseScroll(MouseScrollEvent& e)                 = 0;
        virtual void OnMouseMove(MouseMoveEvent& e)                     = 0;
        virtual void OnWindowMove(WindowMoveEvent& e)                   = 0;

    public:

        SL_DECLARE_WINDOW_EVENT_FN(WindowCloseEvent);
        SL_DECLARE_WINDOW_EVENT_FN(WindowResizeEvent);
        SL_DECLARE_WINDOW_EVENT_FN(KeyPressedEvent);
        SL_DECLARE_WINDOW_EVENT_FN(KeyReleasedEvent);
        SL_DECLARE_WINDOW_EVENT_FN(MouseButtonPressedEvent);
        SL_DECLARE_WINDOW_EVENT_FN(MouseButtonReleasedEvent);
        SL_DECLARE_WINDOW_EVENT_FN(MouseScrollEvent);
        SL_DECLARE_WINDOW_EVENT_FN(MouseMoveEvent);
        SL_DECLARE_WINDOW_EVENT_FN(WindowMoveEvent);

    protected:

        WindowData* data = nullptr;

        static inline Window*              instance;
        static inline WindowCreateFunction createFunction;
    };
}
