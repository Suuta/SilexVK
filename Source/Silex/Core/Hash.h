
#pragma once
#include "Core/CoreType.h"


namespace Silex
{
    template<typename>
    struct fnv1a_constant;

    template<>
    struct fnv1a_constant<uint32>
    {
        using type = uint32;
        static constexpr uint32 offset = 2166136261u;
        static constexpr uint32 prime  = 16777619u;
    };

    template<>
    struct fnv1a_constant<uint64>
    {
        using Type = uint64;
        static constexpr uint64 offset = 14695981039346656037ull;
        static constexpr uint64 prime  = 1099511628211ull;
    };

    struct Hash
    {
        // 長さが不確定な文字配列のハッシュ
        template<typename T = uint64>
        static T FNV(const char* str)
        {
            uint64 length = std::strlen(str);
            T value = fnv1a_constant<T>::offset;

            for (uint64 i = 0; i < length; ++i)
            {
                value ^= *str++;
                value *= fnv1a_constant<T>::prime;
            }

            return value;
        }

        // コンパイル時定数な文字列リテラルのハッシュ
        template<typename T = uint64>
        static consteval T StaticFNV(const char* str)
        {
            T value = fnv1a_constant<T>::offset;
            while (*str != 0)
            {
                value ^= *str++;
                value *= fnv1a_constant<T>::prime;
            }

            return value;
        }

        //==========================================================
        // xxHash
        // https://github.com/stbrumme/xxhash/blob/master/xxhash64.h
        //==========================================================

#define XXHASH64 0
#if XXHASH64
        static consteval uint64 xxhash64(const char* str)
        {
            uint64 length = 0;
            while (str[length] != '\0')
            {
                length++;
            }

            const uint64 prime1 = 0x9E3779B185EBCA87ULL;
            const uint64 prime2 = 0xC2B2AE3D27D4EB4FULL;
            const uint64 prime3 = 0x165667B19E3779F9ULL;
            const uint64 prime4 = 0x85EBCA77C2B2AE63ULL;
            const uint64 prime5 = 0x27D4EB2F165667C5ULL;

            uint64 h64 = prime5;
            const uint8* p = reinterpret_cast<const uint8*>(str);
            const uint8* const bEnd = p + length;

#if 0
            XXH64_finalize(xxh_u64 hash, const xxh_u8 * ptr, size_t len, XXH_alignment align)
            {
                if (ptr == NULL) XXH_ASSERT(len == 0);
                len &= 31;
                while (len >= 8) {
                    xxh_u64 const k1 = XXH64_round(0, XXH_get64bits(ptr));
                    ptr += 8;
                    hash ^= k1;
                    hash = XXH_rotl64(hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
                    len -= 8;
                }
                if (len >= 4) {
                    hash ^= (xxh_u64)(XXH_get32bits(ptr)) * XXH_PRIME64_1;
                    ptr += 4;
                    hash = XXH_rotl64(hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
                    len -= 4;
                }
                while (len > 0) {
                    hash ^= (*ptr++) * XXH_PRIME64_5;
                    hash = XXH_rotl64(hash, 11) * XXH_PRIME64_1;
                    --len;
                }
                return  XXH64_avalanche(hash);
            }
#endif
            while (p + 8 <= bEnd)
            {
                uint64 k1 = *reinterpret_cast<const uint64*>(p);
                k1 *= prime2;
                k1 = (k1 << 31) | (k1 >> (64 - 31));
                k1 *= prime1;
                h64 ^= k1;
                h64 = (h64 << 27) | (h64 >> (64 - 27));
                h64 = h64 * prime1 + prime4;
                p += 8;
            }

            if (p + 4 <= bEnd)
            {
                uint64 k1 = *reinterpret_cast<const uint32*>(p);
                k1 *= prime1;
                k1 = (k1 << 23) | (k1 >> (64 - 23));
                k1 *= prime2;
                h64 ^= k1;
                h64 = h64 * prime1 + prime3;
                p += 4;
            }

            while (p < bEnd)
            {
                h64 ^= (*p) * prime5;
                h64 = (h64 << 11) | (h64 >> (64 - 11));
                h64 *= prime1;
                ++p;
            }


#if 0
            static xxh_u64 XXH64_avalanche(xxh_u64 hash)
            {
                hash ^= hash >> 33;
                hash *= XXH_PRIME64_2;
                hash ^= hash >> 29;
                hash *= XXH_PRIME64_3;
                hash ^= hash >> 32;
                return hash;
            }
#endif

            h64 ^= h64 >> 33;
            h64 *= prime2;
            h64 ^= h64 >> 29;
            h64 *= prime3;
            h64 ^= h64 >> 32;

            return h64;
        }
#endif


    };
}
