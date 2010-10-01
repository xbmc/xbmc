#include "utils.h"

namespace TestObjHandle2
{

#define TESTNAME "TestObjHandle2"

static const char *script1 =
"void TestObjHandle()                   \n"
"{                                      \n"
"   refclass@ b = @getRefClass();       \n"
"   Assert(b.id == 0xdeadc0de);         \n"
// Pass argument with explicit handle
"   refclass@ c = @getRefClass(@b);     \n"
"   Assert(@c == @b);                   \n"
// Pass argument with implicit handle
"   @c = @getRefClass(b);               \n"
"   Assert(@c == @b);                   \n"
// Pass argument with implicit in reference to handle
"   t(b);                               \n"
// Pass argument with explicit in reference to handle
"   t(@b);                              \n"
// Pass argument with implicit out reference to handle
"   s(b);                               \n"
// Pass argument with explicit out reference to handle
"   s(@b);                              \n"
// Handle assignment
"   @c = @b;                            \n"
"   @c = b;                             \n"
// Handle comparison        
"   @c == @b;                           \n"
"   @c == b;                            \n"
"   c == @b;                            \n"
"}                                      \n"
"void t(refclass@ &in a)                \n"
"{                                      \n"
"}                                      \n"
"void s(refclass@ &out a)               \n"
"{                                      \n"
"}                                      \n";

static const char *script2 =
"struct A                               \n"
"{ int a; };                            \n"
"void Test()                            \n"
"{                                      \n"
"  A a, b;                              \n"
"  @a = b;                              \n"
"}                                      \n";

class CRefClass
{
public:
	CRefClass() 
	{
		id = 0xdeadc0de;
//		asIScriptContext *ctx = asGetActiveContext(); 
//		printf("ln:%d ", ctx->GetCurrentLineNumber()); 
//		printf("Construct(%X)\n",this); 
		refCount = 1;
	}
	~CRefClass() 
	{
//		asIScriptContext *ctx = asGetActiveContext(); 
//		printf("ln:%d ", ctx->GetCurrentLineNumber()); 
//		printf("Destruct(%X)\n",this);
	}
	int AddRef() 
	{
//		asIScriptContext *ctx = asGetActiveContext(); 
//		printf("ln:%d ", ctx->GetCurrentLineNumber()); 
//		printf("AddRef(%X)\n",this); 
		return ++refCount;
	}
	int Release() 
	{
//		asIScriptContext *ctx = asGetActiveContext(); 
//		printf("ln:%d ", ctx->GetCurrentLineNumber()); 
//		printf("Release(%X)\n",this); 
		int r = --refCount; 
		if( refCount == 0 ) delete this; 
		return r;
	}
	void Method()
	{
		// Some method
	}
	int refCount;
	int id;
};

CRefClass c;
CRefClass *getRefClass() 
{
//	asIScriptContext *ctx = asGetActiveContext(); 
//	printf("ln:%d ", ctx->GetCurrentLineNumber()); 
//	printf("getRefClass() = %X\n", &c); 

	// Must add the reference before returning it
	c.AddRef();
	return &c;
}

CRefClass *getRefClass(CRefClass *obj)
{
	assert(obj != 0);
	return obj;
}

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	r = engine->RegisterObjectType("refclass", sizeof(CRefClass), asOBJ_CLASS_CDA); assert(r >= 0);
	r = engine->RegisterObjectProperty("refclass", "int id", offsetof(CRefClass, id));
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_ADDREF, "void f()", asMETHOD(CRefClass, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_RELEASE, "void f()", asMETHOD(CRefClass, Release), asCALL_THISCALL); assert(r >= 0);
	
	r = engine->RegisterObjectMethod("refclass", "void Method()", asMETHOD(CRefClass, Method), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("refclass @getRefClass()", asFUNCTIONPR(getRefClass,(),CRefClass*), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("refclass @getRefClass(refclass@)", asFUNCTIONPR(getRefClass,(CRefClass*),CRefClass*), asCALL_CDECL); assert( r >= 0 );

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
	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "TestObjHandle()", &ctx);

	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
		{
			int c;
			int row = ctx->GetExceptionLineNumber(&c);
			printf("Exception\n");
			printf("line: %d, %d\n", row, c);
			printf("desc: %s\n", ctx->GetExceptionString());
		}

		fail = true;
		printf("%s: Execution failed\n", TESTNAME);
	}
	if( ctx ) ctx->Release();

	// Verify that the compiler doesn't implicitly convert the lvalue in an assignment to a handle
	CBufferedOutStream bout;
	engine->SetCommonMessageStream(&bout);
	r = engine->ExecuteString(0, "refclass @a; a = @a;");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 18) : Error   : Can't implicitly convert from 'refclass@' to 'refclass'.\n" ) 
	{
		fail = true;
		printf("%s: failure\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "refclass@ a; a.Method();", &ctx);
	if( r != asEXECUTION_EXCEPTION )
	{
		fail = true;
		printf("%s: No exception\n", TESTNAME);
	}
	if( ctx ) ctx->Release();


	engine->Release();

	// Verify that the compiler doesn't allow the use of handles if addref/release aren't registered
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterObjectType("type", 0, asOBJ_PRIMITIVE);
	engine->RegisterGlobalFunction("type @func()", asFUNCTION(0), asCALL_CDECL);
	engine->Release();

	// Verify that it's not possible to do handle assignment to non object handles
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	bout.buffer = "";
	engine->SetCommonMessageStream(&bout);
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestObjHandle2 (3, 1) : Info    : Compiling void Test()\n"
                       "TestObjHandle2 (6, 6) : Error   : Reference is read-only\n" )
		fail = true;
	engine->Release();

	// Success
	return fail;
}

} // namespace

