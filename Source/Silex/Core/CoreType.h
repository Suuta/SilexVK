
#pragma once
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


using int8   = std::int8_t;
using int16  = std::int16_t;
using int32  = std::int32_t;
using int64  = std::int64_t;

using uint8  = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

using byte    = std::uint8_t;
using float32 = float;
using float64 = double;
using ulong   = unsigned long;


namespace Silex
{
    namespace Traits
    {
        // 真偽値 基底クラス
        template <bool Val> struct BoolConstant { static constexpr bool Value = Val; };
        using TTrue  = BoolConstant<true>;
        using TFalse = BoolConstant<false>;

        // 
        template<class T1, class T2> struct Same       : TFalse {};
        template<class T>            struct Same<T, T> : TTrue  {};
        template<class T>            struct LVRef      : TFalse {};
        template<class T>            struct RVRef      : TFalse {};
        template<class T>            struct LVRef<T&>  : TTrue  {};
        template<class T>            struct RVRef<T&&> : TTrue  {};

        template<class T> struct RemoveRef                  { using Type = T; };
        template<class T> struct RemoveRef<T&>              { using Type = T; };
        template<class T> struct RemoveRef<T&&>             { using Type = T; };
        template<class T> struct RemoveConst                { using Type = T; };
        template<class T> struct RemoveConst<const T>       { using Type = T; };
        template<class T> struct RemoveVolatile             { using Type = T; };
        template<class T> struct RemoveVolatile<volatile T> { using Type = T; };
        template<class T> struct RemoveCV                   { using Type = T; };
        template<class T> struct RemoveCV<const T>          { using Type = T; };
        template<class T> struct RemoveCV<volatile T>       { using Type = T; };
        template<class T> struct RemoveCV<const volatile T> { using Type = T; };

        template<class From, class To>      struct Convertible : BoolConstant<__is_convertible_to(From, To)>{};
        template<class Base, class Derived> struct BaseOf      : BoolConstant<__is_base_of(Base, Derived)>  {};


        // 型
        template<class T> using TRemoveRef       = typename RemoveRef<T>::Type;
        template<class T> using TRemoveConst     = typename RemoveConst<T>::Type;
        template<class T> using TRemoveVolatile  = typename RemoveVolatile<T>::Type;
        template<class T> using TRemoveCV        = typename RemoveCV<T>::Type;
        template<class T> using TRemoveCVR       = typename RemoveRef<TRemoveCV<T>>::Type;

        // 型特性
        template<class T>                   constexpr bool IsLVRef()       { return LVRef<T>::Value;               }
        template<class T>                   constexpr bool IsRVRef()       { return RVRef<T>::Value;               }
        template<class T1,   class T2>      constexpr bool IsSame()        { return Same<T1, T2>::Value;           }
        template<class From, class To>      constexpr bool IsConvertible() { return Convertible<From, To>::Value;  }
        template<class Base, class Derived> constexpr bool IsBaseOf()      { return BaseOf<Base, Derived>::Value;  }

        // 転送・ムーブ
        template<class T> constexpr T&&             Forward(TRemoveRef<T>&  arg) { return static_cast<T&&>(arg);             }
        template<class T> constexpr T&&             Forward(TRemoveRef<T>&& arg) { return static_cast<T&&>(arg);             }
        template<class T> constexpr TRemoveRef<T>&& Move(T&& arg)                { return static_cast<TRemoveRef<T>&&>(arg); }
    }


    //========================================
    // クラス情報ベースクラス
    //========================================
    class Class
    {
    public:

        Class() {};
        virtual ~Class() {};

        virtual const char* GetRuntimeClassName() const = 0;
        virtual uint64      GetRuntimeHashID()    const = 0;

    public:

        // 自身と'T'が一致しているかどうかを調べる
        template<class T>
        bool IsClassOf()
        {
            static_assert(Traits::IsBaseOf<Class, T>(), "T は Class を継承する必要があります");
            return T::staticHashID == GetRuntimeHashID();
        }

        // T と T2 を比較する
        template<class T, class U>
        static bool IsSameClassOf(T* a, U* b)
        {
            static_assert(Traits::IsBaseOf<Class, T>(), "T は Class を継承する必要があります");
            static_assert(Traits::IsBaseOf<Class, U>(), "U は Class を継承する必要があります");
            return a->GetRuntimeHashID() == b->GetRuntimeHashID();
        }

        // 実際のランタイム型へのみキャストを行う
        template<class T>
        T* RuntimeCast()
        {
            static_assert(Traits::IsBaseOf<Class, T>(), "T は Class を継承する必要があります");
            return this->IsClassOf<T>()? (T*)this : nullptr;
        }
    };
}
