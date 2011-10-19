#include "TestFileUtils.h"

#include "FileUtils.h"
#include "TestUtils.h"

void TestFileUtils::testDirName()
{
#ifdef PLATFORM_WINDOWS
	std::string dirName = FileUtils::dirname("E:/Some Dir/App.exe");
	TEST_COMPARE(dirName,"E:/Some Dir/");
#endif
}

int main(int,char**)
{
	TestList<TestFileUtils> tests;
	tests.addTest(&TestFileUtils::testDirName);
	return TestUtils::runTest(tests);
}