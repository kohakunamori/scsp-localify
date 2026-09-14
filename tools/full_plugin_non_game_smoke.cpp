#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <iostream>

static DWORD count_current_process_threads() {
    const DWORD pid = GetCurrentProcessId();
    DWORD count = 0;
    const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snap, &entry)) {
        do {
            if (entry.th32OwnerProcessID == pid) {
                ++count;
            }
        } while (Thread32Next(snap, &entry));
    }
    CloseHandle(snap);
    return count;
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) {
        std::wcerr << L"usage: full_plugin_non_game_smoke.exe <plugin.dll>\n";
        return 2;
    }

    const auto plugin = std::filesystem::absolute(argv[1]);
    if (!std::filesystem::is_regular_file(plugin)) {
        std::wcerr << L"plugin not found: " << plugin << L"\n";
        return 3;
    }

    const wchar_t* preload_names[] = {
        L"d3d11.dll",
        L"urlmon.dll",
        L"crypt32.dll",
        L"user32.dll",
        L"gdi32.dll",
        L"msvcp140.dll",
        L"imm32.dll",
        L"d3dcompiler_47.dll",
        L"vcruntime140.dll",
        L"vcruntime140_1.dll",
    };
    HMODULE preloaded[sizeof(preload_names) / sizeof(preload_names[0])]{};
    for (size_t i = 0; i < std::size(preload_names); ++i) {
        preloaded[i] = LoadLibraryW(preload_names[i]);
        if (!preloaded[i]) {
            std::wcerr << L"dependency preload failed: " << preload_names[i]
                       << L" error=" << GetLastError() << L"\n";
            return 7;
        }
    }
    Sleep(500);
    const DWORD before = count_current_process_threads();
    SetLastError(ERROR_SUCCESS);
    HMODULE module = LoadLibraryW(plugin.c_str());
    const DWORD load_error = GetLastError();
    if (!module) {
        std::wcerr << L"LoadLibraryW failed error=" << load_error << L"\n";
        return 4;
    }

    Sleep(500);
    const DWORD loaded = count_current_process_threads();

    if (!FreeLibrary(module)) {
        std::wcerr << L"FreeLibrary failed error=" << GetLastError() << L"\n";
        return 5;
    }

    Sleep(200);
    const DWORD after = count_current_process_threads();

    std::wcout << L"plugin=" << plugin << L"\n";
    std::wcout << L"threads_before=" << before
               << L" threads_loaded=" << loaded
               << L" threads_after_free=" << after
               << L" load_error=" << load_error << L"\n";

    if (loaded != before || after != before) {
        std::wcerr << L"plugin load changed thread count after dependency warm-up; DllMain process gate/static initialization needs investigation\n";
        return 6;
    }

    for (auto module_handle : preloaded) {
        FreeLibrary(module_handle);
    }

    std::wcout << L"PASS non-game LoadLibrary/FreeLibrary process gate\n";
    return 0;
}
