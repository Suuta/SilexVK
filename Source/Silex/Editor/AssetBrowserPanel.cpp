
#include "PCH.h"
#include "Core/Engine.h"
#include "Asset/Asset.h"
#include "Rendering/Material.h"
#include "Serialize/AssetSerializer.h"
#include "Editor/AssetBrowserPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>


//================================================================================
// 方針
//--------------------------------------------------------------------------------
// ・特定のディレクトリ（マテリアル）を指定し、そのディレクトリ内のファイルに限定して操作を適応する
// ・生成時に物理ファイルを生成し、メタデータファイルを更新
// ・ファイル情報をマテリアルパラメータとし、マテリアルパラメータパネルから編集可能にする
// ・シーンファイルに直接記述していたマテリアル情報をマテリアルファイルに移行する
// ・シーンレンダラーのメッシュインスタンシングデータ判別にマテリアルの判別を追加
// ・操作はコンテキストメニュー形式で行い、削除・生成の2つのコマンドを適応できるようにする
// ・メッシュコンポーネントのスロットにマテリアルのスロットのみ用意する
//================================================================================

/* マテリアル シリアライズ形式
    Albedo: [1, 1, 1]
    AlbedoTexture : 0
    Emission : [0, 0, 0]
    Metallic : 1
    Roughness : 1
    TextureTiling : [1, 1]
    CastShadow : true
=========================*/

namespace Silex
{
    //=============================
    // AssetBrowserItem
    //=============================
    void AssetBrowserItem::Render(AssetBrowserPanel* panel, const glm::vec2& size)
    {
        bool isSelected = false;
        AssetID selectID = 0;

        if (panel->selectAsset)
            selectID = panel->selectAsset->GetAssetID();

        isSelected = id == selectID;
        ImVec4 color = isSelected ? ImVec4(0.25, 0.85, 0.85, 1) : ImVec4(0, 0, 0, 0);

        ImGui::PushStyleColor(ImGuiCol_Button,        color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  color);
      //ImGui::ImageButton(ImTextureID(m_Icon ? m_Icon->GetID() : 0), { size.x - 10.0f, size.y - 10.0f }, { 0, 0 }, { 1, 1 }, 2);
        ImGui::PopStyleColor(3);

        // 左クリック（選択）
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                panel->selectAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>(id);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && type == AssetItemType::Directory)
                panel->moveRequestDirectoryAssetID = id;
        }

        // 右クリック（コンテキストメニュー）
        if (ImGui::BeginPopupContextItem(fileName.c_str()))
        {
            if (ImGui::MenuItem("削除"))
            {
                // アセットリストから削除
                panel->deleteRequestItemAssetID = id;
                AssetManager::Get()->DeleteAsset(id);
            }
        
            ImGui::EndPopup();
        }

        ImGui::TextWrapped(fileName.c_str());
        ImGui::NextColumn();
    }

    //=============================
    // AssetBrowserPanel
    //=============================
    void AssetBrowserPanel::Initialize()
    {
        LoadAssetIcons();

        std::string directory = Engine::Get()->GetEditor()->GetAssetDirectory().string();

        // 指定ディレクトリ内の ファイル/サブディレクトリ を走査して要素をアイテムとして保存
        AssetID rootDirrectoryID = TraversePhysicalDirectories(directory, nullptr);
        currentDirectory = directories[rootDirrectoryID];
        rootDirectory    = currentDirectory;

        // 現在のディレクトリを表示ディレクトリとして適応
        ChangeDirectory(currentDirectory);
    }

    void AssetBrowserPanel::Finalize()
    {
    }

    void AssetBrowserPanel::Render(bool* showBrowser, bool* showProperty)
    {
        if (*showBrowser)
        {
            ImGui::Begin("アセットブラウザ", showBrowser, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        
            ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable
                | ImGuiTableFlags_SizingFixedFit
                | ImGuiTableFlags_BordersInnerV;

            ImGui::PushID("Browser");

            if (ImGui::BeginTable("table", 2, tableFlags, ImVec2(0.0f, 0.0f)))
            {
                ImGui::TableSetupColumn("Outliner", 0, 300.0f);
                ImGui::TableSetupColumn("DirectoryWidth", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                {
                    // ディレクトリー階層表示
                    ImGui::BeginChild("DirectoryHierarchy");

                    for (auto& [id, node] : rootDirectory->childDirectory)
                    {
                        DrawDirectory(node);
                    }

                    ImGui::EndChild();
                }
                
                ImGui::TableSetColumnIndex(1);
                {
                    // 現在のディレクトリのアセットを表示
                    ImGui::BeginChild("DirectoryAssets");
                    DrawCurrentDirectoryAssets();
                    ImGui::EndChild();
                }

                ImGui::EndTable();
            }

            ImGui::PopID();
            ImGui::End();
        }

        // TODO: テンプレート化 or インターフェースで実装する
        if (*showProperty)
        {
            ImGui::Begin("マテリアル", showProperty);
            DrawMaterial();
            ImGui::End();
        }
    }

    AssetID AssetBrowserPanel::TraversePhysicalDirectories(const std::filesystem::path& directory, const Ref<DirectoryNode>& parentDirectory)
    {
        // ディレクトリをアセットとして扱い、アセットIDを生成する（このIDシリアライズされず、起動の度に変化する）
        Ref<DirectoryNode> node = CreateRef<DirectoryNode>();
        node->ID              = AssetManager::Get()->GenerateAssetID();
        node->parentDirectory = parentDirectory;

        if (directory != Engine::Get()->GetEditor()->GetAssetDirectory())
        {
            node->filePath = directory;
        }

        for (auto& entry : std::filesystem::directory_iterator(directory))
        {
            // ディレクトリなら再帰
            if (entry.is_directory())
            {
                AssetID subdirID               = TraversePhysicalDirectories(entry.path(), parentDirectory);
                node->childDirectory[subdirID] = directories[subdirID];
                continue;
            }

            // メタデータの検証
            auto metadata = AssetManager::Get()->GetMetadata(entry.path());
            if (!AssetManager::Get()->IsValidID(metadata.id))
            {
                AssetType type = Asset::FileNameToAssetType(entry.path());
                if (type == AssetType::None)
                    continue;

                // 無ければデータベースに登録？
                // AssetManager::Get()->AddToMetadata(entry.path())
            }

            node->assets.push_back(metadata.id);
        }

        directories[node->ID] = node;
        return node->ID;
    }

    void AssetBrowserPanel::ChangeDirectory(const Ref<DirectoryNode>& directory)
    {
        currentDirectoryAssetItems.clear();

        // ディレクトリ追加
        for (auto& [id, node] : directory->childDirectory)
        {
            std::string fileName = node->filePath.filename().string();
            currentDirectoryAssetItems[id] = CreateRef<AssetBrowserItem>(AssetItemType::Directory, id, std::move(fileName), directoryIcon);
        }

        // アセットファイル追加
        for (auto& id : directory->assets)
        {
            auto metadata = AssetManager::Get()->GetMetadata(id);
            if (metadata.path.empty())
            {
                continue;
            }

            std::string fileName = metadata.path.filename().string();
            currentDirectoryAssetItems[id] = CreateRef<AssetBrowserItem>(AssetItemType::Asset, id, std::move(fileName), assetIcons[metadata.type]);
        }

        currentDirectory = directory;
    }

    void AssetBrowserPanel::DrawDirectory(const Ref<DirectoryNode>& node)
    {
        std::string label    = node->filePath.filename().string();
        bool open            = ImGui::TreeNode(label.c_str());
        bool changeDirectory = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (open)
        {
            for (auto& [id, node] : node->childDirectory)
            {
                DrawDirectory(node);
            }

            ImGui::TreePop();
        }

        if (changeDirectory)
        {
            ChangeDirectory(node);
        }
    }

    void AssetBrowserPanel::DrawCurrentDirectoryAssets()
    {
        // マウスのオーバーラップが無い場合にコンテキストメニューを表示
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::BeginMenu("新規アセット"))
            {
                if (ImGui::MenuItem("マテリアル"))
                {
                    std::string filePath = OS::Get()->SaveFile("Silex Material (*.slmt)\0*.slmt\0", "slmt");
                    if (!filePath.empty())
                    {
                        // アセットパスを
                        std::string filename = std::filesystem::path(filePath).filename().string();

                        std::string directory = currentDirectory->filePath.string();
                        std::string path      = directory + "/" + filename;

                        // 新規アセット生成
                        Ref<MaterialAsset> materialAsset = AssetManager::Get()->CreateAsset<MaterialAsset>(path);

                        // アセットブラウザのアセットリストに追加
                        currentDirectoryAssetItems[materialAsset->GetAssetID()] = CreateRef<AssetBrowserItem>(AssetItemType::Asset, materialAsset->GetAssetID(), std::move(filename), assetIcons[AssetType::Material]);
                        currentDirectory->assets.push_back(materialAsset->GetAssetID());
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
        
        // 空領域クリックで、選択解除
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        {
            selectAsset = nullptr;
        }


        float thumbnailSize = 96.0f;

        float panelWidth = ImGui::GetContentRegionAvail().x - ImGui::GetCurrentWindow()->ScrollbarSizes.x;
        int columnCount = (int)(panelWidth / thumbnailSize);
        if (columnCount < 1)
            columnCount = 1;

        ImGui::Columns(columnCount, 0, false);

        for (auto& [id, item] : currentDirectoryAssetItems)
        {
            // アイテム描画
            item->Render(this, { thumbnailSize, thumbnailSize });
        }

        // ディレクトリ移動要求時に移動する
        if (moveRequestDirectoryAssetID != 0)
        {
            auto& node = directories[moveRequestDirectoryAssetID];
            ChangeDirectory(node);
            moveRequestDirectoryAssetID = 0;
        }

        // 削除要求のアセットを、アセット参照リストから削除
        currentDirectoryAssetItems.erase(deleteRequestItemAssetID);
        deleteRequestItemAssetID = 0;
    }

    void AssetBrowserPanel::DrawMaterial()
    {
        if (!selectAsset)
            return;

        if (selectAsset->GetAssetType() != AssetType::Material)
            return;

        const float windowWidth = ImGui::GetWindowWidth();
        const float offset      = ImGui::GetCurrentWindow()->WindowPadding.x;
        const float buttonWidth = 100;

        ImGui::Button("Standard", { (windowWidth * 0.75f) - offset, 25 });
        ImGui::SameLine(0.0f, 4.0f);
        
        if (ImGui::Button("Save", { windowWidth * 0.25f - offset - 4.0f, 25}))
        {
            AssetSerializer<MaterialAsset>::Serialize(selectAsset.As<MaterialAsset>(), selectAsset->GetFilePath());
        }

        ImGui::Separator();

        Ref<MaterialAsset> material = selectAsset.As<MaterialAsset>();

        //-----------------------------------------------------
        // OpenGL　実装だったので、TextureID を渡すことを期待しているが
        // Vulkan 実装では、デスクリプターセットを渡す必要がある
        // 詳細は vulkanGUI に記載
        //-----------------------------------------------------
        // uint32 albedoTextureThumnail = material->Get()->AlbedoMap? material->Get()->AlbedoMap->Get()->GetID() : 0;
        // DescriptorSet* albedoTextureThumnail = material->Get()->AlbedoMap? material->Get()->AlbedoMap->Get()->GetID() : 0;

        ImGui::Dummy({ 0, 4.0f });
        ImGui::Columns(2);
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;

        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, lineHeight * 0.5f));

            // if (ImGui::ImageButton((ImTextureID)albedoTextureThumnail, { lineHeight * 0.8f, lineHeight * 0.8f }, { 0, 0 }, { 1, 1 }, 1))
            //      ImGui::OpenPopup("##AlbedoPopup");

            if (ImGui::BeginPopup("##AlbedoPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
            {
                AssetID current = {};
                bool selected = false;
                bool modified = false;

                if (ImGui::BeginListBox("##AlbedoPopup", ImVec2(500.f, 0.0f)))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));

                    auto& database = AssetManager::Get()->GetAllAssets();
                    for (auto& [id, asset] : database)
                    {
                        if (asset == nullptr || !asset->IsAssetOf(AssetType::Texture))
                            continue;

                        selected = (current == id);
                        if (ImGui::Selectable(asset->GetName().c_str(), selected))
                        {
                            current  = id;
                            modified = true;

                            material->Get()->albedoMap = asset.As<Texture2DAsset>();
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


            ImGui::SameLine(); ImGui::Dummy({ 8.0f, 0.0f }); ImGui::SameLine();

            ImGui::Text("Albedo");
            ImGui::Text("Emission");
            ImGui::Text("Metallic");
            ImGui::Text("Roughness");
            ImGui::Text("TextureTiling");
            ImGui::Text("ShadingModel");

            ImGui::PopStyleVar();
        }

        ImGui::NextColumn();

        {
            ImGui::PushItemWidth(ImGui::GetColumnWidth(1) - 10);

            ImGui::PushID("Albedo");        ImGui::ColorEdit3("",  glm::value_ptr(material->Get()->albedo));              ImGui::PopID();
            ImGui::PushID("Emission");      ImGui::ColorEdit3("",  glm::value_ptr(material->Get()->emission));            ImGui::PopID();
            ImGui::PushID("Metallic");      ImGui::SliderFloat("", &material->Get()->metallic, 0.0f, 1.0f);               ImGui::PopID();
            ImGui::PushID("Roughness");     ImGui::SliderFloat("", &material->Get()->roughness, 0.0f, 1.0f);              ImGui::PopID();
            ImGui::PushID("TextureTiling"); ImGui::DragFloat2("",  glm::value_ptr(material->Get()->textureTiling), 0.1f); ImGui::PopID();

            ImGui::PushID("ShadingModel");

            struct ShadingModelLabel
            {
                ShadingModelType model;
                const char*      name;
            };

            std::array<ShadingModelLabel, 2> itemList;
            itemList[0] = {ShadingModelType::Lit,   "Lit"};
            itemList[1] = {ShadingModelType::Unlit, "Unlit"};

            static const char* s_currentItem = itemList[material->Get()->shadingModel].name;

            if (ImGui::BeginCombo("", s_currentItem))
            {
                for (int i = 0; i < itemList.size(); ++i)
                {
                    const bool isSelected = (s_currentItem == itemList[i].name);

                    if (ImGui::Selectable(itemList[i].name, isSelected))
                    {
                        s_currentItem = itemList[i].name;
                        material->Get()->shadingModel = itemList[i].model;
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopID();
        }

        ImGui::Columns(1);
        ImGui::Dummy({ 0, 4.0f });
    }

    void AssetBrowserPanel::LoadAssetIcons()
    {
        AssetMetadata meta = AssetManager::Get()->GetMetadata("Assets/Editor/Directory.png");
        directoryIcon = AssetManager::Get()->GetAssetAs<Texture2DAsset>(meta.id);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/Material.png");
        assetIcons[AssetType::Material] = AssetManager::Get()->GetAssetAs<Texture2DAsset>(meta.id);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/Scene.png");
        assetIcons[AssetType::Scene] = AssetManager::Get()->GetAssetAs<Texture2DAsset>(meta.id);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/Texture.png");
        assetIcons[AssetType::Texture] = AssetManager::Get()->GetAssetAs<Texture2DAsset>(meta.id);

        meta = AssetManager::Get()->GetMetadata("Assets/Editor/File.png");
        const auto& icon = AssetManager::Get()->GetAssetAs<Texture2DAsset>(meta.id);
        assetIcons[AssetType::None]        = icon;
        assetIcons[AssetType::Environment] = icon;
        assetIcons[AssetType::Mesh]        = icon;
    }
}
