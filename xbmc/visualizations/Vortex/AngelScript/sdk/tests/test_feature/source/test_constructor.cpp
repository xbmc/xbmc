#include "utils.h"
using namespace std;

#define TESTNAME "TestConstructor"

static const char *script1 =
"obj g_obj1 = g_obj2;                      \n"
"obj g_obj2();                             \n"
"obj g_obj3(12, 3);                        \n";

static const char *script2 = 
"void TestConstructor()                    \n"
"{                                         \n"
"  obj l_obj1;                             \n"
"  l_obj1.a = 5; l_obj1.b = 7;             \n"
"  obj l_obj2();                           \n"
"  obj l_obj3(3, 4);                       \n"
"  a = l_obj1.a + l_obj2.a + l_obj3.a;     \n"
"  b = l_obj1.b + l_obj2.b + l_obj3.b;     \n"
"}                                         \n";
/*
// Illegal situations
static const char *script3 = 
"obj *g_obj4();                            \n";
*/
// Using constructor to create temporary object
static const char *script4 = 
"void TestConstructor2()                   \n"
"{                                         \n"
"  a = obj(11, 2).a;                       \n"
"  b = obj(23, 13).b;                      \n"
"}                                         \n";

class CTestConstructor
{
public:
	CTestConstructor() {a = 0; b = 0;}
	CTestConstructor(int a, int b) {this->a = a; this->b = b;}

	int a;
	int b;
};

void ConstrObj(CTestConstructor *obj)
{
	new(obj) CTestConstructor();
}

void ConstrObj(int a, int b, CTestConstructor *obj)
{
	new(obj) CTestConstructor(a,b);
}

bool TestConstructor()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	int r;
	r = engine->RegisterObjectType("obj", sizeof(CTestConstructor), asOBJ_CLASS_C); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstrObj, (CTestConstructor *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f(int,int)", asFUNCTIONPR(ConstrObj, (int, int, CTestConstructor *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectProperty("obj", "int a", offsetof(CTestConstructor, a)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("obj", "int b", offsetof(CTestConstructor, b)); assert( r >= 0 );

	int a, b;
	r = engine->RegisterGlobalProperty("int a", &a); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("int b", &b); assert( r >= 0 );

	CBufferedOutStream out;	
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	if( out.buffer != "" )
	{
		fail = true;
		printf("%s: Failed to compile global constructors\n", TESTNAME);
	}

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2));
	engine->Build(0);

	if( out.buffer != "" )
	{
		fail = true;
		printf("%s: Failed to compile local constructors\n", TESTNAME);
	}


	engine->ExecuteString(0, "TestConstructor()");

	if( a != 8 || b != 11 )
	{
		printf("%s: Values are not what were expected\n", TESTNAME);
		fail = false;
	}

/*
	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3));
	engine->Build(0);

	if( out.buffer != "TestConstructor (1, 12) : Info    : Compiling obj* g_obj4\n"
	                  "TestConstructor (1, 12) : Error   : Only objects have constructors\n" )
	{
		fail = true;
		printf("%s: Failed to compile global constructors\n", TESTNAME);
	}
*/
	out.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4));
	engine->Build(0);

	if( out.buffer != "" ) 
	{
		fail = true;
		printf(out.buffer.c_str());
		printf("%s: Failed to compile constructor in expression\n", TESTNAME);
	}

	engine->ExecuteString(0, "TestConstructor2()");

	if( a != 11 || b != 13 )
	{
		printf("%s: Values are not what were expected\n", TESTNAME);
		fail = false;
	}

	engine->Release();

	// Success
	return fail;
}
