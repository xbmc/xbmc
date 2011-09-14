#pragma once

#include <string>

/** Class for creating minimal Mac app bundles. */
class MacBundle
{
	public:
		/** Create a MacBundle instance representing the bundle
		  * in <path>/<appName>.app
		  */
		MacBundle(const std::string& path, const std::string& appName);

		/** Create a simple Mac bundle.
		  *
		  * @param infoPlist The content of the Info.plist file
		  * @param icon The content of the app icon
		  * @param exePath The path of the file to use for the main app in the bundle.
		  */
		void create(const std::string& infoPlist,
		            const std::string& icon,
		            const std::string& exePath);
		
		/** Returns the path of the main executable within the Mac bundle. */
		std::string executablePath() const;

		/** Returns the path of the bundle */
		std::string bundlePath() const;

	private:
		std::string m_path;
		std::string m_appName;
		std::string m_exePath;
};

