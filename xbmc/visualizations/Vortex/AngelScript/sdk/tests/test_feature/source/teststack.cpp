// 
// Test designed to verify functionality of the dynamically growing stack
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestStack"


static const char *script =
"void recursive(int n) \n"  // 1
"{                     \n"  // 2
"  if( n > 0 )         \n"  // 3
"    recursive(n - 1); \n"  // 4
"}                     \n"; // 5

bool TestStack()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->SetCommonMessageStream(&out);
	int r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: Failed to build script\n", TESTNAME);
		fail = true;
	}

	asIScriptContext *ctx = engine->CreateContext();
	engine->SetDefaultContextStackSize(0, 32); // Minumum stack, 32 byte limit
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void recursive(int)"));
	ctx->SetArgDWord(0, 100);
	r = ctx->Execute();
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Execution didn't throw an exception as was expected\n", TESTNAME);
		fail = true;
	}

	ctx->Release();
	engine->Release();

	return fail;
}
