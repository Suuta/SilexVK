
#pragma once
#include "Core/Core.h"
#include "Core/Ref.h"


namespace Silex
{
    class AssetImporter
    {
    public:

        template<class T>
        static Ref<T> Import(const std::string& filePath);
    };
}
