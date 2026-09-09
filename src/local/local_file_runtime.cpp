#include "local_file_runtime.hpp"

namespace SCLocalFile {
    namespace {
        std::filesystem::path parent_path_from_underscores(const std::wstring& game_path)
        {
            const auto first = game_path.find(L'_');
            if (first == std::wstring::npos) {
                return L".";
            }

            const auto second_begin = first + 1;
            const auto second = game_path.find(L'_', second_begin);
            const auto second_len = second == std::wstring::npos
                ? std::wstring::npos
                : second - second_begin;

            std::filesystem::path path;
            path /= game_path.substr(0, first);
            path /= game_path.substr(second_begin, second_len);
            return path;
        }
    }

    std::filesystem::path path_for_game_file(
        const std::wstring& game_path,
        bool create_parent_path,
        const std::filesystem::path& parent_base)
    {
        std::filesystem::path relative_path;
        if (game_path.starts_with(L"s")) {
            relative_path /= L"scenario";
        }

        const auto parent_path = relative_path / parent_path_from_underscores(game_path);
        if (create_parent_path) {
            const auto full_parent_path = parent_base / parent_path;
            if (!std::filesystem::exists(full_parent_path)) {
                std::filesystem::create_directories(full_parent_path);
            }
        }
        return parent_path / game_path;
    }

    bool resolve_local_file(
        const std::filesystem::path& localify_base,
        const std::wstring& game_path,
        std::filesystem::path& local_path,
        bool check_exists)
    {
        const auto candidate = localify_base /
            path_for_game_file(game_path, !check_exists, localify_base);
        if (check_exists && !std::filesystem::exists(candidate)) {
            return false;
        }
        local_path = candidate;
        return true;
    }
}
