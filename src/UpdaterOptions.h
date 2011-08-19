#pragma once

#include "UpdateInstaller.h"

class UpdaterOptions
{
	public:
		UpdaterOptions();

		void parse(int argc, char** argv);

		UpdateInstaller::Mode mode;
		std::string installDir;
		std::string packageDir;
		std::string script;
		long long waitPid;
		std::string logFile;
};

