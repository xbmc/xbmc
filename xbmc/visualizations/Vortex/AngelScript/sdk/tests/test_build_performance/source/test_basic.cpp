//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include <string>
using std::string;

namespace TestBasic
{

#define TESTNAME "TestBasic"

static const char *scriptBegin =
"void main()                                                 \n"
"{                                                           \n"
"   int[] array(2);                                          \n"
"   int[][] PWToGuild(26);                                   \n";

static const char *scriptMiddle = 
"   array[0] = 121; array[1] = 196; PWToGuild[0] = array;    \n";

static const char *scriptEnd =
"}                                                           \n";


void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("Machine 1\n");
	printf("AngelScript 1.10.1 WIP 1: ??.?? secs\n");
	printf("\n");
	printf("Machine 2\n");
	printf("AngelScript 1.10.1 WIP 1: 9.544 secs\n");
	printf("AngelScript 1.10.1 WIP 2: .6949 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetCommonMessageStream(&out);

	string script = scriptBegin;
	for( int n = 0; n < 4000; n++ )
		script += scriptMiddle;
	script += scriptEnd;

	double time = GetSystemTimer();

	engine->AddScriptSection(0, TESTNAME, script.c_str(), script.size(), 0);
	int r = engine->Build(0);

	time = GetSystemTimer() - time;

	if( r != 0 )
		printf("Build failed\n", TESTNAME);
	else
		printf("Time = %f secs\n", time);

	engine->Release();
}

} // namespace



