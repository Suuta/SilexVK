#pragma once

#include "Core/Core.h"


namespace Silex
{
    //*********************************************************************************************************************
    // NVIDIAGameWorks/Falcor/ 参照カウントポインタ
    // https://github.com/NVIDIAGameWorks/Falcor/blob/95b516307065216b67aa6700c27e3777b3ad3811/Source/Falcor/Core/Object.h
    //
    // === 追加 ===
    // * DecRef関数: 参照カウントが0になった場合の解放処理
    // * Create関数: ユーティリティ関数
    // * As関数:     静的キャスト
    //*********************************************************************************************************************


    template<typename T>
    class Ref
    {
    private:

        // Object クラスにアップキャストできる
        static_assert(Traits::IsConvertible<T, Object>());

        // 派生クラスに対して、アクセス権を渡す
        template<typename T2> friend class Ref;

    private:

        T* instance = nullptr;

    public:

        Ref()               {}
        Ref(std::nullptr_t) {}


        ~Ref()
        {
            if (instance) DecRef((const Object*)(instance));
        }

        // ポインタ
        template<typename T2 = T>
        explicit Ref(T2* ptr) : instance(ptr)
        {
            if (instance) IncRef((const Object*)(instance));
        }

        // コピー
        Ref(const Ref& r) : instance(r.instance)
        {
            if (instance) IncRef((const Object*)(instance));
        }

        // 派生コピー
        template<typename T2 = T>
        Ref(const Ref<T2>& r) : instance(r.instance)
        {
            if (instance) IncRef((const Object*)(instance));
        }

        // ムーブ
        Ref(Ref&& r) noexcept
        {
            instance   = r.instance;
            r.instance = nullptr;
        }

        // 派生ムーブ
        template<typename T2>
        Ref(Ref<T2>&& r) noexcept
        {
            instance   = r.instance;
            r.instance = nullptr;
        }

        // コピー代入
        Ref& operator=(const Ref& r) noexcept
        {
            if (r != *this)
            {
                if (r.instance) IncRef((const Object*)(r.instance));

                T* prevPtr = instance;
                instance       = r.instance;

                if (prevPtr) DecRef((const Object*)(prevPtr));
            }

            return *this;
        }

        // 派生コピー代入
        template<typename T2>
        Ref& operator=(const Ref<T2>& r) noexcept
        {
            if (r != *this)
            {
                if (r.instance) IncRef((const Object*)(r.instance));

                T* prevPtr = instance;
                instance        = r.instance;

                if (prevPtr) DecRef((const Object*)(prevPtr));
            }

            return *this;
        }

        // ムーブコピー
        Ref& operator=(Ref&& r) noexcept
        {
            if (static_cast<void*>(&r) != this)
            {
                if (instance) DecRef((const Object*)(instance));

                instance   = r.instance;
                r.instance = nullptr;
            }

            return *this;
        }

        // 派生ムーブコピー
        template<typename T2>
        Ref& operator=(Ref<T2>&& r) noexcept
        {
            if (static_cast<void*>(&r) != this)
            {
                if (instance) DecRef((const Object*)(instance));

                instance   = r.instance;
                r.instance = nullptr;
            }

            return *this;
        }

        // リセット
        template<typename T2 = T>
        void Reset(T2* ptr = nullptr) noexcept
        {
            if (ptr != instance)
            {
                if (ptr) IncRef((const Object*)(ptr));

                T* prevPtr = instance;
                instance   = ptr;

                if (prevPtr) DecRef((const Object*)(prevPtr));
            }
        }

        template<typename T2 = T>
        bool operator==(const Ref<T2>& r) const
        {
            return instance == r.instance;
        }

        template<typename T2 = T>
        bool operator!=(const Ref<T2>& r) const
        {
            return instance != r.instance;
        }

        template<typename T2 = T>
        bool operator==(const T2* ptr) const
        {
            return instance == ptr;
        }

        template<typename T2 = T>
        bool operator!=(const T2* ptr) const
        {
            return instance != ptr;
        }

        bool operator==(std::nullptr_t) const { return instance == nullptr; }
        bool operator!=(std::nullptr_t) const { return instance != nullptr; }

        T* operator->() const { return instance;  }
        T& operator*()  const { return *instance; }
        T* Get()        const { return instance;  }

        operator bool() const { return instance != nullptr; }
        bool IsValid()  const { return instance != nullptr; }

        void Swap(Ref& r) noexcept
        {
            std::swap(instance, r.instance);
        }

        template<class T2>
        Ref<T2> As() const
        {
            return Ref<T2>(static_cast<T2*>(this->Get()));
        }

    private:

        void IncRef(const Object* object)
        {
            object->IncRefCount();
        }

        void DecRef(const Object* object)
        {
            object->DecRefCount();

            if (object->GetRefCount() == 0)
            {
                sldelete(object);
            }
        }
    };


    template<class T, class... Args>
    static Ref<T> CreateRef(Args&&... args)
    {
        static_assert(Traits::IsConvertible<T, Object>());

        return Ref<T>(slnew(T, Traits::Forward<Args>(args)...));
    }


#if 0
    //=============================================================================================================
    // C++20 可変長マクロ __VA_OPT__() を MSVC がデフォルトではサポートしていないので、/Zc:preprocessor オプションを有効にしている
    // https://stackoverflow.com/questions/68484818/function-like-macros-with-c20-va-opt-error-in-manual-code
    // 
    // 有効にしないと、展開部分のみならず、プロジェクト全体からエラーが出るので注意
    //=============================================================================================================

#if SL_ENABLE_MEMORY_TRACKING
    #define CreateRef(T, ...) Utils::CreateRef_MemoryTracking<T>(__FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#else
    #define CreateRef(T, ...) Utils::CreateRef_Internal<T>(__VA_ARGS__)
#endif

#endif
}
