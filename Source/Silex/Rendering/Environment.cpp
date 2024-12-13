
#include "PCH.h"
#include "Rendering/Environment.h"
#include "Rendering/Renderer.h"


namespace Silex
{
    struct EnvironmentMapGenerator
    {
        EnvironmentMapGenerator(Texture2D* environmentTexture);
        ~EnvironmentMapGenerator();

        TextureCube* CreateEnvironment();
        TextureCube* CreatePrefilter();
        TextureCube* CreateIrradiance();
        Texture2D*   CreateBRFD();


        RenderingAPI* api = nullptr;


        FramebufferHandle* IBLProcessFB       = nullptr;
        RenderPassHandle*  IBLProcessPass     = nullptr;
        UniformBuffer*     equirectangularUBO = nullptr;

        // キューブマップ変換
        PipelineHandle*    equirectangularPipeline = nullptr;
        ShaderHandle*      equirectangularShader   = nullptr;
        TextureCube*       cubemapTexture          = nullptr;
        TextureViewHandle* cubemapTextureView      = nullptr;
        DescriptorSet*     equirectangularSet      = nullptr;

        // irradiance
        PipelineHandle*    irradiancePipeline    = nullptr;
        ShaderHandle*      irradianceShader      = nullptr;
        TextureCube*       irradianceTexture     = nullptr;
        TextureViewHandle* irradianceTextureView = nullptr;
        DescriptorSet*     irradianceSet         = nullptr;

        // prefilter
        PipelineHandle*    prefilterPipeline    = nullptr;
        ShaderHandle*      prefilterShader      = nullptr;
        TextureCube*       prefilterTexture     = nullptr;
        TextureViewHandle* prefilterTextureView = nullptr;
        DescriptorSet*     prefilterSet         = nullptr;

        // BRDF-LUT
        PipelineHandle*    brdflutPipeline    = nullptr;
        ShaderHandle*      brdflutShader      = nullptr;
        Texture2D*         brdflutTexture     = nullptr;
        TextureViewHandle* brdflutTextureView = nullptr;
    };














    void Environment::Create(Texture2D* environment)
    {
        EnvironmentMapGenerator generator(environment);
        environmentMap = generator.CreateEnvironment();
        irradianceMap  = generator.CreateIrradiance();
        prefilterMap   = generator.CreatePrefilter();
        brdfMap        = generator.CreateBRFD();
    }

    void Environment::Destroy()
    {
        Renderer::Get()->DestroyTexture(environmentMap);
        Renderer::Get()->DestroyTexture(irradianceMap);
        Renderer::Get()->DestroyTexture(prefilterMap);
        Renderer::Get()->DestroyTexture(brdfMap);
    }

    TextureCube* Environment::GetEnvironmentMap() const
    {
        return environmentMap;
    }

    TextureCube* Environment::GetIrradianceMap() const
    {
        return irradianceMap;
    }

    TextureCube* Environment::GetPrefilterMap() const
    {
        return prefilterMap;
    }

    Texture2D* Environment::GetBRDFMap() const
    {
        return brdfMap;
    }

    TextureViewHandle* Environment::GetEnvironmentView() const
    {
        return environmentView;
    }

    TextureViewHandle* Environment::GetIrradianceView() const
    {
        return irradianceView;
    }

    TextureViewHandle* Environment::GetPrefilterView() const
    {
        return prefilterView;
    }

    TextureViewHandle* Environment::GetBRDFView() const
    {
        return brdfView;
    }





    EnvironmentMapGenerator::EnvironmentMapGenerator(Texture2D* environmentTexture)
    {
    }

    EnvironmentMapGenerator::~EnvironmentMapGenerator()
    {
    }

    TextureCube* EnvironmentMapGenerator::CreateEnvironment()
    {
        return nullptr;
    }

    TextureCube* EnvironmentMapGenerator::CreatePrefilter()
    {
        return nullptr;
    }

    TextureCube* EnvironmentMapGenerator::CreateIrradiance()
    {
        return nullptr;
    }

    Texture2D* EnvironmentMapGenerator::CreateBRFD()
    {
        return nullptr;
    }
}

