#include "utils.h"

namespace TestPointer
{

#define TESTNAME "TestPointer"

class ObjectInstance 
{
public:
	int val;
  int val2;
  int val3;

  void Method() {
    val=val2=val3=0;
  }
};

class ObjectType
{
public:
};

static void ObjectFunction(ObjectInstance *obj)
{

}

ObjectInstance obj;
static ObjectInstance *CreateObjectInstance(ObjectType *type)
{
	return &obj;
}

ObjectType type;
static ObjectType *CreateObjectType(std::string &str)
{
	return &type;
}

static void FunctionOnObject(ObjectInstance *obj)
{
}

static const char *script = 
"void Test()                                        \n"
"{                                                  \n"
"  ObjectInstance*[] c(5);                          \n"
"  ObjectType *tbase = CreateObjectType(\"base\");  \n"
"  uint i;                                          \n"
"  for( i = 0; i < 5; ++i )                         \n"
"  {                                                \n"
"    c[i] = CreateObjectInstance(tbase);            \n"
"    c[i]->function();                              \n"
"    c[i]->val = 0;                                 \n"
"  }                                                \n"
"  ObjectInstance *obj = c[0];                      \n"
"  FunctionOnObject(c[0]);                          \n"
"}                                                  \n";

bool Test()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);

	int r;
	r = engine->RegisterObjectType("ObjectInstance", sizeof(ObjectInstance), asOBJ_CLASS); assert(r>=0);
	r = engine->RegisterObjectProperty("ObjectInstance", "int val", offsetof(ObjectInstance, val)); assert(r>=0);
	r = engine->RegisterObjectProperty("ObjectInstance", "int val2", offsetof(ObjectInstance, val)); assert(r>=0);
	r = engine->RegisterObjectProperty("ObjectInstance", "int val3", offsetof(ObjectInstance, val)); assert(r>=0);
	r = engine->RegisterObjectMethod("ObjectInstance", "void function()", asFUNCTION(ObjectFunction), asCALL_CDECL_OBJFIRST); assert(r>=0);
	r = engine->RegisterObjectMethod("ObjectInstance", "void Method()", asMETHOD(ObjectInstance,Method), asCALL_THISCALL); assert(r>=0);

	r = engine->RegisterObjectType("ObjectType", sizeof(ObjectType), asOBJ_CLASS); assert(r>=0);

	r = engine->RegisterGlobalFunction("ObjectType *CreateObjectType(string &in)", asFUNCTION(CreateObjectType), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("ObjectInstance *CreateObjectInstance(ObjectType *type)", asFUNCTION(CreateObjectInstance), asCALL_CDECL); assert(r>=0);
	
	r = engine->RegisterGlobalFunction("void FunctionOnObject(ObjectInstance *)", asFUNCTION(FunctionOnObject), asCALL_CDECL); assert(r>=0);

	// Register an object.
	ObjectInstance obj;
	r = engine->RegisterGlobalProperty("ObjectInstance obj", &obj); assert(r>=0);

	// Register a pointer to object.
	ObjectInstance *pnt = &obj;
	r = engine->RegisterGlobalProperty("ObjectInstance *ptr", &pnt); assert(r>=0);

	COutStream out;
	engine->SetCommonMessageStream(&out);

	// Function call executed fine when using an object.
	r = engine->ExecuteString(0, "obj.function(); obj.val = 23;");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}
	if( obj.val != 23 )
	{
		printf("%s: failed\n", TESTNAME, r);
		fail = true;
	}

	r = engine->ExecuteString(0, "ptr->function(); ptr->val = 13;");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		fail = true;
	}
	if( obj.val != 13 )
	{
		printf("%s: failed\n", TESTNAME, r);
		fail = true;
	}

	engine->AddScriptSection(0, TESTNAME, script, strlen(script));
	r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: failed\n", TESTNAME, r);
		fail = true;
	}

	r = engine->ExecuteString(0, "Test()");
	if( r < 0 )
	{
		printf("%s: failed\n", TESTNAME, r);
		fail = true;
	}

	engine->Release();

	return fail;
}

}


