
#pragma once

#include "Core/Core.h"
#include "Core/Ref.h"


namespace Silex
{
    class AssetCreator
    {
    public:

        template<class T>
        static Ref<T> Create(const std::filesystem::path& directory);
    };
}
