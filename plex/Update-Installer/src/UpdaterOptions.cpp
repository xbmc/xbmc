#include "UpdaterOptions.h"

#include "Log.h"
#include "AnyOption/anyoption.h"
#include "FileUtils.h"
#include "Platform.h"
#include "StringUtils.h"

#include <cstdlib>
#include <iostream>

#ifdef PLATFORM_WINDOWS
long long atoll(const char* string)
{
	return _atoi64(string);
}
#endif

UpdaterOptions::UpdaterOptions()
: mode(UpdateInstaller::Setup)
, waitPid(0)
, showVersion(false)
, forceElevated(false)
, autoClose(false)
{
}

UpdateInstaller::Mode stringToMode(const std::string& modeStr)
{
	if (modeStr == "main")
	{
		return UpdateInstaller::Main;
	}
	else
	{
		if (!modeStr.empty())
		{
			LOG(Error,"Unknown mode " + modeStr);
		}
		return UpdateInstaller::Setup;
	}
}

void UpdaterOptions::parseOldFormatArg(const std::string& arg, std::string* key, std::string* value)
{
	size_t pos = arg.find('=');
	if (pos != std::string::npos)
	{
		*key = arg.substr(0,pos);
		*value = arg.substr(pos+1);
	}
}

// this is a compatibility function to allow the updater binary
// to be involved by legacy versions of Mendeley Desktop
// which used a different syntax for the updater's command-line
// arguments
void UpdaterOptions::parseOldFormatArgs(int argc, char** argv)
{
	for (int i=0; i < argc; i++)
	{
		std::string key;
		std::string value;

		parseOldFormatArg(argv[i],&key,&value);

		if (key == "CurrentDir")
		{
			// CurrentDir is the directory containing the main application
			// binary.  On Mac and Linux this differs from the root of
			// the installation directory

#ifdef PLATFORM_LINUX
			// the main binary is in lib/mendeleydesktop/libexec,
			// go up 3 levels
			installDir = FileUtils::canonicalPath((value + "/../../../").c_str());
#elif defined(PLATFORM_MAC)
			// the main binary is in Contents/MacOS,
			// go up 2 levels
			installDir = FileUtils::canonicalPath((value + "/../../").c_str());
#elif defined(PLATFORM_WINDOWS)
			// the main binary is in the root of the install directory
			installDir = value;
#endif
		}
		else if (key == "TempDir")
		{
			packageDir = value;
		}
		else if (key == "UpdateScriptFileName")
		{
			scriptPath = value;
		}
		else if (key == "AppFileName")
		{
			// TODO - Store app file name
		}
		else if (key == "PID")
		{
			waitPid = static_cast<PLATFORM_PID>(atoll(value.c_str()));
		}
		else if (key == "--main")
		{
			mode = UpdateInstaller::Main;
		}
	}
}

void UpdaterOptions::parse(int argc, char** argv)
{
	AnyOption parser;
	parser.setOption("install-dir");
	parser.setOption("package-dir");
	parser.setOption("script");
	parser.setOption("wait");
	parser.setOption("mode");
	parser.setFlag("version");
	parser.setFlag("force-elevated");
	parser.setFlag("auto-close");

	parser.processCommandArgs(argc,argv);

	if (parser.getValue("mode"))
	{
		mode = stringToMode(parser.getValue("mode"));
	}
	if (parser.getValue("install-dir"))
	{
		installDir = parser.getValue("install-dir");
	}
	if (parser.getValue("package-dir"))
	{
		packageDir = parser.getValue("package-dir");
	}
	if (parser.getValue("script"))
	{
		scriptPath = parser.getValue("script");
	}
	if (parser.getValue("wait"))
	{
		waitPid = static_cast<PLATFORM_PID>(atoll(parser.getValue("wait")));
	}

	showVersion = parser.getFlag("version");
	forceElevated = parser.getFlag("force-elevated");
	autoClose = parser.getFlag("auto-close");
		
	if (installDir.empty())
	{
		// if no --install-dir argument is present, try parsing
		// the command-line arguments in the old format (which uses
		// a list of 'Key=Value' args)
		parseOldFormatArgs(argc,argv);
	}
}

