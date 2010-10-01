//
// Tests constant properties to see if they can be overwritten
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestConstProperty
{

#define TESTNAME "TestConstProperty"


static const char *script =
//"Obj1 myObj1;         \n"
//"Obj2 myObj2;         \n"
"float myFloat;       \n"
"                     \n"
"void Init()          \n"
"{                    \n"
//"  g_Obj1 = myObj1;   \n"
//"  g_Obj2 = myObj2;   \n"
"  g_Float = myFloat; \n"
"}                    \n";

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("Obj1", sizeof(int), asOBJ_PRIMITIVE);
	engine->RegisterObjectProperty("Obj1", "int val", 0);
	engine->RegisterObjectBehaviour("Obj1", asBEHAVE_ASSIGNMENT, "Obj1 &f(Obj1 &in)", asFUNCTION(0), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectType("Obj2", sizeof(int), asOBJ_PRIMITIVE);
	engine->RegisterObjectProperty("Obj2", "int val", 0);

//	int constantProperty1 = 0;
//	engine->RegisterGlobalProperty("const Obj1 g_Obj1", &constantProperty1);

//	int constantProperty2 = 0;
//	engine->RegisterGlobalProperty("const Obj2 g_Obj2", &constantProperty2);

	float constantFloat = 0;
	engine->RegisterGlobalProperty("const float g_Float", &constantFloat);

	CBufferedOutStream out;
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	if( out.buffer != "TestConstProperty (3, 1) : Info    : Compiling void Init()\n"
		              "TestConstProperty (5, 11) : Error   : Reference is read-only\n" )
	{
		printf("%s: Failed to detect all properties as constant\n%s", TESTNAME, out.buffer.c_str());
		fail = true;
	}

	engine->Release();


	// Success
	return fail;
}

} // namespace

