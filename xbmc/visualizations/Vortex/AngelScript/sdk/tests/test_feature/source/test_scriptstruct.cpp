#include "utils.h"

namespace TestScriptStruct
{

#define TESTNAME "TestScriptStruct"

// Normal structure
static const char *script1 =
"struct Test                  \n"
"{                            \n"
"   int a;                    \n"
"   bool b;                   \n"
"};                           \n"
"void TestStruct()            \n"
"{                            \n"
"   Test a;                   \n"
"   a.a = 3;                  \n"
"   a.b = false;              \n"
"   Test b;                   \n"
"   Test @c = @a;             \n"
"   a = b;                    \n"
"   TestStruct2(c);           \n"
"   Test[] d(1);              \n"
"   d[0] = a;                 \n"
"   a = Test();               \n"
"}                            \n"
"void TestStruct2(Test a)     \n"
"{                            \n"
"}                            \n";

// Do not allow const in properties
static const char *script2 = 
"struct Test                  \n"
"{                            \n"
"   const int a;              \n"
"};                           \n";

// Test arrays in struct
static const char *script3 =
"struct Test                  \n"
"{                            \n"
"   int[] a;                  \n"
"};                           \n"
"struct Test2                 \n"
"{                            \n"
"   Test2@[][] a;             \n"
"};                           \n"
"void TestArrayInStruct()     \n"
"{                            \n"
"   Test a;                   \n"
"   a.a.resize(10);           \n"
"   Test2 b;                  \n"
"   b.a.resize(1);            \n"
"   b.a[0].resize(1);         \n"
"   // Circular reference     \n"
"   @b.a[0][0] = b;           \n"
"}                            \n";

// Only allow primitives (at first)
static const char *script4 =
"struct B                     \n"
"{                            \n"
"   A a;                      \n"
"   string b;                 \n"
"   int c;                    \n"
"};                           \n"
"void Test()                  \n"
"{                            \n"
"  B a, b;                    \n"
"  b.a.a = 5;                 \n"
"  b.b = \"Test\";            \n"
"  b.c = 6;                   \n"
"  a = b;                     \n"
"  b.a.a = 6;                 \n"
"  b.b = \"1\";               \n"
"  b.c = 2;                   \n"
"  Assert(a.a.a == 5);        \n"
"  Assert(a.b == \"Test\");   \n"
"  Assert(a.c == 6);          \n"
"}                            \n"
"struct A                     \n"
"{                            \n"
"   bits a;                   \n"
"};                           \n";

// Verify that the struct names cannot conflict with one another
static const char *script5 = 
"struct A {};                 \n"
"struct A {};                 \n"
"struct B                     \n"
"{                            \n"
"  int a;                     \n"
"  float a;                   \n"
"};                           \n";

// Verify that a structure cannot have itself as local member (directly or indirectly)
static const char *script6 = 
"struct A                     \n"
"{                            \n"
"  A a;                       \n"
"};                           \n"
"struct B                     \n"
"{                            \n"
"  C c;                       \n"
"};                           \n"
"struct C                     \n"
"{                            \n"
"  B b;                       \n"
"};                           \n";

static const char *script7 =
"struct A                     \n"
"{                            \n"
"  string@ s;                 \n"
"};                           \n"
"void TestHandleInStruct()    \n"
"{                            \n"
"  A a;                       \n"
"  Assert(a.s == null);       \n"
"  a = a;                     \n"
"  @a.s = \"Test\";           \n"
"  Assert(a.s == \"Test\");   \n"
"}                            \n";

// Verify that circular references are handled by the GC
static const char *script8 = 
"struct A                     \n"
"{                            \n"
"  A@ next;                   \n"
"};                           \n"
"struct B                     \n"
"{                            \n"
"  D@ next;                   \n"
"};                           \n"
"struct C                     \n"
"{                            \n"
"  B b;                       \n"
"};                           \n"
"struct D                     \n"
"{                            \n"
"  C c;                       \n"
"};                           \n"
"void TestHandleInStruct2()   \n"
"{                            \n"
// Simple circular reference
"  A a;                       \n"
"  @a.next = a;               \n"
// More complex circular reference
"  D d1;                      \n"
"  D d2;                      \n"
"  @d1.c.b.next = d2;         \n"
"  @d2.c.b.next = d1;         \n"
"}                            \n";


static const char *script9 = 
"struct MyStruct              \n"
"{                            \n"
"  bits myBits;               \n"
"};                           \n"
"bits MyFunc(bits a)          \n"
"{                            \n"
"  return a;                  \n"
"}                            \n"
"void MyFunc(string@) {}      \n"
"void Test()                  \n"
"{                            \n"
"  bits val = 0x0;            \n"
"  MyStruct s;                \n"
"  s.myBits = 0x5;            \n"
"  val = MyFunc(s.myBits);    \n"
"}                            \n";

// Don't allow arrays of the struct type as members (unless it is handles)
static const char *script10 = 
"struct Test2                 \n"
"{                            \n"
"   Test2[] a;                \n"
"};                           \n";

// Test array constness in members
static const char *script11 = 
"struct A                     \n"
"{                            \n"
"   int[] a;                  \n"
"};                           \n"
"void Test()                  \n"
"{                            \n"
"   const A a;                \n"
"   // Should not compile     \n"
"   a.a[0] = 23;              \n"
"}                            \n";

// Test order independence with declarations
static const char *script12 =
"A Test()                     \n"
"{                            \n"
"  A a;                       \n"
"  return a;                  \n"
"}                            \n";

static const char *script13 =
"struct A                     \n"
"{                            \n"
"  B b;                       \n"
"};                           \n"
"struct B                     \n"
"{                            \n"
"  int val;                   \n"
"};                           \n";


bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL);

	COutStream out;
	CBufferedOutStream bout;
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	asIScriptContext *ctx = 0;
	r = engine->ExecuteString(0, "TestStruct()", &ctx);
	if( r != asEXECUTION_FINISHED ) 
	{
		if( r == asEXECUTION_EXCEPTION ) PrintException(ctx);
		fail = true;
	}
	if( ctx ) ctx->Release();

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	engine->SetCommonMessageStream(&bout);
	r = engine->Build(0);
	if( r >= 0 || bout.buffer != "TestScriptStruct (3, 4) : Error   : Struct properties cannot be declared as const\n" ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0, false);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestArrayInStruct()");
	if( r != 0 ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "Test()");
	if( r != 0 ) fail = true;

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script5, strlen(script5), 0, false);
	engine->SetCommonMessageStream(&bout);
	r = engine->Build(0);
	if( r >= 0 || bout.buffer != 
		"TestScriptStruct (2, 8) : Error   : Name conflict. 'A' is a struct.\n"
		"TestScriptStruct (6, 9) : Error   : Name conflict. 'a' is an object property.\n" ) fail = true;

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script6, strlen(script6), 0, false);
	r = engine->Build(0);
	if( r >= 0 || bout.buffer !=
		"TestScriptStruct (1, 8) : Error   : Illegal member type\n"
		"TestScriptStruct (5, 8) : Error   : Illegal member type\n" ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script7, strlen(script7), 0, false);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestHandleInStruct()", &ctx);
	if( r != 0 )
	{
		if( r == asEXECUTION_EXCEPTION )
		{
			printf("%s\n", ctx->GetExceptionString());
		}
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->AddScriptSection(0, TESTNAME, script8, strlen(script8), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestHandleInStruct2()");
	if( r != 0 ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script9, strlen(script9), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "Test()");
	if( r != 0 ) fail = true;

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script10, strlen(script10), 0, false);
	engine->SetCommonMessageStream(&bout);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestScriptStruct (1, 8) : Error   : Illegal member type\n" ) fail = true;

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script11, strlen(script11), 0, false);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestScriptStruct (5, 1) : Info    : Compiling void Test()\nTestScriptStruct (9, 11) : Error   : Reference is read-only\n" ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script12, strlen(script12), 0, false);
	engine->AddScriptSection(0, TESTNAME, script13, strlen(script13), 0, false);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 ) fail = true;


	// The garbage collection doesn't have to be invoked immediately. Modules
	// can even be discarded before calling the garbage collector.
	engine->GarbageCollect();
	
	engine->Release();

	// Success
	return fail;
}

} // namespace

