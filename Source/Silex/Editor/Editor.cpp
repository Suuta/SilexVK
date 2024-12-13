
#include "PCH.h"

#include "Editor/Editor.h"
#include "Editor/ConsoleLogger.h"
#include "Editor/EditorSplashImage.h"

#include "Core/Timer.h"
#include "Core/Random.h"
#include "Core/Engine.h"
#include "Rendering/Renderer.h"
#include "Serialize/SceneSerializer.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>
#include <glm/gtx/matrix_decompose.hpp>


namespace Silex
{
    void Editor::Initialize()
    {
        SL_LOG_TRACE("Editor::Init");

        // シーン生成
        scene = CreateRef<Scene>();
        
        // アウトラウナー初期化
        scenePropertyPanel.SetScene(scene);
        scenePropertyPanel.onEntitySelectDelegate.Bind(this, &Editor::OnClickHierarchyEntity);

        // アセットブラウザ初期化
        assetBrowserPanel.Initialize();

        // シーンレンダラー初期化
        sceneRenderer = slnew(SceneRenderer);
        sceneRenderer->Initialize();

        INIT_PROCESS("Editor Init", 100.0f);
        OS::Get()->Sleep(500);
    }

    void Editor::Finalize()
    {
        SL_LOG_TRACE("Editor::Finalize");

        sceneRenderer->Finalize();
        sldelete(sceneRenderer);

        assetBrowserPanel.Finalize();
        scenePropertyPanel.onEntitySelectDelegate.Unbind();
    }

    void Editor::Update(float deltaTime)
    {
        HandleInput(deltaTime);
        editorCamera.Update(deltaTime);

        scene->Update(deltaTime, &editorCamera, sceneRenderer);
    }

    void Editor::Render()
    {
        sceneRenderer->Render();
    }

    void Editor::UpdateUI()
    {
        SL_SCOPE_PROFILE("Editor::UpdateUI");

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGuiWindowFlags windowFlags = 0;
        windowFlags |= ImGuiWindowFlags_NoDocking;
        windowFlags |= ImGuiWindowFlags_NoTitleBar;
        windowFlags |= ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoResize;
        windowFlags |= ImGuiWindowFlags_NoCollapse;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        windowFlags |= ImGuiWindowFlags_NoNavFocus;
        windowFlags |= ImGuiWindowFlags_MenuBar;
        windowFlags |= usingEditorCamera ? ImGuiWindowFlags_NoInputs : 0;

        ImGuiWindowFlags usingCameraFlag = usingEditorCamera ? ImGuiWindowFlags_NoInputs : 0;

        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace", nullptr, windowFlags);
            ImGui::PopStyleVar();

            ImGuiWindowFlags docapaceFlag = 0;
            docapaceFlag |= ImGuiDockNodeFlags_NoWindowMenuButton;
            docapaceFlag |= usingEditorCamera ? ImGuiDockNodeFlags_NoResize : 0;

            ImGuiID dockspace_id = ImGui::GetID("Dockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), docapaceFlag);

            //============================
            // メニューバー
            //============================
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("編集"))
                {
                    if (ImGui::MenuItem("シーンを開く", "Ctr+O ")) OpenScene();
                    if (ImGui::MenuItem("シーンを保存", "Ctr+S ")) SaveScene();
                    if (ImGui::MenuItem("終了",       "Alt+F4")) Engine::Get()->Close();

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("ウィンドウ"))
                {
                    ImGui::MenuItem("シーンビューポート", nullptr, &showScene);
                    ImGui::MenuItem("アウトプットロガー", nullptr, &showLogger);
                    ImGui::MenuItem("統計",            nullptr, &showStats);
                    ImGui::MenuItem("アウトライナー",    nullptr, &showOutliner);
                    ImGui::MenuItem("プロパティ",       nullptr, &showProperty);
                    ImGui::MenuItem("マテリアル",       nullptr, &showMaterial);
                    ImGui::MenuItem("アセットブラウザ",  nullptr, &showAssetBrowser);
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        // シーンプロパティ
        scenePropertyPanel.Render(&showOutliner, &showProperty);

        // アセットブラウザ
        assetBrowserPanel.Render(&showAssetBrowser, &showMaterial);

        if (showScene)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("シーン", &showScene, usingCameraFlag);

            // ビューポートへのホバー
            hoveredViewport = ImGui::IsWindowHovered();

            // ウィンドウ内のコンテンツサイズ（タブやパディングは含めないので、ImGui::GetWindowSize関数は使わない）
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
            ImVec2 content    = { contentMax.x - contentMin.x, contentMax.y - contentMin.y };

            // ウィンドウ左端からのオフセット
            ImVec2 viewportOffset = ImGui::GetWindowPos();

            relativeViewportRect[0] = { contentMin.x + viewportOffset.x, contentMin.y + viewportOffset.y };
            relativeViewportRect[1] = { contentMax.x + viewportOffset.x, contentMax.y + viewportOffset.y };

            // シーン描画
            {
                // エディターが有効な状態では、シーンビューポートサイズがフレームバッファサイズになる
                if ((content.x > 0.0f && content.y > 0.0f) && (content.x != sceneViewportFramebufferSize.x || content.y != sceneViewportFramebufferSize.y))
                {
                    sceneViewportFramebufferSize.x = content.x;
                    sceneViewportFramebufferSize.y = content.y;

                    editorCamera.SetViewportSize(sceneViewportFramebufferSize.x, sceneViewportFramebufferSize.y);
                    sceneRenderer->ResizeFramebuffer(sceneViewportFramebufferSize.x, sceneViewportFramebufferSize.y);
                }

                // シーン描画
                GUI::Image(sceneRenderer->GetSceneFinalOutput(), content.x, content.y);
            }

            // ギズモ
            if (activeGizmoForcus)
            {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(relativeViewportRect[0].x, relativeViewportRect[0].y, content.x, content.y);

                const float* viewMatrix       = glm::value_ptr(editorCamera.GetViewMatrix());
                const float* projectionMatrix = glm::value_ptr(editorCamera.GetProjectionMatrix());

                // ギズモ描画位置
                Entity selectEntity = scenePropertyPanel.GetSelectedEntity();

                //if (selectEntity)
                if (selectEntity && false)
                {
                    TransformComponent& tc = selectEntity.GetComponent<TransformComponent>();
                    glm::mat4 transform = tc.GetTransform();

                    selectEntityPosition = tc.position;

                    // ギズモ描画
                    ImGuizmo::Manipulate(viewMatrix, projectionMatrix, manipulateType, manipulateMode, glm::value_ptr(transform));

                    // ギズモ操作（変化量加算）
                    if (ImGuizmo::IsUsing())
                    {
                        glm::vec3 skew;
                        glm::vec4 perspective;
                        glm::vec3 translation, scale, rotation;
                        glm::quat quatRotation;

                        glm::decompose(transform, scale, quatRotation, translation, skew, perspective);
                        rotation = glm::eulerAngles(quatRotation);

                        // TODO: 角度制限する
                        // フレームごとの回転を加算する
                        glm::vec3 dtRot = rotation - tc.rotation;
                        tc.position =  translation;
                        tc.rotation += dtRot;
                        tc.Scale    =  scale;

                        usingManipulater = true;
                    }
                    else
                    {
                        usingManipulater = false;
                    }
                }
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }

        // エディターカメラ操作中に他のウィンドウへのフォーカスを無効にする
        if (usingEditorCamera) ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);

        // アウトプットログ
        if (showLogger)
        {
            ImGui::Begin("アウトプットログ", &showLogger, usingCameraFlag);

            // Clear ボタンを右寄せするための計算
            float buttonWidth = 100.0f;
            float spacingFromRightEdge = ImGui::GetWindowContentRegionMax().x - buttonWidth - 20;

            if (spacingFromRightEdge > 0)
                ImGui::Dummy(ImVec2(spacingFromRightEdge, 0.0f));  // 空のスペースを追加してボタンを右に移動

            ImGui::SameLine();

            if (ImGui::Button("Clear", ImVec2(buttonWidth, 25.0f)))
                ConsoleLogger::Get().Clear();

            ImGui::Separator();

            ConsoleLogger::Get().LogData();

            ImGui::End();
        }

        // 統計
        if (showStats)
        {
            ImGui::Begin("統計", &showStats, usingCameraFlag);
            ImGui::Text("FPS: %d (%.2f)ms", Engine::Get()->GetFrameRate(), Engine::Get()->GetDeltaTime() * 1000);
            ImGui::Text("Resolution: %d, %d", sceneViewportFramebufferSize.x, sceneViewportFramebufferSize.y);

            ImGui::Text("Camera: %.0f, %.0f, %.0f", editorCamera.GetPosition().x, editorCamera.GetPosition().y, editorCamera.GetPosition().z);

            ImGui::RadioButton("hoveredViewport", hoveredViewport);

            //ImGui::SeparatorText("");

            //SceneRenderStats stats = sceneRenderer.GetRenderStats();
            //ImGui::Text("GeometryDrawCall: %d", stats.numGeometryDrawCall);
            //ImGui::Text("ShadowDrawCall:   %d", stats.numShadowDrawCall);
            //ImGui::Text("NumMesh:          %d", stats.numRenderMesh);

            ImGui::SeparatorText("");

            for (const auto& [profile, time] : Engine::Get()->GetPerformanceData())
            {
                ImGui::Text("%-*s %.2f ms", 24, profile, time);
            }

            // メモリー使用量
            //ImGui::SeparatorText("");
            //auto status = PoolAllocator::GetStatus();
            //for (uint32 i = 0; i < status.size(); i++)
            //{
            //    ImGui::Text("メモリプール[%4d byte]:%6d / %6d Block", 32 << i, status[i].TotalAllocated / (32 << i), status[i].TotalSize / (32 << i));
            //}

            ImGui::End();
        }

        // ウィンドウへのフォーカスを有効にする
        if (usingEditorCamera) ImGui::PopItemFlag();

        // pop ImGuiStyleVar_WindowRounding
        // pop ImGuiStyleVar_WindowBorderSize
        ImGui::PopStyleVar(2);
    }

    void Editor::HandleInput(float deltaTime)
    {
        if (Input::IsMouseButtonPressed(Mouse::Right) && hoveredViewport)
        {
            Input::SetCursorMode(CursorMode::Disable);
            usingEditorCamera = true;
        }
        else if(Input::IsMouseButtonReleased(Mouse::Right))
        {
            Input::SetCursorMode(CursorMode::Normal);
            usingEditorCamera = false;
        }
        else
        {
            // エディターカメラ操作
            if (Input::IsMouseButtonDown(Mouse::Right) && usingEditorCamera)
            {
                if (Input::IsKeyDown(Keys::W)) editorCamera.Move(CameraMovementDir::Forward,  deltaTime);
                if (Input::IsKeyDown(Keys::S)) editorCamera.Move(CameraMovementDir::Backward, deltaTime);
                if (Input::IsKeyDown(Keys::A)) editorCamera.Move(CameraMovementDir::Left,     deltaTime);
                if (Input::IsKeyDown(Keys::D)) editorCamera.Move(CameraMovementDir::Right,    deltaTime);
                if (Input::IsKeyDown(Keys::E)) editorCamera.Move(CameraMovementDir::Up,       deltaTime);
                if (Input::IsKeyDown(Keys::Q)) editorCamera.Move(CameraMovementDir::Down,     deltaTime);
            }

            // オブジェクト選択
            if (Input::IsMouseButtonReleased(Mouse::Left) && !usingManipulater && hoveredViewport)
            {
                // SelectViewportEntity();
            }

            if (Input::IsKeyDown(Keys::LeftControl))
            {
                if (Input::IsKeyPressed(Keys::S))
                {
                    SaveScene();
                }
                else if (Input::IsKeyPressed(Keys::O))
                {
                    OpenScene();
                }
                else if (Input::IsKeyPressed(Keys::N))
                {
                    NewScene();
                }
            }

            // ギズモ操作
            if (Input::IsKeyPressed(Keys::W))
            {
                activeGizmoForcus = true;
                manipulateType   = ImGuizmo::TRANSLATE;
            }
            else if (Input::IsKeyPressed(Keys::E))
            {
                activeGizmoForcus = true;
                manipulateType   = ImGuizmo::ROTATE;
            }
            else if (Input::IsKeyPressed(Keys::R))
            {
                activeGizmoForcus = true;
                manipulateType   = ImGuizmo::SCALE;
            }
            else if (Input::IsKeyPressed(Keys::Q))
            {
                manipulateMode = manipulateMode? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }
            else if (Input::IsKeyPressed(Keys::F) && activeGizmoForcus)
            {
                editorCamera.SetPosition(selectEntityPosition - editorCamera.GetFront() * 5.0f);
            }
        }
    }

    Camera* Editor::GetEditorCamera()
    {
        return &editorCamera;
    }

    bool Editor::IsUsingEditorCamera() const
    {
        return usingEditorCamera;
    }

    void Editor::SelectViewportEntity()
    {
        if (!hoveredViewport)
            return;

        // ビューポートの相対座標を取得
        glm::ivec2 mouse       = Input::GetCursorPosition();
        glm::ivec2 viewportPos = { relativeViewportRect[0].x, relativeViewportRect[0].y };
        glm::ivec2 windowPos   = Window::Get()->GetWindowPos();
        glm::ivec2 diff        = viewportPos - windowPos;
        glm::ivec2 mousediff   = mouse - diff;

        // ピクセルID 読み取り
        if (mousediff.x >= 0 && mousediff.y >= 0)
        {
            selectionID = sceneRenderer->ReadEntityIDFromPixel(mousediff.x, mousediff.y);
            SL_LOG_DEBUG("Clicked: EntityID({}) : (x, y) = {}, {}", selectionID, mousediff.x, mousediff.y);
        }

        // 選択したエンティティをシーンヒエラルキーのアクティブエンティティに設定し、ギズモを表示
        if (selectionID >= 0)
        {
            activeGizmoForcus = true;
            scenePropertyPanel.SetSelectedEntity({ (entt::entity)selectionID, scene.Get() });
        }
        else
        {
            activeGizmoForcus = false;
            scenePropertyPanel.SetSelectedEntity({});
        }
    }

    void Editor::OpenScene()
    {
        std::string filePath = OS::Get()->OpenFile("Silex Scene (*.slsc)\0*.slsc\0");
        if (!filePath.empty())
        {
            OpenScene(filePath);
        }
    }

    void Editor::OpenScene(const std::string& filePath)
    {
        Ref<Scene> newScene = CreateRef<Scene>();

        SceneSerializer serializer(newScene.Get());
        serializer.Deserialize(filePath);

        scene = newScene;
        sceneRenderer->ResizeFramebuffer(sceneViewportFramebufferSize.x, sceneViewportFramebufferSize.y);

        scenePropertyPanel.SetScene(scene);
        currentScenePath = filePath;
        currentSceneName = currentScenePath.stem().string();

        Window::Get()->SetTitle(("Silex - " + currentSceneName).c_str());
    }

    void Editor::SaveScene(bool bForceSaveAs)
    {
        // 名前を付けて保存
        if (bForceSaveAs)
        {
            SaveSceneAs();
        }
        else
        {
            // 現在のシーンが名称未指定の場合は、新たにシーンファイルを生成して保存
            if (currentScenePath.empty())
            {
                SaveSceneAs();
            }
            else
            {
                // シーンファイルが生成済みなら、そこに上書き
                SceneSerializer serializer(scene.Get());
                serializer.Serialize(currentScenePath.string());
            }
        }
    }

    void Editor::SaveSceneAs()
    {
        std::string filePath = OS::Get()->SaveFile("Silex Scene (*.slsc)\0*.slsc\0", "slsc");

        SceneSerializer serializer(scene.Get());
        serializer.Serialize(filePath);

        currentScenePath = filePath;
        currentSceneName = currentScenePath.stem().string();

        Window::Get()->SetTitle(("Silex - " + currentSceneName).c_str());
    }

    void Editor::NewScene()
    {
        Ref<Scene> newScene = CreateRef<Scene>();

        scene = newScene;
        sceneRenderer->ResizeFramebuffer(sceneViewportFramebufferSize.x, sceneViewportFramebufferSize.y);

        scenePropertyPanel.SetScene(scene);
        currentScenePath = "";
        currentSceneName = "名称未指定";

        Window::Get()->SetTitle(("Silex - " + currentSceneName).c_str());
    }

    // シーンアウトライナーでエンティティがクリックされた時のイベント
    void Editor::OnClickHierarchyEntity(bool selected)
    {
        activeGizmoForcus = selected;
    }

    void Editor::OnMouseMove(MouseMoveEvent& e)
    {
        // スクリーン座標のため、Y座標は下向きが正の値となる
        float xoffset = e.mouseX - prevCursorPosition.x;
        float yoffset = prevCursorPosition.y - e.mouseY;

        prevCursorPosition = { e.mouseX, e.mouseY };

        if (usingEditorCamera)
        {
            editorCamera.ProcessMouseMovement(xoffset, yoffset);
        }
    }

    void Editor::OnMouseScroll(MouseScrollEvent& e)
    {
        if (hoveredViewport)
        {
            editorCamera.ProcessMouseScroll(e.offsetY);
        }
    }
}
