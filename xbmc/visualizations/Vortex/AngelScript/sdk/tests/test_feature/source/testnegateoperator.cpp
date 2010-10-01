//
// Tests the negate operator behaviour
//
// Test author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestNegateOperator"

static int testVal = 0;
static bool called = false;

static int negate(int *f1) 
{
	called = true;
	return -*f1;
}

static int minus(int *f1, int *f2)
{
	called = true;
	return *f1 - *f2;
}


bool TestNegateOperator()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterObjectType("obj", sizeof(int), asOBJ_PRIMITIVE);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_NEGATE, "obj f()", asFUNCTION(negate), asCALL_CDECL_OBJLAST);
	engine->RegisterGlobalBehaviour(asBEHAVE_SUBTRACT, "obj f(obj &in, obj &in)", asFUNCTION(minus), asCALL_CDECL);

	engine->RegisterGlobalProperty("obj testVal", &testVal);

	testVal = 1000;

	COutStream obj;
	engine->SetCommonMessageStream(&obj);
	engine->ExecuteString(0, "testVal = -testVal");

	if( !called ) 
	{
		// failure
		printf("\n%s: behaviour function was not called from script\n\n", TESTNAME);
		fail = true;
	} 
	else if( testVal != -1000 ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got %d, expected %d\n\n", TESTNAME, testVal, -1000);
		fail = true;
	}

	called = false;
	engine->ExecuteString(0, "testVal = testVal - testVal");

	if( !called ) 
	{
		// failure
		printf("\n%s: behaviour function was not called from script\n\n", TESTNAME);
		fail = true;
	} 
	else if( testVal != 0 ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got %d, expected %d\n\n", TESTNAME, testVal, 0);
		fail = true;
	}
	

	engine->Release();
	
	// Success
	return fail;
}
