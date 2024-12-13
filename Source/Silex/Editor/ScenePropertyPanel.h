
#pragma once
#include "Scene/Entity.h"


namespace Silex
{
    class ScenePropertyPanel
    {
    public:

        ScenePropertyPanel()  = default;
        ~ScenePropertyPanel() = default;

        // 描画
        void Render(bool* showOutlinerPannel, bool* showPropertyPannel);

        // シーンを設定
        void SetScene(const Ref<Scene>& scn) { scene = scn; selectEntity = {}; }

        // 選択エンティティ
        Entity GetSelectedEntity() const        { return selectEntity;   }
        void   SetSelectedEntity(Entity entity) { selectEntity = entity; }

        // アウトライナーのエンティティが選択された時のイベント
        SL_DECLARE_DELEGATE(OnEntitySelected, void, bool)
        OnEntitySelected onEntitySelectDelegate;

    private:

        template<typename T, typename Func>
        void DrawComponent(const std::string& name, Entity entity, Func drawFunc);

        template<typename T>
        void DisplayAddComponentPopup(const std::string& entryName);

        void DrawFloat3ComponentValue(glm::vec3& values, float columnWidth);

    private:

        void DrawHierarchy();
        void DrawComponents(Entity entity);

    private:

        Ref<Scene> scene;
        Entity     selectEntity;
    };
}
