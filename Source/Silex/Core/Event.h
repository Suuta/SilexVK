#pragma once

#include "Core/Input.h"


namespace Silex
{
    struct Event : public Class
    {
        SL_CLASS(Event, Class)
    };



    //======================================================================
    // ウィンドウ
    //======================================================================
    struct WindowResizeEvent : public Event
    {
        SL_CLASS(WindowResizeEvent, Event)
        WindowResizeEvent(uint32 w, uint32 h) : width(w), height(h) {}

        uint32 width;
        uint32 height;
    };

    struct WindowCloseEvent : public Event
    {
        SL_CLASS(WindowCloseEvent, Event)
        WindowCloseEvent() {}
    };

    struct WindowMoveEvent : public Event
    {
        SL_CLASS(WindowMoveEvent, Event)
            WindowMoveEvent(uint32 x, uint32 y) : x(x), y(y) {}

        uint32 x;
        uint32 y;
    };

    //======================================================================
    // キー入力
    //======================================================================
    struct KeyEvent : public Event
    {
        SL_CLASS(KeyEvent, Event)

        KeyEvent(Keys k) : key(k) {}
        Keys key;
    };

    struct KeyPressedEvent : public KeyEvent
    {
        SL_CLASS(KeyPressedEvent, KeyEvent)
        KeyPressedEvent(Keys key) : KeyEvent(key) {}
    };

    struct KeyReleasedEvent : public KeyEvent
    {
        SL_CLASS(KeyReleasedEvent, KeyEvent)
        KeyReleasedEvent(Keys key) : KeyEvent(key) {}
    };

    struct KeyRepeatEvent : public KeyEvent
    {
        SL_CLASS(KeyRepeatEvent, KeyEvent)
        KeyRepeatEvent(Keys key) : KeyEvent(key) {}
    };


    //======================================================================
    // マウス
    //======================================================================
    struct MouseButtonEvent : public Event
    {
        SL_CLASS(MouseButtonEvent, Event)

        MouseButtonEvent(int b) : button(b) {}
        int32 button;
    };

    struct MouseButtonPressedEvent : public MouseButtonEvent
    {
        SL_CLASS(MouseButtonPressedEvent, MouseButtonEvent)
        MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}
    };

    struct MouseButtonReleasedEvent : public MouseButtonEvent
    {
        SL_CLASS(MouseButtonReleasedEvent, MouseButtonEvent)
        MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}
    };

    struct MouseButtonRepeatEvent : public MouseButtonEvent
    {
        SL_CLASS(MouseButtonRepeatEvent, MouseButtonEvent)
        MouseButtonRepeatEvent(int button) : MouseButtonEvent(button) {}
    };

    struct MouseMoveEvent : public Class
    {
        SL_CLASS(MouseMoveEvent, Class)
        MouseMoveEvent(float x, float y) : mouseX(x), mouseY(y) {}

        float mouseX;
        float mouseY;
    };

    struct MouseScrollEvent : public Event
    {
        SL_CLASS(MouseScrollEvent, Event)
        MouseScrollEvent(float x, float y) : offsetX(x), offsetY(y) {}

        float offsetX;
        float offsetY;
    };
}
