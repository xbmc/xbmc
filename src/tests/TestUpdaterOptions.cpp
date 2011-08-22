#include "TestUpdaterOptions.h"

#include "TestUtils.h"
#include "UpdaterOptions.h"

void TestUpdaterOptions::testOldFormatArgs()
{
	const int argc = 6;
	char* argv[argc];
	argv[0] = "updater";
	argv[1] = "CurrentDir=/path/to/app";
	argv[2] = "TempDir=/tmp/updater";
	argv[3] = "UpdateScriptFileName=/tmp/updater/file_list.xml";
	argv[4] = "AppFileName=/path/to/app/theapp";
	argv[5] = "PID=123456";

	UpdaterOptions options;
	options.parse(argc,argv);

	TEST_COMPARE(options.mode,UpdateInstaller::Setup);
	TEST_COMPARE(options.installDir,"/path/to/app");
	TEST_COMPARE(options.packageDir,"/tmp/updater");
	TEST_COMPARE(options.script,"/tmp/updater/file_list.xml");
	TEST_COMPARE(options.waitPid,123456);
}

int main(int,char**)
{
	TestList<TestUpdaterOptions> tests;
	tests.addTest(&TestUpdaterOptions::testOldFormatArgs);
	return TestUtils::runTest(tests);
}

