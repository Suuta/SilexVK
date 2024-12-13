
#pragma once
#include "Rendering/RenderingStructures.h"


namespace Silex
{
    class Environment : public Class
    {
        SL_CLASS(Environment, Class)

    public:

        Environment()  = default;
        ~Environment() = default;

        void Create(Texture2D* environment);
        void Destroy();

        TextureCube* GetEnvironmentMap() const;
        TextureCube* GetIrradianceMap()  const;
        TextureCube* GetPrefilterMap()   const;
        Texture2D*   GetBRDFMap()        const;

        TextureViewHandle* GetEnvironmentView() const;
        TextureViewHandle* GetIrradianceView()  const;
        TextureViewHandle* GetPrefilterView()   const;
        TextureViewHandle* GetBRDFView()        const;

    private:

        TextureCube* environmentMap = nullptr;
        TextureCube* irradianceMap  = nullptr;
        TextureCube* prefilterMap   = nullptr;
        Texture2D*   brdfMap        = nullptr;

        TextureViewHandle* environmentView = nullptr;
        TextureViewHandle* irradianceView  = nullptr;
        TextureViewHandle* prefilterView   = nullptr;
        TextureViewHandle* brdfView        = nullptr;
    };
}

