#include "TestUpdaterOptions.h"

#include "FileUtils.h"
#include "Platform.h"
#include "TestUtils.h"
#include "UpdaterOptions.h"

#include <string.h>
#include <stdlib.h>

void TestUpdaterOptions::testOldFormatArgs()
{
	const int argc = 6;
	char* argv[argc];
	argv[0] = strdup("updater");

	std::string currentDir("CurrentDir=");
	const char* appDir = 0;

	// CurrentDir is the path to the directory containing the main
	// Mendeley Desktop binary, on Linux and Mac this differs from
	// the root of the install directory
#if defined(PLATFORM_LINUX) || defined(PLATFORM_FREEBSD)
	appDir = "/tmp/path-to-app/lib/mendeleydesktop/libexec/";
	FileUtils::mkpath(appDir);
#elif defined(PLATFORM_MAC)
	appDir = "/tmp/path-to-app/Contents/MacOS/";
	FileUtils::mkpath(appDir);
#elif defined(PLATFORM_WINDOWS)
	appDir = "C:/path/to/app/";
#endif
	currentDir += appDir;

	argv[1] = strdup(currentDir.c_str());
	argv[2] = strdup("TempDir=/tmp/updater");
	argv[3] = strdup("UpdateScriptFileName=/tmp/updater/file_list.xml");
	argv[4] = strdup("AppFileName=/path/to/app/theapp");
	argv[5] = strdup("PID=123456");

	UpdaterOptions options;
	options.parse(argc,argv);

	TEST_COMPARE(options.mode,UpdateInstaller::Setup);
#if defined(PLATFORM_LINUX) || defined(PLATFORM_FREEBSD)
	TEST_COMPARE(options.installDir,"/tmp/path-to-app");
#elif defined(PLATFORM_MAC)
	// /tmp is a symlink to /private/tmp on Mac
	TEST_COMPARE(options.installDir,"/private/tmp/path-to-app");
#else
	TEST_COMPARE(options.installDir,"C:/path/to/app/");
#endif
	TEST_COMPARE(options.packageDir,"/tmp/updater");
	TEST_COMPARE(options.scriptPath,"/tmp/updater/file_list.xml");
	TEST_COMPARE(options.waitPid,123456);

	for (int i=0; i < argc; i++)
	{
		free(argv[i]);
	}
}

int main(int,char**)
{
	TestList<TestUpdaterOptions> tests;
	tests.addTest(&TestUpdaterOptions::testOldFormatArgs);
	return TestUtils::runTest(tests);
}

