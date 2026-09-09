#include <Windows.h>
#include <MinHook.h>
#include <rapidjson/document.h>

#include "local/string_map_runtime.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
    struct Il2CppObject
    {
        void* klass;
        void* monitor;
    };

    struct Il2CppString : Il2CppObject
    {
        int32_t length;
        wchar_t start_char[1];
    };

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
    using il2cpp_string_new_t = Il2CppString* (*)(const char*);
    using il2cpp_thread_attach_t = void* (*)(void*);
    using il2cpp_thread_detach_t = void (*)(void*);
    using il2cpp_class_get_type_t = void* (*)(void*);
    using il2cpp_type_get_object_t = void* (*)(const void*);
    using il2cpp_gchandle_new_t = void* (*)(void*, bool);
    using il2cpp_gchandle_get_target_t = void* (*)(void*);

    il2cpp_domain_get_t il2cpp_domain_get = nullptr;
    il2cpp_domain_assembly_open_t il2cpp_domain_assembly_open = nullptr;
    il2cpp_assembly_get_image_t il2cpp_assembly_get_image = nullptr;
    il2cpp_class_from_name_t il2cpp_class_from_name = nullptr;
    il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name = nullptr;
    il2cpp_string_new_t il2cpp_string_new = nullptr;
    il2cpp_thread_attach_t il2cpp_thread_attach = nullptr;
    il2cpp_thread_detach_t il2cpp_thread_detach = nullptr;
    il2cpp_class_get_type_t il2cpp_class_get_type = nullptr;
    il2cpp_type_get_object_t il2cpp_type_get_object = nullptr;
    il2cpp_gchandle_new_t il2cpp_gchandle_new = nullptr;
    il2cpp_gchandle_get_target_t il2cpp_gchandle_get_target = nullptr;

    using LocalizationGetTextFn = Il2CppString* (*)(void*, Il2CppString*, int);
    using UiAwakeFn = void (*)(void*);
    using LyricFn = void (*)(void*, Il2CppString*);
    using UiGetTextFn = Il2CppString* (*)(void*);
    using UiSetTextFn = void (*)(void*, Il2CppString*);
    using AssetBundleLoadFromFileFn = void* (*)(Il2CppString*, uint32_t, uint64_t);
    using AssetBundleLoadAssetFn = void* (*)(void*, Il2CppString*, void*);
    using TmpGetFontFn = void* (*)(void*);
    using TmpSetSourceFontFileFn = void (*)(void*, void*);
    using TmpUpdateFontAssetDataFn = void (*)(void*);

    LocalizationGetTextFn localization_get_text_orig = nullptr;
    UiAwakeFn ui_awake_orig = nullptr;
    LyricFn live_mv_update_lyrics_orig = nullptr;
    LyricFn timeline_set_lyric_orig = nullptr;
    UiGetTextFn ui_get_text = nullptr;
    UiSetTextFn ui_set_text = nullptr;
    AssetBundleLoadFromFileFn asset_bundle_load_from_file = nullptr;
    AssetBundleLoadAssetFn asset_bundle_load_asset = nullptr;
    TmpGetFontFn tmp_get_font = nullptr;
    TmpSetSourceFontFileFn tmp_set_source_font_file = nullptr;
    TmpUpdateFontAssetDataFn tmp_update_font_asset_data = nullptr;

    std::filesystem::path localify_base = L"scsp_localify";
    std::string custom_font_spec = "scsp_localify/scsp-bundle::assets/font/sbtphumminge-regular.ttf";
    void* font_reflection_type = nullptr;
    void* font_bundle_handle = nullptr;
    void* replacement_font_handle = nullptr;
    std::mutex font_mutex;
    std::unordered_map<void*, bool> updated_tmp_fonts;
    std::mutex log_mutex;
    std::unordered_map<std::string, std::unordered_map<int, std::string>> primary_translations;
    SCStringMap::Dictionary exact_translations;
    SCStringMap::Dictionary lyric_translations;
    std::atomic_uint32_t primary_hits{0};
    std::atomic_uint32_t exact_hits{0};
    std::atomic_uint32_t lyric_hits{0};

    std::string utf16_to_utf8(const wchar_t* data, int length)
    {
        if (!data || length <= 0) return {};
        const int needed = WideCharToMultiByte(CP_UTF8, 0, data, length, nullptr, 0, nullptr, nullptr);
        if (needed <= 0) return {};
        std::string out(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_UTF8, 0, data, length, out.data(), needed, nullptr, nullptr);
        return out;
    }

    std::string il2cpp_to_utf8(const Il2CppString* str)
    {
        return str ? utf16_to_utf8(str->start_char, str->length) : std::string{};
    }

    void log_line(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        SYSTEMTIME st{};
        GetLocalTime(&st);
        char stamp[64]{};
        sprintf_s(stamp, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        std::ofstream out("scsp-localify-lite.log", std::ios::app | std::ios::binary);
        if (out) out << '[' << stamp << "] [localify-lite] " << message << "\r\n";
    }

    void log_ptr(const char* name, const void* ptr)
    {
        char line[256]{};
        sprintf_s(line, "%s=%p", name, ptr);
        log_line(line);
    }

    bool read_text_file(const std::filesystem::path& path, std::string& out)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        in.seekg(0, std::ios::end);
        const auto end = in.tellg();
        if (end < 0) return false;
        out.resize(static_cast<size_t>(end));
        in.seekg(0, std::ios::beg);
        if (!out.empty()) in.read(out.data(), static_cast<std::streamsize>(out.size()));
        if (out.size() >= 3 && static_cast<unsigned char>(out[0]) == 0xEF &&
            static_cast<unsigned char>(out[1]) == 0xBB && static_cast<unsigned char>(out[2]) == 0xBF)
        {
            out.erase(0, 3);
        }
        return true;
    }

    bool parse_json_file(const std::filesystem::path& path, rapidjson::Document& doc)
    {
        std::string text;
        if (!read_text_file(path, text))
        {
            log_line("cannot open JSON: " + path.string());
            return false;
        }
        doc.Parse(text.c_str(), text.size());
        if (doc.HasParseError())
        {
            log_line("JSON parse error: " + path.string() + " offset=" + std::to_string(doc.GetErrorOffset()));
            return false;
        }
        return true;
    }

    void load_config()
    {
        rapidjson::Document doc;
        if (!parse_json_file("scsp-config.json", doc) || !doc.IsObject()) return;
        const auto base_it = doc.FindMember("localifyBasePath");
        if (base_it != doc.MemberEnd() && base_it->value.IsString())
        {
            localify_base = std::filesystem::u8path(base_it->value.GetString());
        }
        const auto font_it = doc.FindMember("customFontPath");
        if (font_it != doc.MemberEnd() && font_it->value.IsString())
        {
            custom_font_spec = font_it->value.GetString();
        }
    }

    size_t load_primary_translations()
    {
        primary_translations.clear();
        rapidjson::Document doc;
        if (!parse_json_file(localify_base / L"localify.json", doc) || !doc.IsObject()) return 0;

        size_t count = 0;
        for (auto table = doc.MemberBegin(); table != doc.MemberEnd(); ++table)
        {
            if (!table->name.IsString() || !table->value.IsObject()) continue;
            auto& dst = primary_translations[table->name.GetString()];
            for (auto item = table->value.MemberBegin(); item != table->value.MemberEnd(); ++item)
            {
                if (!item->name.IsString() || !item->value.IsString()) continue;
                try
                {
                    const int id = std::stoi(item->name.GetString());
                    dst[id] = item->value.GetString();
                    ++count;
                }
                catch (...)
                {
                }
            }
        }
        return count;
    }

    template <typename T>
    bool resolve_export(HMODULE module, const char* name, T& out)
    {
        out = reinterpret_cast<T>(GetProcAddress(module, name));
        if (!out) log_line(std::string("missing IL2CPP export: ") + name);
        return out != nullptr;
    }

    bool resolve_il2cpp_exports(HMODULE module)
    {
        return resolve_export(module, "il2cpp_domain_get", il2cpp_domain_get) &&
            resolve_export(module, "il2cpp_domain_assembly_open", il2cpp_domain_assembly_open) &&
            resolve_export(module, "il2cpp_assembly_get_image", il2cpp_assembly_get_image) &&
            resolve_export(module, "il2cpp_class_from_name", il2cpp_class_from_name) &&
            resolve_export(module, "il2cpp_class_get_method_from_name", il2cpp_class_get_method_from_name) &&
            resolve_export(module, "il2cpp_string_new", il2cpp_string_new) &&
            resolve_export(module, "il2cpp_thread_attach", il2cpp_thread_attach) &&
            resolve_export(module, "il2cpp_thread_detach", il2cpp_thread_detach) &&
            resolve_export(module, "il2cpp_class_get_type", il2cpp_class_get_type) &&
            resolve_export(module, "il2cpp_type_get_object", il2cpp_type_get_object) &&
            resolve_export(module, "il2cpp_gchandle_new", il2cpp_gchandle_new) &&
            resolve_export(module, "il2cpp_gchandle_get_target", il2cpp_gchandle_get_target);
    }

    uintptr_t find_method(const char* assembly_name, const char* namespaze, const char* class_name,
        const char* method_name, int arg_count)
    {
        auto domain = il2cpp_domain_get ? il2cpp_domain_get() : nullptr;
        if (!domain) return 0;
        auto assembly = il2cpp_domain_assembly_open(domain, assembly_name);
        if (!assembly) return 0;
        auto image = il2cpp_assembly_get_image(assembly);
        if (!image) return 0;
        auto klass = il2cpp_class_from_name(image, namespaze, class_name);
        if (!klass) return 0;
        auto method = il2cpp_class_get_method_from_name(klass, method_name, arg_count);
        return method ? method->methodPointer : 0;
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

    std::string path_to_utf8(const std::filesystem::path& path)
    {
        const auto wide = path.wstring();
        return utf16_to_utf8(wide.data(), static_cast<int>(wide.size()));
    }

    void* load_replacement_font()
    {
        std::lock_guard<std::mutex> lock(font_mutex);
        if (replacement_font_handle)
        {
            if (auto target = il2cpp_gchandle_get_target(replacement_font_handle)) return target;
        }

        constexpr const char* delimiter = "::";
        const auto split = custom_font_spec.rfind(delimiter);
        if (split == std::string::npos || !asset_bundle_load_from_file || !asset_bundle_load_asset || !font_reflection_type)
        {
            log_line("font loader unavailable or invalid customFontPath=" + custom_font_spec);
            return nullptr;
        }

        const auto bundle_path = std::filesystem::absolute(std::filesystem::u8path(custom_font_spec.substr(0, split)));
        const auto asset_path = custom_font_spec.substr(split + 2);
        if (!std::filesystem::exists(bundle_path))
        {
            log_line("font bundle missing: " + path_to_utf8(bundle_path));
            return nullptr;
        }

        void* bundle = font_bundle_handle ? il2cpp_gchandle_get_target(font_bundle_handle) : nullptr;
        if (!bundle)
        {
            const auto bundle_utf8 = path_to_utf8(bundle_path);
            bundle = asset_bundle_load_from_file(il2cpp_string_new(bundle_utf8.c_str()), 0, 0);
            if (!bundle)
            {
                log_line("AssetBundle.LoadFromFile failed: " + bundle_utf8);
                return nullptr;
            }
            font_bundle_handle = il2cpp_gchandle_new(bundle, false);
            log_line("font AssetBundle loaded: " + bundle_utf8);
        }

        auto font = asset_bundle_load_asset(bundle, il2cpp_string_new(asset_path.c_str()), font_reflection_type);
        if (!font)
        {
            log_line("font asset missing in bundle: " + asset_path);
            return nullptr;
        }
        replacement_font_handle = il2cpp_gchandle_new(font, false);
        log_line("replacement font loaded: " + asset_path);
        return font;
    }

    Il2CppString* LocalizationManager_GetTextOrNull_hook(void* self, Il2CppString* category, int id)
    {
        if (category)
        {
            const auto category_utf8 = il2cpp_to_utf8(category);
            const auto table_it = primary_translations.find(category_utf8);
            if (table_it != primary_translations.end())
            {
                const auto value_it = table_it->second.find(id);
                if (value_it != table_it->second.end())
                {
                    const auto hit = ++primary_hits;
                    if (hit <= 20)
                    {
                        log_line("primary hit #" + std::to_string(hit) + " category=" + category_utf8 +
                            " id=" + std::to_string(id));
                    }
                    return il2cpp_string_new(value_it->second.c_str());
                }
            }
        }
        return localization_get_text_orig(self, category, id);
    }

    void UITextMeshProUGUI_Awake_hook(void* self)
    {
        if (self && ui_get_text && ui_set_text)
        {
            if (auto original = ui_get_text(self))
            {
                const auto original_utf8 = il2cpp_to_utf8(original);
                const auto translated = exact_translations.find(original_utf8);
                if (translated && *translated != original_utf8)
                {
                    const auto hit = ++exact_hits;
                    if (hit <= 20)
                    {
                        std::string preview = original_utf8.substr(0, 96);
                        log_line("local2 hit #" + std::to_string(hit) + " text=" + preview);
                    }
                    ui_set_text(self, il2cpp_string_new(translated->c_str()));
                }
            }
        }

        if (self && tmp_get_font && tmp_set_source_font_file && tmp_update_font_asset_data)
        {
            if (auto replacement_font = load_replacement_font())
            {
                if (auto tmp_font = tmp_get_font(self))
                {
                    tmp_set_source_font_file(tmp_font, replacement_font);
                    if (!updated_tmp_fonts.contains(tmp_font))
                    {
                        updated_tmp_fonts.emplace(tmp_font, true);
                        tmp_update_font_asset_data(tmp_font);
                        log_line("TMP font asset updated with replacement source font");
                    }
                }
            }
        }
        ui_awake_orig(self);
    }

    Il2CppString* translate_lyric_or_original(Il2CppString* original, const char* surface)
    {
        if (!original) return original;

        const auto original_utf8 = il2cpp_to_utf8(original);
        const auto translated = lyric_translations.find(original_utf8);
        if (!translated || translated->empty() || *translated == original_utf8)
        {
            return original;
        }

        auto replacement = il2cpp_string_new(translated->c_str());
        if (!replacement)
        {
            log_line(std::string("lyric allocation failed surface=") + surface);
            return original;
        }

        const auto hit = ++lyric_hits;
        if (hit <= 20)
        {
            std::string preview = original_utf8.substr(0, 96);
            log_line("lyric hit #" + std::to_string(hit) + " surface=" + surface + " text=" + preview);
        }
        return replacement;
    }

    void LiveMVOverlayView_UpdateLyrics_hook(void* self, Il2CppString* text)
    {
        live_mv_update_lyrics_orig(self, translate_lyric_or_original(text, "LiveMVOverlayView.UpdateLyrics"));
    }

    void TimelineController_SetLyric_hook(void* self, Il2CppString* text)
    {
        timeline_set_lyric_orig(self, translate_lyric_or_original(text, "TimelineController.SetLyric"));
    }

    bool install_hook(void* target, void* detour, void** original, const char* name)
    {
        if (!target)
        {
            log_line(std::string("null target: ") + name);
            return false;
        }
        const auto create_status = MH_CreateHook(target, detour, original);
        if (create_status != MH_OK)
        {
            log_line(std::string("MH_CreateHook failed: ") + name + " status=" + MH_StatusToString(create_status));
            return false;
        }
        const auto enable_status = MH_EnableHook(target);
        if (enable_status != MH_OK)
        {
            log_line(std::string("MH_EnableHook failed: ") + name + " status=" + MH_StatusToString(enable_status));
            return false;
        }
        log_line(std::string("hook installed: ") + name);
        return true;
    }

    bool wait_for_runtime_and_methods(uintptr_t& localization_method, uintptr_t& awake_method,
        uintptr_t& get_text_method, uintptr_t& set_text_method)
    {
        for (int attempt = 0; attempt < 2400; ++attempt)
        {
            localization_method = find_method("PRISM.Legacy.dll", "ENTERPRISE.Localization",
                "LocalizationManager", "GetTextOrNull", 2);
            awake_method = find_method("PRISM.Legacy.dll", "ENTERPRISE.UI",
                "UITextMeshProUGUI", "Awake", 0);
            get_text_method = find_method("PRISM.Legacy.dll", "ENTERPRISE.UI",
                "UITextMeshProUGUI", "get_text", 0);
            set_text_method = find_method("PRISM.Legacy.dll", "ENTERPRISE.UI",
                "UITextMeshProUGUI", "set_text", 1);
            if (localization_method && awake_method && get_text_method && set_text_method) return true;
            Sleep(50);
        }
        return false;
    }

    DWORD WINAPI worker_thread(void*)
    {
        std::ofstream("scsp-localify-lite.log", std::ios::trunc | std::ios::binary).close();
        log_line("worker started; standalone localization-only runtime");

        load_config();
        log_line("localify base=" + localify_base.string());
        const auto primary_count = load_primary_translations();
        const auto exact_result = exact_translations.load(localify_base / L"local2.json");
        if (!exact_result)
        {
            log_line("local2 load failed: " + exact_result.error);
        }
        const auto lyric_result = lyric_translations.load(localify_base / L"lyrics.json");
        if (!lyric_result)
        {
            log_line("lyrics load failed: " + lyric_result.error);
        }
        const auto exact_count = exact_result.entries;
        const auto lyric_count = lyric_result.entries;
        log_line("translations loaded: primary=" + std::to_string(primary_count) +
            " local2=" + std::to_string(exact_count) + " lyrics=" + std::to_string(lyric_count));

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
        log_ptr("GameAssembly", game_assembly);

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
        log_ptr("cri_ware_unity", cri_ware);
        log_line("runtime readiness marker observed; delaying 250 ms before IL2CPP access");
        Sleep(250);

        if (!resolve_il2cpp_exports(game_assembly))
        {
            log_line("IL2CPP export resolution failed; aborting");
            return 3;
        }
        log_line("IL2CPP exports resolved");

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
        log_ptr("IL2CPP domain", domain);
        void* attached_thread = il2cpp_thread_attach(domain);
        if (!attached_thread)
        {
            log_line("il2cpp_thread_attach failed; aborting");
            return 5;
        }
        log_ptr("IL2CPP attached thread", attached_thread);

        uintptr_t localization_method = 0;
        uintptr_t awake_method = 0;
        uintptr_t get_text_method = 0;
        uintptr_t set_text_method = 0;
        if (!wait_for_runtime_and_methods(localization_method, awake_method, get_text_method, set_text_method))
        {
            log_line("required PRISM methods did not become available; aborting");
            il2cpp_thread_detach(attached_thread);
            return 6;
        }

        log_ptr("LocalizationManager.GetTextOrNull", reinterpret_cast<void*>(localization_method));
        log_ptr("UITextMeshProUGUI.Awake", reinterpret_cast<void*>(awake_method));
        log_ptr("UITextMeshProUGUI.get_text", reinterpret_cast<void*>(get_text_method));
        log_ptr("UITextMeshProUGUI.set_text", reinterpret_cast<void*>(set_text_method));
        ui_get_text = reinterpret_cast<UiGetTextFn>(get_text_method);
        ui_set_text = reinterpret_cast<UiSetTextFn>(set_text_method);

        const auto live_mv_update_lyrics_method = find_method(
            "PRISM.Interactions.Live.dll", "PRISM.Interactions.Live", "LiveMVOverlayView", "UpdateLyrics", 1);
        const auto timeline_set_lyric_method = find_method(
            "PRISM.Legacy.dll", "PRISM", "TimelineController", "SetLyric", 1);
        log_ptr("LiveMVOverlayView.UpdateLyrics", reinterpret_cast<void*>(live_mv_update_lyrics_method));
        log_ptr("TimelineController.SetLyric", reinterpret_cast<void*>(timeline_set_lyric_method));

        asset_bundle_load_from_file = reinterpret_cast<AssetBundleLoadFromFileFn>(find_method(
            "UnityEngine.AssetBundleModule.dll", "UnityEngine", "AssetBundle", "LoadFromFile", 3));
        asset_bundle_load_asset = reinterpret_cast<AssetBundleLoadAssetFn>(find_method(
            "UnityEngine.AssetBundleModule.dll", "UnityEngine", "AssetBundle", "LoadAsset_Internal", 2));
        tmp_get_font = reinterpret_cast<TmpGetFontFn>(find_method(
            "Unity.TextMeshPro.dll", "TMPro", "TMP_Text", "get_font", 0));
        tmp_set_source_font_file = reinterpret_cast<TmpSetSourceFontFileFn>(find_method(
            "Unity.TextMeshPro.dll", "TMPro", "TMP_FontAsset", "set_sourceFontFile", 1));
        tmp_update_font_asset_data = reinterpret_cast<TmpUpdateFontAssetDataFn>(find_method(
            "Unity.TextMeshPro.dll", "TMPro", "TMP_FontAsset", "UpdateFontAssetData", 0));
        if (auto font_class = find_class("UnityEngine.TextRenderingModule.dll", "UnityEngine", "Font"))
        {
            font_reflection_type = il2cpp_type_get_object(il2cpp_class_get_type(font_class));
        }
        log_ptr("AssetBundle.LoadFromFile", reinterpret_cast<void*>(asset_bundle_load_from_file));
        log_ptr("AssetBundle.LoadAsset_Internal", reinterpret_cast<void*>(asset_bundle_load_asset));
        log_ptr("TMP_Text.get_font", reinterpret_cast<void*>(tmp_get_font));
        log_ptr("TMP_FontAsset.set_sourceFontFile", reinterpret_cast<void*>(tmp_set_source_font_file));
        log_ptr("TMP_FontAsset.UpdateFontAssetData", reinterpret_cast<void*>(tmp_update_font_asset_data));
        log_ptr("UnityEngine.Font type", font_reflection_type);
        log_line("customFontPath=" + custom_font_spec);

        const auto init_status = MH_Initialize();
        if (init_status != MH_OK && init_status != MH_ERROR_ALREADY_INITIALIZED)
        {
            log_line(std::string("MH_Initialize failed: ") + MH_StatusToString(init_status));
            il2cpp_thread_detach(attached_thread);
            return 7;
        }

        const bool primary_ok = install_hook(reinterpret_cast<void*>(localization_method),
            reinterpret_cast<void*>(&LocalizationManager_GetTextOrNull_hook),
            reinterpret_cast<void**>(&localization_get_text_orig), "LocalizationManager.GetTextOrNull");
        const bool local2_ok = install_hook(reinterpret_cast<void*>(awake_method),
            reinterpret_cast<void*>(&UITextMeshProUGUI_Awake_hook),
            reinterpret_cast<void**>(&ui_awake_orig), "UITextMeshProUGUI.Awake");
        const bool live_lyrics_ok = install_hook(reinterpret_cast<void*>(live_mv_update_lyrics_method),
            reinterpret_cast<void*>(&LiveMVOverlayView_UpdateLyrics_hook),
            reinterpret_cast<void**>(&live_mv_update_lyrics_orig), "LiveMVOverlayView.UpdateLyrics");
        const bool timeline_lyrics_ok = install_hook(reinterpret_cast<void*>(timeline_set_lyric_method),
            reinterpret_cast<void*>(&TimelineController_SetLyric_hook),
            reinterpret_cast<void**>(&timeline_set_lyric_orig), "TimelineController.SetLyric");

        log_line(std::string("initialization complete primary=") + (primary_ok ? "ok" : "failed") +
            " local2=" + (local2_ok ? "ok" : "failed") +
            " lyricsLive=" + (live_lyrics_ok ? "ok" : "failed") +
            " lyricsTimeline=" + (timeline_lyrics_ok ? "ok" : "failed"));
        il2cpp_thread_detach(attached_thread);
        log_line("initialization thread detached from IL2CPP");
        return primary_ok ? 0 : 8;
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
