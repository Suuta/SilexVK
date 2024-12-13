
#include "PCH.h"
#include "Script/ScriptLibrary.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>


namespace Silex
{
#define SL_MONO_ADD_INTERNAL_FUNCTION(name) mono_add_internal_call(SL_TEXT(Silex.InternalCall::##name), name);


    static void NativeLog_String(MonoString* string)
    {
        char* str = mono_string_to_utf8(string);
        SL_LOG_DEBUG("{}", str);
        mono_free(str);
    }

    static void NativeLog_Vector3(glm::vec3* value, glm::vec3* out)
    {
        SL_LOG_DEBUG("{}, {}, {}", value->x, value->y, value->z);

        glm::vec3 v(value->x, value->y, value->z);
        *out = (v *= 2);

    }

    void ScriptLibrary::RegisterFunctions()
    {
        SL_MONO_ADD_INTERNAL_FUNCTION(NativeLog_String);
        SL_MONO_ADD_INTERNAL_FUNCTION(NativeLog_Vector3);
    }
}

