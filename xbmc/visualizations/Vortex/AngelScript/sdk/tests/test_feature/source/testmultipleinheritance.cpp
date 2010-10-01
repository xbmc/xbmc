//
// This test was designed to test the functionality of methods 
// from classes with multiple inheritance
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestMultipleInheritance"

static std::string output2;

class CBase1
{
public:
	CBase1() {me1 = "CBase1";}
	virtual void CallMe1() 
	{
		output2 += me1; 
		output2 += ": "; 
		output2 += "CBase1::CallMe1()\n";
	}
	const char *me1;
};

class CBase2
{
public:
	CBase2() {me2 = "CBase2";}
	virtual void CallMe2() 
	{
		output2 += me2; 
		output2 += ": "; 
		output2 += "CBase2::CallMe2()\n";
	}
	const char *me2;
};

class CDerivedMultiple : public CBase1, public CBase2
{
public:
	CDerivedMultiple() : CBase1(), CBase2() {}
};


static CDerivedMultiple d;

bool TestMultipleInheritance()
{
	bool fail = false;
	int r;
	
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Register the CDerived class
	r = engine->RegisterObjectType("class", 0, 0);
	r = engine->RegisterObjectMethod("class", "void CallMe1()", asMETHOD(CDerivedMultiple, CallMe1), asCALL_THISCALL);
	r = engine->RegisterObjectMethod("class", "void CallMe2()", asMETHOD(CDerivedMultiple, CallMe2), asCALL_THISCALL);
	
	// Register the global CDerived object
	r = engine->RegisterGlobalProperty("class d", &d);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->ExecuteString(0, "d.CallMe1(); d.CallMe2();");
	
	if( output2 != "CBase1: CBase1::CallMe1()\nCBase2: CBase2::CallMe2()\n" )
	{
		printf("%s: Method calls failed.\n%s", TESTNAME, output2.c_str());
		fail = true;
	}
	
	engine->Release();

	return fail;
}
