//
// Tests situations with never visited code
//
// Author: Andreas Jonsson
//

#include "utils.h"

#define TESTNAME "TestNeverVisited"

static const char *script1 =
"void TestNeverVisited1()              \n"
"{                                     \n"
"  int a = 1;                          \n"
"  return;                             \n"
"  a = a + 1;                          \n"
"}                                     \n";

static const char *script2 = 
"void TestNeverVisited2()              \n"
"{                                     \n"
"  int a = 0;                          \n"
"  if( true )                          \n"
"    ++a;                              \n"
"  else                                \n"
"    a--;                              \n"
"}                                     \n";

static const char *script3 = 
"void TestNeverVisited3()              \n"
"{                                     \n"
"  int a = 0;                          \n"
"  while( a++ < 10 )                   \n"
"  {                                   \n"
"    if( true ) continue;              \n"
"    a--;                              \n"
"  }                                   \n"
"}                                     \n";

bool TestNeverVisited()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	CBufferedOutStream out;	
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, TESTNAME"1", script1, strlen(script1), 0);
	engine->Build(0);

	engine->AddScriptSection(0, TESTNAME"2", script2, strlen(script2), 0);
	engine->Build(0);

	engine->AddScriptSection(0, TESTNAME"3", script3, strlen(script3), 0);
	engine->Build(0);

	engine->Release();

	// Success
	return fail;
}
