
#include <stdarg.h>
#include "utils.h"

using std::string;

namespace TestGeneric
{

#define TESTNAME "TestGeneric"

int obj;

void GenFunc1(asIScriptGeneric *gen)
{
	assert(gen->GetObject() == 0);

//	printf("GenFunc1\n");

	int arg1 = (int)gen->GetArgDWord(0);
	double arg2 = gen->GetArgDouble(1);
	string arg3 = *(string*)gen->GetArgObject(2);

	assert(arg1 == 23);
	assert(arg2 == 23);
	assert(arg3 == "test");

	gen->SetReturnDouble(23);
}

void GenMethod1(asIScriptGeneric *gen)
{
	assert(gen->GetObject() == &obj);

//	printf("GenMethod1\n");

	int arg1 = (int)gen->GetArgDWord(0);
	double arg2 = gen->GetArgDouble(1);

	assert(arg1 == 23);
	assert(arg2 == 23);

	string s("Hello");
	gen->SetReturnObject(&s);
}

void GenAssign(asIScriptGeneric *gen)
{
//	assert(gen->GetObject() == &obj);

	int *obj2 = (int*)gen->GetArgObject(0);

//	assert(obj2 == &obj);

	gen->SetReturnObject(&obj);
}

void TestDouble(asIScriptGeneric *gen) 
{
	double d = gen->GetArgDouble(0);

	assert(d == 23);
}

void TestString(asIScriptGeneric *gen)
{
	string s = *(string*)gen->GetArgObject(0);

	assert(s == "Hello");
}

void GenericString_Construct(asIScriptGeneric *gen)
{
	string *s = (string*)gen->GetObject();

	new(s) string;
}

void GenericString_Destruct(asIScriptGeneric *gen)
{
	string *s = (string*)gen->GetObject();

	s->~string();
}

void GenericString_Assignment(asIScriptGeneric *gen)
{
	string *other = (string*)gen->GetArgObject(0);
	string *self = (string*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

void GenericString_Factory(asIScriptGeneric *gen)
{
	asUINT length = gen->GetArgDWord(0);
	const char *s = (const char *)gen->GetArgDWord(1);

	string str(s);

	gen->SetReturnObject(&str);
}

bool Test()
{
	bool fail = false;

	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->RegisterObjectType("string", sizeof(string), asOBJ_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(GenericString_Construct), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(GenericString_Destruct), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(string &in)", asFUNCTION(GenericString_Assignment), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterStringFactory("string", asFUNCTION(GenericString_Factory), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("void test(double)", asFUNCTION(TestDouble), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void test(string)", asFUNCTION(TestString), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("double func1(int, double, string)", asFUNCTION(GenFunc1), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectType("obj", 4, asOBJ_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectMethod("obj", "string mthd1(int, double)", asFUNCTION(GenMethod1), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ASSIGNMENT, "obj &f(obj &in)", asFUNCTION(GenAssign), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalProperty("obj o", &obj);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->ExecuteString(0, "test(func1(23, 23, \"test\"))");

	engine->ExecuteString(0, "test(o.mthd1(23, 23))");

	engine->ExecuteString(0, "o = o");

	engine->Release();

	// Success
	return fail;
}

} // namespace

