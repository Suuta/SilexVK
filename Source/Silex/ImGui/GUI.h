
#pragma once

#include "Core/Ref.h"
#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingContext;
    class DescriptorSet;
    using GUICreateFunction = class GUI* (*)();


    class GUI : public Object
    {
        SL_CLASS(GUI, Object)

    public:

        GUI();
        ~GUI();

        // 生成
        static GUI* Create()
        {
            return createFunction();
        }

        // 生成用関数の登録
        static void ResisterCreateFunction(GUICreateFunction func)
        {
            createFunction = func;
        }

    public:

        virtual void Init(RenderingContext* context) = 0;
        virtual void BeginFrame()                    = 0;
        virtual void Render()                        = 0;
        virtual void EndFrame()                      = 0;

        void Update();
        void ViewportPresent();

    public:

        static void Image(DescriptorSet* set, float width, float height);
        static void ImageButton(DescriptorSetHandle* set, float width, float height, uint32 framePadding);

    private:

        static inline GUICreateFunction createFunction = nullptr;
    };
}
