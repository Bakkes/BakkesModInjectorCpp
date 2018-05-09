#pragma once
#include <string>

class WindowsUtils
{
public:
	WindowsUtils();
	~WindowsUtils();
	static std::string GetMyDocumentsFolder();
	std::string GetRocketLeagueDirFromLog();
	std::string GetRocketLeagueBuildID();
	static bool FileExists(std::string path);
	static void OpenFolder(std::string path);
	static bool IsProcessRunning(std::wstring processName);
};
