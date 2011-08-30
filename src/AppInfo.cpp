#include "AppInfo.h"

#include "FileUtils.h"
#include "Platform.h"
#include "StringUtils.h"
#include "StandardDirs.h"

#include <iostream>

std::string AppInfo::logFilePath()
{
	return StandardDirs::appDataPath(organizationName(),appName())  + '/' + "update-log.txt";
}

std::string AppInfo::updateErrorMessage(const std::string& details)
{
	std::string result = "There was a problem installing the update:\n\n";
	result += details;
	result += "\n\nYou can try downloading and installing the latest version of "
	          "Mendeley Desktop from http://www.mendeley.com/download-mendeley-desktop";
	return result;
}

