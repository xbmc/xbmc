#include "Log.h"
#include "Platform.h"
#include "StringUtils.h"
#include "UpdateScript.h"
#include "UpdaterOptions.h"

#include "tinythread.h"

#if defined(PLATFORM_WINDOWS)
  #include "UpdateDialogWin32.h"
#elif defined(PLATFORM_MAC)
  #include "UpdateDialogCocoa.h"
#elif defined(PLATFORM_LINUX)
  #include "UpdateDialogGtk.h"
#endif

#include <iostream>

void setupUi(UpdateInstaller* installer);

void runUpdaterThread(void* arg)
{
	try
	{
		UpdateInstaller* installer = static_cast<UpdateInstaller*>(arg);
		installer->run();
	}
	catch (const std::exception& ex)
	{
		LOG(Error,"Unexpected exception " + std::string(ex.what()));
	}
}

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

	LOG(Info,"started updater. install-dir: " + options.installDir
	         + ", package-dir: " + options.packageDir
	         + ", wait-pid: " + intToStr(options.waitPid)
	         + ", script-path: " + options.script
	         + ", mode: " + intToStr(options.mode));

	installer.setMode(options.mode);
	installer.setInstallDir(options.installDir);
	installer.setPackageDir(options.packageDir);
	installer.setScript(&script);
	installer.setWaitPid(options.waitPid);

	tthread::thread updaterThread(runUpdaterThread,&installer);
	updaterThread.join();

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

