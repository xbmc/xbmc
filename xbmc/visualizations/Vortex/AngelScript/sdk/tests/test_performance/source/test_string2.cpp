//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "../../add_on/scriptstring/scriptstring.h"

namespace TestString2
{

#define TESTNAME "TestString2"

static const char *script =
"string BuildString2(string@ a, string@ b, string@ c)            \n"
"{                                                               \n"
"    return a + b + c;                                           \n"
"}                                                               \n"
"                                                                \n"
"void TestString2()                                              \n"
"{                                                               \n"
"    string a = \"Test\";                                        \n"
"    string b = \" \";                                           \n"
"    string c = \"string\";                                      \n"
"    string res;                                                 \n"
"    int i = 0;                                                  \n"
"                                                                \n"
"    for ( i = 0; i < 1000000; i++ )                             \n"
"    {                                                           \n"
"        res = BuildString2(a, b, c);                            \n"
"    }                                                           \n"
"}                                                               \n";

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.4.1             : 5.490 secs\n");
	printf("AngelScript 2.5.0 WIP 1       : 3.310 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetCommonMessageStream(&out);

	RegisterScriptString(engine);

	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestString2()"));

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();

	int r = ctx->Execute();

	time = GetSystemTimer() - time;

	if( r != 0 )
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







