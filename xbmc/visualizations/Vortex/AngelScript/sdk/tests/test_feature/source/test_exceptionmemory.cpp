#include "utils.h"

namespace TestExceptionMemory
{

#define TESTNAME "TestExceptionMemory"

static const char *script1 =
"void Test1()                   \n"
"{                              \n"
"  Object a;                    \n"
"  RaiseException();            \n"
"}                              \n"
"void Test2()                   \n"
"{                              \n"
"  RaiseException();            \n"
"  Object a;                    \n"
"}                              \n"
"void Test3()                   \n"
"{                              \n"
"  int a;                       \n"
"  Func(Object());              \n"
"}                              \n"
"void Func(Object a)            \n"
"{                              \n"
"  Object b;                    \n"
"  RaiseException();            \n"
"}                              \n"
"void Test4()                   \n"
"{                              \n"
"  Object a = SuspendObj();     \n"
"}                              \n"
"void Test5()                   \n"
"{                              \n"
"  Object a = ExceptionObj();   \n"
"}                              \n"
"void Test6()                   \n"
"{                              \n"
"  Object a(1);                 \n"
"}                              \n";
static const char *script2 =
"void Test7()                      \n"
"{                                 \n"
"  RefObj @a = @ExceptionHandle(); \n"
"}                                 \n";

class CObject
{
public:
	CObject() {val = ('C' | ('O'<<8) | ('b'<<16) | ('j'<<24)); mem = new int[1]; *mem = ('M' | ('e'<<8) | ('m'<<16) | (' '<<24)); /*printf("C: %x\n", this);*/ }
	~CObject() {delete[] mem; /*printf("D: %x\n", this);*/}
	int val;
	int *mem;
};

class CRefObject
{
public:
	CRefObject() {refCount = 1;}
	int AddRef() {return ++refCount;}
	int Release() {int r = --refCount; if( r == 0 ) delete this; return r;}
	int refCount;
};

void ConstructRefObj(CRefObject *o)
{
	new(o) CRefObject();
}
 
void Construct(CObject *o)
{
	new(o) CObject();
}

void Construct2(CObject *o, int)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");
}

void Destruct(CObject *o)
{
	o->~CObject();
}

void RaiseException()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");
}

CObject SuspendObj()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->Suspend();

	return CObject();
}

CObject ExceptionObj()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");

	return CObject();
}

CRefObject *ExceptionHandle()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->SetException("application exception");

	return 0;
}

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("Object", sizeof(CObject), asOBJ_CLASS_CD);	
	engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(Construct2), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectBehaviour("Object", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);

	r = engine->RegisterObjectType("RefObj", sizeof(CRefObject), asOBJ_CLASS_C); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructRefObj), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_ADDREF, "void f()", asMETHOD(CRefObject, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("RefObj", asBEHAVE_RELEASE, "void f()", asMETHOD(CRefObject, Release), asCALL_THISCALL); assert(r >= 0);

	engine->RegisterGlobalFunction("void RaiseException()", asFUNCTION(RaiseException), asCALL_CDECL);
	engine->RegisterGlobalFunction("Object SuspendObj()", asFUNCTION(SuspendObj), asCALL_CDECL);
	engine->RegisterGlobalFunction("Object ExceptionObj()", asFUNCTION(ExceptionObj), asCALL_CDECL);
	engine->RegisterGlobalFunction("RefObj@ ExceptionHandle()", asFUNCTION(ExceptionHandle), asCALL_CDECL);

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "Test1()");
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

//	printf("---\n");

	r = engine->ExecuteString(0, "Test2()");
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

 //	printf("---\n");

	r = engine->ExecuteString(0, "Test3()");
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

//	printf("---\n");

	engine->SetDefaultContextStackSize(20, 4);
	r = engine->ExecuteString(0, "Test3()");
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

//	printf("---\n");

	asIScriptContext *ctx;
	engine->SetDefaultContextStackSize(1024, 0);
	r = engine->ExecuteString(0, "Test4()", &ctx);
	if( r != asEXECUTION_SUSPENDED )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}
	ctx->Abort();
	ctx->Release();

//	printf("---\n");

	r = engine->ExecuteString(0, "Test5()");
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

//	printf("---\n");

	r = engine->ExecuteString(0, "Test6()");
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

//	printf("---\n");

 	engine->Release();

	// Success
	return fail;
}

} // namespace

