
#pragma once
#include "Core/OS.h"
#include "Core/Window.h"


namespace Silex
{
    class WindowsOS : public OS
    {
    public:

        WindowsOS();
        ~WindowsOS();

        void Initialize() override;
        void Finalize()   override;
        void Run()        override;

    public:

        // 時間
        uint64 GetTickSeconds()       override;
        void   Sleep(uint32 millisec) override;

        // ファイル
        std::string OpenFile(const char* filter = "All\0*.*\0")                                  override;
        std::string SaveFile(const char* filter = "All\0*.*\0", const char* extention = nullptr) override;

        // コンソール
        void SetConsoleAttribute(uint16 color)                      override;
        void OutputConsole(uint8 color, const std::string& message) override;
        void OutputDebugConsole(const std::string& message)         override;
        void OutputDebugConsole(const std::wstring& message)        override;

        // メッセージ
        int32 Message(OSMessageType type, const std::string& message) override;
        int32 Message(OSMessageType type, const std::wstring& message) override;

    public:

        // ビットマップ
        static HBITMAP LoadBitmapFile(const std::wstring& filePath);

    public:

        HRESULT TrySetWindowCornerStyle(HWND hWnd, bool tryRound);
        void    CheckOSVersion();

    private:

        // コンソール
        void*  outputHandle      = nullptr;
        void*  errorHandle       = nullptr;
        uint32 defaultConsoleCP = 0;

        // バージョン
        uint32 osVersionMajor = 0;
        uint32 osVersionMinor = 0;
        uint32 osBuildNumber  = 0;

        // 高性能クロックカウンター
        uint64 startTickCount = 0;
        uint64 tickPerSecond  = 0;
    };


#if 1
    namespace Windows
    {
        // S_OK             操作に成功しました
        // E_ABORT          操作は中止されました
        // E_ACCESSDENIED   一般的なアクセス拒否エラーが発生しました
        // E_FAIL           不特定のエラー
        // E_HANDLE         無効なハンドル
        // E_INVALIDARG     1つ以上の引数が無効です
        // E_NOINTERFACE    そのようなインターフェイスはサポートされていません
        // E_NOTIMPL        未実装
        // E_OUTOFMEMORY    必要なメモリの割り当てに失敗しました
        // E_POINTER        無効なポインター
        // E_UNEXPECTED     予期しないエラー

        // HRESULT の値
        //https://learn.microsoft.com/en-us/windows/win32/seccrypto/common-hresult-values
        inline const char* HRESULTToString(HRESULT result)
        {
            switch (result)
            {
                case S_OK           : return "S_OK";
                case E_ABORT        : return "E_ABORT";
                case E_ACCESSDENIED : return "E_ACCESSDENIED";
                case E_FAIL         : return "E_FAIL";
                case E_HANDLE       : return "E_HANDLE";
                case E_INVALIDARG   : return "E_INVALIDARG";
                case E_NOINTERFACE  : return "E_NOINTERFACE";
                case E_NOTIMPL      : return "E_NOTIMPL";
                case E_OUTOFMEMORY  : return "E_OUTOFMEMORY";
                case E_POINTER      : return "E_POINTER";
                case E_UNEXPECTED   : return "E_UNEXPECTED";

                default: break;
            }

            SL_ASSERT(false);
            return nullptr;
        }

        inline void Win32CheckResult(HRESULT result)
        {
            if (result != S_OK)
            {
                SL_LOG_ERROR("HRESULT: '{0}' - {1}:{2}", Windows::HRESULTToString(result), __FILE__, __LINE__);
                SL_ASSERT(false);
            }
        }
    }

#define CHECK_HRESULT(func) \
    {\
        HRESULT _result = func;\
        Silex::Windows::Win32CheckResult(_result);\
    }
#endif

}
