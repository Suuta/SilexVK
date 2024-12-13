
#pragma once
#include "Core/Memory.h"


//==================================================================
// デリゲートクラスではバッファが固定サイズで、ラムダ式のキャプチャが多くなった際に
// サイズが足りなくなるため、可変長サイズな関数オブジェクトを受け取って実行する
// キュークラス
//==================================================================
namespace Silex
{
    // std::invoke で呼び出し可能なオブジェクトのみ受け付ける制約
    template<class T>
    concept Callable = requires(T&& function)
    {
        std::invoke(function);
    };


    class TaskQueue
    {
    public:

        void Initialize(uint64 allocateByteSize = 1024 * 1024) // 1MB
        {
            buffer         = (byte*)Memory::Malloc(allocateByteSize);
            insertLocation = buffer;

            std::memset(buffer, 0, allocateByteSize);
        }

        void Release()
        {
            if (buffer)
            {
                Execute();

                Memory::Free(buffer);
                buffer         = nullptr;
                insertLocation = nullptr;
            }
        }

        //****************************************
        // 右辺値参照のみ受け取る
        //****************************************
        // auto f = []() {};
        // Enqueue("rambda", std::move(f)); // 〇
        // Enqueue("rambda",           f ); // ✕
        // Enqueue("rambda",       [](){}); // 〇

        template<typename Func>
        void Enqueue(const char* taskName, Func&& fn)
        {
            Memory::Construct<Task<Func>>(insertLocation, taskName, fn);

            insertLocation += sizeof(Task<Func>);
            taskCount++;
        }

        template<typename Func>
        void Enqueue(const char* taskName, Func& fn)
        {
            static_assert(sizeof(Func) == 0, "コピーを避けるために、右辺値が渡されることを期待します");
        }

        void Execute()
        {
            byte* ptr = buffer;

            for (uint32 i = 0; i < taskCount; i++)
            {
                ITask* task = reinterpret_cast<ITask*>(ptr);
                task->Execute();
                ptr += task->GetSize();

                Memory::Destruct(task);
            }

            insertLocation = buffer;
            taskCount      = 0;
        }

    private:

        //===========================================
        // 基底タスククラス
        //-------------------------------------------
        // 動的にテンプレートを判別する方法がないので、
        // 型消去で共通のインターフェース呼び出しを行う
        //===========================================
        struct ITask
        {
            virtual ~ITask() {}

            virtual void        Execute() const = 0;
            virtual uint32      GetSize() const = 0;
            virtual const char* GetName() const = 0;
        };

        template <Callable Functor>
        struct Task : ITask
        {
            Functor     func;
            uint32      size;
            const char* name;

            Task(const char* name, Functor fn)
                : func(Traits::Forward<Functor>(fn))
                , size(sizeof(Task))
                , name(name)
            {}

            void        Execute() const override { std::invoke(func); }
            uint32      GetSize() const override { return size;       }
            const char* GetName() const override { return name;       }
        };

    private:

        byte*  buffer         = nullptr;
        byte*  insertLocation = nullptr;
        uint32 taskCount      = 0;
    };
}
