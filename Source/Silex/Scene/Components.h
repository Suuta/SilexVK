
#pragma once

#include "Asset/Asset.h"
#include "Rendering/Mesh.h"
#include "Rendering/Environment.h"
#include "Rendering/Material.h"
#include <glm/gtx/quaternion.hpp>


namespace Silex
{
    struct InstanceComponent : public Class
    {
        SL_CLASS(InstanceComponent, Class)

        uint64      id;
        bool        active;
        std::string name;
    };

    struct TransformComponent : public Class
    {
        SL_CLASS(TransformComponent, Class)

        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale    = { 1.0f, 1.0f, 1.0f };

        glm::mat4 GetTransform() const
        {
            glm::mat4 r = glm::toMat4(glm::quat(rotation));
            glm::mat4 s = glm::scale(glm::mat4(1.0f), Scale);
            glm::mat4 t = glm::translate(glm::mat4(1.0f), position);

            return t * r * s;
        }
    };

    struct ScriptComponent : public Class
    {
        SL_CLASS(ScriptComponent, Class)

        std::string className;
    };

    struct MeshComponent : public Class
    {
        SL_CLASS(MeshComponent, Class)

        Ref<MeshAsset>                  mesh       = nullptr;
        std::vector<Ref<MaterialAsset>> materials  = {};
        bool                            castShadow = true;
    };

    struct DirectionalLightComponent : public Class
    {
        SL_CLASS(DirectionalLightComponent, Class)

        bool      enableSoftShadow = false;
        bool      showCascade      = false;
        glm::vec3 color            = { 1.0f, 1.0f, 1.0f };
        glm::vec3 direction        = { 0.0f, 0.0f, 0.0f };
        float     intencity        = 1.0f;
        float     shadowDepthBias  = 0.001f;
    };

    struct SkyLightComponent : public Class
    {
        SL_CLASS(SkyLightComponent, Class)

        bool  renderSky = true;
        bool  enableIBL = true;
        float intencity = 1.0f;

        Ref<EnvironmentAsset> sky;
    };

    struct PostProcessComponent : public Class
    {
        SL_CLASS(PostProcessComponent, Class)

        // Outline
        bool      enableOutline  = false;
        float     lineWidth      = 2.0f;
        glm::vec3 outlineColor   = glm::vec3(0.0, 0.0, 0.0);

        // FXAA
        bool enableFXAA = false;

        // Bloom
        bool  enableBloom    = false;
        float bloomThreshold = 2.0f;
        float bloomIntencity = 0.1f; // 0.0 ~ 1.0

        // 色縮差
        bool enableChromaticAberration = false;

        // トーンマッピング
        bool  enableTonemap    = true;
        float exposure        = 1.0f;
        float gammaCorrection = 2.2f;
    };
}
