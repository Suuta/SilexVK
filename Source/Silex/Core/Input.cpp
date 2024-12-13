
#include "PCH.h"

#include "Core/Input.h"
#include "Core/Engine.h"

#include <glfw/glfw3.h>


namespace Silex
{
    Input::KeyState   Input::keyCurrent;
    Input::KeyState   Input::keyPrevious;
    Input::MouseState Input::mouseCurrent;
    Input::MouseState Input::mousePrevious;

    void Input::Initialize()
    {
    }

    void Input::Finalize()
    {
    }

    void Input::Flush()
    {
        memcpy(&keyPrevious,   &keyCurrent,   sizeof(KeyState));
        memcpy(&mousePrevious, &mouseCurrent, sizeof(MouseState));
    }

    void Input::ProcessKey(Keys key, bool pressed)
    {
        if (keyCurrent.keys[(uint16)key] != pressed)
        {
            keyCurrent.keys[(uint16)key] = pressed;
        }
    }

    void Input::ProcessButton(Mouse button, bool pressed)
    {
        if (mouseCurrent.buttons[(uint16)button] != pressed)
        {
            mouseCurrent.buttons[(uint16)button] = pressed;
        }
    }

    void Input::ProcessMove(int16 x, int16 y)
    {
        if (mouseCurrent.x != x || mouseCurrent.y != y)
        {
            mouseCurrent.x = x;
            mouseCurrent.y = y;
        }
    }

    void Input::ProcessWheel(int8 zdelta)
    {
    }

    void Input::SetCursorMode(int32 mode)
    {
        auto window = Window::Get()->GetGLFWWindow();
        glfwSetInputMode(window, GLFW_CURSOR, mode);
    }

    glm::ivec2 Input::GetCursorPosition()
    {
        return { mouseCurrent.x, mouseCurrent.y };
    }

    void Input::SetCursorPosition(int32 x, int32 y)
    {
        auto window = Window::Get()->GetGLFWWindow();
        glfwSetCursorPos(window, x, y);
    }


    bool Input::IsKeyDown(Keys key)
    {
        return keyCurrent.keys[(uint16)key] == true;
    }

    bool Input::IsKeyUp(Keys key)
    {
        return keyCurrent.keys[(uint16)key] == false;
    }

    bool Input::IsKeyPressed(Keys key)
    {
        return keyPrevious.keys[(uint16)key] == false && keyCurrent.keys[(uint16)key] == true;
    }

    bool Input::IsKeyReleased(Keys key)
    {
        return keyPrevious.keys[(uint16)key] == true && keyCurrent.keys[(uint16)key] == false;
    }

    bool Input::IsMouseButtonDown(Mouse button)
    {
        return mouseCurrent.buttons[(uint8)button] == true;
    }

    bool Input::IsMouseButtonUp(Mouse button)
    {
        return mouseCurrent.buttons[(uint8)button] == false;
    }

    bool Input::IsMouseButtonPressed(Mouse button)
    {
        return mousePrevious.buttons[(uint8)button] == false && mouseCurrent.buttons[(uint8)button] == true;
    }

    bool Input::IsMouseButtonReleased(Mouse button)
    {
        return mousePrevious.buttons[(uint8)button] == true && mouseCurrent.buttons[(uint8)button] == false;
    }
}
