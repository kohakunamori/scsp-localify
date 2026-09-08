local project_root = path.getabsolute("."):gsub("/", "\\")

workspace "SCSP-Localify-Probe"
    location "build-probe"
    architecture "x64"
    platforms { "x64" }
    configurations { "Release" }
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

project "SCSP-Localify-FullCompatProbe"
    kind "SharedLib"
    language "C++"
    targetname "scsp_localify_plugin"
    targetdir "build-probe/bin/%{cfg.platform}/%{cfg.buildcfg}"
    objdir "build-probe/obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}"
    files { "src/full_compat_probe.cpp" }
    defines { "WIN32_LEAN_AND_MEAN", "NOMINMAX" }
    staticruntime "Off"

    filter "configurations:Release"
        optimize "Full"
        symbols "Off"
