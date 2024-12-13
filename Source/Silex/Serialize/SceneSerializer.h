
#pragma once

#include "Scene/Scene.h"


namespace Silex
{
    class SceneSerializer
    {
    public:

        SceneSerializer(Scene* scene);

        void Serialize(const std::string& filepath);
        void Deserialize(const std::string& filepath);

    private:

        Scene* scene;
    };
}
