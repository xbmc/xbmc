//
// Tests if a c-function can return values to
// the script
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

static bool returned = false;

static bool cfunction() {
	return true;
}

bool TestReturn()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalProperty("bool returned", &returned);
	engine->RegisterGlobalFunction("bool cfunction()", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "returned = cfunction()");

	if (!returned) {
		printf("\nTestReturn: cfunction didn't return properly\n\n");
		ret = true;
	}

	engine->Release();
	engine = NULL;

	return ret;
}
