#pragma once

#include "Core/Macros.h"
#include "Core/CoreType.h"
#include "Core/MemoryPool.h"
#include "Core/OS.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <filesystem>
#include <atomic>
#include <bit>



//===============================================================
// 最小構成 STL アロケータ
//---------------------------------------------------------------
// メモリーアロケーター内の STL による operator new/delete の
// 再帰呼び出しを防ぐため。アロケーター最小構成
//===============================================================
template <typename T>
class DefaultAllocator
{
public:

    using value_type = T;

    DefaultAllocator() = default;

    template <typename U>
    DefaultAllocator(const DefaultAllocator<U>&) noexcept {}

    T* allocate(std::size_t n)
    {
        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
            return p;
        else
            throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        std::free(p);
        p = nullptr;
    }
};


namespace Silex
{
    class SpinLock
    {
        mutable std::atomic_flag locked = ATOMIC_FLAG_INIT;

    public:

        SL_FORCEINLINE void lock() const
        {
            while (locked.test_and_set(std::memory_order_acquire))
            {
            }
        }

        SL_FORCEINLINE void unlock() const
        {
            locked.clear(std::memory_order_release);
        }
    };



    //===============================================================
    // デバッグメモリトラッカー
    //===============================================================
#if 0
    class MemoryTracker
    {
    public:

        static void Initialize();
        static void Finalize();

        static void RecordAllocate(void* allocatedPtr, size_t size, const char* typeName, const char* file, size_t line);
        static void RecordDeallocate(void* ptr);
        static void DumpMemoryStats();

    private:

        // メモリデータ
        struct AllocationElement
        {
            size_t      size = {};
            const char* desc = {};
            const char* file = {};
            size_t      line = {};
        };

        // メモリデータコンテナ
        struct AllocationMap
        {
            using Type              = void*;
            using AllocatorType     = DefaultAllocator<std::pair<const Type, AllocationElement>>;
            using AllocationHashMap = std::unordered_map<Type, AllocationElement, std::hash<Type>, std::equal_to<>, AllocatorType>;

            AllocationHashMap memoryMap;
            std::mutex        mutex;
            uint64            totalAllocationSize;
        };

        static inline AllocationMap* allocationData = nullptr;
    };
#endif

    class MemoryTracker
    {
    public:

        static void RecordAllocate(void* ptr, uint64 size, const char* desc, const char* file, uint64 line);
        static void RecordDeallocate(void* ptr);
        static void DumpMemoryStats();

    private:

        // メモリデータ
        struct AllocationElement
        {
            uint64      size = {};
            const char* desc = {};
            const char* file = {};
            uint64      line = {};
        };

        static inline std::unordered_map<void*, AllocationElement> allocationMap;
    };


    //===============================================================
    // プールロケータ
    //===============================================================
    class PoolAllocator
    {
    public:

        static void Initialize();
        static void Finalize();

        static void* Allocate(uint64 sizeByte);
        static void  Deallocate(void* pointer);

        static const std::array<MemoryPoolStatus, 6>& GetStatus()
        {
            return pool.GetStatus();
        }

    private:

        static inline MemoryPool pool;
    };


    class Memory
    {
    public:

        static void Initialize()
        {
            PoolAllocator::Initialize();
        }

        static void Finalize()
        {
            MemoryTracker::DumpMemoryStats();
            PoolAllocator::Finalize();
        }

        // コンストラクタ / デストラクタ 呼び出し
        template<typename T, typename ... Args>
        static T* Construct(void* ptr, Args&& ... args)
        {
            return std::construct_at(static_cast<T*>(ptr), Traits::Forward<Args>(args)...);
        }

        template<typename T>
        static void Destruct(T* ptr)
        {
            std::destroy_at(ptr);
        }

        // ヒープ確保
        static void* Malloc(uint64 size)
        {
            return std::malloc(size);
        }

        static void Free(void* ptr)
        {
            std::free(ptr);
        }

        // プールから確保
#if SL_ENABLE_ALLOCATION_TRACKER
        template<typename T, typename... Args>
        static T* Allocate(const char* desc, const char* file, uint64 line, Args&& ... args)
        {
            static_assert(sizeof(T) <= 1024);

            void* ptr = PoolAllocator::Allocate(sizeof(T));
            MemoryTracker::RecordAllocate(ptr, sizeof(T), desc, file, line);

            return Memory::Construct<T>(ptr, Traits::Forward<Args>(args)...);
        }

        template<typename T>
        static void Deallocate(T* ptr)
        {
            Memory::Destruct(ptr);
            MemoryTracker::RecordDeallocate((void*)ptr);

            PoolAllocator::Deallocate((void*)ptr);
        }
#else
        template<typename T, typename... Args>
        static T* Allocate(Args&& ... args)
        {
            static_assert(sizeof(T) <= 1024);

            void* ptr = PoolAllocator::Allocate(sizeof(T));
            return Memory::Construct<T>(ptr, Traits::Forward<Args>(args)...);
    }

        template<typename T>
        static void Deallocate(T* ptr)
        {
            Memory::Destruct(ptr);
            PoolAllocator::Deallocate((void*)ptr);
        }
#endif
    };


#if 0
    //=======================================
    // カスタム new / delete
    //=======================================
    template<typename T, typename ... Args>
    T* New(Args&& ... args)
    {
        void* ptr = Memory::Malloc(sizeof(T));
        return new(ptr, SLEmpty()) T(Traits::Forward<Args>(args)...);
    }

    template<typename T, typename ... Args>
    T* NewArray(size_t arraySize, Args&& ... args)
    {
        T* ptrHead = (T*)Memory::Malloc(sizeof(T) * arraySize);
        T* cursor = ptrHead;

        for (uint32 i = 0; i < arraySize; i++)
        {
            new(cursor++, SLEmpty()) T(Traits::Forward<Args>(args)...);
        }

        return ptrHead;
    }

    template<typename T>
    void Delete(T* ptr)
    {
        if (ptr)
        {
            std::destroy_at(ptr);
            Memory::Free(ptr);
        }
    }

    template<typename T>
    void DeleteArray(T* ptr, size_t arraySize)
    {
        if (ptr)
        {
            std::destroy(&ptr[0], &ptr[arraySize - 1]);
            Memory::Free(ptr);
        }
    }
#endif



    template<class T>
    struct ResourceHandle
    {
        uint64 id;
        T*     ptr;
    };

    //=================================================
    // ポインタと検索用ID ハンドルを返すアロケータ
    // TODO: 解放された ID に対するバリデーションが必要
    //=================================================
    template<typename T, uint64 CHUNCK_ELEMENT = 65535>
    class ResourceStorage
    {
    public:

        ResourceStorage()
        {
            elements_in_chunk = sizeof(T) > CHUNCK_ELEMENT? 1 : (CHUNCK_ELEMENT / sizeof(T));
        }

        ~ResourceStorage()
        {
            if (alloc_count)
            {
                for (uint64 i = 0; i < max_alloc; i++)
                {
                    chunks[i / elements_in_chunk][i % elements_in_chunk].~T();
                }
            }

            uint32 chunk_count = max_alloc / elements_in_chunk;
            for (uint32 i = 0; i < chunk_count; i++)
            {
                std::free(chunks[i]);
                std::free(free_list_indices[i]);
            }

            if (chunks)
            {
                std::free(chunks);
                std::free(free_list_indices);
            }
        }

        ResourceHandle<T> Allocate(const T& value)
        {
            uint32 id  = _AllocateID();
            T* pointer = _GetPointer(id);

            std::construct_at(pointer, value);

            return { id, pointer };
        }

        void Deallocate(ResourceHandle<T> handle)
        {
            _Deallocate(handle.id);
        }

        T* GetData(ResourceHandle<T> handle)
        {
            return _GetPointer(handle.id);
        }

    private:

        uint32 _AllocateID()
        {
            if (alloc_count == max_alloc) SL_UNLIKELY
            {
                uint32 chunk_count = alloc_count == 0? 0 : (max_alloc / elements_in_chunk);

                chunks              = (T**)std::realloc(chunks, sizeof(T*) * (chunk_count + 1));
                chunks[chunk_count] = (T* )std::malloc(sizeof(T) * elements_in_chunk);

                free_list_indices              = (uint32**)std::realloc(free_list_indices, sizeof(uint32*) * (chunk_count + 1));
                free_list_indices[chunk_count] = (uint32* )std::malloc(sizeof(uint32) * elements_in_chunk);

                for (uint32 i = 0; i < elements_in_chunk; i++)
                {
                    free_list_indices[chunk_count][i] = alloc_count + i;
                }

                max_alloc += elements_in_chunk;
            }

            uint32 free_index   = free_list_indices[alloc_count / elements_in_chunk][alloc_count % elements_in_chunk];
            uint32 free_chunk   = free_index / elements_in_chunk;
            uint32 free_element = free_index % elements_in_chunk;

            alloc_count++;

            return free_index;
        }

        void _Deallocate(uint32 id)
        {
            alloc_count--;

            uint32 chunk   = id / elements_in_chunk;
            uint32 element = id % elements_in_chunk;

            // デストラクタ
            std::destroy_at(&chunks[chunk][element]);

            // 破棄されたインデックスを、次の割り当て時に使用できるインデックスとして割り当てる
            free_list_indices[alloc_count / elements_in_chunk][alloc_count % elements_in_chunk] = id;
        }

        T* _GetPointer(uint32 id)
        {
            uint32 free_chunk   = id / elements_in_chunk;
            uint32 free_element = id % elements_in_chunk;

            return &chunks[free_chunk][free_element];
        }

    private:

        T**      chunks            = nullptr;
        uint32** free_list_indices = nullptr;

        uint32 max_alloc   = 0;
        uint32 alloc_count = 0;

        uint32 elements_in_chunk;
    };


    template <typename T, bool THREAD_SAFE = false, uint32 DEFAULT_PAGE_SIZE = 4096>
    class PagedAllocator
    {
        uint32 page_shift = 0;
        uint32 page_mask  = 0;
        uint32 page_size  = 0;

        SpinLock spin_lock;

        T**    page_pool = nullptr;
        T***   free_pool = nullptr;
        uint32 num_page  = 0;
        uint32 num_free  = 0;

    public:

        template <typename... Args>
        T* Alloc(Args &&...p_args)
        {
            if constexpr (THREAD_SAFE) spin_lock.lock();

            if (num_free == 0) SL_UNLIKELY
            {
                uint32 page = num_page;
                num_page++;

                // 2ページ以上を使用していた場合、free関数による使用可能領域が1ページ以上（DEFAULT_PAGE_SIZE = 4096）
                // になった場合でも、reset関数を使わない限り、ページサイズは過去使用した最大ページ分確保する
                page_pool       = (T** )std::realloc(page_pool, sizeof(T*) * num_page);
                free_pool       = (T***)std::realloc(free_pool, sizeof(T**) * num_page);
                page_pool[page] = (T* )std::calloc(page_size, sizeof(T));
                free_pool[page] = (T**)std::calloc(page_size, sizeof(T*));

                // 使用可能領域（num_free）は Free 関数によってプールの末尾に追加される
                // 領域が空なので、新規割り当ては先頭に埋める
                for (uint32 i = 0; i < page_size; i++)
                    free_pool[0][i] = &page_pool[page][i];

                num_free += page_size;
            }

            num_free--;

            uint32 p = num_free >> page_shift;
            uint32 e = num_free & page_mask;
            T* alloc = free_pool[p][e];
            std::construct_at(alloc, p_args...);

            if constexpr (THREAD_SAFE) spin_lock.unlock();

            return alloc;
        }

        void Free(T* p_mem)
        {
            if constexpr (THREAD_SAFE) spin_lock.lock();

            std::destroy_at(p_mem);
            uint32 page = num_free >> page_shift;
            uint32 elem = num_free & page_mask;

            free_pool[page][elem] = p_mem;
            num_free++;

            if constexpr (THREAD_SAFE) spin_lock.unlock();
        }

    private:

        // 本家とは異なるが、自分で理解しやすい実装を採用
        uint32 _count_set_bit(uint64 value)
        {
            value = value == 0 ? 1 : value;
            return 63 - std::countl_zero(value);
        }

        void _reset(bool p_allow_unfreed)
        {
            if (!p_allow_unfreed || !std::is_trivially_destructible_v<T>)
            {
                SL_ASSERT(num_free < num_page * page_size);
            }

            if (num_page)
            {
                for (uint32 i = 0; i < num_page; i++)
                {
                    std::free(page_pool[i]);
                    std::free(free_pool[i]);
                }

                std::free(page_pool);
                std::free(free_pool);
                page_pool = nullptr;
                free_pool = nullptr;
                num_page  = 0;
                num_free  = 0;
            }
        }

    public:

        void reset(bool p_allow_unfreed = false)
        {
            if constexpr (THREAD_SAFE) spin_lock.lock();
            _reset(p_allow_unfreed);
            if constexpr (THREAD_SAFE) spin_lock.unlock();
        }

        bool is_configured() const
        {
            if constexpr (THREAD_SAFE) spin_lock.lock();
            bool result = page_size > 0;
            if constexpr (THREAD_SAFE) spin_lock.unlock();

            return result;
        }

        void configure(uint32_t p_page_size)
        {
            if constexpr (THREAD_SAFE) spin_lock.lock();

            SL_ASSERT(page_pool == nullptr);
            SL_ASSERT(p_page_size != 0);

            page_size  = 2 << _count_set_bit(p_page_size - 1);
            page_mask  = page_size - 1;
            page_shift = _count_set_bit(page_size);

            if constexpr (THREAD_SAFE) spin_lock.unlock();
        }

        //----------------------------------------------
        // コンストラクタ・デストラクタはスレッドセーフなのでは？
        //----------------------------------------------

        PagedAllocator(uint32_t p_page_size = DEFAULT_PAGE_SIZE)
        {
            configure(p_page_size);
        }

        ~PagedAllocator()
        {
            if constexpr (THREAD_SAFE) spin_lock.lock();

            bool leaked = num_free < num_page * page_size;
            if (leaked)
            {
                // メモリリーク
            }
            else
            {
                _reset(false);
            }

            if constexpr (THREAD_SAFE) spin_lock.unlock();
        }
    };
}
