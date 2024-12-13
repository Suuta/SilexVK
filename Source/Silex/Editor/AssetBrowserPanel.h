#pragma once

#include "Asset/Asset.h"


namespace Silex
{
    class Texture2D;
    class AssetBrowserPanel;

    enum class AssetItemType : uint8
    {
        Directory,
        Asset,
    };

    class AssetBrowserItem : public Object
    {
    public:

        AssetBrowserItem(AssetItemType type, AssetID id, const std::string& name)
            : type(type)
            , id(id)
            , fileName(name)
        {
        }

        AssetBrowserItem(AssetItemType type, AssetID id, const std::string& name, const Ref<Texture2DAsset>& icon)
            : type(type)
            , id(id)
            , fileName(name)
            , icon(icon)
        {
        }

        void Render(AssetBrowserPanel* panel, const glm::vec2& size);

        AssetID            GetID()   const { return id; }
        AssetItemType      GetType() const { return type; }
        const std::string& GetName() const { return fileName; }

        const Ref<Texture2DAsset>& GetIcon() const  { return icon; }
        void SetIcon(const Ref<Texture2DAsset>& ic) { icon = ic;   }

    protected:

        AssetItemType       type;
        AssetID             id;
        std::string         fileName;
        Ref<Texture2DAsset> icon;
    };

    struct DirectoryNode : public Object
    {
        SL_CLASS(DirectoryNode, Object)

        Ref<DirectoryNode>                              parentDirectory;
        std::unordered_map<AssetID, Ref<DirectoryNode>> childDirectory;

        AssetID               ID;
        std::filesystem::path filePath;
        std::vector<AssetID>  assets;
    };


    class AssetBrowserPanel
    {
    public:

        AssetBrowserPanel()  = default;
        ~AssetBrowserPanel() = default;

        void Initialize();
        void Finalize();
        void Render(bool* show, bool* showProperty);

    private:

        AssetID TraversePhysicalDirectories(const std::filesystem::path& directory, const Ref<DirectoryNode>& parentDirectory);
        void ChangeDirectory(const Ref<DirectoryNode>& directory);

        void DrawDirectory(const Ref<DirectoryNode>& node);
        void DrawCurrentDirectoryAssets();

        void DrawMaterial();
        void LoadAssetIcons();

    private:

        AssetID                                            moveRequestDirectoryAssetID;
        AssetID                                            deleteRequestItemAssetID;
        Ref<Asset>                                         selectAsset;
        Ref<DirectoryNode>                                 currentDirectory;
        Ref<DirectoryNode>                                 rootDirectory;
        std::unordered_map<AssetID, Ref<AssetBrowserItem>> currentDirectoryAssetItems;
        std::unordered_map<AssetID, Ref<DirectoryNode>>    directories;

        std::unordered_map<AssetType, Ref<Texture2DAsset>> assetIcons;
        Ref<Texture2DAsset>                                directoryIcon;

    private: 

        friend class AssetBrowserItem;
    };
}

