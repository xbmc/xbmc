// Written by Alain "abrken" Bridel on October 12th, 2004

#include "utils.h"
using namespace std;

namespace TestConstructor2
{

#define TESTNAME "TestConstructor2"

class CMyObj {
public :
	CMyObj() {};
	virtual ~CMyObj() {};
};

void ConstrMyObj(CMyObj &obj)
{
	new(&obj) CMyObj();
}

void DestrMyObj(CMyObj &obj)
{
	obj.~CMyObj();
}

class CMySecondObj {
	CMyObj *m_myObj;
public :
	CMySecondObj(CMyObj *in_myObj = NULL) {m_myObj = in_myObj;};
	virtual ~CMySecondObj()  {};
};

void ConstrMySecondObj(CMySecondObj &obj)
{
	new(&obj) CMySecondObj();
}

void ConstrMySecondObj(CMyObj &o, CMySecondObj &obj)
{
	new(&obj) CMySecondObj(&o);
}

void DestrMySecondObj(CMySecondObj &obj)
{
	obj.~CMySecondObj();
}

bool Test()
{
	bool fail = false;

	asIScriptEngine *engine;
	int r;
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->RegisterObjectType("MyObj", sizeof(CMyObj), asOBJ_CLASS); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyObj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstrMyObj), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyObj", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestrMyObj), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	r = engine->RegisterObjectType("MySecondObj", sizeof(CMySecondObj), asOBJ_CLASS); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MySecondObj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstrMySecondObj, (CMySecondObj &), void), asCALL_CDECL_OBJLAST);	assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MySecondObj", asBEHAVE_CONSTRUCT, "void f(MyObj &in)", asFUNCTIONPR(ConstrMySecondObj, (CMyObj &, CMySecondObj &), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MySecondObj", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestrMySecondObj), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	
	engine->ExecuteString(0, "MyObj obj; {MySecondObj secObj(obj);}");
	engine->Release();


	// Success
	return fail;
}


}
