
#include "PCH.h"
#include "ConsoleLogger.h"


namespace Silex
{
    static ConsoleLogger s_ConsoleLogger;

    ConsoleLogger& ConsoleLogger::Get()
    {
        return s_ConsoleLogger;
    }
}