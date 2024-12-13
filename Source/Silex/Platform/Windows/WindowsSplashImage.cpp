
//===============================================================================
// UnrealEngine 5.2 の　FSplashScreen を 参照
// Engine/Source/Runtime/ApplicationCore/Public/Windows/WindowsPlatformSplash.h
//===============================================================================

#include "PCH.h"
#include "Editor/EditorSplashImage.h"
#include "Core/OS.h"
#include "Core/Engine.h"
#include "Core/ThreadPool.h"
#include "Platform/Windows/WindowsOS.h"


#ifdef SL_PLATFORM_WINDOWS

namespace Silex
{
    enum TextType
    {
        StartupProgress,
        VersionInfo,
        CopyrightInfo,
        AppName,

        NumTextTypes
    };

    struct WindowsContext
    {
        std::wstring appName;
        std::wstring text[4];
        RECT         textRects[4];
        HWND         windowHandle       = NULL;
        HFONT        textFont           = NULL;
        HFONT        titleFont          = NULL;
        HBITMAP      backgroundBitmap   = NULL; // Unrealと同サイズの 720 x 370 を使用
        HBITMAP      progressBitmap     = NULL;
        int          progressPercentage = 0;

        void SetText(const TextType InType, const wchar_t* InText)
        {
            std::wstring InTextW = InText;
            bool bWasUpdated = false;

            if (InTextW != text[InType].c_str())
            {
                text[InType] = InTextW;
                bWasUpdated = true;
            }

            if (bWasUpdated)
            {
                const BOOL bErase = true;
                InvalidateRect(windowHandle, &textRects[InType], bErase);
            }
        }

        void SetLoadProgressPercentage(float percentage)
        {
            progressPercentage = 720.f * (percentage / 100.0f);
            RECT rect = { 0, 270, 720, 275 };
            InvalidateRect(windowHandle, &rect, FALSE); // WM_PAINT をトリガーさせる
        }
    };

    static WindowsContext context;


    int64 CALLBACK SplashScreenWindowProc(HWND hWnd, uint32 message, uint64 wParam, int64 lParam)
    {
        switch (message)
        {
            case WM_PAINT:
            {
                HDC         hdc;
                PAINTSTRUCT ps;
                hdc = ::BeginPaint(hWnd, &ps);

                // バックグラウンド
                ::DrawStateW(hdc, NULL, NULL, (LPARAM)context.backgroundBitmap, NULL, 0, 0, 0, 0, DST_BITMAP);

                // プログレスバー
                ::DrawStateW(hdc, NULL, NULL, (LPARAM)context.progressBitmap, NULL, 0, 270, 1 + context.progressPercentage, 275, DST_BITMAP);

                // 文字列
                for (int32 CurTypeIndex = 0; CurTypeIndex < TextType::NumTextTypes; ++CurTypeIndex)
                {
                    const std::wstring& SplashText = context.text[CurTypeIndex];
                    const RECT& TextRect = context.textRects[CurTypeIndex];

                    if (!SplashText.empty())
                    {
                        COLORREF title = RGB(255, 255, 255);
                        COLORREF text  = RGB(160, 160, 160);
                        COLORREF black = RGB(0, 0, 0);

                        ::SelectObject(hdc, CurTypeIndex == TextType::AppName ? context.titleFont : context.textFont);
                        ::SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);
                        ::SetBkColor(hdc, black);
                        ::SetBkMode(hdc, TRANSPARENT);

                        RECT ClientRect;
                        ::GetClientRect(hWnd, &ClientRect);
                        ::SetTextColor(hdc, CurTypeIndex == TextType::AppName ? title : text);

                        ::TextOutW(hdc, TextRect.left, TextRect.top, SplashText.c_str(), SplashText.length());
                    }
                }

                ::EndPaint(hWnd, &ps);
                break;
            }

            case WM_DESTROY:
            {
                ::PostQuitMessage(0);
                break;
            }

            default: return DefWindowProc(hWnd, message, wParam, lParam);
        }

        return 0;
    }

    static uint32 SplashScreenThread()
    {
        WNDCLASS wc;
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = &SplashScreenWindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = NULL;
        wc.hIcon         = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
        wc.hCursor       = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = TEXT("Silex");

        ::RegisterClassW(&wc);

        BITMAP bm, pm;
        memset(&bm, 0, sizeof(bm));
        memset(&pm, 0, sizeof(pm));

        context.backgroundBitmap = WindowsOS::LoadBitmapFile(L"Assets/Editor/Splash.png");
        context.progressBitmap   = WindowsOS::LoadBitmapFile(L"Assets/Editor/Progress.png");

        ::GetObjectW(context.backgroundBitmap,         sizeof(bm), &bm);
        ::GetObjectW(context.progressBitmap, sizeof(pm), &pm);

        const int32 w = bm.bmWidth;
        const int32 h = bm.bmHeight;
        const int32 x = (::GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        const int32 y = (::GetSystemMetrics(SM_CYSCREEN) - h) / 2;

        context.windowHandle = ::CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, wc.lpszClassName, WS_POPUP, x, y, w, h, nullptr, nullptr, nullptr, nullptr);

        // フォント生成
        LOGFONT font;
        memset(&font, 0, sizeof(font));
        font.lfQuality    = CLEARTYPE_QUALITY;
        font.lfHeight     = 15;
        font.lfWeight     = FW_LIGHT;
        context.textFont  = ::CreateFontIndirectW(&font);
        font.lfHeight     = 25;
        font.lfWeight     = FW_BOLD;
        context.titleFont = ::CreateFontIndirectW(&font);

        // アプリ名
        context.textRects[TextType::AppName].top           = bm.bmHeight - 80;
        context.textRects[TextType::AppName].bottom        = bm.bmHeight - 60;
        context.textRects[TextType::AppName].left          = 100;
        context.textRects[TextType::AppName].right         = bm.bmWidth - 20;

        // プログレステキスト
        context.textRects[TextType::StartupProgress].top    = bm.bmHeight - 30;
        context.textRects[TextType::StartupProgress].bottom = bm.bmHeight - 10;
        context.textRects[TextType::StartupProgress].left   = 100;
        context.textRects[TextType::StartupProgress].right  = bm.bmWidth - 20;

        // バージョン情報
        context.textRects[TextType::VersionInfo].top       = bm.bmHeight - 50;
        context.textRects[TextType::VersionInfo].bottom    = bm.bmHeight - 30;
        context.textRects[TextType::VersionInfo].left      = 100;
        context.textRects[TextType::VersionInfo].right     = bm.bmWidth - 20;

        // コピーライト表示
        context.textRects[TextType::CopyrightInfo].top      = bm.bmHeight - 25;
        context.textRects[TextType::CopyrightInfo].bottom   = bm.bmHeight - 15;
        context.textRects[TextType::CopyrightInfo].left     = bm.bmWidth  - 100;
        context.textRects[TextType::CopyrightInfo].right    = bm.bmWidth  - 20;

        ::ShowWindow(context.windowHandle, SW_SHOW);
        ::UpdateWindow(context.windowHandle);

        MSG message;
        while (true)
        {
            if (::PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&message);
                ::DispatchMessageW(&message);

                if (message.message == WM_QUIT)
                {
                    break;
                }
            }
        }

        ::DeleteObject(context.backgroundBitmap);
        ::DeleteObject(context.progressBitmap);
        ::UnregisterClassW(wc.lpszClassName, (HINSTANCE)NULL);

        return 0;
    }

    void EditorSplashImage::Show()
    {
        context.appName = TEXT("Silex");
        context.text[TextType::AppName]         = L"Silex";
        context.text[TextType::VersionInfo]     = L"1.0";
        context.text[TextType::StartupProgress] = L"Initialize...";

        // SplashScreen スレッド開始
        ThreadPool::AddTask(&SplashScreenThread);
    }

    void EditorSplashImage::Hide()
    {
        // ウィンドウ破棄
        if (context.windowHandle)
        {
            ::PostMessageW(context.windowHandle, WM_CLOSE, 0, 0);
            context.windowHandle = NULL;
        }

        // SplashScreen スレッドを含む全スレッドを待機 
        // エンジンの初期化完了後に呼ばれるはずだが、残っているものがあれば待機する
        ThreadPool::WaitAll();
    }

    void EditorSplashImage::SetText(const wchar_t* InText, float percentage)
    {
        context.SetText(TextType::StartupProgress, InText);
        context.SetLoadProgressPercentage(percentage);
    }
}

#endif // SL_PLATFORM_WINDOWS
