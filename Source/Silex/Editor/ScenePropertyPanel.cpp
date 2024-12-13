
#include "PCH.h"

#include "Editor/ScenePropertyPanel.h"
#include "Rendering/Environment.h"
#include "Rendering/RenderingStructures.h"
#include "Rendering/Renderer.h"
#include "Script/Script.h"

#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>


namespace Silex
{
    void ScenePropertyPanel::Render(bool* showOutlinerPannel, bool* showPropertyPannel)
    {
        if (*showOutlinerPannel)
        {
            ImGui::Begin("シーンアウトライナー", showOutlinerPannel);

            if (scene)
                DrawHierarchy();

            ImGui::End();
        }

        if (*showPropertyPannel)
        {
            ImGui::Begin("プロパティ", showPropertyPannel);

            if (selectEntity)
                DrawComponents(selectEntity);

            ImGui::End();
        }
    }

    void ScenePropertyPanel::DrawHierarchy()
    {
        float width = ImGui::GetContentRegionMax().x - 10;
        if (ImGui::Button("エンティティ生成", ImVec2(width, 25)))
        {
            selectEntity = scene->CreateEntity();
            onEntitySelectDelegate.Execute(true);
        }

        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            {
                selectEntity = {};
                onEntitySelectDelegate.Execute(false);
            }

            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight))
            {
                if (ImGui::MenuItem("エンティティ生成"))
                    scene->CreateEntity();

                ImGui::EndPopup();
            }

            scene->registry.each([&](auto entityID)
            {
                Entity entity{ entityID , scene.Get() };

                auto& instance = entity.GetComponent<InstanceComponent>();
                ImGui::PushID(instance.id);
#if 1
                ImGui::Selectable(instance.name.c_str(), selectEntity == entity);
                if (ImGui::IsItemClicked())
                {
                    selectEntity = entity;
                    onEntitySelectDelegate.Execute(true);
                }
#else
                ImGuiTreeNodeFlags flags = ((selectEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
                flags |= ImGuiTreeNodeFlags_SpanFullWidth;

                if (ImGui::TreeNodeEx((void*)(uint64)(uint32)entity, flags, name.c_str()))
                {
                    // 開いたツリーを閉じる
                    ImGui::TreePop();
                }

                if (ImGui::IsItemClicked())
                {
                    selectEntity = entity;
                    onEntitySelectDelegate.Execute(true);
                }
#endif
                bool requestDelete = false;
                if (ImGui::BeginPopupContextItem())
                {

                    if (ImGui::MenuItem("削除"))
                        requestDelete = true;

                    ImGui::EndPopup();
                }

                if (requestDelete)
                {
                    scene->DestroyEntity(entity);

                    if (selectEntity == entity)
                    {
                        selectEntity = {};
                    }
                }

                ImGui::PopID();
            });
        }

        ImGui::EndChild();
    }

    void ScenePropertyPanel::DrawComponents(Entity entity)
    {
        float windowWidth = ImGui::GetWindowWidth();

        if (entity.HasComponent<InstanceComponent>())
        {
            auto& instance = entity.GetComponent<InstanceComponent>();
            ImGui::Checkbox(" ", &instance.active);
            ImGui::SameLine();

            // チェックボックスのラベル表示のための余白分を左にずらす
            float pos = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(pos - 15.0f);

            char buffer[256];
            auto& tag = instance.name;
            memset(buffer, 0, sizeof(buffer));
            strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));

            ImGui::PushItemWidth(windowWidth - 15.0f);

            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                tag = std::string(buffer);

            ImGui::PopItemWidth();
        }

        DrawComponent<TransformComponent>("トランスフォーム", entity, [&](TransformComponent& component)
        {
            ImGui::Dummy({ 0, 4.0f });
            ImGui::Columns(2);

            // コンポーネント名
            {
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));

                ImGui::Text("Position");
                ImGui::Text("Rotation");
                ImGui::Text("Scale");

                ImGui::PopStyleVar();
            }

            ImGui::NextColumn();

            // 数値
            {
                ImGui::PushID("Position");
                float columnWidth = ImGui::GetColumnWidth(1) - 5;
                DrawFloat3ComponentValue(component.position, columnWidth);
                ImGui::PopID();

                ImGui::PushID("Rotation");
                glm::vec3 rotation = glm::degrees(component.rotation);
                DrawFloat3ComponentValue(rotation, columnWidth);
                component.rotation = glm::radians(rotation);
                ImGui::PopID();

                ImGui::PushID("Scale");
                DrawFloat3ComponentValue(component.Scale, columnWidth);
                ImGui::PopID();
            }

            ImGui::Columns(1);
        });

        DrawComponent<ScriptComponent>("スクリプト", entity, [](ScriptComponent& component)
        {
            ImGui::Dummy({ 0, 4.0f });
            ImGui::Columns(2);

            // コンポーネント名
            {
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));
                ImGui::Text("Script Class");
                ImGui::PopStyleVar();
            }

            ImGui::NextColumn();

            // 数値
            {
                ImGui::PushID("Script");

                bool existSuitableScriptClass = false;
                const auto& entityClasses     = ScriptManager::GetAllEntityClasses();

                if (entityClasses.contains(component.className))
                    existSuitableScriptClass = true;

                static char buffer[256];
                std::strcpy(buffer, component.className.c_str());

                if (!existSuitableScriptClass)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.2, 0.2, 1.0));

                    if (ImGui::InputText("", buffer, sizeof(buffer)))
                        component.className = buffer;

                    ImGui::PopStyleColor();
                }
                else
                {
                    if (ImGui::InputText("", buffer, sizeof(buffer)))
                        component.className = buffer;
                }

                ImGui::PopID();
            }

            ImGui::Columns(1);
        });

        DrawComponent<MeshComponent>("メッシュ", entity, [](MeshComponent& component)
        {
            ImGui::Dummy({ 0, 4.0f });
            ImGui::Columns(2);

            {
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));
                ImGui::Text("Mesh");
                ImGui::Text("Cast Shadow");
                ImGui::Text("Material");
                ImGui::PopStyleVar();
            }

            ImGui::NextColumn();

            {
                float w = ImGui::GetColumnWidth(1) - 5;
                ImGui::PushItemWidth(w);

                {
                    ImGui::PushID("Mesh");

                    std::string meshName = {};
                    if (component.mesh)
                        meshName = component.mesh->GetName();

                    if (ImGui::Button(meshName.c_str(), ImVec2(w, 0.0f)))
                        ImGui::OpenPopup("##MeshPopup");

                    if (ImGui::BeginPopup("##MeshPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
                    {
                        AssetID current = {};
                        bool selected = false;
                        bool modified = false;
                        
                        if (ImGui::BeginListBox("##MeshPopup", ImVec2(200.f, 0.0f)))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));

                            auto& database = AssetManager::Get()->GetAllAssets();
                            for (auto& [id, asset] : database)
                            {
                                if (asset == nullptr)
                                    continue;

                                if (!asset->IsAssetOf(AssetType::Mesh))
                                    continue;

                                selected = (current == id);
                                if (ImGui::Selectable(asset->GetName().c_str(), selected))
                                {
                                    current  = id;
                                    modified = true;
                                    meshName = asset->GetName().c_str();

                                    Ref<MeshAsset> m = asset.As<MeshAsset>();
                                    component.mesh = m;

                                    uint32 numSlots = m->Get()->GetMaterialSlotCount();
                                    component.materials.resize(numSlots);
                                }

                                if (selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }

                            ImGui::PopStyleVar();
                            ImGui::EndListBox();
                        }

                        if (modified)
                        {
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                {
                    ImGui::PushID("Cast Shadow");
                    ImGui::Checkbox("", &component.castShadow);
                    ImGui::PopID();
                }

                uint32 index = 0;
                for (auto& material : component.materials)
                {
                    std::string materialName = "None";
                    if (material)
                    {
                        materialName = material->GetName();
                    }

                    std::string indexID = std::to_string(index);
                    ImGui::PushID(indexID.c_str());

                    if (ImGui::Button(materialName.c_str(), ImVec2(w, 0.0f)))
                        ImGui::OpenPopup("");

                    if (ImGui::BeginPopup("", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
                    {
                        AssetID current = {};
                        bool selected = false;
                        bool modified = false;

                        if (ImGui::BeginListBox("##", ImVec2(200.f, 0.0f)))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));

                            auto& database = AssetManager::Get()->GetAllAssets();
                            for (auto& [id, asset] : database)
                            {
                                if (asset == nullptr)
                                    continue;

                                if (!asset->IsAssetOf(AssetType::Material))
                                    continue;

                                selected = (current == id);
                                if (ImGui::Selectable(asset->GetName().c_str(), selected))
                                {
                                    current = id;
                                    modified = true;
                                    materialName = asset->GetName().c_str();
                                    material     = asset.As<MaterialAsset>();
                                }

                                if (selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }

                            ImGui::PopStyleVar();
                            ImGui::EndListBox();
                        }

                        if (modified)
                        {
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                    index++;
                }

                ImGui::PopItemWidth();
            }

            ImGui::Columns(1);
            ImGui::Dummy({ 0, 4.0f });
        });

        DrawComponent<SkyLightComponent>("スカイライト", entity, [](SkyLightComponent& component)
        {
            std::string skyName = {};

            if (component.sky)
                skyName = component.sky->GetName();

            ImGui::Dummy({ 0, 4.0f });
            ImGui::Columns(2);

            {
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));

                ImGui::Text("Sky");
                ImGui::Text("RenderSky");
                ImGui::Text("EnableIBL");
                ImGui::Text("IBL Intencity");
                ImGui::PopStyleVar();
            }

            ImGui::NextColumn();
            
            {
                ImGui::PushID("Sky");

                float w = ImGui::GetColumnWidth(1) - 5;
                ImGui::PushItemWidth(w);

                if (ImGui::Button(skyName.c_str(), ImVec2(w, 0.0f)))
                    ImGui::OpenPopup("##SkyPopup");

                if (ImGui::BeginPopup("##SkyPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
                {
                    AssetID current = -1;
                    bool selected = false;
                    bool modified = false;

                    if (ImGui::BeginListBox("##SkyPopup", ImVec2(200.f, 0.0f)))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));

                        auto& database = AssetManager::Get()->GetAllAssets();
                        for (auto& [id, asset] : database)
                        {
                            if (asset == nullptr)
                                continue;

                            if (!asset->IsAssetOf(AssetType::Environment))
                                continue;

                            selected = (current == id);
                            if (ImGui::Selectable(asset->GetName().c_str(), selected))
                            {
                                current  = id;
                                modified = true;
                                skyName  = asset->GetName().c_str();

                                component.sky = AssetManager::Get()->GetAssetAs<EnvironmentAsset>(id);
                            }

                            if (selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }

                        ImGui::PopStyleVar();
                        ImGui::EndListBox();
                    }

                    if (modified)
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
                ImGui::PopItemWidth();


                ImGui::PushID("Render Sky");    ImGui::Checkbox("", &component.renderSky);                     ImGui::PopID();
                ImGui::PushID("Enable IBL");    ImGui::Checkbox("", &component.enableIBL);                     ImGui::PopID();
                ImGui::PushID("IBL Intencity"); ImGui::DragFloat("", &component.intencity, 0.1f, 0.0f, 10.0f); ImGui::PopID();
            }

            ImGui::Columns(1);
        });

        DrawComponent<DirectionalLightComponent>("ディレクショナルライト", entity, [](DirectionalLightComponent& component)
        {
            ImGui::Dummy({ 0, 4.0f });
            ImGui::Columns(2);

            {
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));
                ImGui::Text("EnableSoftShadow");
                ImGui::Text("ShowCascade");
                ImGui::Text("LightColor");
                ImGui::Text("Lightintencity");
                ImGui::Text("DepthBias");
                ImGui::PopStyleVar();
            }

            ImGui::NextColumn();

            {
                float w = ImGui::GetColumnWidth(1) - 5;
                ImGui::PushItemWidth(w);

                ImGui::PushID("EnableSoftShadow"); ImGui::Checkbox("",   &component.enableSoftShadow);                      ImGui::PopID();
                ImGui::PushID("ShowCascade");      ImGui::Checkbox("",   &component.showCascade);                           ImGui::PopID();
                ImGui::PushID("LightColor");       ImGui::ColorEdit3("", glm::value_ptr(component.color));                  ImGui::PopID();
                ImGui::PushID("Lightintencity");   ImGui::DragFloat("",  &component.intencity,       0.001f,   0.0f, 10.0f);ImGui::PopID();
                ImGui::PushID("DepthBias");        ImGui::DragFloat("",  &component.shadowDepthBias, 0.0001f, -1.0f, 1.0f); ImGui::PopID();
                ImGui::PopItemWidth();
            }

            ImGui::Columns(1);
        });

        DrawComponent<PostProcessComponent>("ポストプロセス", entity, [](PostProcessComponent& component)
        {
            ImGui::Dummy({ 0, 4.0f });
            ImGui::Columns(2);

            {
                float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));

                ImGui::Text("EnableOutline");
                ImGui::Text("LineWidth");
                ImGui::Text("OutlineColor");

                ImGui::Text("EnableFXAA");

                ImGui::Text("EnableBloom");
                ImGui::Text("BloomThreshold");
                ImGui::Text("BloomIntencity");

                ImGui::Text("EnableChromaticAberration");

                ImGui::Text("EnableTonemap");
                ImGui::Text("Exposure");
                ImGui::Text("GammaCorrection");

                ImGui::PopStyleVar();
            }

            ImGui::NextColumn();

            {
                float w = ImGui::GetColumnWidth(1) - 5;
                ImGui::PushItemWidth(w);

                ImGui::PushID("EnableOutline");             ImGui::Checkbox("",   &component.enableOutline);                ImGui::PopID();
                ImGui::PushID("LineWidth");                 ImGui::DragFloat("",  &component.lineWidth, 0.1f, 0.0f, 10.0f); ImGui::PopID();
                ImGui::PushID("OutlineColor");              ImGui::ColorEdit3("", glm::value_ptr(component.outlineColor));  ImGui::PopID();
                                                            
                ImGui::PushID("EnableFXAA");                ImGui::Checkbox("", &component.enableFXAA); ImGui::PopID();
                                                            
                ImGui::PushID("EnableBloom");               ImGui::Checkbox("",  &component.enableBloom);                        ImGui::PopID();
                ImGui::PushID("BloomThreshold");            ImGui::DragFloat("", &component.bloomThreshold,  0.1f, 0.0f, 10.0f); ImGui::PopID();
                ImGui::PushID("BloomIntencity");            ImGui::DragFloat("", &component.bloomIntencity,  0.1f, 0.0f,  1.0f); ImGui::PopID();

                ImGui::PushID("EnableChromaticAberration"); ImGui::Checkbox("", &component.enableChromaticAberration); ImGui::PopID();

                ImGui::PushID("EnableTonemap");             ImGui::Checkbox("",  &component.enableTonemap);                      ImGui::PopID();
                ImGui::PushID("Exposure");                  ImGui::DragFloat("", &component.exposure,        0.1f, 0.0f, 10.0f); ImGui::PopID();
                ImGui::PushID("GammaCorrection");           ImGui::DragFloat("", &component.gammaCorrection, 0.1f, 0.0f, 10.0f); ImGui::PopID();

                ImGui::PopItemWidth();
            }

            ImGui::Columns(1);
        });


        ImGui::SeparatorText("");
        ImGui::Dummy({ windowWidth / 3.0f, 0.0f});
        ImGui::SameLine();

        if (ImGui::Button("コンポーネント追加"))
            ImGui::OpenPopup("コンポーネント追加");

        if (ImGui::BeginPopup("コンポーネント追加"))
        {
            DisplayAddComponentPopup<MeshComponent>("Mesh");
            DisplayAddComponentPopup<DirectionalLightComponent>("DirectionalLight");
            DisplayAddComponentPopup<SkyLightComponent>("SkyLightComponent");
            DisplayAddComponentPopup<PostProcessComponent>("PostProcessComponent");
            DisplayAddComponentPopup<ScriptComponent>("ScriptComponent");

            ImGui::EndPopup();
        }
    }

    void ScenePropertyPanel::DrawFloat3ComponentValue(glm::vec3& values, float columnWidth)
    {
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { 3.0f, lineHeight - 1.0f };
        float  inputTextSize = (columnWidth - 3.0f) / 3.0f - 2.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 1 });

        // === X ===
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.7f, 0.2f, 0.2f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.3f, 0.3f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.7f, 0.2f, 0.2f, 1.0f });
            ImGui::Button("", buttonSize);
            ImGui::PopStyleColor(3);

            ImGui::SameLine();

            ImGui::SetNextItemWidth(inputTextSize);
            ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");

            ImGui::SameLine();
        }

        // === Y ===
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.5f, 0.0f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.0f, 0.6f, 0.0f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.0f, 0.5f, 0.0f, 1.0f });
            ImGui::Button("", buttonSize);
            ImGui::PopStyleColor(3);

            ImGui::SameLine();

            ImGui::SetNextItemWidth(inputTextSize);
            ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");

            ImGui::SameLine();
        }

        // === Z ===
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.2f, 0.8f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.3f, 0.9f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.2f, 0.8f, 1.0f });
            ImGui::Button("", buttonSize);
            ImGui::PopStyleColor(3);

            ImGui::SameLine();

            ImGui::SetNextItemWidth(inputTextSize);
            ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        }

        ImGui::PopStyleVar();
    }

    template<typename T, typename Func>
    void ScenePropertyPanel::DrawComponent(const std::string& name, Entity entity, Func drawFunc)
    {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap; // | ImGuiTreeNodeFlags_FramePadding;
        if (entity.HasComponent<T>())
        {
            auto& component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

            ImGui::Separator();

            bool open = ImGui::TreeNodeEx((void*)T::staticHashID, treeNodeFlags, name.c_str());
            ImGui::PopStyleVar();

            // トランスフォームコンポーネント（必須コンポーネントのため）は削除できない
            if constexpr (Traits::IsSame<T, TransformComponent>())
            {
                if (open)
                {
                    drawFunc(component);
                    ImGui::TreePop();
                }
            }
            else
            {
                // 非トランスフォームコンポーネントの場合、削除ポップアップを表示し、要求されれば削除する
                ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
                if (ImGui::Button("×", ImVec2{ lineHeight, lineHeight }))
                {
                    ImGui::OpenPopup(T::staticClassName);
                }

                bool removeComponent = false;
                if (ImGui::BeginPopup(T::staticClassName))
                {
                    if (ImGui::MenuItem("削除"))
                        removeComponent = true;

                    ImGui::EndPopup();
                }

                if (open)
                {
                    drawFunc(component);
                    ImGui::TreePop();
                }

                if (removeComponent)
                    entity.RemoveComponent<T>();
            }
        }
    }

    template<typename T>
    void ScenePropertyPanel::DisplayAddComponentPopup(const std::string& entryName)
    {
        if (ImGui::MenuItem(entryName.c_str()))
        {
            if (!selectEntity.HasComponent<T>())
            {
                selectEntity.AddComponent<T>();
                ImGui::CloseCurrentPopup();
            }
        }
    }
}
