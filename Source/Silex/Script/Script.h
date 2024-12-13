
#pragma once
#include "Core/Core.h"

extern "C"
{
    typedef struct _MonoClass    MonoClass;
    typedef struct _MonoObject   MonoObject;
    typedef struct _MonoMethod   MonoMethod;
    typedef struct _MonoAssembly MonoAssembly;
}

namespace Silex
{
    class ScriptClass : public Object
    {
        SL_CLASS(ScriptClass, Class)
    public:

        ScriptClass()  = default;
        ~ScriptClass() = default;
        ScriptClass(const std::string& classNamespace, const std::string& className);

        MonoObject* Instanciate();
        MonoMethod* GetMethod(const std::string& name, int paramCount);
        MonoObject* InvokeMethod(MonoObject* instance, MonoMethod* method, void** params = nullptr);

    private:

        std::string classNamespace;
        std::string className;
        MonoClass*  monoClass = nullptr;
    };

    class ScriptManager
    {
    public:

        static void Initialize();
        static void Finalize();

        static const std::unordered_map<std::string, Ref<ScriptClass>>& GetAllEntityClasses();

        static void OnRuntimeStart(class Scene* scene);
        static void OnRuntimeUpdate();
        static void OnRuntimeStop();

    private:

        static void        _InitMono();
        static void        _LoadScriptAssembly(const std::string& assemblyPath);
        static void        _RefrectAssemblyMetadata(MonoAssembly* assembly);
        static MonoObject* _InstanciateClass(MonoClass* monoClass);
        static void        _EnumrateClassFunctions(MonoClass* monoClass);

        friend class ScriptClass;
    };
}

