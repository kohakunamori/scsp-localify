local project_root = path.getabsolute("."):gsub("/", "\\")

workspace "SCSP-Localify-Full-2.17"
    location "build-full-2.17"
    architecture "x64"
    platforms { "x64" }
    configurations { "Release" }
    systemversion "10.0.26100.0"
    characterset "ASCII"
    warnings "Off"
    flags { "NoIncrementalLink" }
    buildoptions {
        "/std:c++latest",
        "/utf-8",
        "/Brepro",
        "/experimental:deterministic",
        "/pathmap:" .. project_root .. "=Z:\\scsp-localify",
    }
    linkoptions { "/Brepro" }

project "minhook-full-2.17"
    kind "StaticLib"
    language "C"
    targetdir "build-full-2.17/bin/%{cfg.platform}/%{cfg.buildcfg}"
    objdir "build-full-2.17/obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}"
    includedirs { "deps/minhook/include" }
    files { "deps/minhook/src/**.h", "deps/minhook/src/**.c" }

project "SCSP-Localify-Full-2.17"
    kind "SharedLib"
    language "C++"
    targetname "scsp_localify_plugin"
    targetdir "build-full-2.17/bin/%{cfg.platform}/%{cfg.buildcfg}"
    objdir "build-full-2.17/obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}"
    includedirs {
        "src",
        "deps/minhook/include",
        "deps/rapidjson/include",
        "deps/nlohmann/include",
    }
    files {
        "src/**.hpp",
        "src/**.h",
        "src/**.cpp",
        "src/**.asm",
    }
    removefiles {
        "src/full_compat_probe.cpp",
        "src/localization_lite_standalone.cpp",
        "src/dllproxy/**",
    }
    links {
        "minhook-full-2.17",
        "d3d11",
        "dxgi",
        "dwmapi",
        "psapi",
        "urlmon",
        "crypt32",
    }
    defines { "WIN32_LEAN_AND_MEAN", "NOMINMAX" }
    staticruntime "Off"

    filter "configurations:Release"
        optimize "Full"
        symbols "Off"
