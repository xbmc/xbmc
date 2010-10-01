// This test verifies that each thread has it's own shared string buffer

#include "utils.h"
#include <process.h>
#include <windows.h>

namespace TestSharedString
{

#define TESTNAME "TestSharedString"

static const char *script = 
"void TestSharedString1() \n"
"{                        \n"
"}                        \n"
"void TestSharedString2() \n"
"{                        \n"
"}                        \n";

static asIScriptEngine *engine = 0;

void Thread(void *)
{
	const char *str = engine->GetFunctionDeclaration(engine->GetFunctionIDByIndex(0, 1));

	// Give AngelScript a chance to cleanup some memory 
	asThreadCleanup();
}

bool Test()
{
	bool fail = false;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	// Get a function declaration, this should be the same after the other thread terminates
	const char *str = engine->GetFunctionDeclaration(engine->GetFunctionIDByIndex(0, 0));

	// Create the second thread that in turn will get the declaration of another function
	HANDLE threadId = (HANDLE)_beginthread(Thread, 0, 0);

	// Make sure the other thread completes execution
	WaitForSingleObject(threadId, INFINITE);

	// Verify that the string we retrieved before is still the same
	if( strcmp(str, "void TestSharedString1()") != 0 )
	{
		printf("%s: Shared strings don't work as they should\n", TESTNAME);
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

