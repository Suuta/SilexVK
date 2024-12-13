
#pragma once

#include "Core/Ref.h"
#include "Scene/Camera.h"
#include "Scene/Components.h"
#include <entt/entt.hpp>



namespace Silex
{
    class SceneRenderer;
    class Entity;

    struct MeshDrawData
    {
        MeshComponent mesh;
        int32         entityID;
        glm::mat4     transform;
    };

    class Scene : public Object
    {
        SL_CLASS(Scene, Object)

    public:

        Scene()  = default;
        ~Scene() = default;

        Entity CreateEntity(const std::string& name = std::string(), bool active = true);
        Entity CreateEntity(uint64 id, const std::string& name = std::string(), bool active = true);
        void   DestroyEntity(Entity entity);

        Entity FindEntity(const std::string& name);
        Entity FindEntity(uint64 id);

        void Update(float deltaTime, Camera* camera, SceneRenderer* renderer);

    private:

        entt::registry                           registry;
        std::unordered_map<uint64, entt::entity> entityMap;

    private:

        friend class Entity;
        friend class ScenePropertyPanel;
        friend class SceneSerializer;
    };
}
