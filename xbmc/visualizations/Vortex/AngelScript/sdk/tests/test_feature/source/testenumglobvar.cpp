//
// This test verifies enumeration of global script variables
//
// Author: Andreas Jonsson
//

#include "utils.h"

#define TESTNAME "TestEnumGlobVar"

static const char script[] = "int a; float b; double c; bits d = 0xC0DE; string e = \"test\"; obj @f = @o;";

void AddRef_Release_dummy(int *)
{
}


bool TestEnumGlobVar()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);

	int r;
	r = engine->RegisterObjectType("obj", 0, 0); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ADDREF, "void f()", asFUNCTION(AddRef_Release_dummy), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_RELEASE, "void f()", asFUNCTION(AddRef_Release_dummy), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	int o = 0xBAADF00D;
	r = engine->RegisterGlobalProperty("obj o", &o);

	engine->AddScriptSection(0, "test", script, sizeof(script)-1, 0);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	int count = engine->GetGlobalVarCount(0);
	if( count != 6 )
	{
		printf("%s: GetGlobalVarCount() returned %d, expected 6.\n", TESTNAME, count);
		ret = true;
	}

	const char *buffer = 0;
	if( (buffer = engine->GetGlobalVarDeclaration(0)) == 0 )
	{
		printf("%s: GetGlobalVarDeclaration() failed\n", TESTNAME);
		ret = true;
	}
	else if( strcmp(buffer, "int a") != 0 )
	{
		printf("%s: GetGlobalVarDeclaration() returned %s\n", TESTNAME, buffer);
		ret = true;
	}

	int id = engine->GetGlobalVarIDByName(0, "b");
	if( id < 0 )
	{
		printf("%s: GetGlobalVarIDByName() returned %d\n", TESTNAME, id);
		ret = true;
	}

	id = engine->GetGlobalVarIDByDecl(0, "double c");
	if( id < 0 )
	{
		printf("%s: GetGlobalVarIDByDecl() returned %d\n", TESTNAME, id);
		ret = true;
	}

	if( (buffer = engine->GetGlobalVarName(3)) == 0 )
	{
		printf("%s: GetGlobalVarName() failed\n", TESTNAME);
		ret = true;
	}
	else if( strcmp(buffer, "d") != 0 )
	{
		printf("%s: GetGlobalVarName() returned %s\n", TESTNAME, buffer);
		ret = true;
	}

	unsigned long *d;
	d = (unsigned long *)engine->GetGlobalVarPointer(3);
	if( d == 0 )
	{
		printf("%s: GetGlobalVarPointer() returned %d\n", TESTNAME, r);
		ret = true;
	}
	if( *d != 0xC0DE )
	{
		printf("%s: Failed\n", TESTNAME);
		ret = true;
	}

	std::string *e;
	e = (std::string*)engine->GetGlobalVarPointer(4);
	if( e == 0 )
	{
		printf("%s: Failed\n", TESTNAME);
		ret = true;
	}

	if( *e != "test" )
	{
		printf("%s: Failed\n", TESTNAME);
		ret = true;
	}

	int *f;
	f = (int*)engine->GetGlobalVarPointer(5);
	if( f == 0 )
	{
		printf("%s: failed\n", TESTNAME);
		ret = true;
	}

	if( *f != 0xBAADF00D )
	{
		printf("%s: failed\n", TESTNAME);
		ret = true;
	}

	engine->Release();

	return ret;
}

