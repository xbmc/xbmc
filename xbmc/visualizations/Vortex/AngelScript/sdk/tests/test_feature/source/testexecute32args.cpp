//
// Tests calling of a c-function from a script with 32 parameters
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestExecute32Args"

static bool testVal = false;
static bool called  = false;

static int values[32];

static void cfunction(int f1 , int f2 , int f3 , int f4 ,
                      int f5 , int f6 , int f7 , int f8 ,
                      int f9 , int f10, int f11, int f12,
                      int f13, int f14, int f15, int f16,
                      int f17, int f18, int f19, int f20,
                      int f21, int f22, int f23, int f24,
                      int f25, int f26, int f27, int f28,
                      int f29, int f30, int f31, int f32) 
{
	called = true;
	values[ 0] = f1;
	values[ 1] = f2;
	values[ 2] = f3;
	values[ 3] = f4;
	values[ 4] = f5;
	values[ 5] = f6;
	values[ 6] = f7;
	values[ 7] = f8;
	values[ 8] = f9;
	values[ 9] = f10;
	values[10] = f11;
	values[11] = f12;
	values[12] = f13;
	values[13] = f14;
	values[14] = f15;
	values[15] = f16;
	values[16] = f17;
	values[17] = f18;
	values[18] = f19;
	values[19] = f20;
	values[20] = f21;
	values[21] = f22;
	values[22] = f23;
	values[23] = f24;
	values[24] = f25;
	values[25] = f26;
	values[26] = f27;
	values[27] = f28;
	values[28] = f29;
	values[29] = f30;
	values[30] = f31;
	values[31] = f32;
	

	testVal = (f1  ==  1) && (f2  ==  2) && (f3  ==  3) && (f4  ==  4) &&
	          (f5  ==  5) && (f6  ==  6) && (f7  ==  7) && (f8  ==  8) &&
	          (f9  ==  9) && (f10 == 10) && (f11 == 11) && (f12 == 12) &&
	          (f13 == 13) && (f14 == 14) && (f15 == 15) && (f16 == 16) &&
	          (f17 == 17) && (f18 == 18) && (f19 == 19) && (f20 == 20) &&
	          (f21 == 21) && (f22 == 22) && (f23 == 23) && (f24 == 24) &&
	          (f25 == 25) && (f26 == 26) && (f27 == 27) && (f28 == 28) &&
	          (f29 == 29) && (f30 == 30) && (f31 == 31) && (f32 == 32);
}

bool TestExecute32Args()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void cfunction(int, int, int, int,"
	                                              "int, int, int, int,"
	                                              "int, int, int, int,"
	                                              "int, int, int, int,"
	                                              "int, int, int, int,"
	                                              "int, int, int, int,"
	                                              "int, int, int, int,"
	                                              "int, int, int, int)", 
				                   asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "cfunction( 1,  2,  3,  4,"
	                                " 5,  6,  7,  8,"
	                                " 9, 10, 11, 12,"
	                                "13, 14, 15, 16,"
	                                "17, 18, 19, 20,"
	                                "21, 22, 23, 24,"
	                                "25, 26, 27, 28,"
	                                "29, 30, 31, 32)");

	if( !called ) 
	{
		// failure
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} 
	else if( !testVal ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got:\n\n", TESTNAME);
		for (int i = 0; i < 32; i++) 
			printf("value %d: %d\n", i, values[i]);
		
		ret = true;
	}

	engine->Release();
	
	// Success
	return ret;
}
