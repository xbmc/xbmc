#include "utils.h"

namespace TestObjHandle
{

#define TESTNAME "TestObjHandle"



static const char *script1 =
"refclass@ g;                           \n"
"refclass@ c = @g;                      \n"
"void TestObjHandle()                   \n"
"{                                      \n"
"   refclass@ b = @refclass();          \n"
// Should generate an exception     
// as g isn't initialized yet.      
//"   g = b;                              \n"
//"   b = g;                              \n"
// Do a handle assignment
"   @g = @b;                            \n"
// Now an assignment to g is possible
"   g = b;                              \n"
// Compare with null
"   if( @g != null );                   \n"
"   if( null == @g );                   \n"
// Compare with another object
"   if( @g == @b );                     \n"
"   if( @b == @g );                     \n"
// Value comparison
//"   if( g == b );                       \n"
//"   if( b == g );                       \n"
// Assign null to release the object
"   @g = null;                          \n"
"   @g = @b;                            \n"
// Operators
"   b = g + b;                          \n"
// parameter references
"   @g = null;                          \n"
"   TestObjHandleRef(b, @g);            \n"
"   Assert(@g == @b);                   \n"
// return handles
"   @g = null;                          \n"
"   @g = @TestObjReturnHandle(b);       \n"
"   Assert(@g == @b);                   \n"
"   Assert(@TestReturnNull() == null);  \n"
"}                                      \n"
"void TestObjHandleRef(refclass@ i, refclass@ &out o)  \n"
"{                                                     \n"
"   @o = @i;                                           \n"
"}                                                     \n"
"refclass@ TestObjReturnHandle(refclass@ i)            \n"
"{                                                     \n"
"   return i;                                          \n"
"}                                                     \n"
"refclass@ TestReturnNull()                            \n"
"{                                                     \n"
"   return null;                                       \n"
"}                                                     \n";
 
class CRefClass
{
public:
	CRefClass() 
	{
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
	CRefClass &operator=(const CRefClass &o) 
	{
//		asIScriptContext *ctx = asGetActiveContext(); 
//		printf("ln:%d ", ctx->GetCurrentLineNumber()); 
//		printf("Assign(%X, %X)\n", this, &o); 
		return *this;
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
	static CRefClass &Add(CRefClass &self, CRefClass &other)
	{
//		asIScriptContext *ctx = asGetActiveContext();
//		printf("ln:%d ", ctx->GetCurrentLineNumber());
//		printf("Add(%X, %X)\n", &self, &other);
		return self;
	}
	int refCount;
};

void Construct(CRefClass *o)
{
	new(o) CRefClass;
}

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
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_ADDREF, "void f()", asMETHOD(CRefClass, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_RELEASE, "void f()", asMETHOD(CRefClass, Release), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_ASSIGNMENT, "refclass &f(refclass &in)", asMETHOD(CRefClass, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "refclass &f(refclass &in, refclass &in)", asFUNCTION(CRefClass::Add), asCALL_CDECL); assert(r >= 0);

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
			PrintException(ctx);

		fail = true;
		printf("%s: Execution failed\n", TESTNAME);
	}
	if( ctx ) ctx->Release();

	// Call TestObjReturnHandle() from the application to verify that references are updated as necessary
	ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "refclass@ TestObjReturnHandle(refclass@)"));
	CRefClass *refclass = new CRefClass();

	ctx->SetArgObject(0, refclass);

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		fail = true;
		printf("%s: Execution failed\n", TESTNAME);
	}
	if( refclass->refCount != 2 )
	{
		fail = true;
		printf("%s: Ref count is wrong\n", TESTNAME);
	}

	refclass->Release();
	if( ctx ) ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

