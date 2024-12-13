
#pragma once

#include "Core/Window.h"
#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;
    using RenderingContextCreateFunction = RenderingContext* (*)(void*);


    class RenderingContext : public Object
    {
        SL_CLASS(RenderingContext, Object)

    public:

        // コンテキスト生成
        static RenderingContext* Create(void* platformHandle)
        {
            return createFunction(platformHandle);
        }

        // コンテキスト生成用関数の登録
        static void ResisterCreateFunction(RenderingContextCreateFunction func)
        {
            createFunction = func;
        }

    public:

        // 初期化
        virtual bool Initialize(bool enableValidation) = 0;

        // API実装
        virtual RenderingAPI* CreateRendringAPI() = 0;
        virtual void DestroyRendringAPI(RenderingAPI* api) = 0;

        // サーフェース
        virtual SurfaceHandle* CreateSurface() = 0;
        virtual void DestroySurface(SurfaceHandle* surface) = 0;

        // デバイス情報
        virtual const DeviceInfo& GetDeviceInfo() const = 0;

    private:

        // コンテキスト生成関数ポインタ
        static inline RenderingContextCreateFunction createFunction;
    };
}
