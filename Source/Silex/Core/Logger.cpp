
#include "PCH.h"

#include "Core/OS.h"
#include "Editor/ConsoleLogger.h"


namespace Silex
{
    static std::mutex s_LogMutex;


    void Logger::Initialize()
    {
        OS::Get()->SetConsoleAttribute(8);
    }

    void Logger::Finalize()
    {
        OS::Get()->SetConsoleAttribute(8);
    }

    void Logger::SetLogLevel(LogLevel level)
    {
        logFilter = level;
    }

    void Logger::Log(LogLevel level, const std::string& msg)
    {
        // ログレベルのフィルタリング
        if (level <= logFilter)
        {
            switch (level)
            {
                case LogLevel::Fatal : OS::Get()->OutputDebugConsole("[FATAL] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[FATAL] " + msg + "\n"); break;
                case LogLevel::Error : OS::Get()->OutputDebugConsole("[ERROR] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[ERROR] " + msg + "\n"); break;
                case LogLevel::Warn  : OS::Get()->OutputDebugConsole("[WARN ] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[WARN ] " + msg + "\n"); break;
                case LogLevel::Info  : OS::Get()->OutputDebugConsole("[INFO ] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[INFO ] " + msg + "\n"); break;
                case LogLevel::Trace : OS::Get()->OutputDebugConsole("[TRACE] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[TRACE] " + msg + "\n"); break;
                case LogLevel::Debug : OS::Get()->OutputDebugConsole("[DEBUG] " + msg + "\n"); ConsoleLogger::Get().Log(level, "[DEBUG] " + msg + "\n"); break;

                default: break;
            }
        }
    }
}
