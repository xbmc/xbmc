#include "utils.h"

namespace TestRefArgument
{

#define TESTNAME "TestRefArgument"

static const char *script1 =
"void TestObjHandle(refclass &in ref)   \n"
"{                                      \n"
"   float r;                            \n"
"   test2(r);                           \n"
"   Assert(ref.id == 0xdeadc0de);       \n"
"   test(ref);                          \n"
"   test3(r);                           \n"
"   Assert(r == 1.0f);                  \n"
"}                                      \n"
"void test(refclass &in ref)            \n"
"{                                      \n"
"   Assert(ref.id == 0xdeadc0de);       \n"
"}                                      \n"
"void test2(float &out ref)             \n"
"{                                      \n"
"}                                      \n"
"void test3(float &inout a)             \n"
"{                                      \n"
"   a = 1.0f;                           \n"
"}                                      \n";

static const char *script2 = 
"void Test()                            \n"
"{                                      \n"
"  float[] a(2);                        \n"
"  Testf(a[1]);                         \n"
"}                                      \n"
"void Testf(float &inout a)             \n"
"{                                      \n"
"}                                      \n";

static const char *script3 = 
"void Test()                            \n"
"{                                      \n"
"  string[] a(1);                       \n"
"  Testref(a[0]);                       \n"
"  Assert(a[0] == \"test\");            \n"
"}                                      \n"
"void Testref(string &inout s)          \n"
"{                                      \n"
"  s = \"test\";                        \n"
"}                                      \n";

class CRefClass
{
public:
	CRefClass() 
	{
		id = 0xdeadc0de;
	}
	~CRefClass() 
	{
	}
	CRefClass &operator=(const CRefClass &other) {id = other.id; return *this;}
	int id;
};

static void Assert(bool expr)
{
	if( !expr )
	{
		printf("Assert failed\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			asIScriptEngine *engine = ctx->GetEngine();
			printf("func: %s\n", engine->GetFunctionDeclaration(ctx->GetCurrentFunction()));
			printf("line: %d\n", ctx->GetCurrentLineNumber());
			ctx->SetException("Assert failed");
		}
	}
}

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	r = engine->RegisterObjectType("refclass", sizeof(CRefClass), asOBJ_CLASS_CDA); assert(r >= 0);
	r = engine->RegisterObjectProperty("refclass", "int id", offsetof(CRefClass, id)); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_ASSIGNMENT, "refclass &f(refclass &in)", asMETHOD(CRefClass, operator=), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL); assert( r >= 0 );

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	asIScriptContext *ctx = engine->CreateContext();
	int func = engine->GetFunctionIDByName(0, "TestObjHandle"); assert(r >= 0);

	CRefClass cref;	
	r = ctx->Prepare(r); assert(r >= 0);
	ctx->SetArgObject(0, &cref);
	r = ctx->Execute();  assert(r >= 0);


	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
		printf("%s: Execution failed: %d\n", TESTNAME, r);
	}
	if( ctx ) ctx->Release();

	//-------------------
	CBufferedOutStream bout;
	engine->SetCommonMessageStream(&bout);

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	r = engine->Build(0);
#ifndef AS_ALLOW_UNSAFE_REFERENCES
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestRefArgument (1, 1) : Info    : Compiling void Test()\n"
		               "TestRefArgument (4, 9) : Error   : Cannot guarantee safety of reference. Copy the value to a local variable first\n" ) fail = true;
#else
	if( r != 0 ) fail = true;
#endif

	//----------------------
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->Release();

	// Success
	return fail;
}

} // namespace
