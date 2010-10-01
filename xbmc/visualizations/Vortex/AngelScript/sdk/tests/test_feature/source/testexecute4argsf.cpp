//
// Tests calling of a c-function from a script with four float parameters
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestExecute4Argsf"

static bool testVal = false;
static bool called  = false;

static float  t1 = 0;
static float  t2 = 0;
static double t3 = 0;
static float  t4 = 0;

static void cfunction(float f1, float f2, double f3, float f4)
{
	called = true;
	t1 = f1;
	t2 = f2;
	t3 = f3;
	t4 = f4;
	testVal = (f1 == 9.2f) && (f2 == 13.3f) && (f3 == 18.8) && (f4 == 3.1415f);
}


bool TestExecute4Argsf()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void cfunction(float, float, double, float)", asFUNCTION(cfunction), asCALL_CDECL);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->ExecuteString(0, "cfunction(9.2f, 13.3f, 18.8, 3.1415f)");

	if( !called ) 
	{
		// failure
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} 
	else if( !testVal ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got (%f, %f, %f, %f), expected (%f, %f, %f, %f)\n\n", TESTNAME, t1, t2, t3, t4, 9.2f, 13.3f, 18.8, 3.1415f);
		ret = true;
	}

	engine->Release();
	
	// Success
	return ret;
}
