#pragma once

#include <string>

class AppInfo
{
	public:
		// Basic application information
		static std::string name();
		static std::string appName();

		static std::string logFilePath();
};

inline std::string AppInfo::name()
{
	return "Mendeley Updater";
}

inline std::string AppInfo::appName()
{
	return "Mendeley Desktop";
}

