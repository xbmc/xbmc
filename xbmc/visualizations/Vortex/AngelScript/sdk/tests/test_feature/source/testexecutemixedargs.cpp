//
// Tests calling of a c-function from a script with four parameters
// of different types
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestMixedArgs"

static bool testVal = false;
static bool called = false;

static int    t1 = 0;
static float  t2 = 0;
static double t3 = 0;
static char   t4 = 0;

static void cfunction(int f1, float f2, double f3, int f4) {
	called = true;

	t1 = f1;
	t2 = f2;
	t3 = f3;
	t4 = f4;
	
	testVal = (f1 == 10) && (f2 == 1.92f) && (f3 == 3.88) && (f4 == 97);
}

bool TestExecuteMixedArgs()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void cfunction(int, float, double, int)", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "cfunction(10, 1.92f, 3.88, 97)");
	
	if (!called) {
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} else if (!testVal) {
		printf("\n%s: testVal is not of expected value. Got (%d, %f, %f, %c), expected (%d, %f, %f, %c)\n\n", TESTNAME, t1, t2, t3, t4, 10, 1.92f, 3.88, 97);
		ret = true;
	}

	engine->Release();
	engine = NULL;

	return ret;
}
