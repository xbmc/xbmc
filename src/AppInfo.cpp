#include "AppInfo.h"

#include "FileUtils.h"
#include "Platform.h"
#include "StringUtils.h"

#include <iostream>

#ifdef PLATFORM_UNIX
 #include <stdlib.h>
 #include <pwd.h>
#endif

#ifdef PLATFORM_WINDOWS
#include <shlobj.h>
#endif

#ifdef PLATFORM_UNIX
std::string homeDir()
{
	std::string dir = notNullString(getenv("HOME"));
	if (!dir.empty())
	{
		return dir;
	}
	else
	{
		// note: if this process has been elevated with sudo,
		// this will return the home directory of the root user
		struct passwd* userData = getpwuid(getuid());
		return notNullString(userData->pw_dir);
	}
}
#endif

std::string appDataPath(const std::string& organizationName,
                        const std::string& appName)
{
#ifdef PLATFORM_LINUX
	std::string xdgDataHome = notNullString(getenv("XDG_DATA_HOME"));
	if (xdgDataHome.empty())
	{
		xdgDataHome = homeDir() + "/.local/share";
	}
	xdgDataHome += "/data/" + organizationName + '/' + appName;
	return xdgDataHome;

#elif defined(PLATFORM_MAC)
	// TODO - Mac implementation

#elif defined(PLATFORM_WINDOWS)
	char buffer[MAX_PATH+1];
	if (SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, 0 /* hToken */, SHGFP_TYPE_CURRENT, buffer) == S_OK)
	{
		std::string path = FileUtils::toUnixPathSeparators(notNullString(buffer));
		path += '/' + organizationName + '/' + appName;
		return path;
	}
	else
	{
		return std::string();
	}
#endif
}

std::string AppInfo::logFilePath()
{
	return appDataPath(organizationName(),appName())  + '/' + "update-log.txt";
}

std::string AppInfo::updateErrorMessage(const std::string& details)
{
	std::string result = "There was a problem installing the update:\n\n";
	result += details;
	result += "\n\nYou can try downloading and installing the latest version of "
	          "Mendeley Desktop from http://www.mendeley.com/download-mendeley-desktop";
	return result;
}

