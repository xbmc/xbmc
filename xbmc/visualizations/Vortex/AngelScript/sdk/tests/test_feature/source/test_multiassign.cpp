//
// Tests assigning a value to multiple objects in one statement
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestMultiAssign
{

#define TESTNAME "TestMultiAssign"



static const char *script = 
"void Init()            \n"
"{                      \n"
"  a = b = c = d = clr; \n"
"}                      \n";


static asDWORD a, b, c, d, clr;

static asDWORD &Assign(asDWORD &src, asDWORD &dst)
{
	dst = src;
	return dst;
}

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("CLR", sizeof(asDWORD), asOBJ_PRIMITIVE);
	engine->RegisterObjectBehaviour("CLR", asBEHAVE_ASSIGNMENT, "CLR &f(CLR &in)", asFUNCTION(Assign), asCALL_CDECL_OBJLAST);

	engine->RegisterGlobalProperty("CLR a", &a);
	engine->RegisterGlobalProperty("CLR b", &b);
	engine->RegisterGlobalProperty("CLR c", &c);
	engine->RegisterGlobalProperty("CLR d", &d);
	engine->RegisterGlobalProperty("CLR clr", &clr);

	a = b = c = d = 0;
	clr = 0x12345678;

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	engine->ExecuteString(0, "Init();");

	if( a != 0x12345678 || b != 0x12345678 || c != 0x12345678 || d != 0x12345678 )
	{
		fail = true;
		printf("%s: Failed to assign all objects equally\n", TESTNAME);
	}

	if( clr != 0x12345678 )
	{
		fail = true;
		printf("%s: Src object changed during operation\n", TESTNAME);
	}

	engine->Release();


	// Success
	return fail;
}

} // namespace

