#include "utils.h"
using namespace std;

#define TESTNAME "TestException"

bool TestException()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;	
	asIScriptContext *ctx;
	engine->SetCommonMessageStream(&out);
	int r = engine->ExecuteString(0, "int a = 0;\na = 10/a;", &ctx); // Throws an exception
	if( r == asEXECUTION_EXCEPTION )
	{
		int func = ctx->GetExceptionFunction();
		int line = ctx->GetExceptionLineNumber();
		const char *desc = ctx->GetExceptionString();

		if( func != 0xFFFE )
		{
			printf("%s: Exception function ID is wrong\n", TESTNAME);
			fail = true;
		}
		if( strcmp(engine->GetFunctionName(func), "@ExecuteString") != 0 )
		{
			printf("%s: Exception function name is wrong\n", TESTNAME);
			fail = true;
		}
		if( strcmp(engine->GetFunctionDeclaration(func), "void @ExecuteString()") != 0 )
		{
			printf("%s: Exception function declaration is wrong\n", TESTNAME);
			fail = true;
		}
		if( line != 2 )
		{
			printf("%s: Exception line number is wrong\n", TESTNAME);
			fail = true;
		}
		if( strcmp(desc, "Divide by zero") != 0 )
		{
			printf("%s: Exception string is wrong\n", TESTNAME);
			fail = true;
		}
	}
	else
	{
		printf("%s: Failed to raise exception\n", TESTNAME);
		fail = true;
	}

	ctx->Release();
	engine->Release();

	// Success
	return fail;
}
