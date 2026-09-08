#include <Windows.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    struct MethodInfo
    {
        uintptr_t methodPointer;
        uintptr_t virtualMethodPointer;
        uintptr_t invoker_method;
    };

    using il2cpp_domain_get_t = void* (*)();
    using il2cpp_domain_assembly_open_t = void* (*)(void*, const char*);
    using il2cpp_assembly_get_image_t = void* (*)(void*);
    using il2cpp_class_from_name_t = void* (*)(void*, const char*, const char*);
    using il2cpp_class_get_method_from_name_t = MethodInfo* (*)(void*, const char*, int);
    using il2cpp_class_get_field_from_name_t = void* (*)(void*, const char*);
    using il2cpp_class_get_nested_types_t = void* (*)(void*, void**);
    using il2cpp_class_get_name_t = const char* (*)(void*);
    using il2cpp_thread_attach_t = void* (*)(void*);
    using il2cpp_thread_detach_t = void (*)(void*);

    il2cpp_domain_get_t il2cpp_domain_get = nullptr;
    il2cpp_domain_assembly_open_t il2cpp_domain_assembly_open = nullptr;
    il2cpp_assembly_get_image_t il2cpp_assembly_get_image = nullptr;
    il2cpp_class_from_name_t il2cpp_class_from_name = nullptr;
    il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name = nullptr;
    il2cpp_class_get_field_from_name_t il2cpp_class_get_field_from_name = nullptr;
    il2cpp_class_get_nested_types_t il2cpp_class_get_nested_types = nullptr;
    il2cpp_class_get_name_t il2cpp_class_get_name = nullptr;
    il2cpp_thread_attach_t il2cpp_thread_attach = nullptr;
    il2cpp_thread_detach_t il2cpp_thread_detach = nullptr;

    std::mutex log_mutex;
    unsigned ok_count = 0;
    unsigned missing_count = 0;

    void log_line(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        SYSTEMTIME st{};
        GetLocalTime(&st);
        char stamp[64]{};
        sprintf_s(stamp, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        std::ofstream out("scsp-full-compat-probe.log", std::ios::app | std::ios::binary);
        if (out) out << '[' << stamp << "] [full-probe] " << message << "\r\n";
    }

    template <typename T>
    bool resolve_export(HMODULE module, const char* name, T& out)
    {
        out = reinterpret_cast<T>(GetProcAddress(module, name));
        if (!out) log_line(std::string("missing IL2CPP export: ") + name);
        return out != nullptr;
    }

    bool resolve_exports(HMODULE module)
    {
        bool ok = true;
        ok &= resolve_export(module, "il2cpp_domain_get", il2cpp_domain_get);
        ok &= resolve_export(module, "il2cpp_domain_assembly_open", il2cpp_domain_assembly_open);
        ok &= resolve_export(module, "il2cpp_assembly_get_image", il2cpp_assembly_get_image);
        ok &= resolve_export(module, "il2cpp_class_from_name", il2cpp_class_from_name);
        ok &= resolve_export(module, "il2cpp_class_get_method_from_name", il2cpp_class_get_method_from_name);
        ok &= resolve_export(module, "il2cpp_class_get_field_from_name", il2cpp_class_get_field_from_name);
        ok &= resolve_export(module, "il2cpp_class_get_nested_types", il2cpp_class_get_nested_types);
        ok &= resolve_export(module, "il2cpp_class_get_name", il2cpp_class_get_name);
        ok &= resolve_export(module, "il2cpp_thread_attach", il2cpp_thread_attach);
        ok &= resolve_export(module, "il2cpp_thread_detach", il2cpp_thread_detach);
        return ok;
    }

    void* find_class(const char* assembly_name, const char* namespaze, const char* class_name)
    {
        auto domain = il2cpp_domain_get ? il2cpp_domain_get() : nullptr;
        if (!domain) return nullptr;
        auto assembly = il2cpp_domain_assembly_open(domain, assembly_name);
        if (!assembly) return nullptr;
        auto image = il2cpp_assembly_get_image(assembly);
        if (!image) return nullptr;
        return il2cpp_class_from_name(image, namespaze, class_name);
    }

    MethodInfo* find_method_info(const char* assembly_name, const char* namespaze, const char* class_name,
        const char* method_name, int arg_count)
    {
        auto klass = find_class(assembly_name, namespaze, class_name);
        return klass ? il2cpp_class_get_method_from_name(klass, method_name, arg_count) : nullptr;
    }

    void record(const std::string& kind, const std::string& label, const void* ptr)
    {
        if (ptr)
        {
            ++ok_count;
            char line[768]{};
            sprintf_s(line, "%s OK %s ptr=%p", kind.c_str(), label.c_str(), ptr);
            log_line(line);
        }
        else
        {
            ++missing_count;
            log_line(kind + " MISSING " + label);
        }
    }

    void probe_class(const char* assembly_name, const char* namespaze, const char* class_name)
    {
        record("CLASS", std::string(assembly_name) + "|" + namespaze + "|" + class_name,
            find_class(assembly_name, namespaze, class_name));
    }

    void probe_method(const char* assembly_name, const char* namespaze, const char* class_name,
        const char* method_name, int arg_count)
    {
        auto method = find_method_info(assembly_name, namespaze, class_name, method_name, arg_count);
        const auto label = std::string(assembly_name) + "|" + namespaze + "|" + class_name + "|" +
            method_name + "/" + std::to_string(arg_count);
        record("METHOD", label, method && method->methodPointer ? reinterpret_cast<void*>(method->methodPointer) : nullptr);
    }

    void probe_field(void* klass, const char* class_label, const char* field_name)
    {
        auto field = klass ? il2cpp_class_get_field_from_name(klass, field_name) : nullptr;
        record("FIELD", std::string(class_label) + "|" + field_name, field);
    }

    void* find_nested(void* parent, const char* name)
    {
        if (!parent || !il2cpp_class_get_nested_types || !il2cpp_class_get_name) return nullptr;
        void* iter = nullptr;
        while (auto nested = il2cpp_class_get_nested_types(parent, &iter))
        {
            const char* nested_name = il2cpp_class_get_name(nested);
            if (nested_name && std::string(nested_name) == name) return nested;
        }
        return nullptr;
    }

    int read_probe_level()
    {
        char value[32]{};
        const DWORD length = GetEnvironmentVariableA("SCSP_FULL_PROBE_LEVEL", value, static_cast<DWORD>(sizeof(value)));
        if (length == 0 || length >= sizeof(value)) return 3;
        const int level = atoi(value);
        return level < 0 ? 0 : (level > 3 ? 3 : level);
    }

    int read_probe_stage()
    {
        char value[32]{};
        const DWORD length = GetEnvironmentVariableA("SCSP_FULL_PROBE_STAGE", value, static_cast<DWORD>(sizeof(value)));
        if (length == 0 || length >= sizeof(value)) return 5;
        const int stage = atoi(value);
        return stage < 1 ? 1 : (stage > 5 ? 5 : stage);
    }

    void run_probe(int level)
    {
        log_line("probe begin: upstream v1.3.13 pre-hook metadata compatibility against SCSP 2.17 level=" + std::to_string(level));

        auto dmm = find_class("PRISM.Legacy.dll", "PRISM", "DMMGameGuard");
        record("CLASS", "PRISM.Legacy.dll|PRISM|DMMGameGuard", dmm);
        probe_field(dmm, "DMMGameGuard", "_isCheck");
        probe_field(dmm, "DMMGameGuard", "_isInit");
        probe_field(dmm, "DMMGameGuard", "_bAppExit");
        probe_field(dmm, "DMMGameGuard", "_errCode");

        probe_class("UnityEngine.TextRenderingModule.dll", "UnityEngine", "Font");
        probe_class("PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel");
        probe_class("UniRx.dll", "UniRx", "Subject`1");
        probe_class("PRISM.Module.Networking.dll", "PRISM.Module.Networking", "ICostumeStatus");

        auto presenter = find_class("PRISM.Adapters.dll", "PRISM.Adapters", "LiveMvUnitMemberChangePresenter");
        record("CLASS", "PRISM.Adapters.dll|PRISM.Adapters|LiveMvUnitMemberChangePresenter", presenter);
        auto display = find_nested(presenter, "<>c__DisplayClass5_0");
        record("NESTED", "LiveMvUnitMemberChangePresenter|<>c__DisplayClass5_0", display);
        auto state_machine = find_nested(display, "<<InitializeAsync>b__0>d");
        record("NESTED", "<>c__DisplayClass5_0|<<InitializeAsync>b__0>d", state_machine);
        auto move_next = state_machine ? il2cpp_class_get_method_from_name(state_machine, "MoveNext", 0) : nullptr;
        record("METHOD", "LiveMvUnitMemberChangePresenter nested MoveNext/0",
            move_next && move_next->methodPointer ? reinterpret_cast<void*>(move_next->methodPointer) : nullptr);

        if (level < 3)
        {
            log_line("probe done level=" + std::to_string(level) + " ok=" + std::to_string(ok_count) + " missing=" + std::to_string(missing_count));
            return;
        }

        struct MethodSpec
        {
            const char* assembly_name;
            const char* namespaze;
            const char* class_name;
            const char* method_name;
            int arg_count;
        };

        static constexpr MethodSpec specs[] = {
            {"mscorlib.dll", "System", "Environment", "get_StackTrace", 0},
            {"mscorlib.dll", "System.IO", "File", "ReadAllBytes", 1},
            {"UnityEngine.AssetBundleModule.dll", "UnityEngine", "AssetBundle", "LoadFromFile", 3},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Object", "IsNativeObjectAlive", 1},
            {"UnityEngine.AssetBundleModule.dll", "UnityEngine", "AssetBundle", "LoadAsset_Internal", 2},
            {"Unity.TextMeshPro.dll", "TMPro", "TMP_FontAsset", "CreateFontAsset", 1},
            {"Unity.TextMeshPro.dll", "TMPro", "TMP_Text", "set_font", 1},
            {"Unity.TextMeshPro.dll", "TMPro", "TMP_Text", "get_font", 0},
            {"Unity.TextMeshPro.dll", "TMPro", "TMP_Text", "set_text", 1},
            {"PRISM.Legacy.dll", "ENTERPRISE.UI", "UITextMeshProUGUI", "Awake", 0},
            {"PRISM.Legacy.dll", "PRISM.Scenario", "ScenarioManager", "_initializeAsync", 1},
            {"PRISM.Legacy.dll", "PRISM", "DataFile", "GetBytes", 1},
            {"PRISM.Legacy.dll", "PRISM", "DataFile", "IsKeyExist", 1},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Screen", "SetResolution", 3},
            {"PRISM.Legacy.dll", "PRISM.Scenario", "TextLog", "AddLog", 4},
            {"PRISM.Legacy.dll", "PRISM.Domain", "StoryExtensions", "IsLocked", 1},
            {"PRISM.Legacy.dll", "ENTERPRISE.Localization", "LocalizationManager", "GetTextOrNull", 2},
            {"Prism.Rendering.Runtime.dll", "PRISM.Rendering", "RenderManager", "GetResolutionSize", 1},
            {"PRISM.Interactions.Live.dll", "PRISM.Interactions.Live", "LiveMVOverlayView", "UpdateLyrics", 1},
            {"PRISM.Legacy.dll", "PRISM", "TimelineController", "SetLyric", 1},
            {"PRISM.Legacy.dll", "PRISM", "CameraController", "get_BaseCamera", 0},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "get_position", 0},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "set_position", 1},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "get_fieldOfView", 0},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "set_fieldOfView", 1},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "Internal_LookAt", 2},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "set_nearClipPlane", 1},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "get_nearClipPlane", 0},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "get_farClipPlane", 0},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "set_farClipPlane", 1},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "get_rotation", 0},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "set_rotation", 1},
            {"UnityEngine.CoreModule.dll", "UnityEngine", "SetupCoroutine", "InvokeMoveNext", 2},
            {"PRISM.Legacy.dll", "PRISM", "DepthOfFieldClip", "CreatePlayable", 2},
            {"PRISM.Legacy.dll", "PRISM", "AssembleCharacter", "ApplyParam", 6},
            {"UniRx.dll", "UniRx", "MainThreadDispatcher", "LateUpdate", 0},
            {"PRISM.Legacy.dll", "PRISM.Live", "LiveMVUnit", "GetMemberChangeRequestData", 3},
            {"PRISM.Legacy.dll", "PRISM", "MvUnitSlotGenerator", "NewMvUnitSlot", 2},
            {"CriMw.CriWare.Runtime.dll", "CriWare", "CriWareErrorHandler", "HandleMessage", 1},
            {"PRISM.Legacy.dll", "PRISM", "GGIregualDetector", "ShowPopup", 1},
            {"PRISM.Legacy.dll", "PRISM", "DMMGameGuard", "NPGameMonCallback", 2},
            {"PRISM.Legacy.dll", "PRISM", "DMMGameGuard", "SetCheckMode", 1},
            {"PRISM.Service.dll", "PRISM.Service.Live", "LiveMvUnitConfirmationModel", ".ctor", 5},
            {"PRISM.Adapters.dll", "PRISM.Adapters", "RunwayUnitConfirmationModel", ".ctor", 6},
            {"PRISM.Legacy.dll", "PRISM", "SwayString", "SetupPoint", 0},
            {"PRISM.Legacy", "PRISM.Live", "LiveMVStartData", ".ctor", 9},
            {"PRISM.Legacy.dll", "PRISM.Live", "LiveStartDataExtensions", "PreLoadAsync", 1},
            {"MagicaClothV2.dll", "MagicaCloth2", "MagicaCloth", "BuildAndRun", 0},
            {"MagicaClothV2.dll", "MagicaCloth2", "MagicaCloth", "SetParameterChange", 0},
            {"PRISM.Module.CustomMagicaCloth.dll", "PRISM.Module.CustomMagicaCloth", "MagicaClothController", "get_Inertia", 0},
            {"PRISM.Module.CustomMagicaCloth.dll", "PRISM.Module.CustomMagicaCloth", "MagicaClothController", "get_Radius", 0},
            {"PRISM.Module.CustomMagicaCloth.dll", "PRISM.Module.CustomMagicaCloth", "MagicaClothController", "Awake", 0},
            {"PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", "Apply", 0},
            {"PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", ".ctor", 6},
            {"PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", "CanDecide", 0},
            {"PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", "CanDecide", 2},
            {"PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", "RefreshViewModels", 0},
            {"PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", "ModifyPreview", 1},
        };

        for (const auto& spec : specs)
        {
            probe_method(spec.assembly_name, spec.namespaze, spec.class_name, spec.method_name, spec.arg_count);
        }

        log_line("HIGH_RISK NOTE Dictionary<int, ICostumeStatus>.Add and closed UniRx.Subject<CostumeChangeViewModel>.OnNext require managed generic reflection; intentionally not invoked by read-only probe phase 1");
        log_line("probe done level=" + std::to_string(level) + " ok=" + std::to_string(ok_count) + " missing=" + std::to_string(missing_count));
    }

    DWORD WINAPI worker_thread(void*)
    {
        std::ofstream("scsp-full-compat-probe.log", std::ios::trunc | std::ios::binary).close();
        const int probe_level = read_probe_level();
        const int probe_stage = read_probe_stage();
        log_line("worker started; read-only full-hook compatibility probe level=" + std::to_string(probe_level) +
            " stage=" + std::to_string(probe_stage));
        if (probe_level == 0)
        {
            log_line("level 0: no IL2CPP access; worker exiting");
            return 0;
        }

        HMODULE game_assembly = nullptr;
        for (int attempt = 0; attempt < 2400 && !game_assembly; ++attempt)
        {
            game_assembly = GetModuleHandleW(L"GameAssembly.dll");
            if (!game_assembly) Sleep(50);
        }
        if (!game_assembly)
        {
            log_line("GameAssembly.dll not found; aborting");
            return 1;
        }

        HMODULE cri_ware = nullptr;
        for (int attempt = 0; attempt < 2400 && !cri_ware; ++attempt)
        {
            cri_ware = GetModuleHandleW(L"cri_ware_unity.dll");
            if (!cri_ware) Sleep(50);
        }
        if (!cri_ware)
        {
            log_line("cri_ware_unity.dll not found; aborting before IL2CPP access");
            return 2;
        }
        Sleep(250);

        if (probe_level == 1 && probe_stage == 1)
        {
            log_line("level 1 stage 1: module readiness only; no IL2CPP export access");
            return 0;
        }

        if (!resolve_exports(game_assembly))
        {
            log_line("IL2CPP export resolution failed; aborting");
            return 3;
        }

        if (probe_level == 1 && probe_stage == 2)
        {
            log_line("level 1 stage 2: IL2CPP exports resolved only; no domain access");
            return 0;
        }

        void* domain = nullptr;
        for (int attempt = 0; attempt < 2400 && !domain; ++attempt)
        {
            domain = il2cpp_domain_get();
            if (!domain) Sleep(50);
        }
        if (!domain)
        {
            log_line("IL2CPP domain unavailable; aborting");
            return 4;
        }

        if (probe_level == 1 && probe_stage == 3)
        {
            log_line("level 1 stage 3: domain resolved only; no thread attach");
            return 0;
        }

        void* attached_thread = il2cpp_thread_attach(domain);
        if (!attached_thread)
        {
            log_line("il2cpp_thread_attach failed; aborting");
            return 5;
        }

        if (probe_level == 1 && probe_stage == 4)
        {
            log_line("level 1 stage 4: thread attach reached; detaching immediately without method lookup");
            il2cpp_thread_detach(attached_thread);
            log_line("probe thread detached from IL2CPP");
            return 0;
        }

        for (int attempt = 0; attempt < 2400; ++attempt)
        {
            if (find_method_info("PRISM.Legacy.dll", "ENTERPRISE.Localization", "LocalizationManager", "GetTextOrNull", 2)) break;
            Sleep(50);
        }

        if (probe_level == 1)
        {
            log_line("level 1 stage 5: runtime attach plus LocalizationManager readiness lookup; no compatibility scan");
            il2cpp_thread_detach(attached_thread);
            log_line("probe thread detached from IL2CPP");
            return 0;
        }

        run_probe(probe_level);
        il2cpp_thread_detach(attached_thread);
        log_line("probe thread detached from IL2CPP");
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        if (HANDLE thread = CreateThread(nullptr, 0, worker_thread, nullptr, 0, nullptr))
        {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
