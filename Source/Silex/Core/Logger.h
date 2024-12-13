
#pragma once

#include "Core/CoreType.h"
#include <format>
#include <string>


namespace Silex
{
    enum class LogLevel
    {
        Fatal,
        Error,
        Warn,
        Info,
        Trace,
        Debug,

        Count,
    };

    class Logger
    {
    public:

        static void Initialize();
        static void Finalize();
        static void SetLogLevel(LogLevel level);

        static void Log(LogLevel level, const std::string& message);

    private:

        static inline LogLevel logFilter = LogLevel::Debug;
    };
}
