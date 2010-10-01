#include "stdvector.h"
#include "utils.h"


namespace TestArrayObject
{

#define TESTNAME "TestArrayObject"

class CIntArray
{
public:
	CIntArray() 
	{
		length = 0; 
		buffer = new int[0];
	} 
	CIntArray(int l) 
	{
		length=l; 
		buffer = new int[l];
	}
	CIntArray(const CIntArray &other)
	{
		length = other.length;
		buffer = new int[length];
		for( int n = 0; n < length; n++ )
			buffer[n] = other.buffer[n];
	}
	~CIntArray() 
	{
		delete[] buffer;
	}

	CIntArray &operator=(const CIntArray &other) 
	{
		delete[] buffer; 
		length = other.length; 
		buffer = new int[length]; 
		memcpy(buffer, other.buffer, length*4); 
		return *this;
	}

	int size() {return length;}
	void push_back(int &v) 
	{
		int *b = new int[length+1]; 
		memcpy(b, buffer, length*4); 
		delete[] buffer; 
		buffer = b; 
		b[length++] = v;
	}
	int pop_back() 
	{
		return buffer[--length];
	}
	int &operator[](int i) 
	{
		return buffer[i];
	}

	int length;
	int *buffer;
};

void ConstructIntArray(CIntArray *a)
{
	new(a) CIntArray();
}

void ConstructIntArray(int l, CIntArray *a)
{
	new(a) CIntArray(l);
}

void DestructIntArray(CIntArray *a)
{
	a->~CIntArray();
}

class CIntArrayArray
{
public:
	CIntArrayArray() 
	{
		length = 0; 
		buffer = new CIntArray[0];
	} 
	CIntArrayArray(int l) 
	{
		length=l; 
		buffer = new CIntArray[l];
	}
	CIntArrayArray(const CIntArrayArray &other) 
	{
		length = other.length;
		buffer = new CIntArray[length];
		for( int n = 0; n < length; n++ )
			buffer[n] = other.buffer[n];
	}
	~CIntArrayArray() 
	{
		delete[] buffer;
	}

	CIntArrayArray &operator=(CIntArrayArray &other) 
	{
		delete[] buffer; 
		length = other.length; 
		buffer = new CIntArray[length]; 
		for( int n = 0; n < length; n++ )
			buffer[n] = other.buffer[n];
		return *this;
	}

	int size() {return length;}
	void push_back(CIntArray &v) 
	{
		CIntArray *b = new CIntArray[length+1]; 
		for( int n = 0; n < length; n++ )
			b[n] = buffer[n];
		delete[] buffer; 
		buffer = b; 
		b[length++] = v;
	}
	CIntArray pop_back() 
	{
		return buffer[--length];
	}
	CIntArray &operator[](int i) 
	{
		return buffer[i];
	}

	int length;
	CIntArray *buffer;
};

void ConstructIntArrayArray(CIntArrayArray *a)
{
	new(a) CIntArrayArray();
}

void ConstructIntArrayArray(int l, CIntArrayArray *a)
{
	new(a) CIntArrayArray(l);
}

void DestructIntArrayArray(CIntArrayArray *a)
{
	a->~CIntArrayArray();
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

static const char *script1 =
"void Test()                         \n"
"{                                   \n"
"   TestInt();                       \n"
"   Test2D();                        \n"
"}                                   \n"
"                                    \n"
"void TestInt()                      \n"
"{                                   \n"
"   int[] A(5);                      \n"
"   Assert(A.size() == 5);           \n"
"	A.push_back(6);                  \n"
"   Assert(A.size() == 6);           \n"
"	A.pop_back();                    \n"
"   Assert(A.size() == 5);           \n"
"	A[1] = 20;                       \n"
"	Assert(A[1] == 20);              \n"
"   char[] B(5);                     \n"
"   Assert(B.size() == 5);           \n"
"   int[] c = {2,3};                 \n"
"   Assert(c.size() == 2);           \n"
"}                                   \n"
"                                    \n"
"void Test2D()                       \n"
"{                                   \n"
"   int[][] A(2);                    \n"
"   int[] B(2);                      \n"
"   A[0] = B;                        \n"
"   A[1] = B;                        \n"
"                                    \n"
"   A[0][0] = 0;                     \n"
"   A[0][1] = 1;                     \n"
"   A[1][0] = 2;                     \n"
"   A[1][1] = 3;                     \n"
"                                    \n"
"   Assert(A[0][0] == 0);            \n"
"   Assert(A[0][1] == 1);            \n"
"   Assert(A[1][0] == 2);            \n"
"   Assert(A[1][1] == 3);            \n"
"}                                   \n";

using namespace std;

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Verify that it is possible to register arrays of registered types
	r = engine->RegisterObjectType("char", sizeof(int), asOBJ_PRIMITIVE); assert( r>= 0 );
	r = engine->RegisterObjectType( "char[]", sizeof(CIntArray), asOBJ_CLASS_CDA ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("char[]", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructIntArray), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("char[]", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructIntArray, (CIntArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("char[]", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ConstructIntArray, (int, CIntArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("char[]", "int size()", asMETHOD(CIntArray, size), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectType("char[][]", sizeof(CIntArrayArray), asOBJ_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("char[][]", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructIntArrayArray, (CIntArrayArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("char[][]", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ConstructIntArrayArray, (int, CIntArrayArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("char[][]", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructIntArrayArray), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("char[][]", "int size()", asMETHOD(CIntArrayArray, size), asCALL_THISCALL); assert( r >= 0 );

	// Verify that it is possible to register arrays of built-in types
	r = engine->RegisterObjectType("int[]", sizeof(CIntArray), asOBJ_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[]", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructIntArray, (CIntArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[]", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ConstructIntArray, (int, CIntArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[]", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructIntArray), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[]", asBEHAVE_ASSIGNMENT, "int[] &f(int[]&in)", asMETHOD(CIntArray, operator=), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[]", asBEHAVE_INDEX, "int &f(int)", asMETHOD(CIntArray, operator[]), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("int[]", "int size()", asMETHOD(CIntArray, size), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("int[]", "void push_back(int &in)", asMETHOD(CIntArray, push_back), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("int[]", "int pop_back()", asMETHOD(CIntArray, pop_back), asCALL_THISCALL); assert( r >= 0 );

//	RegisterVector<CIntArray>("int[][]", "int[]", engine);

	r = engine->RegisterObjectType("int[][]", sizeof(CIntArrayArray), asOBJ_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[][]", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructIntArrayArray, (CIntArrayArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[][]", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ConstructIntArrayArray, (int, CIntArrayArray *), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[][]", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructIntArrayArray), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[][]", asBEHAVE_ASSIGNMENT, "int[][] &f(int[][]&in)", asMETHOD(CIntArrayArray, operator=), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("int[][]", asBEHAVE_INDEX, "int[] &f(int)", asMETHOD(CIntArrayArray, operator[]), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("int[][]", "int size()", asMETHOD(CIntArrayArray, size), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("int[][]", "void push_back(int[] &in)", asMETHOD(CIntArrayArray, push_back), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("int[][]", "int[] pop_back()", asMETHOD(CIntArrayArray, pop_back), asCALL_THISCALL); assert( r >= 0 );


	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL);

	COutStream out;
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}


	asIScriptContext *ctx = 0;
	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Failed to execute script\n", TESTNAME);

		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		
		fail = true;
	}
	
	if( ctx ) ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

