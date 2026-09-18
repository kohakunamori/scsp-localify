#include <stdinclude.hpp>
#include "string_map_runtime.hpp"
#include "local_file_runtime.hpp"


namespace SCLocal {
	namespace {
		std::unordered_map<std::string, std::unordered_map<int, std::string>> localTrans{};
		SCStringMap::Dictionary lrcTrans{};
		SCStringMap::Dictionary unLocalTrans{};
		std::vector<std::pair<std::string, std::string>> dynamicUnLocalTrans{};
		std::unordered_map<std::string, DramaTranslation> dramaTrans{};

		std::string makeDramaKey(const std::string& uniqueId, const std::string& source) {
			std::string key;
			key.reserve(uniqueId.size() + 1 + source.size());
			key.append(uniqueId);
			key.push_back('\0');
			key.append(source);
			return key;
		}

		bool numericPlaceholderAt(const std::string& value, size_t pos, int* index = nullptr) {
			if (pos + 2 >= value.size() || value[pos] != '{' || value[pos + 2] != '}') {
				return false;
			}
			const auto ch = value[pos + 1];
			if (ch < '0' || ch > '9') {
				return false;
			}
			if (index) {
				*index = ch - '0';
			}
			return true;
		}

		bool containsNumericPlaceholder(const std::string& value) {
			for (size_t i = 0; i + 2 < value.size(); i++) {
				if (numericPlaceholderAt(value, i)) {
					return true;
				}
			}
			return false;
		}

		bool matchNumericTemplate(
			const std::string& value,
			const std::string& sourceTemplate,
			std::array<std::string, 10>* captures
		) {
			std::array<bool, 10> seen{};
			size_t valuePos = 0;
			for (size_t templatePos = 0; templatePos < sourceTemplate.size();) {
				int placeholderIndex = -1;
				if (numericPlaceholderAt(sourceTemplate, templatePos, &placeholderIndex)) {
					const auto start = valuePos;
					while (valuePos < value.size() &&
						value[valuePos] >= '0' && value[valuePos] <= '9') {
						valuePos++;
					}
					if (valuePos == start) {
						return false;
					}
					const auto captured = value.substr(start, valuePos - start);
					if (seen[placeholderIndex] && (*captures)[placeholderIndex] != captured) {
						return false;
					}
					(*captures)[placeholderIndex] = captured;
					seen[placeholderIndex] = true;
					templatePos += 3;
					continue;
				}
				if (valuePos >= value.size() || value[valuePos] != sourceTemplate[templatePos]) {
					return false;
				}
				valuePos++;
				templatePos++;
			}
			return valuePos == value.size();
		}

		std::string applyNumericTemplate(
			const std::string& targetTemplate,
			const std::array<std::string, 10>& captures
		) {
			std::string result;
			result.reserve(targetTemplate.size() + 16);
			for (size_t pos = 0; pos < targetTemplate.size();) {
				int placeholderIndex = -1;
				if (numericPlaceholderAt(targetTemplate, pos, &placeholderIndex)) {
					result += captures[placeholderIndex];
					pos += 3;
					continue;
				}
				result.push_back(targetTemplate[pos]);
				pos++;
			}
			return result;
		}

		bool getDynamicUnlocalTrans(const std::string& value, std::string* translated) {
			if (value.find_first_of("0123456789") == std::string::npos) {
				return false;
			}
			for (const auto& [sourceTemplate, targetTemplate] : dynamicUnLocalTrans) {
				std::array<std::string, 10> captures{};
				if (matchNumericTemplate(value, sourceTemplate, &captures)) {
					*translated = applyNumericTemplate(targetTemplate, captures);
					return true;
				}
			}
			return false;
		}
	}

	void loadGenericTrans(const char* fileName, SCStringMap::Dictionary& transDict) {
		const auto result = transDict.load(g_localify_base / fileName);
		if (!result) {
			printf("Load %s failed: %s\n", fileName, result.error.c_str());
		}
	}

	void loadLrcTrans() {
		loadGenericTrans("lyrics.json", lrcTrans);
	}
	void loadUnlocalTrans() {
		loadGenericTrans("local2.json", unLocalTrans);
		dynamicUnLocalTrans.clear();
		try {
			std::ifstream file(g_localify_base / "local2.json");
			if (!file.is_open()) {
				return;
			}
			auto fileData = nlohmann::json::parse(file);
			for (auto& item : fileData.items()) {
				if (!item.value().is_string() || !containsNumericPlaceholder(item.key())) {
					continue;
				}
				dynamicUnLocalTrans.emplace_back(item.key(), item.value().get<std::string>());
			}
			printf("%zu dynamic local2 templates loaded.\n", dynamicUnLocalTrans.size());
		}
		catch (std::exception& e) {
			printf("Load dynamic local2 templates failed: %s\n", e.what());
		}
	}

	void loadDramaTrans() {
		dramaTrans.clear();
		try {
			std::ifstream file(g_localify_base / "drama.json");
			if (!file.is_open()) {
				printf("Load drama.json failed: file not found.\n");
				return;
			}
			auto fileData = nlohmann::json::parse(file);
			const nlohmann::json* entries = nullptr;
			if (fileData.is_array()) {
				entries = &fileData;
			}
			else if (fileData.is_object() && fileData.contains("entries") && fileData["entries"].is_array()) {
				entries = &fileData["entries"];
			}
			if (!entries) {
				printf("Load drama.json failed: expected array or entries[].\n");
				return;
			}

			size_t duplicateCount = 0;
			size_t dramaSubtitleCount = 0;
			size_t sujigakiCount = 0;
			for (const auto& entry : *entries) {
				if (!entry.is_object() ||
					!entry.contains("uniqueId") || !entry["uniqueId"].is_string() ||
					!entry.contains("source") || !entry["source"].is_string() ||
					!entry.contains("text") || !entry["text"].is_string()) {
					continue;
				}

				DramaTranslation translation;
				translation.sourceSchema = entry.value("sourceSchema", "");
				translation.text = entry["text"].get<std::string>();
				if (entry.contains("talkerName") && entry["talkerName"].is_string()) {
					translation.talkerSource = entry["talkerName"].get<std::string>();
				}
				if (entry.contains("talkerText") && entry["talkerText"].is_string()) {
					translation.talkerText = entry["talkerText"].get<std::string>();
				}
				if (entry.contains("displayTalkerName") && entry["displayTalkerName"].is_string()) {
					translation.displayTalkerSource = entry["displayTalkerName"].get<std::string>();
				}
				if (entry.contains("displayTalkerText") && entry["displayTalkerText"].is_string()) {
					translation.displayTalkerText = entry["displayTalkerText"].get<std::string>();
				}

				if (translation.sourceSchema == "DramaSubtitlePlayableAsset") {
					dramaSubtitleCount++;
				}
				else if (translation.sourceSchema == "SujigakiLinePlayableAsset") {
					sujigakiCount++;
				}

				auto key = makeDramaKey(
					entry["uniqueId"].get<std::string>(),
					entry["source"].get<std::string>()
				);
				if (dramaTrans.contains(key)) {
					duplicateCount++;
				}
				dramaTrans[std::move(key)] = std::move(translation);
			}
			printf(
				"%zu items in drama.json loaded (DramaSubtitle=%zu, Sujigaki=%zu, duplicateKeys=%zu).\n",
				dramaTrans.size(), dramaSubtitleCount, sujigakiCount, duplicateCount
			);
		}
		catch (std::exception& e) {
			printf("Load drama.json failed: %s\n", e.what());
		}
	}

	void loadLocalTrans() {
		loadLrcTrans();
		loadUnlocalTrans();
		loadDramaTrans();
		localTrans.clear();
		printf("Loading localify.json...\n");
		int totalItemCount = 0;
		try {
			std::ifstream file(g_localify_base / "localify.json");
			if (!file.is_open()) {
				printf("Load localify.json failed: file not found.\n");
				return;
			}
			std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
			auto fileData = nlohmann::json::parse(fileContent);
			for (auto& i : fileData.items()) {
				const auto& key = i.key();
				localTrans[key] = {};
				for (auto& v : i.value().items()) {
					const auto& subIdStr = v.key();
					const auto subId = std::stoi(subIdStr);
					std::string localText = v.value();
					if (const auto translated = unLocalTrans.find(localText)) {
						localText = *translated;
					}
					localTrans[key][subId] = localText;
					totalItemCount++;
				}
			}
		}
		catch (std::exception& e) {
			printf("Load localify.json failed: %s\n", e.what());
		}
		printf("%d items in localify.json loaded.\n", totalItemCount);
	}

	bool getLocalifyText(const std::string& category, int id, std::string* getStr) {
		if (auto it = localTrans.find(category); it != localTrans.end()) {
			const auto& value = it->second;
			if (auto vIt = value.find(id); vIt != value.end()) {
				*getStr = vIt->second;
				return true;
			}
		}
		return false;
	}

	/*
	注意几个特殊的 category: mlStory_MainStoryEpisode, mlMusic_CueSheet, mlMusic_MVScene
	除非了解游戏文件结构，否则不要修改这几个的值 (不排除还有其他的)
	*/
	bool getLocalifyText(const std::wstring& category, int id, std::wstring* getStr) {
		const auto categoryS = utility::conversions::to_utf8string(category);
		std::string resultS = "";
		if (getLocalifyText(categoryS, id, &resultS)) {
			const auto resultWs = utility::conversions::to_utf16string(resultS);
			*getStr = resultWs;
			return true;
		}
		return false;
	}

	std::filesystem::path getFilePathByName(const std::wstring& gamePath, bool createFatherPath, const std::filesystem::path& fatherBase) {
		return SCLocalFile::path_for_game_file(gamePath, createFatherPath, fatherBase);
	}

	bool getLocalFileName(const std::wstring& gamePath, std::filesystem::path* localPath, bool checkExists) {
		if (!localPath) return false;
		return SCLocalFile::resolve_local_file(g_localify_base, gamePath, *localPath, checkExists);
	}

	void dumpGenericText(const std::string& dumpStr, const char* fileName, bool withOrigText = false) {
		try {
			const std::filesystem::path dumpBasePath("dumps");
			const auto dumpFilePath = dumpBasePath / fileName;

			if (!std::filesystem::is_directory(dumpBasePath)) {
				std::filesystem::create_directories(dumpBasePath);
			}
			if (!std::filesystem::exists(dumpFilePath)) {
				std::ofstream dumpWriteLrcFile(dumpFilePath, std::ofstream::out);
				dumpWriteLrcFile << "{}";
				dumpWriteLrcFile.close();
			}

			std::ifstream dumpLrcFile(dumpFilePath);
			std::string fileContent((std::istreambuf_iterator<char>(dumpLrcFile)), std::istreambuf_iterator<char>());
			dumpLrcFile.close();
			auto fileData = nlohmann::ordered_json::parse(fileContent);
			fileData[dumpStr] = withOrigText ? dumpStr : "";
			const auto newStr = fileData.dump(4, 32, false);
			std::ofstream dumpWriteLrcFile(dumpFilePath, std::ofstream::out);
			dumpWriteLrcFile << newStr.c_str();
			dumpWriteLrcFile.close();
		}
		catch (std::exception& e) {
			printf("Dump text to %s error: %s\n", fileName, e.what());
		}

	}

	std::string replaceAll(const std::string& str, const std::string& oldStr, const std::string& newStr) {
		std::string result = str;
		size_t pos = 0;
		while ((pos = result.find(oldStr, pos)) != std::string::npos) {
			result.replace(pos, oldStr.length(), newStr);
			pos += newStr.length();
		}
		return result;
	}

	std::string getLyricsTrans(const std::wstring& orig) {
		// const auto lrcStr = replaceAll(replaceAll(utility::conversions::to_utf8string(orig), "\n", "\\n"), "\r", "\\r");
		const auto lrcStr = utility::conversions::to_utf8string(orig);
		if (const auto translated = lrcTrans.find(lrcStr)) {
			return *translated;
		}
		else {
			if (g_dump_untrans_lyrics) {
				dumpGenericText(lrcStr, "lyrics.json", true);
			}
		}
		return lrcStr;
	}

	bool getGameUnlocalTrans(const std::wstring& orig, std::string* newStr) {
		// const auto origStr = replaceAll(replaceAll(utility::conversions::to_utf8string(orig), "\n", "\\n"), "\r", "\\r");
		const auto origStr = utility::conversions::to_utf8string(orig);
		if (const auto translated = unLocalTrans.find(origStr)) {
			*newStr = *translated;
			return true;
		}
		else if (getDynamicUnlocalTrans(origStr, newStr)) {
			return true;
		}
		else {
			if (g_dump_untrans_unlocal) {
				dumpGenericText(origStr, "local2.json");
			}
		}
		return false;
	}
	bool getDramaTranslation(
		const std::string& uniqueId,
		const std::string& source,
		DramaTranslation* translation
	) {
		if (!translation) return false;
		const auto it = dramaTrans.find(makeDramaKey(uniqueId, source));
		if (it == dramaTrans.end()) return false;
		*translation = it->second;
		return true;
	}

	size_t getDramaTranslationCount() {
		return dramaTrans.size();
	}


}
