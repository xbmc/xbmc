#include "UpdaterOptions.h"
#include "UpdateScript.h"
#include "Platform.h"

#if defined(PLATFORM_WINDOWS)
  #include "UpdateDialogWin32.h"
#elif defined(PLATFORM_MAC)
  #include "UpdateDialogCocoa.h"
#elif defined(PLATFORM_LINUX)
  #include "UpdateDialogGtk.h"
#endif

#include <iostream>

void setupUi(UpdateInstaller* installer);

int main(int argc, char** argv)
{
	UpdaterOptions options;
	options.parse(argc,argv);

	UpdateInstaller installer;
	UpdateScript script;

	if (!options.script.empty())
	{
		script.parse(options.script);
	}

	if (options.mode == UpdateInstaller::Main)
	{
		setupUi(&installer);
	}

	std::cout << "install dir " << options.installDir << std::endl;
	std::cout << "package dir " << options.packageDir << std::endl;
	std::cout << "wait pid " << options.waitPid << std::endl;
	std::cout << "script " << options.script << std::endl;
	std::cout << "mode " << options.mode << std::endl;

	installer.setMode(options.mode);
	installer.setInstallDir(options.installDir);
	installer.setPackageDir(options.packageDir);
	installer.setScript(&script);
	installer.setWaitPid(options.waitPid);
	installer.run();

	return 0;
}

void setupUi(UpdateInstaller* installer)
{
#if defined(PLATFORM_WINDOWS)
	UpdateDialogWin32 dialog;
#elif defined(PLATFORM_MAC)
	UpdateDialogCocoa dialog;
#elif defined(PLATFORM_LINUX)
	UpdateDialogGtk dialog;
#endif
}

