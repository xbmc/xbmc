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

void TestFileUtils::testIsRelative()
{
#ifdef PLATFORM_WINDOWS
	TEST_COMPARE(FileUtils::isRelative("temp"),true);
	TEST_COMPARE(FileUtils::isRelative("D:/temp"),false);
	TEST_COMPARE(FileUtils::isRelative("d:/temp"),false);
#else
	TEST_COMPARE(FileUtils::isRelative("/tmp"),false);
	TEST_COMPARE(FileUtils::isRelative("tmp"),true);
#endif
}

int main(int,char**)
{
	TestList<TestFileUtils> tests;
	tests.addTest(&TestFileUtils::testDirName);
	return TestUtils::runTest(tests);
}
