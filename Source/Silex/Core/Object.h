#pragma once

#include "Core/OS.h"
#include "Core/Logger.h"
#include "Core/TypeInfo.h"
#include <atomic>


namespace Silex
{
    //===========================================
    // Class クラスを継承した全てのデータ型の情報を保持
    //===========================================
    class GlobalClassDataBase
    {
    public:

        template<typename T>
        static uint64 Register(const char* className)
        {
            SL_ASSERT(!classInfoMap.contains(className));

            auto type = TypeInfo::Query<T>();
            classInfoMap.emplace(className, type);

            return type.hashID;
        }

        static void DumpClassInfoList()
        {
            for (const auto& [name, info] : classInfoMap)
            {
                SL_LOG_DEBUG("{:32}: {:4}, {}", name, info.typeSize, info.hashID);
            }
        }

    private:

        static inline std::unordered_map<std::string, TypeInfo> classInfoMap;
    };


    //========================================
    // 参照カウントオブジェクト
    //========================================
    class Object : public Class
    {
        SL_CLASS(Object, Class)

    public:

        Object()              {};
        Object(const Object&) {};

    public:

        uint32 GetRefCount() const { return refCount; }

    private:

        void IncRefCount() const { ++refCount; }
        void DecRefCount() const { --refCount; }

        mutable std::atomic<uint32> refCount = 0;

    private:
        
        // 参照カウント操作は参照カウントポインタからのみ操作可能にする
        template<class T>
        friend class Ref;
    };

    //============================================
    // 抽象化汎用ハンドル
    //============================================
    class Handle : public Class
    {
        SL_CLASS(Handle, Class)

        Handle()          { pointer = uint64(this); }
        Handle(void* ptr) { pointer = uint64(ptr);  }

    protected:

        uint64 pointer = 0;
    };
}
