#include <stdinclude.hpp>
#include "string_map_runtime.hpp"
#include "local_file_runtime.hpp"


namespace SCLocal {
	namespace {
		std::unordered_map<std::string, std::unordered_map<int, std::string>> localTrans{};
		SCStringMap::Dictionary lrcTrans{};
		SCStringMap::Dictionary unLocalTrans{};
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
	}

	void loadLocalTrans() {
		loadLrcTrans();
		loadUnlocalTrans();
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
		else {
			if (g_dump_untrans_unlocal) {
				dumpGenericText(origStr, "local2.json");
			}
		}
		return false;
	}

}
