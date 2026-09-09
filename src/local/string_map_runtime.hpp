#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace SCStringMap {
    struct LoadResult {
        std::size_t entries = 0;
        std::string error;

        explicit operator bool() const noexcept { return error.empty(); }
    };

    class Dictionary {
    public:
        LoadResult load(const std::filesystem::path& path);
        const std::string* find(const std::string& source) const noexcept;
        std::size_t size() const noexcept;
        void clear() noexcept;

    private:
        std::unordered_map<std::string, std::string> values_;
    };
}
