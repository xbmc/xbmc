//
// Tests sending 8byte objects in a three levels deep function stack
//
// Sent in by: Adam Hoult (2004/07/02)
//

#include "utils.h"

#define TESTNAME "TestInt64"

static char *script = 
"int Main()\n"
"{\n"
"    // Call test\n"
"    foo();\n"

"    // WHEN THE ABOVE 'foo' IS CALLED, THE SCRIPT JUST BAILS ONCE IT HAS RETURNED\n"
"    // AND NOTHING AFTER THIS POINT IS EVER EXECUTED. IF WE CALL 'BAR' DIRECTLY\n"
"    // FROM HERE, BYPASSING 'foo' AS BELOW, IT CONTINUES ON WITHOUT ISSUE.\n"

"    // I added some debug output here to a 'MessageBox' call, but it too was never\n"
"    // executed.\n"
"    cfunction();\n"

"    int64 var;\n"
"    bar( var );\n"
    
"    // Some value we'll know when we return\n"
"    return 31337;\n"
"}\n"

"void foo( )\n"
"{\n"
"    int64 var;\n"
"    bar( var );\n"
"}\n"

"void bar( int64 )\n"
"{\n"
"    // If any of the three cases are satisfied, the code works as expected.\n"
"    // --------------------------------------------------------------------\n"
"    // 1) We bypass the outer 'foo' function, and call 'bar' directly.\n"
"    // 2) We change the size of the user defined type to be 4 bytes.\n"
"    // 3) We make the parameter a reference of the user defined type.\n"
"    // --------------------------------------------------------------------\n"
"    cfunction();\n"
"}\n";

static int called = 0;

static void cfunction() {
	++called;
}

bool TestInt64()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterObjectType("int64", 8, 0);
	engine->RegisterGlobalFunction("void cfunction()", asFUNCTION(cfunction), asCALL_CDECL);

	COutStream out;
	engine->AddScriptSection(0, "test", script, strlen(script), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	int f = engine->GetFunctionIDByDecl(0, "int Main()");
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(f);
	int r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		printf("\n%s: The execution didn't finish correctly (code %d)\n", TESTNAME, r);
		ret = true;

		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
	}
	
    if( called != 3 ) 
	{
		printf("\n%s: cfunction called %d times. Expected 3 times\n", TESTNAME, called);
		ret = true;
	}

	ctx->Release();
	engine->Release();

	return ret;
}
