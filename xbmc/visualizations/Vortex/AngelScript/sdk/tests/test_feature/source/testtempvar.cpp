//
// Tests to verify that temporary variables are 
// correctly released when calling object methods.
//
// Author: Peter Marshall (2004/06/14)
//

#include "utils.h"

#define TESTNAME "TestTempVar"

struct Object1
{
    int GetInt(void);
    int m_nID;
};
struct Object2
{
    Object1 GetObject1(void);
};
int Object1::GetInt(void)
{
    return m_nID;
}
Object1 Object2::GetObject1(void)
{
    Object1 Object;
    Object.m_nID = 0xdeadbeef;
    return Object;
}
Object2 ScriptObject;

bool TestTempVar()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

    engine->RegisterObjectType("Object1", sizeof(Object1), asOBJ_CLASS);
    engine->RegisterObjectMethod("Object1", "int GetInt()", asMETHOD(Object1,GetInt), asCALL_THISCALL);
    engine->RegisterObjectType("Object2", sizeof(Object2), asOBJ_CLASS); 
    engine->RegisterObjectMethod("Object2", "Object1 GetObject1()", asMETHOD(Object2,GetObject1), asCALL_THISCALL);
    engine->RegisterGlobalProperty("Object2 GlobalObject", &ScriptObject);

	engine->ExecuteString(0, "GlobalObject.GetObject1().GetInt();");

	engine->Release();

	// Success
	return fail;
}
