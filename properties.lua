--==================================================
-- ソリューション
--==================================================
workspace "Silex"

    architecture "x64"
    flags        "MultiProcessorCompile"
    startproject "Silex"

    configurations
    {
        "Debug",
        "Release",
    }

--==================================================
-- C# スクリプトプロジェクト
--==================================================
project "Script"

    location        "Source/Script"
    language        "C#"
    kind            "SharedLib"
    dotnetframework "4.8"

    debugdir   "%{wks.location}"
    targetdir  "Binary/%{cfg.buildcfg}/"
    objdir     "Binary/%{cfg.buildcfg}/Intermediate"

    files
    {
        "Source/%{prj.name}/**.cs",
    }

    filter "configurations:Debug"

        optimize "Off"
        symbols  "Default"

    filter "configurations:Release"

        optimize "On"
        symbols  "Default"

--==================================================
-- C++ エディタープロジェクト
--==================================================
project "Silex"

    location      "Source/Silex"
    language      "C++"
    cppdialect    "C++20"
    staticruntime "on"
    characterset  "Unicode"

    debugdir   "%{wks.location}"
    targetdir  "Binary/%{cfg.buildcfg}/"
    objdir     "Binary/%{cfg.buildcfg}/Intermediate"

    pchheader "PCH.h"
    pchsource "Source/Silex/Core/PCH/PCH.cpp"

    -- 追加ファイル
    files
    {
        -----------------------------------
        -- Resources
        -----------------------------------
        "Resources/Resource.h",
        "Resources/Resource.rc",

        -----------------------------------
        -- Source
        -----------------------------------
        "Source/%{prj.name}/**.c",
        "Source/%{prj.name}/**.h",
        "Source/%{prj.name}/**.cpp",
        "Source/%{prj.name}/**.hpp",

        -----------------------------------
        -- External
        -----------------------------------
        -- GLFW
        "External/glfw/src/context.c",
        "External/glfw/src/egl_context.c",
        "External/glfw/src/init.c",
        "External/glfw/src/input.c",
        "External/glfw/src/monitor.c",
        "External/glfw/src/window.c",
        "External/glfw/src/null_init.c",
        "External/glfw/src/null_joystick.c",
        "External/glfw/src/null_monitor.c",
        "External/glfw/src/null_window.c",
        "External/glfw/src/osmesa_context.c",
        "External/glfw/src/platform.c",
        "External/glfw/src/vulkan.c",
        "External/glfw/src/wgl_context.c",

        "External/glfw/src/win32_init.c",
        "External/glfw/src/win32_joystick.c",
        "External/glfw/src/win32_module.c",
        "External/glfw/src/win32_monitor.c",
        "External/glfw/src/win32_time.c",
        "External/glfw/src/win32_thread.c",
        "External/glfw/src/win32_window.c",

        -- YAML-cpp
        "External/yaml-cpp/src/**.cpp",

        -- ImGui
        "External/imgui/imgui.cpp",
        "External/imgui/imgui_draw.cpp",
        "External/imgui/imgui_widgets.cpp",
        "External/imgui/imgui_tables.cpp",
        "External/imgui/imgui_demo.cpp",
        "External/imgui/backends/imgui_impl_glfw.cpp",
        "External/imgui/backends/imgui_impl_opengl3.cpp",
        "External/imgui/backends/imgui_impl_win32.cpp",
        "External/imgui/backends/imgui_impl_vulkan.cpp",

        -- ImGuizmo
        "External/imguizmo/ImGuizmo.cpp",
        
        -- Vulkan Memory Allocator
        "External/vulkan/vk_mem_alloc.cpp"
    }

    includedirs
    {
        "Source/%{prj.name}/",
        "Source/%{prj.name}/Core/PCH",
        "Resources",
        "External",
        "External/vulkan/include",
        "External/yaml-cpp/include",
        "External/glad/include",
        "External/glfw/include",
        "External/glm",
        "External/imgui",
        "External/assimp/include",
        "External/mono/include",
    }

    links
    {
        "External/vulkan/Lib/vulkan-1.lib",
    }

    defines
    {
        "YAML_CPP_STATIC_DEFINE",
    }

    buildoptions
    {
        "/wd4244",
        "/wd4267",
        "/wd4312",
        "/wd4305",
        "/wd4244",
        "/wd4291", -- 初期化により例外がスローされると、メモリが解放されません
        "/wd6011", -- NULLポインターの逆参照
        "/wd6255", -- alloca関数の 例外処理が行われていない
        "/wd6263", -- ループ内で alloca関数を使用している

        "/utf-8",  -- 文字列リテラルを utf8 として認識する (ImGuiが utf8 のみ対応しているため)

        --=======================================================================================================
        -- C++20 可変長マクロ __VA_OPT__() を サポートしていない MSVC バージョンのために
        -- /Zc:preprocessor オプションを有効にすることで回避する
        -- https://stackoverflow.com/questions/68484818/function-like-macros-with-c20-va-opt-error-in-manual-code
        -- https://learn.microsoft.com/ja-jp/cpp/preprocessor/preprocessor-experimental-overview?view=msvc-170#comma-elision-in-variadic-macros
        -- 
        -- 有効にしないと、展開部分のみならず、プロジェクト全体からエラーが出るので注意
        --=======================================================================================================
        "/Zc:preprocessor", -- C++20 __VA_OPT__ を MSVCがデフォルトで正しく展開しないため
    }

    postbuildcommands
    {
        '{COPY} "%{cfg.targetdir}/*.exe" "%{wks.location}"',
        '{COPY} "%{cfg.targetdir}/*.dll" "%{wks.location}/Resources"',
    }

    -- プリコンパイルヘッダー 無視リスト
    ----------------------------------------------------
    filter "files:External/yaml-cpp/src/**.cpp" flags { "NoPCH" }
    filter "files:External/imgui/**.cpp"        flags { "NoPCH" }
    filter "files:External/imguizmo/**.cpp"     flags { "NoPCH" }
    filter "files:External/glfw/src/**.c"       flags { "NoPCH" }
    filter "files:External/glad/src/**.c"       flags { "NoPCH" }
    filter "files:External/vulkan/**.cpp"       flags { "NoPCH" }

    -- Windows
    ----------------------------------------------------
    filter "system:windows"
    
        systemversion "latest"
        
        defines
        { 
            "SL_PLATFORM_WINDOWS",
            "NOMINMAX",
            "_CRT_SECURE_NO_WARNINGS",
            "_GLFW_WIN32",
        }

        links
        {
            "Dwmapi.lib",
            "Winmm.lib",
            "delayimp.lib", -- 遅延 DLL 読み込み
        }

    -- デバッグ
    ----------------------------------------------------
    filter "configurations:Debug"
    
        defines    "SL_DEBUG"
        symbols    "On"
        optimize   "Off"
        kind       "ConsoleApp"
        targetname "%{prj.name}d"

        links
        {
            -- assimp
            "External/assimp/lib/Debug/assimp-vc143-mtd.lib",

            -- spirv_cross
            "External/vulkan/lib/spirv-cross-cored.lib",

            -- shaderc
            "External/vulkan/lib/shaderc_sharedd.lib",
            "External/vulkan/lib/shaderc_utild.lib",

            -- mono
            "External/mono/lib/mono-2.0-sgend.lib",
        }

        linkoptions
        {
            -- 遅延 DLL 読み込み
            "/DELAYLOAD:assimp-vc143-mtd.dll",
            "/DELAYLOAD:shaderc_sharedd.dll",
            "/DELAYLOAD:mono-2.0-sgend.dll",
        }

    -- リリース
    ----------------------------------------------------
    filter "configurations:Release"

        defines    "SL_RELEASE"
        optimize   "Speed"
        kind       "WindowedApp"
        targetname "%{prj.name}"

        links
        {
            -- assimp
            "External/assimp/lib/Release/assimp-vc143-mt.lib",

            -- spirv_cross
            "External/vulkan/lib/spirv-cross-core.lib",

            -- shaderc
            "External/vulkan/lib/shaderc_shared.lib",
            "External/vulkan/lib/shaderc_util.lib",

            -- mono
            "External/mono/lib/mono-2.0-sgen.lib",
        }

        linkoptions
        { 
            -- 遅延 DLL 読み込み
            "/DELAYLOAD:assimp-vc143-mt.dll",
            "/DELAYLOAD:shaderc_shared.dll",
            "/DELAYLOAD:mono-2.0-sgen.dll",
        }
