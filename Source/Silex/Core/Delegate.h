#pragma once

#include "Core/Memory.h"

#include <unordered_map>
#include <queue>


namespace Silex
{
#define SL_BIND_FN(fun)\
    [](auto&&... args) -> decltype(auto)\
    {\
        return std::invoke(fun, Traits::Forward<decltype(args)>(args)...);\
    }

#define SL_BIND_MEMBER_FN(ptr, fun)\
    [ptr, fun](auto&&... args) -> decltype(auto)\
    {\
        return std::invoke(fun, ptr, Traits::Forward<decltype(args)>(args)...);\
    }



    template <typename Signature, std::size_t BufferSize>
    class Function;

    template <typename Signature, std::size_t BufferSize = 16>
    class Delegate;

    template <typename Signature, std::size_t BufferSize = 16>
    class MulticastDelegate;


    //=====================================================================================
    // 固定サイズバッファ 簡易 std::function
    //-------------------------------------------------------------------------------------
    // 実際のサイズは、BufferSize + 8(Vtable)
    // ファンクターサイズはラムダ式のキャプチャに依存し、BufferSizeはそれに合わせた数値を指定する必要がある
    // 非静的メンバ関数のサポートのため、デフォルトで16バイト確保する（インスタンスポインタ + 関数ポインタ）
    //=====================================================================================
    template <typename ReturnT, typename... Args, std::size_t BufferSize>
    class Function<ReturnT(Args...), BufferSize>
    {
    public:

        Function() = default;
        ~Function() { Unbind(); }

        // コピー代入
        Function& operator=(const Function& other)
        {
            // 自己代入ダメ
            if (&other == this)
                return *this;

            Unbind();

            if (other.isBound)
            {
                other.callable->Clone(buffer, &callable);
                isBound = true;
            }

            return *this;
        }

        // ムーブコピー
        Function& operator=(Function&& other)
        {
            // 自己代入ダメ
            if (&other == this)
                return *this;

            Unbind();

            if (other.isBound)
            {
                other.callable->Clone(buffer, &callable);
                isBound = true;

                other.Unbind();
            }

            return *this;
        }

        // コピーコンストラクタ
        Function(const Function& other)
        {
            if (other.isBound)
            {
                other.callable->Clone(buffer, &callable);
                isBound = true;
            }
        }

        // ムーブコンストラクタ
        Function(Function&& other)
        {
            if (other.isBound)
            {
                other.callable->Clone(buffer, &callable);
                isBound = true;

                other.Unbind();
            }
        }

    public:

        // 関数オブジェクト割り当て
        template <typename F>
        void Bind(F&& f)
        {
            static_assert(sizeof(Callable<F>) <= sizeof(buffer), "割り当てようとする関数オブジェクトのサイズがバッファーサイズを超えています");

            if (isBound)
                Unbind();

            callable = Memory::Construct<Callable<F>>(buffer, Traits::Forward<F>(f));
            isBound = true;
        }

        // 割り当て解除
        void Unbind()
        {
            if (callable)
            {
                Memory::Destruct(callable);
                callable = nullptr;
            }

            std::memset(buffer, 0, BufferSize + sizeof(void*));
            isBound = false;
        }

        // 関数オブジェクト呼び出し
        ReturnT Execute(Args... args)
        {
            if (isBound)
            {
                return std::invoke(*callable, Traits::Forward<Args>(args)...);
            }
        }

        bool IsBound() const
        {
            return isBound;
        }

    private:

        // 関数オブジェクトインターフェース
        struct ICallable
        {
            virtual ~ICallable() = default;
            virtual ReturnT operator()(Args...)                       const = 0;
            virtual void Clone(byte* buffer, ICallable** callablePtr) const = 0;
        };

        template <typename T>
        struct Callable : public ICallable
        {
            T functor;

            Callable(T f) : functor(f)
            {
            }

            ReturnT operator()(Args... args) const override
            {
                return std::invoke(functor, Traits::Forward<Args>(args)...);
            }

            void Clone(byte* buffer, ICallable** callablePtr) const override
            {
                *callablePtr = Memory::Construct<Callable>(buffer, functor);
            }
        };

    private:

        bool       isBound = false;
        byte       buffer[BufferSize + sizeof(void*)];
        ICallable* callable = nullptr;
    };


    //=========================================================
    // シングルデリゲート
    //---------------------------------------------------------
    // 1つの Function を格納できるデリゲート
    // デフォルトの場合（16バイト）BufferSize 省略可
    //=========================================================
    template <typename ReturnT, typename... Args, std::size_t BufferSize>
    class Delegate<ReturnT(Args...), BufferSize>
    {
    public:

        using FuncT = Function<ReturnT(Args...), BufferSize>;

    public:

        // メンバ関数割り当て
        template <typename T, typename F>
        void Bind(T* instance, F memberFunc)
        {
            function.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
        }

        // 関数割り当て
        template <typename T>
        void Bind(T&& f)
        {
            function.Bind(Traits::Forward<T>(f));
        }

        // 実行
        ReturnT Execute(Args... args)
        {
            return function.Execute(Traits::Forward<Args>(args)...);
        }

        void Unbind()
        {
            function.Unbind();
        }

    private:

        FuncT function;
    };


    //=========================================================
    // マルチキャストデリゲート
    //---------------------------------------------------------
    // 複数の Function を格納できるデリゲート
    // デフォルトの場合（16バイト）BufferSize 省略可
    //=========================================================
    template<typename ReturnT, typename... Args, std::size_t BufferSize>
    class MulticastDelegate<ReturnT(Args...), BufferSize>
    {
    public:

        using FuncT          = Function<ReturnT(Args...), BufferSize>;
        using DelegateHandle = uint64;

    public:

        template <typename T>
        DelegateHandle Add(T&& f)
        {
            DelegateHandle handle = GetHandle();

            FuncT& func = functions[handle];
            func.Bind(Traits::Forward<T>(f));

            return handle;
        }

        template <typename T, typename F>
        DelegateHandle Add(T* instance, F memberFunc)
        {
            DelegateHandle handle = GetHandle();

            FuncT& func = functions[handle];
            func.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));

            return handle;
        }

        void Remove(DelegateHandle handle)
        {
            if (functions.erase(handle))
            {
                usedHandles.push(handle);
            }
        }

        void Broadcast(Args... args)
        {
            for (auto& [handle, func] : functions)
            {
                func.Execute(Traits::Forward<Args>(args)...);
            }
        }

        void RemoveAll()
        {
            for (const auto& [handle, func] : functions)
            {
                usedHandles.push(handle);
            }

            functions.clear();
        }

    private:

        DelegateHandle GetHandle()
        {
            if (!usedHandles.empty())
            {
                DelegateHandle handle = usedHandles.front();
                usedHandles.pop();
                return handle;
            }

            return nextHandle++;
        }

        std::unordered_map<DelegateHandle, FuncT> functions;
        std::queue<DelegateHandle>                usedHandles;

        DelegateHandle nextHandle = 0;
    };


#define SL_DECLARE_DELEGATE( DelegateName, ReturnType, ... )           using DelegateName = Delegate<ReturnType(__VA_ARGS__)>;
#define SL_DECLARE_MULTICAST_DELEGATE( DelegateName, ReturnType, ... ) using DelegateName = MulticastDelegate<ReturnType(__VA_ARGS__)>;

    //=======================================================================================================================
    // NOTE:
    //-----------------------------------------------------------------------------------------------------------------------
    // 引数が無いデリゲートは  'DECLARE_DELEGATE'
    // 引数がN個のデリゲートは 'DECLARE_DELEGATE_NParam'
    // 
    // 戻り値をデリゲート先に通知することを想定していないので、戻り値有りのマクロは用意されていません
    // 戻り値があるデリゲートを定義したい( bool xxx(int, int); の場合 )は、Delegate<bool(int, int)>; の様にマクロを使わず定義してください
    //=======================================================================================================================

    // 引数なし
#define SL_DECLARE_DELEGATE_0( DelegateName )           SL_DECLARE_DELEGATE(DelegateName, void)
#define SL_DECLARE_MULTICAST_DELEGATE_0( DelegateName ) SL_DECLARE_MULTICAST_DELEGATE(DelegateName, void)

// 引数あり
#define SL_DECLARE_DELEGATE_1_PARAM( DelegateName, Param1Type )                                                                                                 SL_DECLARE_DELEGATE( DelegateName, void, Param1Type )
#define SL_DECLARE_DELEGATE_2_PARAM( DelegateName, Param1Type, Param2Type )                                                                                     SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type )
#define SL_DECLARE_DELEGATE_3_PARAM( DelegateName, Param1Type, Param2Type, Param3Type )                                                                         SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type )
#define SL_DECLARE_DELEGATE_4_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type )                                                             SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type )
#define SL_DECLARE_DELEGATE_5_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type )                                                 SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type )
#define SL_DECLARE_DELEGATE_6_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type )                                     SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type )
#define SL_DECLARE_DELEGATE_7_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type )                         SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type )
#define SL_DECLARE_DELEGATE_8_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type )             SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type )
#define SL_DECLARE_DELEGATE_9_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type, Param9Type ) SL_DECLARE_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type, Param9Type )

#define SL_DECLARE_MULTICAST_DELEGATE_1_PARAM( DelegateName, Param1Type )                                                                                                 SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type )
#define SL_DECLARE_MULTICAST_DELEGATE_2_PARAM( DelegateName, Param1Type, Param2Type )                                                                                     SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type )
#define SL_DECLARE_MULTICAST_DELEGATE_3_PARAM( DelegateName, Param1Type, Param2Type, Param3Type )                                                                         SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type )
#define SL_DECLARE_MULTICAST_DELEGATE_4_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type )                                                             SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type )
#define SL_DECLARE_MULTICAST_DELEGATE_5_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type )                                                 SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type )
#define SL_DECLARE_MULTICAST_DELEGATE_6_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type )                                     SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type )
#define SL_DECLARE_MULTICAST_DELEGATE_7_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type )                         SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type )
#define SL_DECLARE_MULTICAST_DELEGATE_8_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type )             SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type )
#define SL_DECLARE_MULTICAST_DELEGATE_9_PARAM( DelegateName, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type, Param9Type ) SL_DECLARE_MULTICAST_DELEGATE( DelegateName, void, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type, Param6Type, Param7Type, Param8Type, Param9Type )
}
