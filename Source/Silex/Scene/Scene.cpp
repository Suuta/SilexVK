
#include "PCH.h"

#include "Core/Random.h"
#include "Core/Timer.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/SceneRenderer.h"

#include <glm/glm.hpp>


namespace Silex
{
    Entity Scene::CreateEntity(const std::string& name, bool active)
    {
        return CreateEntity(Random<uint64>::Rand(), name, active);
    }

    Entity Scene::CreateEntity(uint64 id, const std::string& name, bool active)
    {
        Entity entity = { registry.create(), this };
        entity.AddComponent<InstanceComponent>();
        entity.AddComponent<TransformComponent>();

        auto& c  = entity.GetComponent<InstanceComponent>();
        c.name   = name.empty()? "Empty" : name;
        c.id     = id;
        c.active = active;

        entityMap[id] = entity;

        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        entityMap.erase(entity.GetID());
        registry.destroy(entity);
    }

    Entity Scene::FindEntity(const std::string& name)
    {
        const auto& view = registry.view<InstanceComponent>();
        for (auto entity : view)
        {
            const InstanceComponent& nc = view.get<InstanceComponent>(entity);
            if (nc.name == name)
            {
                return { entity, this };
            }
        }

        return {};
    }

    Entity Scene::FindEntity(uint64 id)
    {
        if (entityMap.contains(id))
        {
            return { entityMap.at(id), this };
        }

        return {};
    }

    void Scene::Update(float deltaTime, Camera* camera, SceneRenderer* renderer)
    {
        // 描画データリセット
        renderer->Reset(this, camera);

        {
            SL_SCOPE_PROFILE("Scene::Update");

            const auto& sky         = registry.group<SkyLightComponent>(entt::get<TransformComponent, InstanceComponent>);
            const auto& directional = registry.group<DirectionalLightComponent>(entt::get<TransformComponent, InstanceComponent>);
            const auto& meshes      = registry.group<MeshComponent>(entt::get<TransformComponent, InstanceComponent>);
            const auto& postProcess = registry.group<PostProcessComponent>(entt::get<TransformComponent, InstanceComponent>);

            // スカイライト
            for (auto entity : sky)
            {
                auto [sc, ic] = sky.get<SkyLightComponent, InstanceComponent>(entity);
                if (ic.active)
                {
                    renderer->SetSkyLight(sc);
                }

                // 1つしか存在しないから、Unity/Environment ように別で管理する方が良いか？
                break;
            }

            // ポストプロセス
            for (entt::entity entity : postProcess)
            {
                auto [pc, ic] = postProcess.get<PostProcessComponent, InstanceComponent>(entity);
                if (ic.active)
                {
                    renderer->SetPostProcess(pc);
                }

                // 現状1つのみ受け付ける
                break;
            }

            // ディレクショナルライト
            for (auto entity : directional)
            {
                // シーンレンダラーの平行光源リストへ追加（現状 1個のみ）
                auto [tc, dc, ic] = directional.get<TransformComponent, DirectionalLightComponent, InstanceComponent>(entity);
                if (ic.active)
                {
                    // (0, 0, 1)ベクトル を基準（0°）として回転させた値を適応
                    dc.direction = -glm::mat3(tc.GetTransform()) * glm::vec3(0.0f, 0.0f, 1.0f);
                    renderer->SetDirectionalLight(dc);
                }

                // 現状1つのみ受け付ける
                break;
            }

            // メッシュ
            for (entt::entity entity : meshes)
            {
                auto [tc, mc, ic] = meshes.get<TransformComponent, MeshComponent, InstanceComponent>(entity);

                if (ic.active)
                {
                    MeshDrawData data;
                    data.mesh      = mc;
                    data.transform = tc.GetTransform();
                    data.entityID  = (int32)entity;

                    // シーンレンダラーの描画リストへ追加
                    renderer->AddMeshDrawList(data);
                }
            }
        }
    }
}
