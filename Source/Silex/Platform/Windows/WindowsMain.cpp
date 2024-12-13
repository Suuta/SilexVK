
#include "PCH.h"
#include "Platform/Windows/WindowsOS.h"


#if SL_PLATFORM_WINDOWS

namespace Silex
{
    extern bool LaunchEngine();
    extern void ShutdownEngine();

    int32 Main()
    {
        WindowsOS os;

        bool result = LaunchEngine();
        if (result)
        {
            os.Run();
        }

        ShutdownEngine();
        return result;
    }
}

#if SL_RELEASE
int32 WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ char* lpCmdLine, _In_ int32 nCmdShow)
#else
int32 main()
#endif
{
    return Silex::Main();
}

#endif // SL_PLATFORM_WINDOWS