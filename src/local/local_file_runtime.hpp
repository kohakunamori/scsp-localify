#pragma once

#include <filesystem>
#include <string>

namespace SCLocalFile {
    std::filesystem::path path_for_game_file(
        const std::wstring& game_path,
        bool create_parent_path,
        const std::filesystem::path& parent_base);

    bool resolve_local_file(
        const std::filesystem::path& localify_base,
        const std::wstring& game_path,
        std::filesystem::path& local_path,
        bool check_exists = true);
}
