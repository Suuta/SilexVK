
#pragma once

#include "Core/CoreType.h"


namespace Silex
{
    struct MemoryPoolStatus
    {
        uint32 chunkSize      = 0;
        uint32 totalAllocated = 0;
        uint32 totalSize      = 0;
    };


    class MemoryPool
    {
    public:

        MemoryPool()  = default;
        ~MemoryPool() = default;

        void Initialize();
        void Finalize();

        void* Allocate(const uint64 allocationSize);
        void  Deallocate(void* pointer);

        const std::array<MemoryPoolStatus, 6>& GetStatus() const { return status; }

    private:

        struct Header
        {
            uint32  blockIndex;
            Header* next;
        };

        struct Pool
        {
            Header* head = nullptr;
            void*   ptr  = nullptr;

            uint32 blockByteSize = 0;
            uint32 numBlocks     = 0;

            void Create(const uint32 chunkSize, const uint32 poolSize);
            void Destroy();

            void    PushFront(Header* header);
            Header* PopFront();
        };

        std::array<Pool,             6> pools;
        std::array<MemoryPoolStatus, 6> status;

    private:

        MemoryPool(MemoryPool&)             = delete;
        MemoryPool(MemoryPool&&)            = delete;
        MemoryPool& operator=(MemoryPool&)  = delete;
        MemoryPool& operator=(MemoryPool&&) = delete;

        friend class Allocator;
    };
}
