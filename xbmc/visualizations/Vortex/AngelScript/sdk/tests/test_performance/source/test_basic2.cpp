//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestBasic2
{

#define TESTNAME "TestBasic2"

static const char *script =
"void TestBasic2()                      \n"
"{                                      \n"
"    float a = 1, b = 2, c = 3;         \n"
"    int i = 0;                         \n"
"                                       \n"
"    for ( i = 0; i < 10000000; i++ )   \n"
"    {                                  \n"
"       a = a + b * c;                  \n"
"       if( a == 0 )                    \n"
"         a = 100.0f;                   \n"
"       if( b == 1 )                    \n"
"         b = 2;                        \n"
"    }                                  \n"
"}                                      \n";

void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.4.1              : 2.365 secs\n");
	printf("AngelScript 2.5.0 WIP 1        : 1.937 secs\n");


	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestBasic2()"));

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();

	int r = ctx->Execute();

	time = GetSystemTimer() - time;

	if( r != asEXECUTION_FINISHED )
	{
		printf("Execution didn't terminate with asEXECUTION_FINISHED\n", TESTNAME);
		if( r == asEXECUTION_EXCEPTION )
		{
			printf("Script exception\n");
			printf("Func: %s\n", engine->GetFunctionName(ctx->GetExceptionFunction()));
			printf("Line: %d\n", ctx->GetExceptionLineNumber());
			printf("Desc: %s\n", ctx->GetExceptionString());
		}
	}
	else
		printf("Time = %f secs\n", time);

	ctx->Release();
	engine->Release();
}

} // namespace


