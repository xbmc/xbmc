//
// Tests the ternary operator ?:
//
// Author: Andreas Jonsson
//

#include "utils.h"
#include "stdstring.h"

#define TESTNAME "TestCondition"

using std::string;
static string a;

static const char *script1 =
"void Test(string strA, string strB)   \n"
"{                                     \n"
"  a = true ? strA : strB;             \n"
"  a = false ? \"t\" : \"f\";          \n"
"  SetAttrib(true ? strA : strB);      \n"
"  SetAttrib(false ? \"t\" : \"f\");   \n"
"}                                     \n"
"void SetAttrib(string str) {}         \n";
/*
static const char *script2 = 
"void Test()                     \n"
"{                               \n"
"  int a = 0;                    \n"
"  Data *v = 0;                  \n"
"  Data *p;                      \n"
"  p = a != 0 ? v : 0;           \n"
"  p = a == 0 ? 0 : v;           \n"
"}                               \n";
*/
static const char *script3 =
"void Test()                                  \n"
"{                                            \n"
"  int test = 5;                              \n"
"  int test2 = int((test == 5) ? 23 : 12);    \n"
"}                                            \n";

static string format(float f)
{
	char buffer[25];
	sprintf(buffer, "%f", f);
	return string(buffer);
}

static string format(asUINT ui)
{
	char buffer[25];
	sprintf(buffer, "%d", ui);
	return string(buffer);
}

static void print(string &str)
{
//	printf((str + "\n").c_str());
}

bool TestCondition()
{
	bool fail = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterStdString(engine);
	engine->RegisterGlobalProperty("string a", &a);

	engine->RegisterObjectType("Data", 0, 0);

	engine->RegisterGlobalFunction("string format(float)", asFUNCTIONPR(format, (float), string), asCALL_CDECL);
	engine->RegisterGlobalFunction("string format(uint)", asFUNCTIONPR(format, (asUINT), string), asCALL_CDECL);
	engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(print), asCALL_CDECL);

	COutStream out;	
	engine->SetCommonMessageStream(&out);
	r = engine->ExecuteString(0, "print(a == \"a\" ? \"t\" : \"f\")");
	if( r < 0 ) 
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->Build(0);

	r = engine->ExecuteString(0, "Test(\"t\", \"f\")");
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

/*	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->Build(0);

	r = engine->ExecuteString(0, "Test()");
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}
*/
	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	engine->Build(0);

	r = engine->ExecuteString(0, "Test()");
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "bool b = true; print(\"Test: \" + format(float(b ? 15 : 0)));");
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "bool b = true; print(\"Test: \" + format(b ? 15 : 0));");
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	engine->Release();

	// Success
	return fail;
}
