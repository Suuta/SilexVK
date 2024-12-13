
#pragma once

namespace Silex
{
    class EditorSplashImage
    {
    public:

        static void Show();
        static void Hide();
        static void SetText(const wchar_t* InText, float percentage);
    };

#define INIT_PROCESS(text, percentage) EditorSplashImage::SetText(L##text, percentage);
}
