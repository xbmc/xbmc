//
// This test was designed to test the asOBJ_IS_NOT_COMPLEX flag
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestNotComplexStdcall"

#ifdef __GNUC__
#define STDCALL __attribute__((stdcall))
#else
#define STDCALL __stdcall
#endif

class Class1
{
public:
	unsigned long a;
};

class Class2
{
public:
	unsigned long a;
	unsigned long b;
};

class Class3
{
public:
	unsigned long a;
	unsigned long b;
	unsigned long c;
};

static Class1 STDCALL notComplex1()
{
	Class1 c = {0xDEADC0DE};
	return c;
}

static Class2 STDCALL notComplex2()
{
	Class2 c = {0xDEADC0DE, 0x01234567};
	return c;
}

static Class3 STDCALL notComplex3()
{
	Class3 c = {0xDEADC0DE, 0x01234567, 0x89ABCDEF};
	return c;
}

static Class1 c1;
static Class2 c2;
static Class3 c3;


bool TestNotComplexStdcall()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("class1", sizeof(Class1), asOBJ_CLASS);
	engine->RegisterObjectType("class2", sizeof(Class2), asOBJ_CLASS);
	engine->RegisterObjectType("class3", sizeof(Class3), asOBJ_CLASS);
	
	engine->RegisterGlobalProperty("class1 c1", &c1);
	engine->RegisterGlobalProperty("class2 c2", &c2);
	engine->RegisterGlobalProperty("class3 c3", &c3);

	engine->RegisterGlobalFunction("class1 notComplex1()", asFUNCTION(notComplex1), asCALL_STDCALL);
	engine->RegisterGlobalFunction("class2 notComplex2()", asFUNCTION(notComplex2), asCALL_STDCALL);
	engine->RegisterGlobalFunction("class3 notComplex3()", asFUNCTION(notComplex3), asCALL_STDCALL);

	COutStream out;

	c1.a = 0;
	engine->SetCommonMessageStream(&out);

	int r = engine->ExecuteString(0, "c1 = notComplex1();");
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

	r = engine->ExecuteString(0, "c2 = notComplex2();");
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

	r = engine->ExecuteString(0, "c3 = notComplex3();");
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
