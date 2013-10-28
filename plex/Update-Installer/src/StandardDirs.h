#pragma once

#include "Platform.h"

#include <string>

class StandardDirs
{
	public:
		static std::string appDataPath(const std::string& organizationName,
                                       const std::string& appName);

	private:
#ifdef PLATFORM_UNIX
		static std::string homeDir();
#endif

#ifdef PLATFORM_MAC
		static std::string applicationSupportFolderPath();
#endif
};

