//
// Test to verify that modules are released correct after use
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestModuleRef"
static const char *script = "int global; void Test() {global = 0;}";

bool TestModuleRef()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->AddScriptSection("a", "script", script, strlen(script), 0);
	if( engine->Build("a") < 0 )
	{
		printf("%s: failed to build module a\n", TESTNAME);
		ret = true;
	}

	int funcID = engine->GetFunctionIDByDecl("a", "void Test()");
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(funcID);

	if( engine->GetFunctionCount("a") < 0 )
	{
		printf("%s: Failed to get function count\n", TESTNAME);
		ret = true;
	}

	engine->Discard("a");
	if( engine->GetFunctionCount("a") != asNO_MODULE )
	{
		printf("%s: Module was not discarded\n", TESTNAME);
		ret = true;
	}

	int r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Execution failed\n", TESTNAME);
		ret = true;
	}

	ctx->Release();

	engine->Release();

	return ret;
}
