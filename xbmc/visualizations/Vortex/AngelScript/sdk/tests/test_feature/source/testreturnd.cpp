//
// Tests if a c-function can return double
// values to the script
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestReturn (double)"

static double returnValue = 0.0f;

static double cfunction() 
{
	return 88.32;
}

bool TestReturnD()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalProperty("double returnValue", &returnValue);
	engine->RegisterGlobalFunction("double cfunction()", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "returnValue = cfunction()");

	if( returnValue != 88.32 ) 
	{
		printf("\n%s: cfunction didn't return properly. Expected %f, got %f\n\n", TESTNAME, 88.32, returnValue);
		ret = true;
	}

	engine->Release();
	
	// Success
	return ret;
}
