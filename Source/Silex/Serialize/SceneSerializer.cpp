
#include "PCH.h"

#include "Scene/Entity.h"
#include "Serialize/SceneSerializer.h"
#include "Serialize/Serialize.h"
#include "Rendering/Renderer.h"


namespace Silex
{
    SceneSerializer::SceneSerializer(Scene* scene)
        : scene(scene)
    {
    }

    void SceneSerializer::Serialize(const std::string& filepath)
    {
        YAML::Emitter out;

        out << YAML::BeginMap;
        out << YAML::Key << "Scene"    << YAML::Value << "Untitled";
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        scene->registry.each([&](auto entityID)
        {
            Entity entity = { entityID, scene };
            if (!entity)
                return;

            out << YAML::BeginMap;
            out << YAML::Key << "Entity" << YAML::Value << entity.GetID();

            // インスタンス
            if (entity.HasComponent<InstanceComponent>())
            {
                out << YAML::Key << "InstanceComponent";
                out << YAML::BeginMap;

                auto& c = entity.GetComponent<InstanceComponent>();
                out << YAML::Key << "name"   << YAML::Value << c.name;
                out << YAML::Key << "active" << YAML::Value << c.active;

                out << YAML::EndMap;
            }

            // トランスフォーム
            if (entity.HasComponent<TransformComponent>())
            {
                out << YAML::Key << "TransformComponent";
                out << YAML::BeginMap;

                auto& tc = entity.GetComponent<TransformComponent>();
                out << YAML::Key << "position" << YAML::Value << tc.position;
                out << YAML::Key << "rotation" << YAML::Value << tc.rotation;
                out << YAML::Key << "scale"    << YAML::Value << tc.Scale;

                out << YAML::EndMap;
            }

            // スクリプト
            if (entity.HasComponent<ScriptComponent>())
            {
                out << YAML::Key << "ScriptComponent";
                out << YAML::BeginMap;

                auto& sc = entity.GetComponent<ScriptComponent>();
                out << YAML::Key << "className" << YAML::Value << sc.className;
                out << YAML::EndMap;
            }

            // メッシュコンポーネント
            if (entity.HasComponent<MeshComponent>())
            {
                out << YAML::Key << "MeshComponent";
                out << YAML::BeginMap;

                auto& mc = entity.GetComponent<MeshComponent>();
                out << YAML::Key << "mesh" << YAML::Value << mc.mesh->GetAssetID();
                out << YAML::Key << "castShadow" << YAML::Value << mc.castShadow;
                out << YAML::Key << "material";
                out << YAML::BeginMap;

                uint32 index = 0;
                for (auto& material : mc.materials)
                {
                    // AssetID id = material ? material->GetAssetID() : Renderer::Get()->GetDefaultMaterial()->GetAssetID();
                    // out << YAML::Key << std::to_string(index++) << id;
                }

                out << YAML::EndMap;
                out << YAML::EndMap;
            }

            // ディレクショナルライトコンポーネント
            if (entity.HasComponent<DirectionalLightComponent>())
            {
                out << YAML::Key << "DirectionalLightComponent";
                out << YAML::BeginMap;

                auto& dl = entity.GetComponent<DirectionalLightComponent>();
                out << YAML::Key << "color"               << YAML::Value << dl.color;
                out << YAML::Key << "enableSoftShadow"    << YAML::Value << dl.enableSoftShadow;
                out << YAML::Key << "intencity"           << YAML::Value << dl.intencity;
                out << YAML::Key << "showCascade"         << YAML::Value << dl.showCascade;
                out << YAML::Key << "shadowDepthBias"     << YAML::Value << dl.shadowDepthBias;

                out << YAML::EndMap;
            }

            // スカイライトコンポーネント
            if (entity.HasComponent<SkyLightComponent>())
            {
                out << YAML::Key << "SkyLightComponent";
                out << YAML::BeginMap;

                auto& sl = entity.GetComponent<SkyLightComponent>();
                // out << YAML::Key << "sky"          << YAML::Value << sl.sky->GetAssetID();
                out << YAML::Key << "intencity"    << YAML::Value << sl.intencity;
                out << YAML::Key << "renderSky"    << YAML::Value << sl.renderSky;
                out << YAML::Key << "enableIBL"    << YAML::Value << sl.enableIBL;

                out << YAML::EndMap;
            }

            // ポストプロセスコンポーネント
            if (entity.HasComponent<PostProcessComponent>())
            {
                out << YAML::Key << "PostProcessComponent";
                out << YAML::BeginMap;

                auto& pp = entity.GetComponent<PostProcessComponent>();
                out << YAML::Key << "enableOutline"             << YAML::Value << pp.enableOutline;
                out << YAML::Key << "lineWidth"                 << YAML::Value << pp.lineWidth;
                out << YAML::Key << "outlineColor"              << YAML::Value << pp.outlineColor;
                out << YAML::Key << "enableFXAA"                << YAML::Value << pp.enableFXAA;
                out << YAML::Key << "enableBloom"               << YAML::Value << pp.enableBloom;
                out << YAML::Key << "bloomThreshold"            << YAML::Value << pp.bloomThreshold;
                out << YAML::Key << "bloomIntencity"            << YAML::Value << pp.bloomIntencity;
                out << YAML::Key << "enableChromaticAberration" << YAML::Value << pp.enableChromaticAberration;
                out << YAML::Key << "enableTonemap"             << YAML::Value << pp.enableTonemap;
                out << YAML::Key << "exposure"                  << YAML::Value << pp.exposure;
                out << YAML::Key << "gammaCorrection"           << YAML::Value << pp.gammaCorrection;

                out << YAML::EndMap;
            }

            out << YAML::EndMap;
        });

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    void SceneSerializer::Deserialize(const std::string& filepath)
    {
        YAML::Node data = YAML::LoadFile(filepath);

        if (auto entities = data["Entities"])
        {
            for (auto entity : entities)
            {
                std::string name;
                bool active;

                auto id = entity["Entity"].as<uint64>();
                auto c = entity["InstanceComponent"];
                name   = c["name"].as<std::string>();
                active = c["active"].as<bool>();

                Entity e = scene->CreateEntity(id, name, active);

                // トランスフォーム
                if (auto transform = entity["TransformComponent"])
                {
                    TransformComponent& tc = e.GetComponent<TransformComponent>();
                    tc.position = transform["position"].as<glm::vec3>();
                    tc.rotation = transform["rotation"].as<glm::vec3>();
                    tc.Scale    = transform["scale"].as<glm::vec3>();
                }

                // スクリプト
                if (auto script = entity["ScriptComponent"])
                {
                    ScriptComponent& sc = e.GetComponent<ScriptComponent>();
                    sc.className = script["className"].as<std::string>();
                }

                // メッシュ
                if (auto mesh = entity["MeshComponent"])
                {
                    MeshComponent& mc = e.AddComponent<MeshComponent>();

                    AssetID id = mesh["mesh"].as<uint64>();
                    mc.mesh = AssetManager::Get()->GetAssetAs<MeshAsset>(id);

                    mc.castShadow = mesh["castShadow"].as<bool>();

                    auto material = mesh["material"];
                    auto numSlots = mc.mesh->Get()->GetMaterialSlotCount();

                    for (uint32 i = 0; i < numSlots; i++)
                    {
                        auto id = material[std::to_string(i)].as<AssetID>();
                        Ref<MaterialAsset> m = AssetManager::Get()->GetAssetAs<MaterialAsset>(id);

                        mc.materials.emplace_back(m);
                    }
                }

                // ディレクショナルライトコンポーネント
                if (auto directional = entity["DirectionalLightComponent"])
                {
                    DirectionalLightComponent& dl = e.AddComponent<DirectionalLightComponent>();
                    dl.color               = directional["color"].as<glm::vec3>();
                    dl.enableSoftShadow    = directional["enableSoftShadow"].as<bool>();
                    dl.intencity           = directional["intencity"].as<float>();
                    dl.showCascade         = directional["showCascade"].as<bool>();
                    dl.shadowDepthBias     = directional["shadowDepthBias"].as<float>();
                }

                // スカイライトコンポーネント
                if (auto sky = entity["SkyLightComponent"])
                {
                    SkyLightComponent& sl = e.AddComponent<SkyLightComponent>();

                    sl.sky       = AssetManager::Get()->GetAssetAs<EnvironmentAsset>(sky["sky"].as<uint64>());
                    sl.intencity = sky["intencity"].as<float>();
                    sl.renderSky = sky["renderSky"].as<bool>();
                    sl.enableIBL = sky["enableIBL"].as<bool>();
                }

                // ポストプロセスコンポーネント
                if (auto postProcess = entity["PostProcessComponent"])
                {
                    PostProcessComponent& pp = e.AddComponent<PostProcessComponent>();

                    pp.enableOutline             = postProcess["enableOutline"].as<bool>();
                    pp.lineWidth                 = postProcess["lineWidth"].as<float>();
                    pp.outlineColor              = postProcess["outlineColor"].as<glm::vec3>();
                    pp.enableFXAA                = postProcess["enableFXAA"].as<bool>();
                    pp.enableBloom               = postProcess["enableBloom"].as<bool>();
                    pp.bloomThreshold            = postProcess["bloomThreshold"].as<float>();
                    pp.bloomIntencity            = postProcess["bloomIntencity"].as<float>();
                    pp.enableChromaticAberration = postProcess["enableChromaticAberration"].as<bool>();
                    pp.enableTonemap             = postProcess["enableTonemap"].as<bool>();
                    pp.exposure                  = postProcess["exposure"].as<float>();
                    pp.gammaCorrection           = postProcess["gammaCorrection"].as<float>();
                }
            }
        }
    }
}
