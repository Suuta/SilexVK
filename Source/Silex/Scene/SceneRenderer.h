
#pragma once
#include "Core/CoreType.h"
#include "Scene/Scene.h"
#include "Rendering/RenderingAPI.h"


namespace Silex
{
    struct SceneRenderStats
    {
        uint32 numRenderMesh       = 0;
        uint64 numGeometryDrawCall = 0;
        uint64 numShadowDrawCall   = 0;
    };

    struct GBufferData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        Texture2D* albedo   = nullptr;
        Texture2D* normal   = nullptr;
        Texture2D* emission = nullptr;
        Texture2D* id       = nullptr;
        Texture2D* depth    = nullptr;

        TextureView* albedoView   = nullptr;
        TextureView* normalView   = nullptr;
        TextureView* emissionView = nullptr;
        TextureView* idView       = nullptr;
        TextureView* depthView    = nullptr;

        UniformBuffer* materialUBO;
        UniformBuffer* transformUBO;

        DescriptorSet* transformSet;
        DescriptorSet* materialSet;
    };

    struct LightingData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        Texture2D*         color       = nullptr;
        TextureView*       view        = nullptr;

        PipelineHandle* pipeline = nullptr;
        ShaderHandle*   shader   = nullptr;

        UniformBuffer* sceneUBO;
        DescriptorSet* set;
    };

    struct EnvironmentData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        TextureHandle*     depth       = nullptr;
        TextureViewHandle* view        = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        UniformBuffer*     ubo;
        DescriptorSet*     set;
    };

    struct ShadowData
    {
        RenderPassHandle*  pass        = nullptr;
        FramebufferHandle* framebuffer = nullptr;
        Texture2DArray*    depth       = nullptr;
        TextureView*       depthView   = nullptr;
        PipelineHandle*    pipeline    = nullptr;
        ShaderHandle*      shader      = nullptr;

        UniformBuffer* transformUBO;
        UniformBuffer* lightTransformUBO;
        UniformBuffer* cascadeUBO;
        DescriptorSet* set;
    };

    struct BloomData
    {
        const uint32 numDefaultSampling = 6;
        std::vector<Extent> resolutions = {};

        RenderPassHandle* pass = nullptr;

        std::vector<FramebufferHandle*> samplingFB    = {};
        std::vector<Texture2D*>         sampling      = {};
        std::vector<TextureView*>       samplingView  = {};
        Texture2D*                      prefilter     = {};
        TextureView*                    prefilterView = {};
        Texture2D*                      bloom         = {};
        TextureView*                    bloomView     = {};
        FramebufferHandle*              bloomFB       = {};

        PipelineHandle* prefilterPipeline = nullptr;
        ShaderHandle*   prefilterShader   = nullptr;
        DescriptorSet*  prefilterSet      = nullptr;

        PipelineHandle*             downSamplingPipeline = nullptr;
        ShaderHandle*               downSamplingShader   = nullptr;
        std::vector<DescriptorSet*> downSamplingSet      = {};

        PipelineHandle*             upSamplingPipeline = nullptr;
        ShaderHandle*               upSamplingShader   = nullptr;
        std::vector<DescriptorSet*> upSamplingSet      = {};

        PipelineHandle* bloomPipeline = nullptr;
        ShaderHandle*   bloomShader   = nullptr;
        DescriptorSet*  bloomSet      = nullptr;
    };

    class SceneRenderer
    {
    public:

        SceneRenderer();
        ~SceneRenderer();

        void Initialize();
        void Finalize();

        void Reset(Scene* scene, Camera* camera);
        void Render();

        // フレームバッファのリサイズ
        void ResizeFramebuffer(uint32 width, uint32 height);

        // シーンの各描画コンポーネントを描画リストに追加
        void SetSkyLight(const SkyLightComponent& data);
        void SetDirectionalLight(const DirectionalLightComponent& data);
        void SetPostProcess(const PostProcessComponent& data);

        // メッシュデータを描画リストに追加
        void AddMeshDrawList(const MeshDrawData& data);

        // ピクセルのエンティティIDを取得
        int32 ReadEntityIDFromPixel(uint32 x, uint32 y);

        // シーンの最終描画結果を取得
        DescriptorSet* GetSceneFinalOutput();

        // 描画の統計データを取得
        SceneRenderStats GetRenderStats();

    private:

        void _InitializePasses();
        void _FinalizeePasses();
        void _UpdateUniformBuffer();
        void _ExcutePasses();

        // Gバッファ
        void _PrepareGBuffer(uint32 width, uint32 height);
        void _ResizeGBuffer(uint32 width, uint32 height);
        void _CleanupGBuffer();
        GBufferData* gbuffer;

        // ライティング
        void _PrepareLightingBuffer(uint32 width, uint32 height);
        void _ResizeLightingBuffer(uint32 width, uint32 height);
        void _CleanupLightingBuffer();
        LightingData* lighting;

        // 環境マップ
        void _PrepareEnvironmentBuffer(uint32 width, uint32 height);
        void _ResizeEnvironmentBuffer(uint32 width, uint32 height);
        void _CleanupEnvironmentBuffer();
        EnvironmentData* environment;

        // IBL
        void _PrepareIBL(const char* environmentTexturePath);
        void _CleanupIBL();
        void _CreateIrradiance();
        void _CreatePrefilter();
        void _CreateBRDF();

        // シャドウマップ
        void      _PrepareShadowBuffer();
        void      _CleanupShadowBuffer();
        void      _CalculateLightSapceMatrices(glm::vec3 directionalLightDir, Camera* camera, std::array<glm::mat4, 4>& out_result);
        void      _GetFrustumCornersWorldSpace(const glm::mat4& projview, std::array<glm::vec4, 8>& out_result);
        glm::mat4 _GetLightSpaceMatrix(glm::vec3 directionalLightDir, Camera* camera, const float nearPlane, const float farPlane);
        ShadowData* shadow;

        // ブルーム
        std::vector<Extent> _CalculateBlomSampling(uint32 width, uint32 height);
        void                _PrepareBloomBuffer(uint32 width, uint32 height);
        void                _ResizeBloomBuffer(uint32 width, uint32 height);
        void                _CleanupBloomBuffer();
        BloomData* bloom;

        // エンティティID リードバック
        Buffer* pixelIDBuffer = nullptr;

    private:

        // IBL
        FramebufferHandle* IBLProcessFB       = nullptr;
        RenderPassHandle*  IBLProcessPass     = nullptr;
        UniformBuffer*     equirectangularUBO = nullptr;

        // キューブマップ変換
        PipelineHandle* equirectangularPipeline = nullptr;
        ShaderHandle*   equirectangularShader   = nullptr;
        TextureCube*    cubemapTexture          = nullptr;
        TextureView*    cubemapTextureView      = nullptr;
        DescriptorSet*  equirectangularSet      = nullptr;

        // irradiance
        PipelineHandle* irradiancePipeline    = nullptr;
        ShaderHandle*   irradianceShader      = nullptr;
        TextureCube*    irradianceTexture     = nullptr;
        TextureView*    irradianceTextureView = nullptr;
        DescriptorSet*  irradianceSet         = nullptr;

        // prefilter
        PipelineHandle* prefilterPipeline    = nullptr;
        ShaderHandle*   prefilterShader      = nullptr;
        TextureCube*    prefilterTexture     = nullptr;
        TextureView*    prefilterTextureView = nullptr;
        DescriptorSet*  prefilterSet         = nullptr;

        // BRDF-LUT
        PipelineHandle* brdflutPipeline    = nullptr;
        ShaderHandle*   brdflutShader      = nullptr;
        Texture2D*      brdflutTexture     = nullptr;
        TextureView*    brdflutTextureView = nullptr;

        // シーンBlit
        FramebufferHandle* compositeFB          = nullptr;
        Texture2D*         compositeTexture     = nullptr;
        TextureView*       compositeTextureView = nullptr;
        RenderPassHandle*  compositePass        = nullptr;
        ShaderHandle*      compositeShader      = nullptr;
        PipelineHandle*    compositePipeline    = nullptr;
        DescriptorSet*     compositeSet;

        // グリッド
        ShaderHandle*   gridShader   = nullptr;
        PipelineHandle* gridPipeline = nullptr;
        DescriptorSet*  gridSet      = nullptr;
        UniformBuffer*  gridUBO      = nullptr;

        // ImGui::Image
        DescriptorSet* imageSet;

        // シャドウマップ
        static const uint32  shadowMapResolution = 2048;
        std::array<float, 4> shadowCascadeLevels = { 10.0f, 40.0f, 100.0f, 200.0f };

        // メッシュ
        Mesh* cubeMesh   = nullptr;
        Mesh* sponzaMesh = nullptr;

        // サンプラー
        Sampler* linearSampler = nullptr;
        Sampler* shadowSampler = nullptr;

        // 頂点レイアウト
        InputLayout defaultLayout;

        // テクスチャ
        Texture2D*   defaultTexture     = nullptr;
        TextureView* defaultTextureView = nullptr;
        Texture2D*   envTexture         = nullptr;
        TextureView* envTextureView     = nullptr;

        // ライティングコンポーネント
        SkyLightComponent         skyLight;
        DirectionalLightComponent directionalLight;
        PostProcessComponent      postProcess;

        // シーン情報
        glm::ivec2 sceneViewportSize = { 1280, 720 };
        Scene*     renderScene       = nullptr;
        Camera*    sceneCamera       = nullptr;

        // 描画要求されたメッシュコンポーネントリスト
        std::vector<MeshDrawData> meshDrawList;

        // インスタンシング用トランスフォーム
        //MeshParameter* meshParameters = nullptr;
        //Shared<StorageBuffer> meshParameterSBO;

        // シャドウインスタンシングデータ
        //std::unordered_map<InstancingUnitID, InstancingUnitData>      shadowDrawData;
        //std::unordered_map<InstancingUnitID, InstancingUnitParameter> ShadowParameterData;

        // ジオメトリインスタンシングデータ
        //std::unordered_map<InstancingUnitID, InstancingUnitData>      meshDrawData;
        //std::unordered_map<InstancingUnitID, InstancingUnitParameter> meshParameterData;

        // 描画フラグ
        bool enablePostProcess  = true;
        bool shouldRenderShadow = true;

        // 計測
        SceneRenderStats stats;

        // 描画API
        RenderingAPI* api = nullptr;
    };
}
