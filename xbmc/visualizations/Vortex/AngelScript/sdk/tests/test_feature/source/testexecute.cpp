//
// Tests calling of a c-function from a script
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestExecute"

static bool called = false;

static void cfunction() {
	called = true;
}

bool TestExecute()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void cfunction()", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "cfunction()");

	if (!called) {
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	}

	engine->Release();
	engine = NULL;

	return ret;
}
