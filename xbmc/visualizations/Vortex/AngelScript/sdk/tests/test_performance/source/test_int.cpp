//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestInt
{

#define TESTNAME "TestInt"

static const char *script =
"int N;                                    \n"
"                                          \n"
"void ifunc5()                             \n"
"{                                         \n"
"    N += Average( N, N );                 \n"
"}                                         \n"
"                                          \n"
"void ifunc4()                             \n"
"{                                         \n"
"    N += 2 * Average( N + 1, N + 2 );     \n"
"}                                         \n"
"                                          \n"
"void ifunc3()                             \n"
"{                                         \n"
"    N *= 2 * N;                           \n"
"}                                         \n"
"                                          \n"
"void ifunc2()                             \n"
"{                                         \n"
"    N /= 3;                               \n"
"}                                         \n"
"                                          \n"
"void iRecursion( int nRec )               \n"
"{                                         \n"
"    if ( nRec >= 1 )                      \n"
"        iRecursion( nRec - 1 );           \n"
"                                          \n"
"    if ( nRec == 5 )                      \n"
"        ifunc5();                         \n"
"    else if ( nRec == 4 )                 \n"
"        ifunc4();                         \n"
"    else if ( nRec == 3 )                 \n"
"        ifunc3();                         \n"
"    else if ( nRec == 2 )                 \n"
"        ifunc2();                         \n"
"    else                                  \n"
"        N *= 2;                           \n"
"}                                         \n"
"                                          \n"
"int TestInt()                             \n"
"{                                         \n"
"    N = 0;                                \n"
"    int i = 0;                            \n"
"                                          \n"
"    for ( i = 0; i < 250000; i++ )        \n"
"    {                                     \n"
"        Average( i, i );                  \n"
"        iRecursion( 5 );                  \n"
"                                          \n"
"        if ( N > 100 ) N = 0;             \n"
"    }                                     \n"
"                                          \n"
"    return 0;                             \n"
"}                                         \n";

int Average(int a, int b)
{
	return (a+b)/2;
}

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.4.1             : .6500 secs\n");
	printf("AngelScript 2.5.0 WIP 1       : .6525 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetCommonMessageStream(&out);

	engine->RegisterGlobalFunction("int Average(int, int)", asFUNCTION(Average), asCALL_CDECL);

	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "int TestInt()"));

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







