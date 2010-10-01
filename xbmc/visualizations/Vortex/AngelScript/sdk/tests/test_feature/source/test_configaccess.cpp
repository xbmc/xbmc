#include "utils.h"

namespace TestConfigAccess
{
          
const char * const TESTNAME = "TestConfigAccess";

static void Func()
{
}

static int TypeAdd(int &a, int &b)
{
	return a + b;
}

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;

	float val;

	//------------
	// Test global properties
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream((asOUTPUTFUNC_t)printf, 0);

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("float val", &val); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	// Make sure the default access is granted
	r = engine->ExecuteString(0, "val = 1.3f"); 
	if( r < 0 )
		fail = true;

	// Make sure the default access can be turned off
	engine->SetCommonMessageStream(&bout);
	r = engine->SetConfigGroupModuleAccess("group", asALL_MODULES, false); assert( r >= 0 );

	r = engine->ExecuteString(0, "val = 1.0f");
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "ExecuteString (1, 1) : Error   : 'val' is not declared\n"
		               "ExecuteString (1, 5) : Error   : Reference is read-only\n"
		               "ExecuteString (1, 5) : Error   : Not a valid lvalue\n")
		fail = true;

	// Make sure the default access can be overridden
	engine->SetCommonMessageStream((asOUTPUTFUNC_t)printf, 0);
	r = engine->SetConfigGroupModuleAccess("group", 0, true); assert( r >= 0 );

	r = engine->ExecuteString(0, "val = 1.0f");
	if( r < 0 )
		fail = true;

	engine->Release();

	//----------
	// Test global functions
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream((asOUTPUTFUNC_t)printf, 0);

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void Func()", asFUNCTION(Func), asCALL_CDECL); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->SetConfigGroupModuleAccess("group", "m", false); assert( r >= 0 );

	engine->SetCommonMessageStream(&bout);
	bout.buffer = "";
	r = engine->ExecuteString("m", "Func()");
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "ExecuteString (1, 1) : Error   : No matching signatures to 'Func()'\n" )
		fail = true;

	r = engine->SetConfigGroupModuleAccess("group", "m", true); assert( r >= 0 );

	engine->SetCommonMessageStream((asOUTPUTFUNC_t)printf, 0);
	r = engine->ExecuteString("m", "Func()");
	if( r < 0 )
		fail = true;

	engine->Release();

	//------------
	// Test object types
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream((asOUTPUTFUNC_t)printf, 0);

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_PRIMITIVE); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->SetConfigGroupModuleAccess("group", 0, false); assert( r >= 0 );

	engine->SetCommonMessageStream(&bout);
	bout.buffer = "";
	r = engine->ExecuteString(0, "mytype a");
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "ExecuteString (1, 1) : Error   : Type 'mytype' is not available for this module\n")
		fail = true;

	engine->Release();

	//------------
	// Test global operators
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream((asOUTPUTFUNC_t)printf, 0);

	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_PRIMITIVE); assert( r >= 0 );

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "mytype f(mytype &in, mytype &in)", asFUNCTION(TypeAdd), asCALL_CDECL); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->SetConfigGroupModuleAccess("group", 0, false); assert( r >= 0 );

	engine->SetCommonMessageStream(&bout);
	bout.buffer = "";
	r = engine->ExecuteString(0, "mytype a; a + a;");
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "ExecuteString (1, 13) : Error   : No matching operator that takes the type 'mytype&' found\n")
		fail = true;

	engine->Release();

	// Success
	return fail;
}

}
