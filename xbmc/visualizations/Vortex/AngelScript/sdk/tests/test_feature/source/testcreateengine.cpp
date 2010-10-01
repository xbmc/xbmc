//
// Test to see if engine can be created.
//
// Author: Fredrik Ehnbom
//

#include "utils.h"

bool TestCreateEngine()
{
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( engine == 0 )
	{
		// Failure
		printf("TestCreateEngine: asCreateScriptEngine() failed\n");
		return true;
	}
	else
		engine->Release();
	
	// Success
	return false;
}
