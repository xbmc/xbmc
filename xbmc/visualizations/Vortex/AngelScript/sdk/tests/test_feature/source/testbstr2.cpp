//
// This test verifies that a temporary variable passed to  
// a function is freed when the function isn't found.
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestBStr2"


bool TestBStr2()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);

	COutStream out;
	int r = engine->ExecuteString(0, "MissingFunction(\"test\")");
	if( r >= 0 )
	{
		fail = true;
		printf("%s: ExecuteString() succeeded even though it shouldn't\n", TESTNAME);
	}

	engine->Release();

	return fail;
}
