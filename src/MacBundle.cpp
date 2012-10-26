#include "MacBundle.h"

#include "FileUtils.h"
#include "Log.h"

MacBundle::MacBundle(const std::string& path, const std::string& appName)
: m_appName(appName)
{
	m_path = path + '/' + appName + ".app";
}

std::string MacBundle::bundlePath() const
{
	return m_path;
}

void MacBundle::create(const std::string& infoPlist,
                       const std::string& icon,
                       const std::string& exePath)
{
	try
	{
		// create the bundle directories
		FileUtils::mkpath(m_path.c_str());

		std::string contentDir = m_path + "/Contents";
		std::string resourceDir = contentDir + "/Resources";
		std::string binDir = contentDir + "/MacOS";

		FileUtils::mkpath(resourceDir.c_str());
		FileUtils::mkpath(binDir.c_str());
		
		// create the Contents/Info.plist file
		FileUtils::writeFile((contentDir + "/Info.plist").c_str(),infoPlist.c_str(),static_cast<int>(infoPlist.size()));

		// save the icon to Contents/Resources/<appname>.icns
		FileUtils::writeFile((resourceDir + '/' + m_appName + ".icns").c_str(),icon.c_str(),static_cast<int>(icon.size()));

		// copy the app binary to Contents/MacOS/<appname>
		m_exePath = binDir + '/' + m_appName;
		FileUtils::copyFile(exePath.c_str(),m_exePath.c_str());
	}
	catch (const FileUtils::IOException& exception)
	{
		LOG(Error,"Unable to create app bundle. " + std::string(exception.what()));
	}
}

std::string MacBundle::executablePath() const
{
	return m_exePath;
}

