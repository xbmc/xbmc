#include "TestUpdaterOptions.h"

#include "TestUtils.h"
#include "UpdaterOptions.h"

#include <string.h>
#include <stdlib.h>

void TestUpdaterOptions::testOldFormatArgs()
{
	const int argc = 6;
	char* argv[argc];
	argv[0] = strdup("updater");
	argv[1] = strdup("CurrentDir=/path/to/app");
	argv[2] = strdup("TempDir=/tmp/updater");
	argv[3] = strdup("UpdateScriptFileName=/tmp/updater/file_list.xml");
	argv[4] = strdup("AppFileName=/path/to/app/theapp");
	argv[5] = strdup("PID=123456");

	UpdaterOptions options;
	options.parse(argc,argv);

	TEST_COMPARE(options.mode,UpdateInstaller::Setup);
	TEST_COMPARE(options.installDir,"/path/to/app");
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

