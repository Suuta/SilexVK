
#pragma once

#include "Core/Core.h"
#include "Core/Ref.h"
#include "Rendering/Mesh.h"
#include "Scene/Camera.h"
#include "Scene/SceneRenderer.h"
#include "Editor/ScenePropertyPanel.h"
#include "Editor/AssetBrowserPanel.h"

#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>


struct GLFWwindow;

namespace Silex
{
    class Editor : public Object
    {
        SL_CLASS(Editor, Object)

    public:

        void Initialize();
        void Finalize();
        void Update(float deltaTime);
        void UpdateUI();
        void Render();

    public:

        void OpenScene();                          // シーン開く
        void SaveScene(bool bForceSaveAs = false); // シーン保存
        void NewScene();

        // ウィンドウイベント
        void OnMouseMove(MouseMoveEvent& e);
        void OnMouseScroll(MouseScrollEvent& e);

    public:

        const std::filesystem::path& GetAssetDirectory() const { return assetDirectory; }
        Camera* GetEditorCamera();

        bool IsUsingEditorCamera() const;

    private:

        // ヒエラルキー選択イベント
        void OnClickHierarchyEntity(bool selected);

        void OpenScene(const std::string& filePath);
        void SaveSceneAs();

        void SelectViewportEntity();
        void HandleInput(float deltaTime);

    private:

        // パス
        std::filesystem::path currentScenePath = "";
        std::string           currentSceneName = "名称未指定";

        // シーンビューポート
        glm::ivec2 sceneViewportFramebufferSize = { 1280, 720 };
        glm::ivec2 relativeViewportRect[2];
        glm::ivec2 prevCursorPosition;

        // カメラ
        bool   usingEditorCamera  = false;
        bool   canUseEditorCamera = false;
        Camera editorCamera       = { glm::vec3(0.0f, 1.0f, 10.0f) };

        // シーン
        Ref<Scene>     scene;
        SceneRenderer* sceneRenderer;

        // パネル
        ScenePropertyPanel scenePropertyPanel;
        AssetBrowserPanel  assetBrowserPanel;

        std::filesystem::path assetDirectory = "Assets/";

        // オブジェクト選択・ギズモ
        bool                usingManipulater     = false;
        bool                hoveredViewport      = false;
        bool                activeGizmoForcus    = true;
        int32               selectionID          = -1;
        ImGuizmo::OPERATION manipulateType       = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE      manipulateMode       = ImGuizmo::LOCAL;
        glm::vec3           selectEntityPosition = {};

        // パネルの表示・非表示フラグ
        bool showScene        = true;
        bool showProperty     = true;
        bool showOutliner     = true;
        bool showLogger       = true;
        bool showStats        = true;
        bool showMaterial     = true;
        bool showAssetBrowser = true;
    };
}
