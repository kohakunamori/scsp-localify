#include "string_map_runtime.hpp"

#include <fstream>
#include <iterator>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace SCStringMap {
    LoadResult Dictionary::load(const std::filesystem::path& path)
    {
        values_.clear();

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return {0, "file not found: " + path.string()};
        }

        const std::string content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        rapidjson::Document doc;
        doc.Parse(content.data(), content.size());
        if (doc.HasParseError()) {
            return {0, std::string("JSON parse error: ") +
                rapidjson::GetParseError_En(doc.GetParseError())};
        }
        if (!doc.IsObject()) {
            return {0, "root is not an object"};
        }

        values_.reserve(doc.MemberCount());
        for (auto item = doc.MemberBegin(); item != doc.MemberEnd(); ++item) {
            if (!item->name.IsString() || !item->value.IsString()) {
                continue;
            }
            values_[item->name.GetString()] = item->value.GetString();
        }

        return {values_.size(), {}};
    }

    const std::string* Dictionary::find(const std::string& source) const noexcept
    {
        const auto it = values_.find(source);
        return it == values_.end() ? nullptr : &it->second;
    }

    std::size_t Dictionary::size() const noexcept
    {
        return values_.size();
    }

    void Dictionary::clear() noexcept
    {
        values_.clear();
    }
}
