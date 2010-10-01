//
// Tests if a c-function can return float
// values to the script
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestReturn (float)"

static float returnValue = 0.0f;

static float cfunction() 
{
	return 18.87f;
}

bool TestReturnF()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalProperty("float returnValue", &returnValue);
	engine->RegisterGlobalFunction("float cfunction()", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "returnValue = cfunction()");

	if( returnValue != 18.87f ) 
	{
		printf("\n%s: cfunction didn't return properly. Expected %f, got %f\n\n", TESTNAME, 18.87f, returnValue);
		ret = true;
	}

	engine->Release();
	
	// Success
	return ret;
}
