//
// Tests calling of a c-function from a script with 32 parameters
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestExecute32Args (mixed arguments)"

static bool testVal = false;
static bool called  = false;

static int ivalues[16];
static float fvalues[16];

static void cfunction(	int f1 , int f2 , int f3 , int f4 ,
		float f5 , float f6 , float f7 , float f8 ,
		int f9 , int f10, int f11, int f12,
		float f13, float f14, float f15, float f16,
		int f17, int f18, int f19, int f20,
		float f21, float f22, float f23, float f24,
		int f25, int f26, int f27, int f28,
		float f29, float f30, float f31, float f32)
{
	called = true;
	ivalues[ 0] = f1;
	ivalues[ 1] = f2;
	ivalues[ 2] = f3;
	ivalues[ 3] = f4;
	fvalues[ 0] = f5;
	fvalues[ 1] = f6;
	fvalues[ 2] = f7;
	fvalues[ 3] = f8;
	ivalues[ 4] = f9;
	ivalues[ 5] = f10;
	ivalues[ 6] = f11;
	ivalues[ 7] = f12;
	fvalues[ 4] = f13;
	fvalues[ 5] = f14;
	fvalues[ 6] = f15;
	fvalues[ 7] = f16;
	ivalues[ 8] = f17;
	ivalues[ 9] = f18;
	ivalues[10] = f19;
	ivalues[11] = f20;
	fvalues[ 8] = f21;
	fvalues[ 9] = f22;
	fvalues[10] = f23;
	fvalues[11] = f24;
	ivalues[12] = f25;
	ivalues[13] = f26;
	ivalues[14] = f27;
	ivalues[15] = f28;
	fvalues[12] = f29;
	fvalues[13] = f30;
	fvalues[14] = f31;
	fvalues[15] = f32;
	

	testVal =	(f1  ==  1) && (f2  ==  2) && (f3  ==  3) && (f4  ==  4) &&
			(f5  ==  5.0f) && (f6  ==  6.0f) && (f7  ==  7.0f) && (f8  ==  8.0f) &&
			(f9  ==  9) && (f10 == 10) && (f11 == 11) && (f12 == 12) &&
			(f13 == 13.0f) && (f14 == 14.0f) && (f15 == 15.0f) && (f16 == 16.0f) &&
			(f17 == 17) && (f18 == 18) && (f19 == 19) && (f20 == 20) &&
			(f21 == 21.0f) && (f22 == 22.0f) && (f23 == 23.0f) && (f24 == 24.0f) &&
			(f25 == 25) && (f26 == 26) && (f27 == 27) && (f28 == 28) &&
			(f29 == 29.0f) && (f30 == 30.0f) && (f31 == 31.0f) && (f32 == 32.0f);

}

bool TestExecute32MixedArgs()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction(		"void cfunction("
			"int, int, int, int,"
			"float, float, float, float,"
			"int, int, int, int,"
			"float, float, float, float,"
			"int, int, int, int,"
			"float, float, float, float,"
			"int, int, int, int,"
			"float, float, float, float"
		")", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, 
		"cfunction("
			" 1,  2,  3,  4,"
			" 5.0f,  6.0f,  7.0f,  8.0f,"
			" 9, 10, 11, 12,"
			"13.0f, 14.0f, 15.0f, 16.0f,"
			"17, 18, 19, 20,"
			"21.0f, 22.0f, 23.0f, 24.0f,"
			"25, 26, 27, 28,"
			"29.0f, 30.0f, 31.0f, 32.0f"
		")");

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
		int pos = 0;
		for( int i = 0; i < 4; i++ ) 
		{
			int j;
			for( j = 0; j < 4; j++ ) 
				printf("ivalue[%d]: %d\n", pos+j, ivalues[pos+j]);
			for( j = 0; j < 4; j++ ) 
				printf("fvalue[%d]: %f\n", pos+j, fvalues[pos+j]);
			pos += 4;
		}
		ret = true;
	}

	engine->Release();
	
	// Success
	return ret;
}
