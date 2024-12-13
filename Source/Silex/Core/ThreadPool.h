
#pragma once
#include "Core/CoreType.h"
#include <functional>


namespace Silex
{
    using Task = std::function<void()>;

    class ThreadPool
    {
    public:

        static void Initialize();
        static void Finalize();

        static void AddTask(Task&& task);
        static void WaitAll();

        static uint32 GetThreadCount();
        static uint32 GetWorkingThreadCount();
        static uint32 GetIdleThreadCount();
        static bool   HasRunningTask();
    };
}

