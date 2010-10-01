//
// Tests releasing a suspended context
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestSuspend
{

#define TESTNAME "TestSuspend"

#ifdef __GNUC__
#define STDCALL __attribute__((stdcall))
#else
#define STDCALL __stdcall
#endif

static int loopCount = 0;

static const char *script1 =
"string g_str = \"test\";    \n" // variable that must be released when module is released
"void TestSuspend()          \n"
"{                           \n"
"  string str = \"hello\";   \n" // variable that must be released before exiting the function
"  while( true )             \n" // never ending loop
"  {                         \n"
"    string a = str + g_str; \n" // variable inside the loop
"    Suspend();              \n"
"    loopCount++;            \n"
"  }                         \n"
"}                           \n";

static const char *script2 = 
"void TestSuspend2()         \n"
"{                           \n"
"  loopCount++;              \n"
"  loopCount++;              \n"
"  loopCount++;              \n"
"}                           \n";

bool doSuspend = false;
void Suspend()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->Suspend();
	doSuspend = true;
}

void STDCALL LineCallback(asIScriptContext *ctx, void *param)
{
	// Suspend immediately
	ctx->Suspend();
}

bool Test()
{
	bool fail = false;

	//---
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	// Verify that the function doesn't crash when the stack is empty
	asIScriptContext *ctx = asGetActiveContext();
	assert( ctx == 0 );

	RegisterScriptString(engine);
	
	engine->RegisterGlobalFunction("void Suspend()", asFUNCTION(Suspend), asCALL_CDECL);
	engine->RegisterGlobalProperty("int loopCount", &loopCount);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);

	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	ctx = engine->CreateContext();
	ctx->SetLineCallback(asFUNCTION(LineCallback), 0, asCALL_STDCALL);
	if( ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestSuspend()")) >= 0 )
	{
		while( loopCount < 5 && !doSuspend )
			ctx->Execute();
	}
	else
		fail = true;

	// Release the engine first
	engine->Release();

	// Now release the context
	ctx->Release();
	//---
	// If the library was built with the flag BUILD_WITH_LINE_CUES the script
	// will return after each increment of the loopCount variable.
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalProperty("int loopCount", &loopCount);
	engine->AddScriptSection(0, TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build(0);

	ctx = engine->CreateContext();
	ctx->SetLineCallback(asFUNCTION(LineCallback), 0, asCALL_STDCALL);
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestSuspend2()"));
	loopCount = 0;
	while( ctx->GetState() != asEXECUTION_FINISHED )
		ctx->Execute();
	if( loopCount != 3 )
	{
		printf("%s: failed\n", TESTNAME);
		fail = true;
	}

	ctx->Prepare(asPREPARE_PREVIOUS);
	loopCount = 0;
	while( ctx->GetState() != asEXECUTION_FINISHED )
		ctx->Execute();
	if( loopCount != 3 )
	{
		printf("%s: failed\n", TESTNAME);
		fail = true;
	}

	ctx->Release();
	engine->Release();

	// Success
	return fail;
}

} // namespace

