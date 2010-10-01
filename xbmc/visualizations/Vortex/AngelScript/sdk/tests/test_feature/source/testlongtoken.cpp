//
// Tests to verify that long tokens doesn't crash the library
//

#include "utils.h"

#define TESTNAME "TestLongToken"


bool TestLongToken()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	std::string str;

	str.resize(400);
	memset(&str[0], 'a', 400);
	str += " = 1";

	COutStream out;
	engine->ExecuteString(0, str.c_str());

	engine->Release();

	// Success
	return fail;
}
