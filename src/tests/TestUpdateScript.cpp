#include "TestUpdateScript.h"

#include "TestUtils.h"
#include "UpdateScript.h"

#include <iostream>
#include <algorithm>

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

void TestUpdateScript::testPermissions()
{
	UpdateScript script;
	script.parse("file_list.xml");

	for (std::vector<UpdateScriptFile>::const_iterator iter = script.filesToInstall().begin();
	     iter != script.filesToInstall().end();
	     iter++)
	{
		if (iter->isMainBinary)
		{
			TEST_COMPARE(iter->permissions,0755);
		}
		if (!iter->linkTarget.empty())
		{
			TEST_COMPARE(iter->permissions,0);
		}
	}
}

int main(int,char**)
{
	TestList<TestUpdateScript> tests;
	tests.addTest(&TestUpdateScript::testV2Script);
	tests.addTest(&TestUpdateScript::testPermissions);
	return TestUtils::runTest(tests);
}

