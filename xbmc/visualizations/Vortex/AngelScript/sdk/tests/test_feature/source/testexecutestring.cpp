//
// Tests ExecuteString() with multiple lines of code
//
// Test author: Andreas Jonsson
//

#include "utils.h"

#define TESTNAME "TestExecuteString"

struct Obj
{
	bool a;
	bool b;
} g_Obj;


bool TestExecuteString()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("Obj", sizeof(Obj), asOBJ_CLASS);
	engine->RegisterObjectProperty("Obj", "bool a", offsetof(Obj,a));
	engine->RegisterObjectProperty("Obj", "bool b", offsetof(Obj,b));

	engine->RegisterGlobalProperty("Obj g_Obj", &g_Obj);

	g_Obj.a = false;
	g_Obj.b = true;

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->ExecuteString(0, "g_Obj.a = true;\n"
		                     "g_Obj.b = false;\n");

	engine->Release();

	if( !g_Obj.a || g_Obj.b )
	{
		printf("%s: ExecuteString() didn't execute correctly\n", TESTNAME);
		fail = true;
	}
	
	// Success
	return fail;
}
