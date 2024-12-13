#pragma once

#include <glm/glm.hpp>


namespace Silex
{
    enum InputType
    {
        Cursor            = 0x00033001,
        StickyKey         = 0x00033002,
        StickyMouseButton = 0x00033003,
        LockKeyMod        = 0x00033004,
        RawMouseMotion    = 0x00033005,
    };

    enum CursorMode : int32
    {
        Normal   = 0x00034001,
        Hidden   = 0x00034002,
        Disable  = 0x00034003,
        Captured = 0x00034004,
    };


    enum class Mouse
    {
        Left,
        Right,
        Middle,
        Count,
    };

    enum class Keys : uint16
    {
        Space        = 32,
        Apostrophe   = 39,
        Comma        = 44,
        Minus        = 45,
        Period       = 46,
        Slash        = 47,

        D0           = 48,
        D1           = 49,
        D2           = 50,
        D3           = 51,
        D4           = 52,
        D5           = 53,
        D6           = 54,
        D7           = 55,
        D8           = 56,
        D9           = 57,

        Semicolon    = 59,
        Equal        = 61,

        A            = 65,
        B            = 66,
        C            = 67,
        D            = 68,
        E            = 69,
        F            = 70,
        G            = 71,
        H            = 72,
        I            = 73,
        J            = 74,
        K            = 75,
        L            = 76,
        M            = 77,
        N            = 78,
        O            = 79,
        P            = 80,
        Q            = 81,
        R            = 82,
        S            = 83,
        T            = 84,
        U            = 85,
        V            = 86,
        W            = 87,
        X            = 88,
        Y            = 89,
        Z            = 90,

        LeftBracket  = 91,
        Backslash    = 92,
        RightBracket = 93,
        GraveAccent  = 96,

        World1       = 161,
        World2       = 162,

        Escape       = 256,
        Enter        = 257,
        Tab          = 258,
        Backspace    = 259,
        Insert       = 260,
        Delete       = 261,
        Right        = 262,
        Left         = 263,
        Down         = 264,
        Up           = 265,
        PageUp       = 266,
        PageDown     = 267,
        Home         = 268,
        End          = 269,
        CapsLock     = 280,
        ScrollLock   = 281,
        NumLock      = 282,
        PrintScreen  = 283,
        Pause        = 284,
        F1           = 290,
        F2           = 291,
        F3           = 292,
        F4           = 293,
        F5           = 294,
        F6           = 295,
        F7           = 296,
        F8           = 297,
        F9           = 298,
        F10          = 299,
        F11          = 300,
        F12          = 301,
        F13          = 302,
        F14          = 303,
        F15          = 304,
        F16          = 305,
        F17          = 306,
        F18          = 307,
        F19          = 308,
        F20          = 309,
        F21          = 310,
        F22          = 311,
        F23          = 312,
        F24          = 313,
        F25          = 314,

        KP0          = 320,
        KP1          = 321,
        KP2          = 322,
        KP3          = 323,
        KP4          = 324,
        KP5          = 325,
        KP6          = 326,
        KP7          = 327,
        KP8          = 328,
        KP9          = 329,
        KPDecimal    = 330,
        KPDivide     = 331,
        KPMultiply   = 332,
        KPSubtract   = 333,
        KPAdd        = 334,
        KPEnter      = 335,
        KPEqual      = 336,

        LeftShift    = 340,
        LeftControl  = 341,
        LeftAlt      = 342,
        LeftSuper    = 343,
        RightShift   = 344,
        RightControl = 345,
        RightAlt     = 346,
        RightSuper   = 347,
        Menu         = 348,

        Count,
    };


    class Input
    {
    public:

        static void Initialize();
        static void Finalize();
        static void Flush();

        static void ProcessKey(Keys key, bool pressed);
        static void ProcessButton(Mouse button, bool pressed);
        static void ProcessMove(int16 x, int16 y);
        static void ProcessWheel(int8 zdelta);

    public:

        static void SetCursorMode(int32 mode);

        static bool IsKeyDown(Keys key);
        static bool IsKeyUp(Keys key);
        static bool IsKeyPressed(Keys key);
        static bool IsKeyReleased(Keys key);

        static bool IsMouseButtonDown(Mouse button);
        static bool IsMouseButtonUp(Mouse button);
        static bool IsMouseButtonPressed(Mouse button);
        static bool IsMouseButtonReleased(Mouse button);

        static glm::ivec2 GetCursorPosition();
        static void       SetCursorPosition(int32 x, int32 y);

    private:

        struct KeyState
        {
            bool keys[(uint16)Keys::Count];
        };

        struct MouseState
        {
            int16 x;
            int16 y;
            bool  buttons[(uint8)Mouse::Count];
        };

        static KeyState keyCurrent;
        static KeyState keyPrevious;

        static MouseState mouseCurrent;
        static MouseState mousePrevious;
    };
}
