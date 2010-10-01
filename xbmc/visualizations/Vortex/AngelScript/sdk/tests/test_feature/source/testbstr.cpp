// 
// Test designed to verify functionality of the bstr type
//
// Written by Andreas Jönsson 
//

#include "utils.h"
#include "bstr.h"

#define TESTNAME "TestBStr"

static asBSTR NewString(int length)
{
	return asBStrAlloc(length);
}

bool TestBStr()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterBStr(engine);

	engine->RegisterGlobalFunction("bstr NewString(int)", asFUNCTION(NewString), asCALL_CDECL);

	int r = engine->ExecuteString(0, "bstr s = NewString(10)");
	if( r < 0 ) 
	{
		printf("%s: ExecuteString() failed\n", TESTNAME);
		ret = true;
	}
	else if( r != asEXECUTION_FINISHED )
	{
		printf("%s: ExecuteString() returned %d\n", TESTNAME, r);
		ret = true;
	}

	engine->Release();

	return ret;
}
