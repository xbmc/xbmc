//
// This test shows how to register the std::string to be used in the scripts.
// It also used to verify that objects are always constructed before destructed.
//
// Author: Andreas Jönsson
//

#include "utils.h"
#include "stdstring.h"
using namespace std;

#define TESTNAME "TestStdString"

static string printOutput;

static void PrintString(string &str)
{
	printOutput = str;
}

static void PrintStringVal(string str)
{
	printOutput = str;
}

// This script tests that variables are created and destroyed in the correct order
static const char *script =
"void blah1()\n"
"{\n"
"	if(true)\n"
"		return;\n"
"\n"
"	string blah = \"Bleh1!\";\n"
"}\n"
"\n"
"void blah2()\n"
"{\n"
"	string blah = \"Bleh2!\";\n"
"\n"
"	if(true)\n"
"		return;\n"
"}\n";

static const char *script2 =
"void testString()                         \n"
"{                                         \n"
"  print(getString(\"I\" \"d\" \"a\"));    \n"
"}                                         \n"
"string getString(string &in str)          \n"
"{                                         \n"
"  return \"hello \" + str;                \n"
"}                                         \n"
"void testString2()                        \n"
"{                                         \n"
"  string str = \"Hello World!\";          \n"
"  printVal(str);                          \n"
"}                                         \n";

static const char *script3 = 
"string str = 1;                \n"
"const string str2 = \"test\";  \n"
"obj a(\"test\");               \n"
"void test()                    \n"
"{                              \n"
"   string s = str2;            \n"
"}                              \n";

static void Construct1(void *o)
{

}

static void Construct2(string &str, void *o)
{

}

static void Destruct(void *o)
{

}

static void StringByVal(string &str1, string str2)
{
	assert( str1 == str2 );
}

//--> new: object method string argument test
class StringConsumer 
{
public:
	void Consume(string str)
	{
		printOutput = str;
	} 
};
static StringConsumer consumerObject;
//<-- new: object method string argument test


bool TestStdString()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(PrintString), asCALL_CDECL);
	engine->RegisterGlobalFunction("void printVal(string)", asFUNCTION(PrintStringVal), asCALL_CDECL);

	engine->RegisterObjectType("obj", 4, asOBJ_PRIMITIVE);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct1), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f(string &in)", asFUNCTION(Construct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);

	engine->RegisterGlobalFunction("void StringByVal(string &in, string)", asFUNCTION(StringByVal), asCALL_CDECL);

	//--> new: object method string argument test
	engine->RegisterObjectType("StringConsumer", 0, asOBJ_CLASS);
	engine->RegisterObjectMethod("StringConsumer", "void Consume(string str)", asMETHOD(StringConsumer,Consume), asCALL_THISCALL);
	engine->RegisterGlobalProperty("StringConsumer consumerObject", &consumerObject);
	//<-- new: object method string argument test

	COutStream out;

	engine->AddScriptSection(0, "string", script, strlen(script), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	int r = engine->ExecuteString(0, "blah1(); blah2();");
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->Build(0);

	engine->ExecuteString(0, "testString()");

	if( printOutput != "hello Ida" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}

	engine->ExecuteString(0, "string s = \"test\\\\test\\\\\"");

	// Verify that it is possible to use the string in constructor parameters
	engine->ExecuteString(0, "obj a; a = obj(\"test\")");
	engine->ExecuteString(0, "obj a(\"test\")");

	// Verify that it is possible to pass strings by value
	printOutput = "";
	engine->ExecuteString(0, "testString2()");
	if( printOutput != "Hello World!" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}

	printOutput = "";
	engine->ExecuteString(0, "string a; a = 1; print(a);");
	if( printOutput != "1" ) fail = true;
	
	printOutput = "";
	engine->ExecuteString(0, "string a; a += 1; print(a);");
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = \"a\" + 1; print(a);");
	if( printOutput != "a1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = 1 + \"a\"; print(a);");
	if( printOutput != "1a" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = 1; print(a);");
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "print(\"a\" + 1.2)");
	if( printOutput != "a1.2") fail = true;

	printOutput = "";
	engine->ExecuteString(0, "print(1.2 + \"a\")");
	if( printOutput != "1.2a") fail = true;

	engine->ExecuteString(0, "StringByVal(\"test\", \"test\")");

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	if( engine->Build(0) < 0 )
	{
		fail = true;
	}

	//--> new: object method string argument test
	printOutput = "";
	engine->ExecuteString(0, "consumerObject.Consume(\"This is my string\")");
	if( printOutput != "This is my string") fail = true;
	//<-- new: object method string argument test


	engine->Release();

	return fail;
}
