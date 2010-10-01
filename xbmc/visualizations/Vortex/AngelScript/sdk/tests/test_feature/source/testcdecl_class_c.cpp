//
// This test was designed to test the asOBJ_CLASS_C flag with cdecl
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestCDecl_ClassC"

// A complex class is a class that has a defined constructor or 
// destructor, or an overridden assignment operator. Compilers
// normally treat these classes differently in that they are
// returned by reference even though they are small enough to 
// fit in registers. This is because of the need of exception 
// handling in case something goes wrong.

class ClassC1
{
public:
	ClassC1() {a = 0xDEADC0DE;}
	unsigned long a;
};

class ClassC2
{
public:
	ClassC2() {a = 0xDEADC0DE; b = 0x01234567;}
	unsigned long a;
	unsigned long b;
};

class ClassC3
{
public:
	ClassC3() {a = 0xDEADC0DE; b = 0x01234567; c = 0x89ABCDEF;}
	unsigned long a;
	unsigned long b;
	unsigned long c;
};

static ClassC1 classC1()
{
	ClassC1 c;
	return c;
}

static ClassC2 classC2()
{
	ClassC2 c;
	return c;
}

static ClassC3 classC3()
{
	ClassC3 c;
	return c;
}

static ClassC1 c1;
static ClassC2 c2;
static ClassC3 c3;


bool TestCDecl_ClassC()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("class1", sizeof(ClassC1), asOBJ_CLASS_C);
	engine->RegisterObjectType("class2", sizeof(ClassC2), asOBJ_CLASS_C);
	engine->RegisterObjectType("class3", sizeof(ClassC3), asOBJ_CLASS_C);
	
	engine->RegisterGlobalProperty("class1 c1", &c1);
	engine->RegisterGlobalProperty("class2 c2", &c2);
	engine->RegisterGlobalProperty("class3 c3", &c3);

	engine->RegisterGlobalFunction("class1 _class1()", asFUNCTION(classC1), asCALL_CDECL);
	engine->RegisterGlobalFunction("class2 _class2()", asFUNCTION(classC2), asCALL_CDECL);
	engine->RegisterGlobalFunction("class3 _class3()", asFUNCTION(classC3), asCALL_CDECL);

	COutStream out;

	c1.a = 0;

	engine->SetCommonMessageStream(&out);
	int r = engine->ExecuteString(0, "c1 = _class1();");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}

	if( c1.a != 0xDEADC0DE )
	{
		printf("%s: Failed to assign complex object returned from function. c1.a = %X\n", TESTNAME, c1.a);
		fail = true;
	}


	c2.a = 0;
	c2.b = 0;

	r = engine->ExecuteString(0, "c2 = _class2();");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}

	if( c2.a != 0xDEADC0DE )
	{
		printf("%s: Failed to assign complex object returned from function. c2.a = %X\n", TESTNAME, c2.a);
		fail = true;
	}

	if( c2.b != 0x01234567 )
	{
		printf("%s: Failed to assign complex object returned from function. c2.b = %X\n", TESTNAME, c2.b);
		fail = true;
	}

	c3.a = 0;
	c3.b = 0;
	c3.c = 0;

	r = engine->ExecuteString(0, "c3 = _class3();");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}

	if( c3.a != 0xDEADC0DE )
	{
		printf("%s: Failed to assign complex object returned from function. c3.a = %X\n", TESTNAME, c3.a);
		fail = true;
	}

	if( c3.b != 0x01234567 )
	{
		printf("%s: Failed to assign complex object returned from function. c3.b = %X\n", TESTNAME, c3.b);
		fail = true;
	}

	if( c3.c != 0x89ABCDEF )
	{
		printf("%s: Failed to assign complex object returned from function. c3.c = %X\n", TESTNAME, c3.c);
		fail = true;
	}

	engine->Release();

	return fail;
}
