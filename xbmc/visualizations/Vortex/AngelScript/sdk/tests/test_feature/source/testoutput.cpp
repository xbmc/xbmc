// 
// Test designed to verify the compiler message output
//
// Author: Andreas Jönsson
//

#include "utils.h"

namespace TestOutput
{
#define TESTNAME "TestOutput"


static const char *script1 =
"void test()                   \n"  // 1
"{                             \n"  // 2
"  // This is a semantic error \n"  // 3
"  a = a;                      \n"  // 4
"}                             \n"; // 5

static const char *script2 =
"// This is a syntax error     \n"  // 1
"a = a;                        \n"; // 2

bool Test()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	CBufferedOutStream out;
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->Build(0);

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->Build(0);

	if( out.buffer != "TestOutput (1, 1) : Info    : Compiling void test()\n"
		              "TestOutput (4, 7) : Error   : 'a' is not declared\n"
		              "TestOutput (2, 3) : Error   : Expected identifier\n" )
	{
		fail = true;
		printf("%s: Output is not what was expected, got:\n%s\n", TESTNAME, out.buffer.c_str());
	}

	engine->Release();

	return fail;
}

} // namespace
