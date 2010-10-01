//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestCircularImport
{

#define TESTNAME "TestCircularImport"


static const char *script1 =
"import void Test2() from \"Module2\";   \n"
"void Test1() {}                         \n";

static const char *script2 =
"import void Test1() from \"Module1\";   \n"
"void Test2() {}                         \n";

static void BindImportedFunctions(asIScriptEngine *engine, const char *module);

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection("Module1", TESTNAME ":1", script1, strlen(script1), 0);
	engine->Build("Module1");

	engine->AddScriptSection("Module2", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("Module2");

	BindImportedFunctions(engine, "Module1");
	BindImportedFunctions(engine, "Module2");

	// Discard the modules
	engine->Discard("Module1");
	engine->Discard("Module2");
	
	engine->Release();

	// Success
	return fail;
}

static void BindImportedFunctions(asIScriptEngine *engine, const char *module)
{
	// Bind imported functions
	int c = engine->GetImportedFunctionCount(module);
	for( int n = 0; n < c; ++n )
	{
		const char *decl = engine->GetImportedFunctionDeclaration(module, n);

		// Get module name from where the function should be imported
		const char *moduleName = engine->GetImportedFunctionSourceModule(module, n);

		int funcID = engine->GetFunctionIDByDecl(moduleName, decl);
		engine->BindImportedFunction(module, n, funcID);
	}
}

} // namespace

