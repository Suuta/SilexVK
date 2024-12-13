
#pragma once

#include "Asset/Asset.h"
#include "Rendering/RenderingStructures.h"


namespace Silex
{
    enum ShadingModelType
    {
        Lit   = 0,
        Unlit = 1,
    };

    class Material : public Class
    {
        SL_CLASS(Material, Class)

    public:

        Ref<Texture2DAsset> albedoMap     = nullptr;
        glm::vec3           albedo        = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3           emission      = glm::vec3(0.0f, 0.0f, 0.0f);
        float               roughness     = 1.0f;
        float               metallic      = 1.0f;
        glm::vec2           textureTiling = glm::vec2(1.0f, 1.0f);
        ShadingModelType    shadingModel  = Lit;

    public:

        DescriptorSet* descriptorset = nullptr;
    };
}
