
#include <stdarg.h>
#include "utils.h"

using std::string;

namespace TestCustomMem
{

#define TESTNAME "TestCustomMem"

int objectsAllocated = 0;
void *MyAlloc(asUINT size)
{
	objectsAllocated++;

	void *mem = new asBYTE[size];
//	printf("MyAlloc(%d) %X\n", size, mem);
	return mem;
}

void MyFree(void *mem)
{
	objectsAllocated--;

//	printf("MyFree(%X)\n", mem);
	delete[] mem;
}

int ReturnObj()
{
	return 0;
}

void ReturnObjGeneric(asIScriptGeneric *gen)
{
	int v = 0;
	gen->SetReturnObject(&v);
}

static const char *script =
"void test(obj o) { }";

bool Test()
{
	bool fail = false;

	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	r = engine->RegisterObjectType("obj", 4, asOBJ_PRIMITIVE); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ALLOC, "obj &f(uint)", asFUNCTION(MyAlloc), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_FREE, "void f(obj &in)", asFUNCTION(MyFree), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("obj retObj()", asFUNCTION(ReturnObj), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("obj retObj2(obj)", asFUNCTION(ReturnObjGeneric), asCALL_GENERIC); assert( r >= 0 );

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->ExecuteString(0, "obj o");

	engine->ExecuteString(0, "retObj()");

	engine->ExecuteString(0, "obj o; retObj2(o)");

	engine->ExecuteString(0, "obj[] o(2)");

	engine->AddScriptSection(0, 0, script, strlen(script));
	engine->Build(0);
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByName(0, "test"));
	int v = 0;
	ctx->SetArgObject(0, &v);
	ctx->Execute();
	ctx->Release();

	engine->Release();

	if( objectsAllocated )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

	// Success
	return fail;
}

} // namespace

