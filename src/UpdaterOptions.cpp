#include "UpdaterOptions.h"

#include "Log.h"
#include "AnyOption/anyoption.h"
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
{
}

UpdateInstaller::Mode stringToMode(const std::string& modeStr)
{
	if (modeStr == "main")
	{
		return UpdateInstaller::Main;
	}
	else if (modeStr == "cleanup")
	{
		return UpdateInstaller::Cleanup;
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
	unsigned int pos = arg.find('=');
	if (pos != std::string::npos)
	{
		*key = arg.substr(0,pos);
		*value = arg.substr(pos+1);
	}
}

void UpdaterOptions::parseOldFormatArgs(int argc, char** argv)
{
	for (int i=0; i < argc; i++)
	{
		std::string key;
		std::string value;

		parseOldFormatArg(argv[i],&key,&value);

		if (key == "CurrentDir")
		{
			installDir = value;
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
			waitPid = atoll(value.c_str());
		}
		else if (key == "--main")
		{
			mode = UpdateInstaller::Main;
		}
		else if (key == "--clean")
		{
			mode = UpdateInstaller::Cleanup;
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
		waitPid = atoll(parser.getValue("wait"));
	}
	
	if (installDir.empty())
	{
		// if no --install-dir argument is present, try parsing
		// the command-line arguments in the old format (which uses
		// a list of 'Key=Value' args)
		parseOldFormatArgs(argc,argv);
	}
}

