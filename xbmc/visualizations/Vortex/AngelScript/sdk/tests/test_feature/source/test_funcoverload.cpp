#include "utils.h"

#define TESTNAME "TestFuncOverload"

static const char *script1 =
"void Test()                               \n"
"{                                         \n"
"  Obj i;                                  \n"
"  TX.Set(\"user\", i.Value());            \n"
"}                                         \n";

class Obj
{
public:
	void *p;
	void *Value() {return p;}
	void Set(const std::string&, void *) {}
};

static Obj o;

void FuncVoid()
{
}

void FuncInt(int v)
{
}

bool TestFuncOverload()
{
	bool fail = false;
	COutStream out;	

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream(&out);
	RegisterScriptString(engine);

	engine->RegisterObjectType("Data", sizeof(void*), asOBJ_PRIMITIVE);

	engine->RegisterObjectType("Obj", sizeof(Obj), 0);
	engine->RegisterObjectMethod("Obj", "Data &Value()", asMETHOD(Obj, Value), asCALL_THISCALL);
	engine->RegisterObjectMethod("Obj", "void Set(string &in, Data &in)", asMETHOD(Obj, Set), asCALL_THISCALL);
	engine->RegisterObjectMethod("Obj", "void Set(string &in, string &in)", asMETHOD(Obj, Set), asCALL_THISCALL);
	engine->RegisterGlobalProperty("Obj TX", &o);

	engine->RegisterGlobalFunction("void func()", asFUNCTION(FuncVoid), asCALL_CDECL);
	engine->RegisterGlobalFunction("void func(int)", asFUNCTION(FuncInt), asCALL_CDECL);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->Build(0);

	engine->ExecuteString(0, "func(func(3));");

	engine->Release();

	// Success
	return fail;
}
