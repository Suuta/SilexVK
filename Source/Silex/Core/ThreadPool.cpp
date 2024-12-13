
#include "PCH.h"
#include "ThreadPool.h"


namespace Silex
{
    static uint32                   threadCount        = 0;
    static std::atomic<uint32>      workingThreadCount = 0;
    static std::mutex               taskMutex;
    static std::condition_variable  condition;
    static std::vector<std::thread> threads;
    static std::deque<Task>         taskQueue;
    static bool                     isStopping;

    thread_local uint32        threadID        = 0;
    static std::atomic<uint32> threadIDCounter = 0;

    //--------------------------------------------------------------------------------
    // std::condition_variable::wait()
    //--------------------------------------------------------------------------------
    // 条件が指定されているなら、満たされるまで(whileでビジーループ & 待機)
    // そうでないならなら、notify_one() / notify_all() が呼び出されるまで 待機する
    //--------------------------------------------------------------------------------

    static void ThreadLoop()
    {
        threadID = threadIDCounter++;

        while (true)
        {
            Task task;

            {
                std::unique_lock<std::mutex> lock(taskMutex);

                // タスクが空の場合は wait する
                while (!isStopping && taskQueue.empty())
                {
                    condition.wait(lock);
                }

                if (isStopping && taskQueue.empty())
                    return;

                task = taskQueue.front();
                taskQueue.pop_front();
            }

            // タスク実行
            workingThreadCount++;
            task();
            workingThreadCount--;
        }
    }

    void ThreadPool::Initialize()
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------

        isStopping  = false;
        threadCount = std::thread::hardware_concurrency() - 1;

        // スレッドループを予約
        for (uint32 i = 0; i < threadCount; i++)
        {
            threads.emplace_back(std::thread(&Silex::ThreadLoop));
        }
    }

    void ThreadPool::Finalize()
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------

        // 実行中のタスクを完了させる
        WaitAll();
        taskQueue.clear();

        {
            // 終了フラグを立てる
            std::unique_lock<std::mutex> lock(taskMutex);
            isStopping = true;
        }

        // 全ての wait スレッド起動
        condition.notify_all();

        // 全スレッドが終了するまで待機
        for (auto& thread : threads)
            thread.join();

        threads.clear();
    }

    void ThreadPool::AddTask(Task&& task)
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------

        {
            // キューにタスクを追加
            std::unique_lock<std::mutex> lock(taskMutex);
            taskQueue.emplace_back(std::bind(std::forward<Task>(task)));
        }

        // 待機中のスレッドを1つだけ起動させる
        condition.notify_one();
    }

    void ThreadPool::WaitAll()
    {
        //---------------------------
        // SL_ASSERT(IsMainThread());
        //---------------------------
        while (HasRunningTask())
        {
            OS::Get()->Sleep(16);
        }
    }




    uint32 ThreadPool::GetThreadCount()
    { 
        return threadCount;
    }

    uint32 ThreadPool::GetWorkingThreadCount()
    { 
        return workingThreadCount;
    }

    uint32 ThreadPool::GetIdleThreadCount()
    { 
        return threadCount - workingThreadCount;
    }

    bool ThreadPool::HasRunningTask()
    { 
        return GetIdleThreadCount() != GetThreadCount();
    }
}
