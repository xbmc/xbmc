#include "UpdaterOptions.h"

#include "Log.h"
#include "AnyOption/anyoption.h"
#include <cstdlib>

#include <iostream>

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
		script = parser.getValue("script");
	}
	if (parser.getValue("wait"))
	{
		waitPid = atoll(parser.getValue("wait"));
	}
}

