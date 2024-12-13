
#include "PCH.h"
#include "Asset/Asset.h"
#include "Serialize/AssetSerializer.h"
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"
#include "Rendering/Environment.h"


namespace Silex
{
    template<>
    static Ref<MaterialAsset> AssetCreator::Create<MaterialAsset>(const std::filesystem::path& directory)
    {
        Material* material = slnew(Material);

        Ref<MaterialAsset> asset = CreateRef<MaterialAsset>(material);
        asset->SetupAssetProperties(directory.string(), AssetType::Material);

        //TODO: レンダラー？ アセットマネージャー？ から取得するように変更
        // asset->AlbedoMap = Renderer::Get()->GetDefaultTexture();

        AssetSerializer<MaterialAsset>::Serialize(asset, directory.string());

        return asset;
    }

}
