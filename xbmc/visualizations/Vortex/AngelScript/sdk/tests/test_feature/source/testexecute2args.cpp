//
// Tests calling of a c-function from a script with two parameters
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestExecute2Args"

static int testVal = 0;
static bool called = false;

static void cfunction(int f1, int f2) 
{
	called = true;
	testVal = f1 + f2;
}

bool TestExecute2Args()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void cfunction(int, int)", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "cfunction(5, 9)");

	if( !called ) 
	{
		// failure
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} 
	else if( testVal != (5 + 9) ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got %d, expected %d\n\n", TESTNAME, testVal, (5 + 9));
		ret = true;
	}

	engine->Release();
	
	// Success
	return ret;
}
