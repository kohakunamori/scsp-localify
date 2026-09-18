#pragma once
#include <string>

namespace SCLocal {
	struct DramaTranslation {
		std::string sourceSchema;
		std::string text;
		std::string talkerSource;
		std::string talkerText;
		std::string displayTalkerSource;
		std::string displayTalkerText;
	};

	void loadLocalTrans();
	bool getLocalifyText(const std::string& category, int id, std::string* getStr);
	bool getLocalifyText(const std::wstring& category, int id, std::wstring* getStr);
	std::filesystem::path getFilePathByName(const std::wstring& gamePath, bool createFatherPath, const std::filesystem::path& fatherBase);
	bool getLocalFileName(const std::wstring& gamePath, std::filesystem::path* localPath, bool checkExists = true);
	std::string getLyricsTrans(const std::wstring& orig);
	bool getGameUnlocalTrans(const std::wstring& orig, std::string* newStr);
	bool getDramaTranslation(const std::string& uniqueId, const std::string& source, DramaTranslation* translation);
	size_t getDramaTranslationCount();
}
