#include "utils.h"

namespace TestFloat
{

#define TESTNAME "TestFloat"



static const char *script =
"void TestFloat()                               \n"
"{                                              \n"
"  float a = 2, b = 3, c = 1;                   \n"
"  c = a + b;                                   \n"
"  a = b + 23;                                  \n"
"  b = 12 + c;                                  \n"
"  c = a - b;                                   \n"
"  a = b - 23;                                  \n"
"  b = 12 - c;                                  \n"
"  c = a * b;                                   \n"
"  a = b * 23;                                  \n"
"  b = 12 * c;                                  \n"
"  c = a / b;                                   \n"
"  a = b / 23;                                  \n"
"  b = 12 / c;                                  \n"
"  c = a % b;                                   \n"
"  a = b % 23;                                  \n"
"  b = 12 % c;                                  \n"
"  a++;                                         \n"
"  ++a;                                         \n"
"  a += b;                                      \n"
"  a += 3;                                      \n"
"  a /= c;                                      \n"
"  a /= 5;                                      \n"
"  a = b = c;                                   \n"
"  func( a-1, b, c );                           \n"
"  a = -b;                                      \n"
"  a = func2();                                 \n"
"}                                              \n"
"void func(float a, float &in b, float &out c)  \n"
"{                                              \n"
"  c = a + b;                                   \n"
"  b = c;                                       \n"
"  g = g;                                       \n"
"}                                              \n"
"float g = 0;                                   \n"
"float func2()                                  \n"
"{                                              \n"
"  return g + 1;                                \n"
"}                                              \n";


bool Test()
{
	bool fail = false;
	COutStream out;
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, "script", script, strlen(script));
 	int r = engine->Build(0);
	if( r < 0 ) fail = true; 

	if( fail )
		printf("%s: failed\n", TESTNAME);

	engine->Release();

	// Success
 	return fail;
}

} // namespace

