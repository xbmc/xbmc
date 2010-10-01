#include "utils.h"
using namespace std;

#define TESTNAME "TestNested"

static const char *script1 =
"void TestNested()                         \n"
"{                                         \n"
"  CallExecuteString(\"i = 2\");           \n"
"  i = i + 2;                              \n"
"}                                         \n";

static void CallExecuteString(string &str)
{
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptEngine *engine = ctx->GetEngine();
	if( engine->ExecuteString(0, str.c_str()) < 0 )
		ctx->SetException("ExecuteString() failed\n");
}

static int i = 0;

bool TestNested()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	engine->RegisterGlobalProperty("int i", &i);
	engine->RegisterGlobalFunction("void CallExecuteString(string &in)", asFUNCTION(CallExecuteString), asCALL_CDECL);

	COutStream out;	

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	// Make the call with a separate context (should work)
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByIndex(0, 0));
	ctx->Execute();

	if( i != 4 )
	{
		printf("%s: Failed to call nested ExecuteString() from other context\n", TESTNAME);
		fail = true;
	}

	ctx->Release();

	// Make the call with ExecuteString 
	i = 0;
	int r = engine->ExecuteString(0, "TestNested()");
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: ExecuteString() didn't succeed\n", TESTNAME);
		fail = true;
	}

	if( i != 4 )
	{
		printf("%s: Failed to call nested ExecuteString() from ExecuteString()\n", TESTNAME);
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}
