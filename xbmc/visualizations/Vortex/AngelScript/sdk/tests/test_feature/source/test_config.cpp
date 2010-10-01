#include "utils.h"

namespace TestConfig
{

#define TESTNAME "TestConfig"

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;
	CBufferedOutStream bout;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream(0); // Make sure this works
	engine->SetCommonMessageStream(&bout);

	r = engine->RegisterGlobalFunction("void func(mytype)", asFUNCTION(0), asCALL_CDECL);
	if( r >= 0 ) fail = true;

	r = engine->RegisterGlobalFunction("void func(int &)", asFUNCTION(0), asCALL_CDECL);
#ifndef AS_ALLOW_UNSAFE_REFERENCES
	if( r >= 0 ) fail = true;
#else
	if( r < 0 ) fail = true;
#endif
	
	r = engine->RegisterObjectType("mytype", 0, 0);
	if( r < 0 ) fail = true;

	r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_CONSTRUCT, "void f(othertype)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	if( r >= 0 ) fail = true;

	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "type f(type &, int)", asFUNCTION(0), asCALL_CDECL);
	if( r >= 0 ) fail = true;

	r = engine->RegisterGlobalProperty("type a", 0);
	if( r >= 0 ) fail = true;

	r = engine->RegisterObjectMethod("mytype", "void method(int &)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
#ifndef AS_ALLOW_UNSAFE_REFERENCES
	if( r >= 0 ) fail = true;
#else
	if( r < 0 ) fail = true;
#endif

	r = engine->RegisterObjectProperty("mytype", "type a", 0);
	if( r >= 0 ) fail = true;

	r = engine->RegisterStringFactory("type", asFUNCTION(0), asCALL_CDECL);
	if( r >= 0 ) fail = true;

	engine->Release();

	// Verify the output messages
#ifndef AS_ALLOW_UNSAFE_REFERENCES
	if( bout.buffer != "System function (1, 11) : Error   : Identifier 'mytype' is not a data type\n"
					   "System function (1, 16) : Error   : Expected one of: in, out, inout\n"
					   "System function (1, 8) : Error   : Identifier 'othertype' is not a data type\n"
					   "System function (1, 14) : Error   : Expected one of: in, out, inout\n"
					   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
					   "System function (1, 18) : Error   : Expected one of: in, out, inout\n"
					   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
					   " (1, 1) : Error   : Identifier 'type' is not a data type\n" )
#else
	if( bout.buffer != "System function (1, 11) : Error   : Identifier 'mytype' is not a data type\n"
		               "System function (1, 8) : Error   : Identifier 'othertype' is not a data type\n"
					   "System function (1, 1) : Error   : Identifier 'type' is not a data type\n"
					   "System function (1, 8) : Error   : Identifier 'type' is not a data type\n"
					   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
					   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
					   " (1, 1) : Error   : Identifier 'type' is not a data type\n")
#endif
		fail = true;

	// Success
 	return fail;
}

} // namespace

