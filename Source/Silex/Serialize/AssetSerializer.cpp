
#include "PCH.h"

#include "Core/Random.h"
#include "Asset/Asset.h"
#include "Serialize/Serialize.h"
#include "Serialize/AssetSerializer.h"
#include "Rendering/Material.h"
#include "Rendering/Renderer.h"


namespace Silex
{
    template<>
    void AssetSerializer<MaterialAsset>::Serialize(const Ref<MaterialAsset>& aseet, const std::string& filePath)
    {
        //AssetID albedoAssetID = Renderer::Get()->GetDefaultTexture()->GetAssetID();
        //if (aseet->AlbedoMap)
        //    albedoAssetID = aseet->AlbedoMap->GetAssetID();

        YAML::Emitter out;

        out << YAML::BeginMap;
        out << YAML::Key << "shadingModel"  << YAML::Value << aseet->Get()->shadingModel;
        out << YAML::Key << "albedo"        << YAML::Value << aseet->Get()->albedo;
      //out << YAML::Key << "albedoMap"     << YAML::Value << albedoAssetID;
        out << YAML::Key << "emission"      << YAML::Value << aseet->Get()->emission;
        out << YAML::Key << "metallic"      << YAML::Value << aseet->Get()->metallic;
        out << YAML::Key << "roughness"     << YAML::Value << aseet->Get()->roughness;
        out << YAML::Key << "textureTiling" << YAML::Value << aseet->Get()->textureTiling;
        out << YAML::EndMap;

        std::ofstream fout(filePath);
        fout << out.c_str();
        fout.close();
    }

    template<>
    Ref<MaterialAsset> AssetSerializer<MaterialAsset>::Deserialize(const std::string& filePath)
    {
        Material* material = slnew(Material);
        Ref<MaterialAsset> asset = CreateRef<MaterialAsset>(material);


        YAML::Node data = YAML::LoadFile(filePath);

        auto shadingModel  = data["shadingModel"].as<int32>();
        auto albedo        = data["albedo"].as<glm::vec3>();
        auto albedoMap     = data["albedoMap"].as<AssetID>();
        auto emission      = data["emission"].as<glm::vec3>();
        auto metallic      = data["metallic"].as<float>();
        auto roughness     = data["roughness"].as<float>();
        auto textureTiling = data["textureTiling"].as<glm::vec2>();

        asset->Get()->shadingModel  = (ShadingModelType)shadingModel;
        asset->Get()->emission      = emission;
        asset->Get()->metallic      = metallic;
        asset->Get()->roughness     = roughness;
        asset->Get()->textureTiling = textureTiling;
        asset->Get()->albedo        = albedo;

        // テクスチャ読み込み完了を前提とする
        const Ref<Texture2DAsset>& texture = AssetManager::Get()->GetAssetAs<Texture2DAsset>(albedoMap);
        asset->Get()->albedoMap = texture;

        return asset;
    }
}