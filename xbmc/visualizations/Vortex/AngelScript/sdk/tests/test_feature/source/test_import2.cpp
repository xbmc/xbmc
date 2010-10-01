//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestImport2
{

#define TESTNAME "TestImport2"




static const char *script1 =
"import void Test() from \"DynamicModule\";   \n"
"void Run()                                   \n"
"{                                            \n"
"  Test();                                    \n"
"}                                            \n";

static const char *script2 =
"void Test()             \n"
"{                       \n"
"  // Cause an exception \n"
"  CheckFunc();          \n"
"}                       \n";

bool failed = false;

void CheckFunc()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
	{
		asIScriptEngine *engine = ctx->GetEngine();
		int funcID = ctx->GetCurrentFunction();
		if( strcmp(engine->GetModuleNameFromIndex(funcID>>16), "DynamicModule") != 0 )
			failed = true;

		if( strcmp(engine->GetFunctionDeclaration(funcID), "void Test()") != 0 )
			failed = true;

		ctx->SetException("Generated exception");
	}
	else
		failed = true;
}

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalFunction("void CheckFunc()", asFUNCTION(CheckFunc), asCALL_CDECL);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule");

	// Bind all functions that the module imports
	engine->BindAllImportedFunctions(0);

	asIScriptContext *ctx;
	int r = engine->ExecuteString(0, "Run()", &ctx);
	if( r == asEXECUTION_EXCEPTION )
	{
		int funcID = ctx->GetExceptionFunction();
		if( strcmp(engine->GetModuleNameFromIndex(funcID>>16), "DynamicModule") != 0 )
			failed = true;

		if( strcmp(engine->GetFunctionDeclaration(funcID), "void Test()") != 0 )
			failed = true;
	}
	if( ctx ) ctx->Release();
	engine->Release();

	if( failed )
	{
		fail = true;
		printf("%s: failed\n", TESTNAME);
	}
	

	// Success
	return fail;
}

} // namespace

