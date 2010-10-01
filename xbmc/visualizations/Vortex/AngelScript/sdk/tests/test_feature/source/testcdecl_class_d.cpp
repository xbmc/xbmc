//
// This test was designed to test the asOBJ_CLASS_D flag with cdecl
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestCDecl_ClassD"

class ClassD1
{
public:
	~ClassD1() {a = 0;}
	unsigned long a;
};

class ClassD2
{
public:
	~ClassD2() {a = 0; b = 0;}
	unsigned long a;
	unsigned long b;
};

class ClassD3
{
public:
	~ClassD3() {a = 0; b = 0; c = 0;}
	unsigned long a;
	unsigned long b;
	unsigned long c;
};

static ClassD1 classD1()
{
	ClassD1 c = {0xDEADC0DE};
	return c;
}

static ClassD2 classD2()
{
	ClassD2 c = {0xDEADC0DE, 0x01234567};
	return c;
}

static ClassD3 classD3()
{
	ClassD3 c = {0xDEADC0DE, 0x01234567, 0x89ABCDEF};
	return c;
}

static ClassD1 c1;
static ClassD2 c2;
static ClassD3 c3;


bool TestCDecl_ClassD()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("class1", sizeof(ClassD1), asOBJ_CLASS_D);
	engine->RegisterObjectType("class2", sizeof(ClassD2), asOBJ_CLASS_D);
	engine->RegisterObjectType("class3", sizeof(ClassD3), asOBJ_CLASS_D);
	
	engine->RegisterGlobalProperty("class1 c1", &c1);
	engine->RegisterGlobalProperty("class2 c2", &c2);
	engine->RegisterGlobalProperty("class3 c3", &c3);

	engine->RegisterGlobalFunction("class1 _class1()", asFUNCTION(classD1), asCALL_CDECL);
	engine->RegisterGlobalFunction("class2 _class2()", asFUNCTION(classD2), asCALL_CDECL);
	engine->RegisterGlobalFunction("class3 _class3()", asFUNCTION(classD3), asCALL_CDECL);

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
		printf("%s: Failed to assign object returned from function. c1.a = %X\n", TESTNAME, c1.a);
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
		printf("%s: Failed to assign object returned from function. c2.a = %X\n", TESTNAME, c2.a);
		fail = true;
	}

	if( c2.b != 0x01234567 )
	{
		printf("%s: Failed to assign object returned from function. c2.b = %X\n", TESTNAME, c2.b);
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
		printf("%s: Failed to assign object returned from function. c3.a = %X\n", TESTNAME, c3.a);
		fail = true;
	}

	if( c3.b != 0x01234567 )
	{
		printf("%s: Failed to assign object returned from function. c3.b = %X\n", TESTNAME, c3.b);
		fail = true;
	}

	if( c3.c != 0x89ABCDEF )
	{
		printf("%s: Failed to assign object returned from function. c3.c = %X\n", TESTNAME, c3.c);
		fail = true;
	}

	engine->Release();

	return fail;
}
