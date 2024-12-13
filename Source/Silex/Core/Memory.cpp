
#include "PCH.h"

#include "Core/Memory.h"
#include "Core/Logger.h"

#include <filesystem>


namespace Silex
{
    static bool s_Initialized = false;


    void PoolAllocator::Initialize()
    {
        pool.Initialize();
    }

    void PoolAllocator::Finalize()
    {
        pool.Finalize();
    }

    void* PoolAllocator::Allocate(uint64 sizeByte)
    {
        return pool.Allocate(sizeByte);
    }

    void PoolAllocator::Deallocate(void* pointer)
    {
        pool.Deallocate(pointer);
    }

    const uint64 t = 10 * 1024 * 1024;

    void MemoryTracker::RecordAllocate(void* ptr, uint64 size, const char* desc, const char* file, uint64 line)
    {
        allocationMap[ptr] = {size, desc, file, line};
    }

    void MemoryTracker::RecordDeallocate(void* ptr)
    {
        allocationMap.erase(ptr);
    }

    void MemoryTracker::DumpMemoryStats()
    {
#if SL_ENABLE_ALLOCATION_TRACKER
        SL_LOG_WARN("*************************************************************************************************************");
        SL_LOG_WARN(" Memory In Use:");
        SL_LOG_WARN("*************************************************************************************************************");
        for (auto& [ptr, info] : allocationMap)
        {
            SL_LOG_WARN(" {:>4} byte | {:<20} | {} [{}]", info.size, info.desc, info.file, info.line);
        }
        SL_LOG_WARN("*************************************************************************************************************");
#endif
    }



#if 0
    void MemoryTracker::Initialize()
    {
        if (s_Initialized)
            return;

        allocationData = Memory::Construct<AllocationMap>(Memory::Malloc(sizeof(AllocationMap)));;
        allocationData->totalAllocationSize = 0;

        s_Initialized = true;
    }

    void MemoryTracker::Finalize()
    {
        s_Initialized = false;

        Memory::Destruct(allocationData);
        Memory::Free(allocationData);

        allocationData = nullptr;
    }

    void MemoryTracker::RecordAllocate(void* allocatedPtr, size_t size, const char* desc, const char* file, size_t line)
    {
        // Init() 呼び出し前に、静的変数がoperator new() を呼び出す可能性があるため
        if (!s_Initialized)
            Initialize();

        {
            std::scoped_lock lock(allocationData->mutex);

            AllocationElement& element = allocationData->memoryMap[allocatedPtr];
            element.size = size;
            element.file = file;
            element.line = line;
            element.desc = desc;

            allocationData->totalAllocationSize += size;
        }
    }

    void MemoryTracker::RecordDeallocate(void* ptr)
    {
        if (ptr == nullptr)
            return;

        // メモリートラッカー解放後のアクセスを抑制
        if (s_Initialized)
        {
            std::scoped_lock lock(allocationData->mutex);

            auto itr = allocationData->memoryMap.find(ptr);
            if (itr != allocationData->memoryMap.end())
            {
                auto& [p, element] = *itr;

                allocationData->totalAllocationSize -= element.size;
                allocationData->memoryMap.erase(ptr);
            }
        }
    }

    void MemoryTracker::DumpMemoryStats()
    {
        SL_LOG_DEBUG("***************************************************************************************************");
        SL_LOG_DEBUG("MemoryUsage: {} byte", s_AllocationData->TotalAllocationSize);

        if (!s_AllocationData->memoryMap.empty())
        SL_LOG_DEBUG("***************************************************************************************************");

        for (auto& [ptr, info] : s_AllocationData->memoryMap)
        {
            SL_LOG_DEBUG(" {:>8} byte | {:<16} | {} [{}]", info.size, info.desc, info.file, info.line);
        }
        SL_LOG_DEBUG("***************************************************************************************************");
    }
#endif
}
