
#include "PCH.h"

#include "Core/Random.h"
#include "Asset/Asset.h"
#include "Editor/EditorSplashImage.h"
#include "Rendering/Mesh.h"
#include "Rendering/Environment.h"
#include "Rendering/RenderingStructures.h"
#include "Rendering/Renderer.h"
#include "Serialize/AssetSerializer.h"

#include <yaml-cpp/yaml.h>
#include <numbers>

namespace Silex
{
    //==================================================================================
    // ①アセットデータベースファイルの有無確認
    //----------------------------------------------------------------------------------
    // ＜存在しない＞
    // アセットディレクトリを再帰検索し、すべてのメタデータをデータベースに登録する（IDを生成する）
    // この時点でシーンファイル自体は存在している場合、（データベースファイルのみ存在しない）シーンが参照
    // するアセットIDとデータベースファイルのアセットIDは異なる（IDが新規生成されるため）
    // 
    // ＜存在する＞
    // ファイルからメタデータを読み込んでデータベースに登録する
    // 
    //----------------------------------------------------------------------------------
    // ②メタデータをデータベースファイルに書き込み
    //----------------------------------------------------------------------------------
    // 
    //----------------------------------------------------------------------------------
    // ③任意（未定）タイミングでアセットをメモリに乗せる
    //----------------------------------------------------------------------------------
    // アセットデータベースとアセットIDが異なる場合は読み込まない（AssetFlag::Missing）
    //
    //----------------------------------------------------------------------------------
    // ③シーンの保存
    //----------------------------------------------------------------------------------
    // 読み込まれていないアセットは（AssetFlag::Missing）
    // 再度プロパティから設定し、シーンの保存を行えば、そのIDでシーンファイル内のIDが保存されるので
    // 次回読み込み時は、そのデータベースファイルから参照が成功する
    // 
    //----------------------------------------------------------------------------------
    // ④エディター終了時
    //----------------------------------------------------------------------------------
    // 現在のデータベースをファイルに書き込む
    // 現在のIDを書き込むため、新規データベースファイルを使った次回起動時に正しく読み込まれない場合がある
    // 参照するアセットIDに不整合やシーンの保存・再適応をしない限りアセットはMissingのままであり、
    // 読み込みはスキップされる
    //
    //==================================================================================
    // 検討中: アセットを実際にメモリに乗せるタイミングは　シーン読み込み時 / エディター起動時 ？
    //==================================================================================


    MeshAsset::MeshAsset() {}
    MeshAsset::MeshAsset(Mesh* asset) : mesh(asset) {}
    MeshAsset::~MeshAsset() { sldelete(mesh); }

    MaterialAsset::MaterialAsset() {}
    MaterialAsset::MaterialAsset(Material* asset) : material(asset) {}
    MaterialAsset::~MaterialAsset() { sldelete(material); }

    Texture2DAsset::Texture2DAsset() {}
    Texture2DAsset::Texture2DAsset(Texture2D* asset) : texture(asset) {}
    Texture2DAsset::~Texture2DAsset() { Renderer::Get()->DestroyTexture(texture); }

    EnvironmentAsset::EnvironmentAsset() {}
    EnvironmentAsset::EnvironmentAsset(Environment* asset) : environment(asset) {}
    EnvironmentAsset::~EnvironmentAsset() { sldelete(environment); }




    AssetManager* AssetManager::Get()
    {
        return instance;
    }

    void AssetManager::Initialize()
    {
        SL_ASSERT(instance == nullptr)
        instance = slnew(AssetManager);

        // ビルトインデータ(ID: 1 - 256 に割り当て)
        // アセットデータベースには登録されないが（シリアライズされず、ランタイムのみ存在するメモリオンリーアセット）
        // メタデータとしては登録され、ランタイム中のみ情報の取得が可能
        instance->_CreateBuiltinAssets();


        if (std::filesystem::exists(assetDatabasePath))
        {
            // データベースからメタデータを取得
            instance->_LoadAssetMetaDataFromDatabaseFile(assetDatabasePath);
        }

        // 物理ファイルとメタデータと照合しながらアセットディレクトリ全体を走査
        instance->_InspectAssetDirectory(assetDiectoryPath);

        // データベースファイルのメタデータを更新する
        instance->_WriteDatabaseToFile(assetDatabasePath);

        // メタデータを元に実際にアセットをメモリにロードする
        instance->_LoadAssetToMemory(assetDatabasePath);
    }

    void AssetManager::Finalize()
    {
        // データベースファイルのメタデータを更新する
        instance->_WriteDatabaseToFile(assetDatabasePath);

        instance->_DestroyBuiltinAssets();

        if (instance)
        {
            sldelete(instance);
        }
    }

    bool AssetManager::IsExistInMetadata(const std::filesystem::path& directory)
    {
        for (auto& [id, metadata] : metadata)
        {
            if (metadata.path == directory)
                return true;
        }

        return false;
    }

    AssetMetadata AssetManager::GetMetadata(const std::filesystem::path& directory)
    {
        for (auto& [id, metadata] : metadata)
        {
            if (metadata.path == directory)
                return metadata;
        }

        return {};
    }

    bool AssetManager::IsLoaded(const AssetID id)
    {
        return assetData.contains(id);
    }

    AssetMetadata AssetManager::GetMetadata(AssetID id)
    {
        return metadata[id];
    }

    std::unordered_map<AssetID, Ref<Asset>>& AssetManager::GetAllAssets()
    {
        return assetData;
    }

    std::unordered_map<AssetID, AssetMetadata>& AssetManager::GetMetadatas()
    {
        return metadata;
    }

    uint64 AssetManager::GenerateAssetID()
    {
        // ビルトインアセット予約済み: 1 ~ 256
        // 無効なアセットID        : 0
        return Random<uint64>::Range(reservedBuiltinAssetCount + 1, std::numeric_limits<uint64>::max());
    }

    bool AssetManager::IsBuiltInAssetID(AssetID id)
    {
        // 1 ~ 256
        return 1 <= id && id <= reservedBuiltinAssetCount;
    }

    bool AssetManager::IsValidID(AssetID id)
    {
        return id != 0 && !IsBuiltInAssetID(id);
    }

    void AssetManager::_CreateBuiltinAssets()
    {
        //---------------------------------------------------------------------------------------------------
        // 従来のバージョンではビルトインアセットはメモリーオンリー（シリアライズされない・物理ファイルが存在しない）なので
        // レンダラーで生成されたものを割り当てていたが、今回はビルトインアセットとして物理ファイルを扱う必要が発生した。
        // 物理ファイルが必要なのは、vulkanパイプラインでプリミティブ型が静的になり、メッシュ間で統一する必要があることに起因する
        // "TORIANGLE_LIST" で統一することにしたが、ランタイム生成の場合、"球" データを "TORIANGLE_LIST" で計算する方法が
        // わからなかったので、DCCツールから出力された データを使用している。すべての基本プリミティブのデータを "TORIANGLE_LIST"
        // 形式のデータで生成できるようになれば、外部データは不要になる。
        // 
        // <IsBuiltInAssetID 関数によるチェックで>
        // _AddToMetadata 関数で新規追加されない
        // _WriteDatabaseToFile 関数でシリアライスの対象にはならない
        // _LoadAssetToMemory 関数でロードされない（多重ロードのため）
        //---------------------------------------------------------------------------------------------------

        _LoadBuiltinAsset<Texture2DAsset>("Assets/Editor/WhiteTexture.png");
        _LoadBuiltinAsset<Texture2DAsset>("Assets/Editor/CheckerboardTexture.png");
        _LoadBuiltinAsset<MeshAsset>("Assets/Editor/Cube.fbx");
        _LoadBuiltinAsset<MeshAsset>("Assets/Editor/Sphere.fbx");
        _LoadBuiltinAsset<MaterialAsset>("Assets/Editor/DefaultMaterial.slmt");
    }

    void AssetManager::_DestroyBuiltinAssets()
    {
        //=================================================================================
        // アセットは参照カウンタでラップするから、マネージャー解放時に全アセットが自動解放されるようになる
        //
        // ・各アセットは、リソースクラスをデストラクタで開放するようになっているので、各リソースが内部で
        //   レンダーAPI の Destroy関数を呼ぶとよい。
        //   Ref<Texture2DAsset>
        //      -> ~Texture2DAsset() -> delete (Texture2D)
        //      -> ~Texture2D()      -> Renderer::Get()->DestroyTexture()
        // 
        // ✔ レンダーAPI（Destroy系）が実装済みなので、統一するためにも、破棄はRHIレイヤーで行うべき
        //   Ref<Texture2DAsset>
        //      -> ~Texture2DAsset() -> Renderer::Get()->DestroyTexture()
        //=================================================================================
#if 0
        const auto& white = GetAssetAs<Texture2DAsset>(1);
        Renderer::Get()->DestroyTexture(white->Get()->GetHandle());

        const auto& checker = GetAssetAs<Texture2DAsset>(2);
        Renderer::Get()->DestroyTexture(checker->Get()->GetHandle());

        const auto& cube = GetAssetAs<MeshAsset>(3);
        cube->Get();
#endif
    }

    void AssetManager::_LoadAssetMetaDataFromDatabaseFile(const std::filesystem::path& filePath)
    {
        // ファイル読み込み
        std::ifstream stream(filePath);
        SL_ASSERT(stream);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        auto IDs = data["AssetDatabase"];
        if (!IDs)
        {
            SL_LOG_ERROR("データベースファイルが破損しているか、存在しません");
            return;
        }

        // 各メタデータを走査し、エラーが無ければ登録
        for (auto n : IDs)
        {
            //NOTE:======================================
            // ここでデータベースファイルが存在していた場合に
            // メタデータをもとにアセットの読み込みを試みるが
            // 現状、アセットが変更・削除されている場合は無視する
            //===========================================

            AssetMetadata md;
            md.id   = n["id"].as<uint64_t>();
            md.type = (AssetType)n["type"].as<uint32>();
            md.path = n["path"].as<std::string>();

            // 物理ファイルが存在しない
            if (!std::filesystem::exists(md.path))
            {
                SL_LOG_ERROR("{}: が存在しません", md.path.string().c_str());
                continue;
            }

            // メタデータを登録
            AssetID id = md.id;
            metadata[id] = md;
        }
    }

    void AssetManager::_InspectAssetDirectory(const std::filesystem::path& directory)
    {
        _InspectRecursive(directory);
    }

    void AssetManager::_InspectRecursive(const std::filesystem::path& directory)
    {
        // データベースファイルを読み込んでメタデータを取得した場合は LoadAssetMetaDataFromFile 関数で追加済みだが、
        // データベースファイルが存在しなかった場合、このタイミングにAddToMetadata関数で登録される
        for (auto& dir : std::filesystem::directory_iterator(directory))
        {
            // データベースファイルは無視する
            if (dir.path() == assetDatabasePath)
                continue;

            if (dir.is_directory()) _InspectRecursive(dir.path());
            else                    _AddToMetadata(dir.path());
        }
    }

    void AssetManager::_AddToAssetAndID(const AssetID id, Ref<Asset> asset)
    {
        asset->SetAssetID(id);
        assetData[id] = asset;
    }

    void AssetManager::_AddToAsset(Ref<Asset> asset)
    {
        assetData[asset->GetAssetID()] = asset;
    }

    AssetMetadata AssetManager::_AddToMetadata(const std::filesystem::path& directory)
    {
        // ディレクトリ区切り文字変換
        std::string path = directory.string();
        std::replace(path.begin(), path.end(), '\\', '/');
        std::filesystem::path dir = path;

        // メタデータ内に存在するなら追加しない (前回読み込まれてシリアライズされたアセットや、ビルトインアセットは既に登録されている)
        if (IsExistInMetadata(dir))
            return {};

        // なければ新規登録
        AssetMetadata md;
        md.id   = GenerateAssetID();
        md.path = dir;
        md.type = Asset::FileNameToAssetType(dir);

        metadata[md.id] = md;
        return md;
    }

    void AssetManager::_RemoveFromMetadata(const AssetID id)
    {
        if (metadata.contains(id))
        {
            metadata.erase(id);
        }
    }

    void AssetManager::_RemoveFromAsset(const AssetID id)
    {
        if (assetData.contains(id))
        {
            assetData.erase(id);
        }
    }

    void AssetManager::_WriteDatabaseToFile(const std::filesystem::path& directory)
    {
        YAML::Emitter out;

        out << YAML::BeginMap << YAML::Key << "AssetDatabase";
        out << YAML::BeginSeq;

        for (auto& [id, metadata] : metadata)
        {
            // ビルトインアセットはシリアライズしない
            if (!IsBuiltInAssetID(metadata.id))
            {
                out << YAML::BeginMap;
                out << YAML::Key << "id"   << YAML::Value << metadata.id;
                out << YAML::Key << "type" << YAML::Value << (uint32)metadata.type;
                out << YAML::Key << "path" << YAML::Value << metadata.path.string();
                out << YAML::EndMap;
            }
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(directory);
        fout << out.c_str();
    }

    //===========================================================================
    // マテリアルがテクスチャに依存するので、アセットタイプごとにイテレーションするようにする
    // 必要があれば、アセットタイプごとのデータ群を返す関数を追加する
    //===========================================================================
    void AssetManager::_LoadAssetToMemory(const std::filesystem::path& filePath)
    {
        INIT_PROCESS("Load Texture", 20);

        // テクスチャ2D: マテリアルから参照されるので、最初に読み込むこと!
        for (auto& [aid, md] : metadata)
        {
            if (md.type == AssetType::Texture && !IsBuiltInAssetID(aid))
            {
                AssetID id       = md.id;
                std::string path = md.path.string();

                Ref<Asset> asset = nullptr;
                asset = LoadAssetFromFile<Texture2DAsset>(path);

                instance->_AddToAssetAndID(id, asset);
            }
        }

        INIT_PROCESS("Load EnvironmentMap", 40);

        // 環境マップ
        for (auto& [aid, md] : metadata)
        {
            if (md.type == AssetType::Environment && !IsBuiltInAssetID(aid))
            {
                AssetID id       = md.id;
                std::string path = md.path.string();

                Ref<Asset> asset = LoadAssetFromFile<EnvironmentAsset>(path);
                instance->_AddToAssetAndID(id, asset);
            }
        }

        INIT_PROCESS("Load Material", 60);

        // マテリアル
        for (auto& [aid, md] : metadata)
        {
            if (md.type == AssetType::Material && !IsBuiltInAssetID(aid))
            {
                AssetID id       = md.id;
                std::string path = md.path.string();

                Ref<Asset> asset = LoadAssetFromFile<MaterialAsset>(path);
                instance->_AddToAssetAndID(id, asset);
            }
        }

        INIT_PROCESS("Load Mesh", 80);

        // メッシュ
        for (auto& [aid, md] : metadata)
        {
            if (md.type == AssetType::Mesh && !IsBuiltInAssetID(aid))
            {
                AssetID id       = md.id;
                std::string path = md.path.string();

                Ref<Asset> asset = LoadAssetFromFile<MeshAsset>(path);
                instance->_AddToAssetAndID(id, asset);
            }
        }
    }
}
