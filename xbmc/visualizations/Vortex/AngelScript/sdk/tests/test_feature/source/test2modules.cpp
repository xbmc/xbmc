//
// Test compiling scripts in two named modules
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "Test2Modules"
static const char *script = "int global; void Test() {global = 0;} float Test2() {Test(); return 0;}";

bool Test2Modules()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->AddScriptSection("a", "script", script, strlen(script), 0, false);
	if( engine->Build("a") < 0 )
	{
		printf("%s: failed to build module a\n", TESTNAME);
		ret = true;
	}

	engine->AddScriptSection("b", "script", script, strlen(script), 0, false);
	if( engine->Build("b") < 0 )
	{
		printf("%s: failed to build module b\n", TESTNAME);
		ret = true;
	}

	if( !ret )
	{
		int aFuncID = engine->GetFunctionIDByName("a", "Test");
		if( aFuncID < 0 )
		{
			printf("%s: failed to retrieve func ID for module a\n", TESTNAME);
			ret = true;
		}

		int bFuncID = engine->GetFunctionIDByName("b", "Test");
		if( bFuncID < 0 )
		{
			printf("%s: failed to retrieve func ID for module b\n", TESTNAME);
			ret = true;
		}
	}

	engine->Release();

	return ret;
}
