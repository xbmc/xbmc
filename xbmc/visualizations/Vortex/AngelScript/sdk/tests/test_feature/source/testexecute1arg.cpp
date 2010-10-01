//
// Tests calling of a c-function from a script with one parameter
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestExecute1Arg"

static int testVal = 0;
static bool called = false;

static void cfunction(int f1) 
{
	called = true;
	testVal = f1;
}

bool TestExecute1Arg()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void cfunction(int)", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "cfunction(5)");

	if( !called ) 
	{
		// failure
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} 
	else if( testVal != 5 ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got %d, expected %d\n\n", TESTNAME, testVal, 5);
		ret = true;
	}

	engine->Release();
	
	// Success
	return ret;
}
