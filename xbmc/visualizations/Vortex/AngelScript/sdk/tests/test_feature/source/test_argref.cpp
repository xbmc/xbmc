#include "utils.h"

namespace TestArgRef
{

#define TESTNAME "TestArgRef"



static const char *script1 =
"int g;                                 \n"
"void TestArgRef()                      \n"
"{                                      \n"
"  int a = 0;                           \n"
"  int[] b;                             \n"
"  Obj o;                               \n"
"  TestArgRef1(a);                      \n"
"  TestArgRef1(g);                      \n"
"  TestArgRef1(b[0]);                   \n"
"  TestArgRef1(o.v);                    \n"
"  string s;                            \n"
"  TestArgRef2(s);                      \n"
"}                                      \n"
"void TestArgRef1(int &in arg)          \n"
"{                                      \n"
"}                                      \n"
"void TestArgRef2(string &in str)       \n"
"{                                      \n"
"}                                      \n";

struct Obj
{
	int v;
};

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	engine->RegisterObjectType("Obj", sizeof(Obj), asOBJ_CLASS);
	engine->RegisterObjectProperty("Obj", "int v", offsetof(Obj, v));

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

