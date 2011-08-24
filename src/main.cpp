#include "Log.h"
#include "Platform.h"
#include "StringUtils.h"
#include "UpdateScript.h"
#include "UpdaterOptions.h"

#include "tinythread.h"

#if defined(PLATFORM_LINUX) and defined(ENABLE_GTK)
  #include "UpdateDialogGtk.h"
#endif

#if defined(PLATFORM_MAC)
  #include "UpdateDialogCocoa.h"
#endif

#if defined(PLATFORM_WINDOWS)
  #include "UpdateDialogWin32.h"
#endif

#include <iostream>

void runWithUi(int argc, char** argv, UpdateInstaller* installer);

void runUpdaterThread(void* arg)
{
#ifdef PLATFORM_MAC
	// create an autorelease pool to free any temporary objects
	// created by Cocoa whilst handling notifications from the UpdateInstaller
	void* pool = UpdateDialogCocoa::createAutoreleasePool();
#endif

	try
	{
		UpdateInstaller* installer = static_cast<UpdateInstaller*>(arg);
		installer->run();
	}
	catch (const std::exception& ex)
	{
		LOG(Error,"Unexpected exception " + std::string(ex.what()));
	}

#ifdef PLATFORM_MAC
	UpdateDialogCocoa::releaseAutoreleasePool(pool);
#endif
}

int main(int argc, char** argv)
{
	Log::instance()->open(Log::defaultPath());
	UpdaterOptions options;
	options.parse(argc,argv);

	UpdateInstaller installer;
	UpdateScript script;

	if (!options.scriptPath.empty())
	{
		script.parse(options.scriptPath);
	}

	LOG(Info,"started updater. install-dir: " + options.installDir
	         + ", package-dir: " + options.packageDir
	         + ", wait-pid: " + intToStr(options.waitPid)
	         + ", script-path: " + options.scriptPath
	         + ", mode: " + intToStr(options.mode));

	installer.setMode(options.mode);
	installer.setInstallDir(options.installDir);
	installer.setPackageDir(options.packageDir);
	installer.setScript(&script);
	installer.setWaitPid(options.waitPid);

	if (options.mode == UpdateInstaller::Main)
	{
		runWithUi(argc,argv,&installer);
	}
	else
	{
		installer.run();
	}

	return 0;
}

#ifdef PLATFORM_LINUX
void runWithUi(int argc, char** argv, UpdateInstaller* installer)
{
#ifdef ENABLE_GTK
	UpdateDialogGtk dialog;
	installer->setObserver(&dialog);
	dialog.init(argc,argv);
	tthread::thread updaterThread(runUpdaterThread,installer);
	dialog.exec();
	updaterThread.join();

	if (dialog.restartApp())
	{
		installer->restartMainApp();
	}
#else
	// no UI available - do a silent install
	installer->run();
	installer->restartMainApp();
#endif
}
#endif

#ifdef PLATFORM_MAC
void runWithUi(int argc, char** argv, UpdateInstaller* installer)
{
	UpdateDialogCocoa dialog;
	installer->setObserver(&dialog);
	dialog.init();
	tthread::thread updaterThread(runUpdaterThread,installer);
	dialog.exec();
	updaterThread.join();
	installer->restartMainApp();
}
#endif

#ifdef PLATFORM_WINDOWS
void runWithUi(int argc, char** argv, UpdateInstaller* installer)
{
	UpdateDialogWin32 dialog;
	installer->setObserver(&dialog);
	dialog.init();
	tthread::thread updaterThread(runUpdaterThread,installer);
	dialog.exec();
	updaterThread.join();
	installer->restartMainApp();
}
#endif
