
#pragma once

#include "Scene/Scene.h"
#include "Scene/Components.h"

#include <entt/entt.hpp>


namespace Silex
{
    class Entity
    {
    public:

        Entity() {};

        Entity(entt::entity handle, Scene* scene)
            : entityHandle(handle)
            , scene(scene)
        {
        }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            SL_ASSERT(!HasComponent<T>());
            T& component = scene->registry.emplace<T>(entityHandle, Traits::Forward<Args>(args)...);
            return component;
        }

        template<typename T>
        void RemoveComponent()
        {
            SL_ASSERT(HasComponent<T>());
            scene->registry.remove<T>(entityHandle);
        }

        template<typename T>
        T& GetComponent()
        {
            SL_ASSERT(HasComponent<T>());
            return scene->registry.get<T>(entityHandle);
        }

        template<typename T>
        bool HasComponent()
        {
            return scene->registry.has<T>(entityHandle);
        }

        operator bool()         const { return entityHandle != entt::null; }
        operator entt::entity() const { return entityHandle;               }
        operator uint32()       const { return (uint32)entityHandle;       }

        uint64             GetID()   { return GetComponent<InstanceComponent>().id;   }
        const std::string& GetName() { return GetComponent<InstanceComponent>().name; }

        bool operator==(const Entity& other) const
        {
            return entityHandle == other.entityHandle && scene == other.scene;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

    private:

        entt::entity entityHandle = entt::null;
        Scene*       scene        = nullptr;

    private:

        friend class ScenePropertyPanel;
        friend class Scene;
    };
}