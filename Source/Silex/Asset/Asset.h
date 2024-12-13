
#pragma once

#include "Core/Core.h"
#include "Core/Random.h"
#include "Asset/AssetImporter.h"
#include "Asset/AssetCreator.h"


namespace Silex
{
    class Mesh;
    class Material;
    class Texture2D;
    class Environment;

    using AssetID = uint64;

    enum class AssetType : uint32
    {
        Scene       = SL_BIT(0),
        Mesh        = SL_BIT(1),
        Texture     = SL_BIT(2),
        Material    = SL_BIT(3),
        Environment = SL_BIT(4),

        None = 0,
    };

    struct AssetMetadata
    {
        AssetID               id;
        AssetType             type;
        std::filesystem::path path;
    };

    class Asset : public Object
    {
        SL_CLASS(Asset, Object)

    public:

        bool IsAssetOf(AssetType flag)  { return assetFlag == flag; }
        bool operator==(AssetType flag) { return assetFlag == flag; }
        bool operator!=(AssetType flag) { return assetFlag != flag; }

        // フラグ
        void      SetAssetType(AssetType flag) { assetFlag = flag; }
        AssetType GetAssetType()               { return assetFlag; }

        // ファイルパス
        const std::string& GetFilePath() const                  { return assetFilePath; }
        void               SetFilePath(const std::string& path) { assetFilePath = path; }

        // ID
        AssetID GetAssetID() const     { return assetID; }
        void    SetAssetID(AssetID id) { assetID = id;   }

        // 名前
        void               SetName(const std::string& name) { assetName = name; }
        const std::string& GetName() const                  { return assetName; }

        // プロパティ設定
        void SetupAssetProperties(const std::string& filePath, AssetType flag)
        {
            SetAssetType(flag);
            SetFilePath(filePath);

            std::filesystem::path path(filePath);
            SetName(path.stem().string());
        }

        static AssetType FileNameToAssetType(const std::filesystem::path& filePath)
        {
            std::string extention = filePath.extension().string();

            if      (extention == ".slmt")                       return AssetType::Material;
            else if (extention == ".slsc")                       return AssetType::Scene;
            else if (extention == ".fbx" || extention == ".obj") return AssetType::Mesh;
            else if (extention == ".png" || extention == ".jpg") return AssetType::Texture;

            // 環境マップは拡張子ではなく、マテリアルと同様な扱いに変更。ただし、シリアライズ化(.slenv) は未実装
            //else if (extention == ".hdr") return AssetType::Environment;

            return AssetType::None;
        }

    protected:

        AssetID     assetID        = 0;
        AssetType   assetFlag      = AssetType::None;
        std::string assetFilePath  = {};
        std::string assetName      = {};
    };


    class MeshAsset : public Asset
    {
    public:

        SL_CLASS(MeshAsset, Asset)

        MeshAsset();
        MeshAsset(Mesh* asset);
        ~MeshAsset();

        Mesh* Get() const      { return mesh;  }
        void  Set(Mesh* asset) { mesh = asset; }

    private:

        Mesh* mesh;
    };

    class MaterialAsset : public Asset
    {
    public:

        SL_CLASS(MaterialAsset, Asset)

        MaterialAsset();
        MaterialAsset(Material* asset);
        ~MaterialAsset();

        Material* Get() const          { return material;  }
        void      Set(Material* asset) { material = asset; }

    private:

        Material* material;
    };

    class Texture2DAsset : public Asset
    {
    public:

        SL_CLASS(Texture2DAsset, Asset)

        Texture2DAsset();
        Texture2DAsset(Texture2D* asset);
        ~Texture2DAsset();

        Texture2D* Get() const           { return texture;  }
        void       Set(Texture2D* asset) { texture = asset; }

    private:

        Texture2D* texture;
    };


    class EnvironmentAsset : public Asset
    {
    public:

        SL_CLASS(EnvironmentAsset, Asset)

        EnvironmentAsset();
        EnvironmentAsset(Environment* asset);
        ~EnvironmentAsset();

        Environment* Get() const             { return environment;  }
        void         Set(Environment* asset) { environment = asset; }

    private:

        Environment* environment;
    };







    class AssetManager
    {
    public:

        static void Initialize();
        static void Finalize();
        static AssetManager* Get();

    public:

        //=================================
        // アセットID
        //=================================
        uint64 GenerateAssetID();
        bool IsValidID(AssetID id);
        bool IsBuiltInAssetID(AssetID id);

        //=================================
        // メタデータ
        //=================================
        bool          IsExistInMetadata(const std::filesystem::path& directory);
        AssetMetadata GetMetadata(const std::filesystem::path& directory);
        AssetMetadata GetMetadata(AssetID id);

        std::unordered_map<AssetID, AssetMetadata>& GetMetadatas();

        //=================================
        // アセット
        //=================================
        bool IsLoaded(const AssetID id);
        std::unordered_map<AssetID, Ref<Asset>>& GetAllAssets();

        template<class T>
        Ref<T> GetAssetAs(const AssetID id)
        {
            return assetData[id].As<T>();
        }

        Ref<Asset> GetAsset(const AssetID id)
        {
            return assetData[id];
        }

        template<class T, class... Args>
        Ref<T> CreateAsset(const std::filesystem::path& directory, Args&&... args)
        {
            //　現状はマテリアルのみサポート (インポートと生成との意味合いが混同しているため)
            static_assert(Traits::IsSame<MaterialAsset, T>() && Traits::IsBaseOf<Asset, T>());

            AssetMetadata metadata = instance->_AddToMetadata(directory);
            Ref<T> asset = AssetCreator::Create<T>(directory, Traits::Forward<Args>(args)...);

            instance->_AddToAssetAndID(metadata.id, asset);
            instance->_WriteDatabaseToFile(assetDatabasePath);

            return asset;
        }

        void DeleteAsset(const AssetID id)
        {
            AssetMetadata data = instance->GetMetadata(id);
            std::string path   = data.path.string();

            // アセットファイルを削除
            std::remove(path.c_str());

            // リストから削除
            instance->_RemoveFromMetadata(id);
            instance->_RemoveFromAsset(id);

            // シリアライズ
            instance->_WriteDatabaseToFile(assetDatabasePath);
        }

        template<typename T>
        Ref<Asset> LoadAssetFromFile(const std::string& filePath)
        {
            static_assert(Traits::IsBaseOf<Asset, T>());

            return AssetImporter::Import<T>(filePath);
        }

    private:

        template<class T>
        void _LoadBuiltinAsset(const std::string& filePath)
        {
            currentBuiltinAssetCount++;
            AssetType type = Asset::FileNameToAssetType(filePath);

            std::filesystem::path assetName = filePath;
            std::string name = assetName.stem().string();

            Ref<T> asset = AssetImporter::Import<T>(filePath);
            asset->SetAssetType(type);
            asset->SetName(name);
            instance->_AddToAssetAndID(currentBuiltinAssetCount, asset);

            AssetMetadata md = {};
            md.id   = currentBuiltinAssetCount;
            md.path = filePath;
            md.type = type;

            instance->metadata[currentBuiltinAssetCount] = md;
        }


        // ビルトインアセット (レンダラーと密な依存関係あり)
        void _CreateBuiltinAssets();
        void _DestroyBuiltinAssets();

        // アセットデータベースファイルからメタデータを読み込む
        void _LoadAssetMetaDataFromDatabaseFile(const std::filesystem::path& filePath);

        // 物理ファイルとメタデータと照合しながら、アセットディレクトリ全体を走査
        void _InspectAssetDirectory(const std::filesystem::path& directory);
        void _InspectRecursive(const std::filesystem::path& directory);

        // アセット・メタデータ追加
        AssetMetadata _AddToMetadata(const std::filesystem::path& directory);
        void          _AddToAssetAndID(const AssetID id, Ref<Asset> asset);
        void          _AddToAsset(Ref<Asset> asset);

        // アセット・メタデータ削除
        void _RemoveFromMetadata(const AssetID id);
        void _RemoveFromAsset(const AssetID id);

        // アセットデータベースファイルにメタデータを書き込む
        void _WriteDatabaseToFile(const std::filesystem::path& directory);

        // メモリにアセットをロードする
        void _LoadAssetToMemory(const std::filesystem::path& filePath);

    private:

        uint32 currentBuiltinAssetCount  = 0;
        const uint32 reservedBuiltinAssetCount = 256;

        std::unordered_map<AssetID, Ref<Asset>>    assetData;
        std::unordered_map<AssetID, AssetMetadata> metadata;

        static inline const char* assetDatabasePath = "Assets/AssetDatabase.meta";
        static inline const char* assetDiectoryPath = "Assets";

        static inline AssetManager* instance;
    };
}
