// 
// Test designed to verify functionality of the switch case
//
// Author: Andreas Jönsson
//

#include "utils.h"
using namespace std;

#define TESTNAME "TestSwitch"


static const char *script =
"void _switch()                  \n"  // 1
"{                               \n"  // 2
"  for( int n = -1; n < 2; ++n ) \n"  // 3
"    switch( n )                 \n"  // 4
"    {                           \n"  // 5
"    case 0:                     \n"  // 6
"      add(0);                   \n"  // 7
"      break;                    \n"  // 8
"    case -1:                    \n"  // 9
"      add(-1);                  \n"  // 10
"      break;                    \n"  // 11
"    case 5:                     \n"  // 12
"      add(5);                   \n"  // 13
"      break;                    \n"  // 14
"    case 15:                    \n"  // 15
"      add(15);                  \n"  // 16
"      break;                    \n"  // 17
"    default:                    \n"  // 18
"      add(255);                 \n"  // 19
"      break;                    \n"  // 20
"    }                           \n"  // 21
"}                               \n"; // 22

static const char *script2 =
"const int a = 1;                   \n"
"const int8 b = 2;                  \n"
"void _switch()                     \n"
"{                                  \n"
"  const uint c = 3;                \n"
"  for( uint8 n = 0; n <= 5; ++n )  \n"
"  {                                \n"
"    switch( n )                    \n"
"    {                              \n"
"    case 5: Log(\"5\"); break;     \n"
"    case 4: Log(\"4\"); break;     \n"
"    case c: Log(\"3\"); break;     \n"
"    case b: Log(\"2\"); break;     \n"
"    case a: Log(\"1\"); break;     \n"
"    default: Log(\"d\"); break;    \n"
"    }                              \n"
"  }                                \n"
"}                                  \n";

static int sum = 0;

static void add(int val)
{
	sum += val;
}

static string log;
static void Log(const char *txt)
{
	log += txt;
}

static const char *StrFactory(int length, const char *txt)
{
	return txt;
}

bool TestSwitch()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("staticstring", sizeof(char*), asOBJ_PRIMITIVE);
	engine->RegisterStringFactory("staticstring", asFUNCTION(StrFactory), asCALL_CDECL);
	engine->RegisterGlobalFunction("void Log(staticstring)", asFUNCTION(Log), asCALL_CDECL);

	engine->RegisterGlobalFunction("void add(int)", asFUNCTION(add), asCALL_CDECL);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection(0, "switch", script, strlen(script), 0);
	int r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: Failed to build script\n", TESTNAME);
		fail = true;
	}

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void _switch()"));
	ctx->Execute();

	if( sum != 254 )
	{
		printf("%s: Expected %d, got %d\n", TESTNAME, 254, sum);
		fail = true;
	}

	ctx->Release();

	engine->AddScriptSection(0, "switch", script2, strlen(script2), 0);
	engine->Build(0);

	engine->ExecuteString(0, "_switch()");

	if( log != "d12345" )
	{
		fail = true;
		printf("%s: Switch failed. Got: %s\n", TESTNAME, log.c_str());
	}

	engine->Release();

	return fail;
}
