//
// This test was designed to test the functionality of methods 
// from classes with virtual inheritance
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestVirtualInheritance"

static std::string output2;

class CVBase1
{
public:
	CVBase1() {me1 = "CVBase1";}
	virtual void CallMe1() 
	{
		output2 += me1; 
		output2 += ": "; 
		output2 += "CVBase1::CallMe1()\n";
	}
	const char *me1;
};

class CVBase2
{
public:
	CVBase2() {me2 = "CVBase2";}
	virtual void CallMe2() 
	{
		output2 += me2; 
		output2 += ": "; 
		output2 += "CVBase2::CallMe2()\n";
	}
	const char *me2;
};

#ifdef _MSC_VER
// This part forces the compiler to use a generic method pointer for CDerivedVirtual methods
class CDerivedVirtual;
static const int CDerivedVirtual_ptrsize = sizeof(void (CDerivedVirtual::*)());
#endif

class CDerivedVirtual : virtual public CVBase1, virtual public CVBase2
{
public:
	CDerivedVirtual() : CVBase1(), CVBase2() {}
};


static CDerivedVirtual d;

bool TestVirtualInheritance()
{
#ifdef __GNUC__
	printf("%s: GNUC: AngelScript cannot detect virtual inheritance thus this test doesn't apply\n", TESTNAME);
	return false;
#endif

	bool fail = false;
	int r;
	
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Register the CDerived class
	r = engine->RegisterObjectType("class", 0, 0);
	r = engine->RegisterObjectMethod("class", "void CallMe1()", asMETHOD(CDerivedVirtual, CallMe1), asCALL_THISCALL);
	if( r != asNOT_SUPPORTED )
	{
		printf("%s: Registering virtual methods shouldn't be supported.\n", TESTNAME);
		fail = true;
	}

	r = engine->RegisterObjectMethod("class", "void CallMe2()", asMETHOD(CDerivedVirtual, CallMe2), asCALL_THISCALL);
	if( r != asNOT_SUPPORTED )
	{
		printf("%s: Registering virtual methods shouldn't be supported.\n", TESTNAME);
		fail = true;
	}

/*
	// Register the global CDerived object
	r = engine->RegisterGlobalProperty("class d", &d);

	COutStream out;
	engine->ExecuteString(0, "d.CallMe1(); d.CallMe2();", &out);
	
	if( output2 != "CVBase1: CVBase1::CallMe1()\nCVBase2: CVBase2::CallMe2()\n" )
	{
		printf("%s: Method calls failed.\n%s", TESTNAME, output2.c_str());
		fail = true;
	}
*/
	
	engine->Release();

	return fail;
}
