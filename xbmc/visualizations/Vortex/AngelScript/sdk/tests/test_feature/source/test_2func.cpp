//
// Tests compiling with 2 equal functions
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace Test2Func
{

#define TESTNAME "Test2Func"


static const char *script1 =
"void Test() { } \n"
"void Test() { } \n";

static const char *script2 = 
"void Test(void) { } \n";

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	engine->Build(0);

	if( out.buffer != "Test2Func (2, 1) : Error   : A function with the same name and parameters already exist\n" )
	{
		fail = true;
		printf("%s: Failed to identify the error with two equal functions\n", TESTNAME);
	}

	out.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( out.buffer != "Test2Func (1, 1) : Info    : Compiling void Test(void)\nTest2Func (1, 11) : Error   : Parameter type can't be 'void'\n" )
		fail = true;





	engine->Release();

	if( fail )
	{
		printf("%s: failed\n", TESTNAME);
	}

	// Success
	return fail;
}

} // namespace

