#include "TestUpdateScript.h"

#include "TestUtils.h"
#include "UpdateScript.h"

#include <iostream>

void TestUpdateScript::testV2Script()
{
	UpdateScript newFormat;
	UpdateScript oldFormat;

	newFormat.parse("file_list.xml");
	oldFormat.parse("v2_file_list.xml");

	TEST_COMPARE(newFormat.dependencies(),oldFormat.dependencies());
	TEST_COMPARE(newFormat.packages(),oldFormat.packages());
	TEST_COMPARE(newFormat.filesToInstall(),oldFormat.filesToInstall());
	TEST_COMPARE(newFormat.filesToUninstall(),oldFormat.filesToUninstall());
}

int main(int,char**)
{
	TestList<TestUpdateScript> tests;
	tests.addTest(&TestUpdateScript::testV2Script);
	return TestUtils::runTest(tests);
}

