
#include "PCH.h"

#include "Core/MemoryPool.h"
#include "Core/Memory.h"


namespace Silex
{
    namespace Internal
    {
        // 最上位ビット側から見て最初に見つかったビットの位置を返す
        SL_FORCEINLINE static ulong CountSetBitFromTop(uint64 value)
        {
            ulong index; _BitScanReverse64(&index, value);
            return index;
        }

        // 最下位ビット側から見て最初に見つかったビットの位置を返す
        SL_FORCEINLINE static ulong CountSetBitFromLast(uint64 value)
        {
            ulong index; _BitScanForward64(&index, value);
            return index;
        }

        //***********************************************************************************
        // 最も小さいサイズのメモリブロックのサイズ(現在は32)が0ビットとなるようにビットシフトしたビット列から
        // 最も位が大きいビットのビットインデックスを求める。
        //***********************************************************************************

        // BIT        9876543210     BIT        9876543210
        //----------------------    ----------------------
        //  32 = 000000000100000;     32 = 000000000000001; >>6 [0]
        //  64 = 000000001000000;     64 = 000000000000010; >>6 [1]
        // 128 = 000000010000000; -> 128 = 000000000000100; >>6 [2]
        // 256 = 000000100000000;    256 = 000000000001000; >>6 [3]
        // 512 = 000001000000000;    512 = 000000000010000; >>6 [4]

        // 各バイトブロック
        // m_BlockSize[0] ==   32
        // m_BlockSize[1] ==   64
        // m_BlockSize[2] ==  128
        // m_BlockSize[3] ==  256
        // m_BlockSize[4] ==  512
        // m_BlockSize[5] == 1024

        //template<class Size>
        //static constexpr uint32 SelectPoolIndexFromSize(Size requestByteSize)
        //{
        //    if      constexpr(  1 <= requestByteSize && requestByteSize <=   32) { return 0; }
        //    else if constexpr( 33 <= requestByteSize && requestByteSize <=   64) { return 1; }
        //    else if constexpr( 65 <= requestByteSize && requestByteSize <=  128) { return 2; }
        //    else if constexpr(129 <= requestByteSize && requestByteSize <=  256) { return 3; }
        //    else if constexpr(257 <= requestByteSize && requestByteSize <=  512) { return 4; }
        //    else if constexpr(513 <= requestByteSize && requestByteSize <= 1024) { return 5; }
        //
        //    return 0;
        //}

        //====================================================
        // if分岐の方がパフォーマンスが良かったので、ビット操作を行わない
        //====================================================
        static uint32 SelectPoolIndex(uint32 requestByteSize)
        {
            if      (  1 <= requestByteSize && requestByteSize <=   32) { return 0; }
            else if ( 33 <= requestByteSize && requestByteSize <=   64) { return 1; }
            else if ( 65 <= requestByteSize && requestByteSize <=  128) { return 2; }
            else if (129 <= requestByteSize && requestByteSize <=  256) { return 3; }
            else if (257 <= requestByteSize && requestByteSize <=  512) { return 4; }
            else if (513 <= requestByteSize && requestByteSize <= 1024) { return 5; }

            return 0;
        }
    }


    //============================================================================
    // プールデータ
    //============================================================================
    void MemoryPool::Pool::Create(const uint32 chunkSize, const uint32 poolSize)
    {
        blockByteSize = chunkSize;
        numBlocks     = poolSize / blockByteSize;
        ptr           = Memory::Malloc(poolSize + (numBlocks * sizeof(Header)));

        for (uint32 i = 0; i < numBlocks; i++)
        {
            uint64 address = (uint64)ptr + (i * sizeof(Header)) + (i * blockByteSize);
            PushFront(reinterpret_cast<Header*>(address));
        }
    }

    void MemoryPool::Pool::Destroy()
    {
        Memory::Free(ptr);
        ptr = nullptr;
    }

    MemoryPool::Header* MemoryPool::Pool::PopFront()
    {
        Header* top = head;
        head = head->next;
        return top;
    }

    void MemoryPool::Pool::PushFront(Header* header)
    {
        header->next = head;
        head = header;
    }


    //============================================================================
    // メモリープール
    //============================================================================
    void MemoryPool::Initialize()
    {
        const uint64 minPoolBlockByteSize = 32;
        const uint64 poolByteSize         = 10 * 1024 * 1024;

        for (uint32 i = 0; i < pools.size(); i++)
        {
            uint64 blockSize = minPoolBlockByteSize << i;
            pools[i].Create(blockSize, poolByteSize);

            status[i] = MemoryPoolStatus(blockSize, 0, poolByteSize);
        }
    }

    void MemoryPool::Finalize()
    {
        for (uint32 i = 0; i < pools.size(); i++)
        {
            pools[i].Destroy();
        }
    }

    void* MemoryPool::Allocate(const uint64 allocationSize)
    {
        // バイトサイズからプールを選択
        uint32 index = Internal::SelectPoolIndex(allocationSize);

        // ヘッダーにプールインデックスを格納
        Header* header = pools[index].PopFront();
        header->blockIndex = index;

        status[index].totalAllocated += pools[index].blockByteSize;

        // ヘッダー分ポインタをずらす
        return ++header;
    }

    void MemoryPool::Deallocate(void* pointer)
    {
        // ヘッダーの次のアドレスが渡されるはずなので、ヘッダーの先頭に戻す
        Header* header = static_cast<Header*>(pointer);
        --header;

        uint32 index = header->blockIndex;
        pools[index].PushFront(header);
        status[index].totalAllocated -= pools[index].blockByteSize;
    }
}
