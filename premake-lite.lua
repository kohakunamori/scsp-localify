local project_root = path.getabsolute("."):gsub("/", "\\")

workspace "SCSP-Localify-Lite"
    location "build-lite"
    architecture "x64"
    platforms { "x64" }
    configurations { "Release" }
    -- B9FB was real-client verified with Windows SDK 10.0.26100.0. Do not let
    -- the generated project silently drift with the newest SDK on each runner.
    systemversion "10.0.26100.0"
    characterset "Unicode"
    warnings "Off"
    flags { "NoIncrementalLink", "MultiProcessorCompile" }
    buildoptions {
        "/std:c++latest",
        "/utf-8",
        "/Brepro",
        "/experimental:deterministic",
        "/pathmap:" .. project_root .. "=Z:\\scsp-localify",
    }
    linkoptions { "/Brepro" }

project "minhook-lite"
    kind "StaticLib"
    language "C"
    targetdir "build-lite/bin/%{cfg.platform}/%{cfg.buildcfg}"
    objdir "build-lite/obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}"
    includedirs { "deps/minhook/include" }
    files {
        "deps/minhook/src/**.h",
        "deps/minhook/src/**.c",
    }

project "SCSP-Localify-Lite"
    kind "SharedLib"
    language "C++"
    targetname "scsp_localify_plugin"
    targetdir "build-lite/bin/%{cfg.platform}/%{cfg.buildcfg}"
    objdir "build-lite/obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}"
    includedirs {
        "deps/minhook/include",
        "deps/rapidjson/include",
    }
    files {
        "src/localization_lite_standalone.cpp",
        "src/local/string_map_runtime.cpp",
        "src/local/string_map_runtime.hpp",
    }
    links { "minhook-lite" }
    defines { "WIN32_LEAN_AND_MEAN", "NOMINMAX" }
    staticruntime "Off"

    filter "configurations:Release"
        optimize "Full"
        symbols "Off"
