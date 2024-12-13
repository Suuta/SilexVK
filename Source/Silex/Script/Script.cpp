
#include "PCH.h"
#include "Core/Ref.h"
#include "Script/Script.h"
#include "Script/ScriptLibrary.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>


namespace Silex
{
    namespace Util
    {
        static char* ReadBytes(const std::string& assemblyPath, uint32* out_fileSize)
        {
            std::ifstream stream(assemblyPath, std::ios::binary | std::ios::ate);
            if (!stream)
            {
                return nullptr;
            }

            std::streampos end = stream.tellg();
            stream.seekg(0, std::ios::beg);
            uint32 fileSize = end - stream.tellg();
            if (fileSize == 0)
            {
                return nullptr;
            }

            char* fileData = new char[fileSize];
            stream.read(fileData, fileSize);
            stream.close();

            *out_fileSize = fileSize;
            return fileData;
        }
    }


    struct ScriptManagerData
    {
        MonoDomain*   rootDomain        = nullptr;
        MonoDomain*   applicationDomain = nullptr;
        MonoAssembly* coreAssembly      = nullptr;
        MonoImage*    coreAssemblyImage = nullptr;

        std::unordered_map<std::string, Ref<ScriptClass>> entityClasses;

        Scene* currentScene = nullptr;
    };

    static ScriptManagerData* data = nullptr;


    void ScriptManager::Initialize()
    {
        data = slnew(ScriptManagerData);

        // JIT 初期化
        _InitMono();
        
        // スクリプトをロード
        _LoadScriptAssembly("Resources/Script.dll");

        // アセンブリデータからクラス情報を取得
        _RefrectAssemblyMetadata(data->coreAssembly);

        // internal_call 関数を追加
        ScriptLibrary::RegisterFunctions();


#define TEST 0
#if TEST
        // クラス取得
        data->entityClass = ScriptClass("Silex", "Main");

        // インスタンス化（コンストラクタ）
        MonoObject* instance = data->entityClass.Instanciate();

        // 関数呼び出し
        MonoMethod* method = data->entityClass.GetMethod("Print", 0);
        data->entityClass.InvokeMethod(instance, method);

        // 関数呼び出し(引数あり)
        int v = 111;
        void* param[1] = { &v};
        MonoMethod* methodParam = data->entityClass.GetMethod("Print", 1);
        data->entityClass.InvokeMethod(instance, methodParam, param);

        // 関数呼び出し（文字列引数）
        MonoString* string = mono_string_new(data->rootDomain, "hello world");
        void* message[1] = { string };
        MonoMethod* methodString = data->entityClass.GetMethod("Print", 1);
        data->entityClass.InvokeMethod(instance, methodString, message);
#endif
    }

    void ScriptManager::Finalize()
    {
        mono_jit_cleanup(data->rootDomain);
        data->rootDomain = nullptr;

        // mono_jit_cleanup(data->rootDomain);
        // data->rootDomain = nullptr;

        data->entityClasses.clear();
        sldelete(data);
    }

    const std::unordered_map<std::string, Ref<ScriptClass>>& ScriptManager::GetAllEntityClasses()
    {
        return data->entityClasses;
    }

    void ScriptManager::OnRuntimeStart(Scene* scene)
    {
        data->currentScene = scene;
    }

    void ScriptManager::OnRuntimeUpdate()
    {
    }

    void ScriptManager::OnRuntimeStop()
    {
        data->currentScene = nullptr;
    }

    void ScriptManager::_InitMono()
    {
        // mono ランタイムの設定
        mono_set_assemblies_path("Resources");
        MonoDomain* root = mono_jit_init("JIT");
        data->rootDomain = root;
    }

    void ScriptManager::_LoadScriptAssembly(const std::string& assemblyPath)
    {
        uint32 fileSize = 0;
        char* fileData = Util::ReadBytes(assemblyPath, &fileSize);

        MonoImageOpenStatus status;
        MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);
        if (status != MONO_IMAGE_OK)
        {
            const char* errorMessage = mono_image_strerror(status);
            return;
        }

        // ドメイン設定
        // data->applicationDomain = mono_domain_create_appdomain(const_cast<char*>("Script"), nullptr);
        // mono_domain_set(data->applicationDomain, true);

        // アセンブリ・イメージ取得
        data->coreAssembly      = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, 0);
        data->coreAssemblyImage = mono_assembly_get_image(data->coreAssembly);

        mono_image_close(image);
        delete[] fileData;
    }

    // スクリプトアセンブリからメタデータ出力
    void ScriptManager::_RefrectAssemblyMetadata(MonoAssembly* assembly)
    {
        // Entity クラスリストのクリア
        data->entityClasses.clear();

        // アセンブリからイメージを取得
        MonoImage* image               = mono_assembly_get_image(assembly);
        const MonoTableInfo* tableInfo = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
        uint32 numTypes                = mono_table_info_get_rows(tableInfo);

        //=====================================================================
        // 継承するべき super クラス
        //---------------------------------------------------------------------
        // unity の MonoBehaviour のような扱いをし、このクラスを継承するクラスが
        // Silex の シーンコンポーネントとして扱われ、 Start() Update() End() 等の
        // イベント関数のリスナーになる。
        //=====================================================================
        MonoClass* superClass = mono_class_from_name(image, "Silex", "Entity");


        for (uint32 i = 0; i < numTypes; i++)
        {
            // メタデータを取得
            uint32 cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(tableInfo, i, cols, MONO_TYPEDEF_SIZE);

            // 名前空間・クラス名取得
            const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
            const char* nameClass = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
            MonoClass* subClass   = mono_class_from_name(image, nameSpace, nameClass);

            // super クラス自身は除外する
            if (subClass == superClass)
                continue;

            // super クラス(Entity) を継承している サブクラスを検知する
            if (mono_class_is_subclass_of(subClass, superClass, false))
            {
                // 名前空間が存在すれば、{名前空間}.{クラス名} 形式の文字列をキーにする
                std::string fullname = (std::strlen(nameSpace) != 0)? std::format("{}.{}", nameSpace, nameClass): nameClass;
                data->entityClasses[fullname] = CreateRef<ScriptClass>(nameSpace, nameClass);
            }

            SL_LOG_DEBUG("{}.{}", nameSpace, nameClass);
        }
    }

    MonoObject* ScriptManager::_InstanciateClass(MonoClass* monoClass)
    {
        // インスタンス化（コンストラクタ）
        MonoObject* instance = mono_object_new(data->rootDomain, monoClass);
        mono_runtime_object_init(instance);

        return instance;
    }

    // クラス内の関数を列挙する
    void ScriptManager::_EnumrateClassFunctions(MonoClass* monoClass)
    {
        // 全関数取得
        void* iter = nullptr;
        MonoMethod* m;
        while (m = mono_class_get_methods(monoClass, &iter))
        {
            const char* name = mono_method_get_name(m);

            // シグネチャを取得
            MonoMethodSignature* sig = mono_method_signature(m);
            MonoType* ret_type  = mono_signature_get_return_type(sig);
            char* ret_type_name = mono_type_get_name(ret_type);
            uint32 param_count  = mono_signature_get_param_count(sig);

            // 引数の型を取得
            std::string param_str = "";
            void* param_iter = nullptr;
            MonoType* param_type;
            while ((param_type = mono_signature_get_params(sig, &param_iter)) != nullptr)
            {
                char* param_type_name = mono_type_get_name(param_type);
                param_str.append(param_type_name);
                param_str.append(" ");
                mono_free(param_type_name);
            }

            if (param_str.empty()) param_str.append("---");
            SL_LOG_DEBUG("Name: {:<20} | return: {:<20} | param({}): {}", name, ret_type_name, param_count, param_str);

            mono_free(ret_type_name);
        }
    }



    ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className)
        : classNamespace(classNamespace)
        , className(className)
    {
        monoClass = mono_class_from_name(data->coreAssemblyImage, classNamespace.c_str(), className.c_str());
    }

    MonoObject* ScriptClass::Instanciate()
    {
        return ScriptManager::_InstanciateClass(monoClass);
    }

    MonoMethod* ScriptClass::GetMethod(const std::string& name, int paramCount)
    {
        return mono_class_get_method_from_name(monoClass, name.c_str(), paramCount);
    }

    MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
    {
        return mono_runtime_invoke(method, instance, params, nullptr);
    }
}
