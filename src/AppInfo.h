#pragma once

#include <string>

class AppInfo
{
	public:
		static std::string name();
		static std::string appName();
};

inline std::string AppInfo::name()
{
	return "Mendeley Updater";
}

inline std::string AppInfo::appName()
{
	return "Mendeley Desktop";
}

