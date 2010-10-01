#include "utils.h"
using std::string;
#include "../../../add_on/scriptstring/scriptstring.h"

namespace TestScriptString
{

#define TESTNAME "TestScriptString"

static string printOutput;

// This function receives the string by reference
// (in fact it is a reference to copy of the string)
static void PrintString(asCScriptString &str)
{
	printOutput = str.buffer;
}

// This function shows how to receive an 
// object handle from the script engine
static void SetString(asCScriptString *str)
{
	if( str )
	{
		str->buffer = "Handle to a string";

		// Release the string before returning
		str->Release();
	}
}

// This script tests that variables are created and destroyed in the correct order
static const char *script2 =
"void testString()                         \n"
"{                                         \n"
"  print(getString(\"I\" \"d\" \"a\"));    \n"
"}                                         \n"
"string getString(string &in str)          \n"
"{                                         \n"
"  return \"hello \" + str;                \n"
"}                                         \n";

static const char *script3 = 
"string str = 1;                \n"
"const string str2 = \"test\";  \n"
"void test()                    \n"
"{                              \n"
"   string s = str2;            \n"
"}                              \n";

static const char *script4 = 
"void test()                    \n"
"{                              \n"
"   string s = \"\"\"           \n"
"Heredoc\\x20test\n"
"            \"\"\" \"\\x21\";  \n"
"   print(s);                   \n"
"}                              \n";

static const char *script5 =
"void test()                    \n"
"{                              \n"
"   Func(\"test\");             \n"
"}                              \n"
"void Func(const string &in str)\n"
"{                              \n"
"}                              \n";

bool Test()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(PrintString), asCALL_CDECL);
	engine->RegisterGlobalFunction("void set(string@)", asFUNCTION(SetString), asCALL_CDECL);

	COutStream out;
	engine->SetCommonMessageStream(&out);

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

	printOutput = "";
	engine->ExecuteString(0, "string a; set(@a); print(a);");
	if( printOutput != "Handle to a string" ) fail = true;

    printOutput = "";
    engine->ExecuteString(0, "string a = \" \"; a[0] = 65; print(a);");
    if( printOutput != "A" ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	if( engine->Build(0) < 0 )
		fail = true;

	printOutput = "";
	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0);
	if( engine->Build(0) < 0 )
		fail = true;
	engine->ExecuteString(0, "test()");
	if( printOutput != "Heredoc\\x20test!" ) fail = true;

	asCScriptString *a = new asCScriptString("a");
	engine->RegisterGlobalProperty("string a", a);
	int r = engine->ExecuteString(0, "print(a == \"a\" ? \"t\" : \"f\")");
	if( r != asEXECUTION_FINISHED ) 
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}
	a->Release();

	// Test character literals
	printOutput = "";
	r = engine->ExecuteString(0, "print(\"\" + 'a')");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( printOutput != "97" ) fail = true;

	printOutput = "";
	r = engine->ExecuteString(0, "print(\"\" + '\\'')");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( printOutput != "39" ) fail = true;

	CBufferedOutStream bout;
	engine->SetCommonMessageStream(&bout);
	r = engine->ExecuteString(0, "print(\"\" + '')");
	if( r != -1 ) fail = true;

	//-------------------------------------
	engine->AddScriptSection(0, "test", script5, strlen(script5), 0, false);
	engine->Build(0);
	r = engine->ExecuteString(0, "test()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->Release();

	return fail;
}

} // namespace

