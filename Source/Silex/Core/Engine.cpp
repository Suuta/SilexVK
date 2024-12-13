
#include "PCH.h"

#include "Core/Engine.h"
#include "Core/ThreadPool.h"
#include "Asset/Asset.h"
#include "Editor/EditorSplashImage.h"
#include "Rendering/RenderingContext.h"
#include "Script/Script.h"


namespace Silex
{
    static Engine* engine = nullptr;


    bool LaunchEngine()
    {
        // OS初期化
        OS::Get()->Initialize();

        // コア機能初期化
        Logger::Initialize();
        Memory::Initialize();
        Input::Initialize();
        ThreadPool::Initialize();

        // スプラッシュイメージ表示
        EditorSplashImage::Show();

        // エンジン初期化
        engine = slnew(Engine);
        if (!engine->Initialize())
        {
            EditorSplashImage::Hide();
            return false;
        }

        // スプラッシュイメージ非表示
        EditorSplashImage::Hide();

        return true;
    }

    void ShutdownEngine()
    {
        if (engine)
        {
            engine->Finalize();
            sldelete(engine);
        }

        ThreadPool::Finalize();
        Input::Finalize();
        Memory::Finalize();
        Logger::Finalize();

        OS::Get()->Finalize();
    }

    //=========================================
    // Engine
    //=========================================
    Engine* Engine::Get()
    {
        return engine;
    }

    bool Engine::Initialize()
    {
        // ウィンドウ
        mainWindow = Window::Create(applicationName.c_str(), 1280, 720);
        bool result = mainWindow->Initialize();
        SL_CHECK(!result, false);

        // レンダリングコンテキスト生成
        context = RenderingContext::Create(mainWindow->GetPlatformHandle());
        result = context->Initialize(SL_RENDERER_VALIDATION);
        SL_CHECK(!result, false);

        // API抽象化レイヤー生成
        renderer = slnew(Renderer);
        result = renderer->Initialize(context);
        SL_CHECK(!result, false);

        // ウィンドウコンテキスト生成
        result = mainWindow->SetupWindowContext(context);
        SL_CHECK(!result, false);

        // コールバック登録
        mainWindow->BindWindowCloseEvent(this,  &Engine::OnWindowClose);
        mainWindow->BindWindowResizeEvent(this, &Engine::OnWindowResize);
        mainWindow->BindMouseMoveEvent(this,    &Engine::OnMouseMove);
        mainWindow->BindMouseScrollEvent(this,  &Engine::OnMouseScroll);

        // スクリプトマネージャー
        ScriptManager::Initialize();

        // アセットマネージャー
        AssetManager::Initialize();

        // エディターUI (ImGui)
        editorUI = GUI::Create();
        editorUI->Init(context);

        // エディター
        editor = slnew(Editor);
        editor->Initialize();

        // ウィンドウ表示
        mainWindow->Show();

        return true;
    }

    void Engine::Finalize()
    {
        editor->Finalize();
        sldelete(editor);
        sldelete(editorUI);

        AssetManager::Finalize();
        ScriptManager::Finalize();

        mainWindow->CleanupWindowContext(context);

        sldelete(renderer);
        sldelete(context);
        sldelete(mainWindow);
    }

    bool Engine::MainLoop()
    {
        CalcurateFrameTime();

        if (!minimized)
        {
            // prepare
            renderer->BeginFrame();
            editorUI->BeginFrame();

            // update
            editor->Update(deltaTime);
            editor->UpdateUI();
            editorUI->Update();

            // render
            editor->Render();
            editorUI->Render();

            // submit
            renderer->EndFrame();
            editorUI->EndFrame();

            // present
            renderer->Present();
            editorUI->ViewportPresent();

            Input::Flush();
        }

        PerformanceProfiler::Get().GetFrameData(&performanceData, true);

        // メインループ抜け出し確認
        return isRunning;
    }

    void Engine::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.width == 0 || e.height == 0)
        {
            minimized = true;
            return;
        }

        minimized = false;
    }

    void Engine::OnWindowClose(WindowCloseEvent& e)
    {
        isRunning = false;
    }

    void Engine::OnMouseMove(MouseMoveEvent& e)
    {
        editor->OnMouseMove(e);
    }

    void Engine::OnMouseScroll(MouseScrollEvent& e)
    {
        editor->OnMouseScroll(e);
    }

    void Engine::CalcurateFrameTime()
    {
        uint64 time = OS::Get()->GetTickSeconds();
        deltaTime     = (double)(time - lastFrameTime) / 1'000'000;
        lastFrameTime = time;

        static float  secondLeft = 0.0f;
        static uint32 frame = 0;
        secondLeft += deltaTime;
        frame++;

        if (secondLeft >= 1.0f)
        {
            frameRate  = frame;
            frame      = 0;
            secondLeft = 0.0f;
        }
    }

    // OnWindowCloseイベント（Xボタン / Alt + F4）以外で、エンジンループを終了させる場合に使用
    void Engine::Close()
    {
        isRunning = false;
    }
}
