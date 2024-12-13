
#pragma once

#include "Core/OS.h"
#include <unordered_map>


namespace Silex
{
    class Timer
    {
    public:

        Timer()
        {
            Reset();
        }

        void Reset()
        {
            start = OS::Get()->GetTickSeconds() / (float)1'000'000;
        }

        float Elapsed()
        {
            return OS::Get()->GetTickSeconds() / (float)1'000'000 - start;
        }

        float ElapsedMilli()
        {
            return Elapsed() * 1000.0f;
        }

        float ElapsedMicro()
        {
            return Elapsed();
        }

    private:

        float start;
    };


    class PerformanceProfiler
    {
    public:

        static PerformanceProfiler& Get()
        {
            static PerformanceProfiler profiler;
            return profiler;
        }

        void AddProfile(const char* name, float time)
        {
            perFrameData[name] = time;
        }

        void Reset()
        {
            perFrameData.clear();
        }

        void GetFrameData(std::unordered_map<const char*, float>* outData, bool shouldReset = false)
        {
            *outData = perFrameData;

            if (shouldReset)
            {
                Reset();
            }
        }

    private:

        static inline std::unordered_map<const char*, float> perFrameData;
    };


    // デストラクタを利用した、スコープ寿命のタイマー
    // 生成されてから破棄されるまでの時間を計測し、プロファイラーに登録する
    class ScopePerformanceTimer
    {
    public:

        ScopePerformanceTimer(const char* name, const PerformanceProfiler& profiler)
            : name(name)
            , profiler(profiler)
        {
        }

        ~ScopePerformanceTimer()
        {
            float time = timer.ElapsedMilli();
            profiler.AddProfile(name, time);
        }

    private:

        const char*         name;
        PerformanceProfiler profiler;
        Timer               timer;
    };

#define SL_SCOPE_PROFILE(name)  ScopePerformanceTimer timer__LINE__(name, PerformanceProfiler::Get());
}