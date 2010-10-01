//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestImport
{

#define TESTNAME "TestImport"




static const char *script1 =
"import void Test(string &in) from \"DynamicModule\"; \n"
"void main()                                          \n"
"{                                                    \n"
"  Test(\"test\");                                    \n"
"}                                                    \n";

static const char *script2 =
"void Test(string &in)  \n"
"{                      \n"
"  number = 1234567890; \n"
"}                      \n";

bool Test()
{
	bool fail = false;

	int number = 0;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);
	engine->RegisterGlobalProperty("int number", &number);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule");

	// Bind all functions that the module imports
	r = engine->BindAllImportedFunctions(0); assert( r >= 0 );

	engine->ExecuteString(0, "main()");

	engine->Release();

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		fail = true;
	}

	// Success
	return fail;
}

} // namespace

