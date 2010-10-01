//
// This test was designed to test the functionality of virtual methods
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestVirtualMethod"

static std::string output1;

class CBase
{
public:
	CBase() {me = "CBase";}
	virtual void CallMe() 
	{
		output1 += me; 
		output1 += ": "; 
		output1 += "CBase::CallMe()\n";
	}
	const char *me;
};

class CDerived : public CBase
{
public:
	CDerived() : CBase() {me = "CDerived";}
	void CallMe() 
	{
		output1 += me; 
		output1 += ": "; 
		output1 += "CDerived::CallMe()\n";
	}
};


static CBase b;
static CDerived d;

bool TestVirtualMethod()
{
	bool fail = false;
	int r;
	
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->RegisterObjectType("class", 0, 0);
	r = engine->RegisterObjectMethod("class", "void CallMe()", asMETHOD(CBase, CallMe), asCALL_THISCALL);
	
	// We must register the property as a pointer to the base class since
	// all registered methods are taken from the base class. This is 
	// especially important when there is multiple or virtual inheritance
	r = engine->RegisterGlobalProperty("class b", &b);
	r = engine->RegisterGlobalProperty("class d", (CBase*)&d);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->ExecuteString(0, "b.CallMe(); d.CallMe();");
	
	if( output1 != "CBase: CBase::CallMe()\nCDerived: CDerived::CallMe()\n" )
	{
		printf("%s: Virtual method calls failed.\n%s", TESTNAME, output1.c_str());
		fail = true;
	}
	
	engine->Release();

	return fail;
}
