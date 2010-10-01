#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#endif
#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

#ifdef __dreamcast__
#include <kos.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

#endif

bool TestCreateEngine();
bool TestExecute();
bool TestExecute1Arg();
bool TestExecute2Args();
bool TestExecute4Args();
bool TestExecute4Argsf();
bool TestExecute32Args();
bool TestExecuteMixedArgs();
bool TestExecute32MixedArgs();
bool TestExecuteThis32MixedArgs();
bool TestReturn();
bool TestReturnF();
bool TestReturnD();
bool TestTempVar();
bool TestExecuteScript();
bool Test2Modules();
bool TestStdcall4Args();
bool TestInt64();
bool TestModuleRef();
bool TestEnumGlobVar();
bool TestGlobalVar();
bool TestBStr();
bool TestBStr2();
bool TestSwitch();
bool TestNegateOperator();
bool TestException();
bool TestCDecl_Class();
bool TestCDecl_ClassA();
bool TestCDecl_ClassC();
bool TestCDecl_ClassD();
bool TestNotComplexThisCall();
bool TestNotComplexStdcall();
bool TestReturnWithCDeclObjFirst();
bool TestStdString();
bool TestLongToken();
bool TestVirtualMethod();
bool TestMultipleInheritance();
bool TestVirtualInheritance();
bool TestStack();
bool TestExecuteString();
bool TestCondition();
bool TestFuncOverload();
bool TestNeverVisited();
bool TestNested();
bool TestConstructor();
bool TestOptimize();
bool TestNotInitialized();
bool TestVector3();

namespace TestCustomMem       { bool Test(); }
namespace TestGeneric         { bool Test(); }
namespace TestDebug           { bool Test(); }
namespace TestSuspend         { bool Test(); }
namespace TestConstProperty   { bool Test(); }
namespace TestConstObject     { bool Test(); }
namespace TestOutput          { bool Test(); }
namespace TestImport          { bool Test(); }
namespace TestImport2         { bool Test(); }
namespace Test2Func           { bool Test(); }
namespace TestDiscard         { bool Test(); }
namespace TestCircularImport  { bool Test(); }
namespace TestMultiAssign     { bool Test(); }
namespace TestSaveLoad        { bool Test(); }
namespace TestConstructor2    { bool Test(); }
namespace TestScriptCall      { bool Test(); }
namespace TestArray           { bool Test(); }
namespace TestArrayHandle     { bool Test(); }
namespace TestStdVector       { bool Test(); }
namespace TestArrayObject     { bool Test(); }
namespace TestPointer         { bool Test(); }
namespace TestConversion      { bool Test(); }
namespace TestObject          { bool Test(); }
namespace TestObject2         { bool Test(); }
namespace TestObject3         { bool Test(); }
namespace TestExceptionMemory { bool Test(); }
namespace TestArgRef          { bool Test(); }
namespace TestObjHandle       { bool Test(); }
namespace TestObjHandle2      { bool Test(); }
namespace TestObjZeroSize     { bool Test(); }
namespace TestRefArgument     { bool Test(); }
namespace TestStack2          { bool Test(); }
namespace TestScriptString    { bool Test(); }
namespace TestScriptStruct    { bool Test(); }
namespace TestStructIntf      { bool Test(); }
namespace TestAutoHandle      { bool Test(); }
namespace TestAny             { bool Test(); }
namespace TestArrayIntf       { bool Test(); }
namespace TestDynamicConfig   { bool Test(); }
namespace TestStream          { bool Test(); }
namespace TestConfig          { bool Test(); }
namespace TestConfigAccess    { bool Test(); }
namespace TestFloat           { bool Test(); }
namespace TestVector3_2       { bool Test(); }
namespace TestDict            { bool Test(); }
namespace TestUnsafeRef       { bool Test(); }
namespace TestReturnString    { bool Test(); }


void DetectMemoryLeaks()
{
#if defined(_MSC_VER)
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
	_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
#endif
}

extern "C" void BreakPoint()
{
	printf("Breakpoint\n");
}

//----------------------------------
// Test with these flags as well
//
// + AS_MAX_PORTABILITY
// + AS_ALLOW_UNSAFE_REFERENCES
//----------------------------------

int main(int argc, char **argv)
{
	DetectMemoryLeaks();

#ifdef __dreamcast__
	fs_chdir(asTestDir);
#endif

#ifndef AS_MAX_PORTABILITY
	if( TestAny::Test()               ) goto failed; else printf("-- TestAny passed\n");
	if( TestArrayObject::Test()       ) goto failed; else printf("-- TestArrayObject passed\n");
	if( TestConfig::Test()            ) goto failed; else printf("-- TestConfig passed\n");
	if( TestSwitch()                  ) goto failed; else printf("-- TestSwitch passed\n");
	if( TestRefArgument::Test()       ) goto failed; else printf("-- TestRefArgument passed\n");
	if( TestStream::Test()            ) goto failed; else printf("-- TestStream passed\n");
	if( TestSaveLoad::Test()          ) goto failed; else printf("-- TestSaveLoad passed\n");
	if( TestArray::Test()             ) goto failed; else printf("-- TestArray passed\n");
	if( TestObject::Test()            ) goto failed; else printf("-- TestObject passed\n");
	if( TestDict::Test()              ) goto failed; else printf("-- TestDict passed\n");
	if( TestSuspend::Test()           ) goto failed; else printf("-- TestSuspend passed\n");
	if( TestArrayHandle::Test()       ) goto failed; else printf("-- TestArrayHandle passed\n");
	if( TestObject3::Test()           ) goto failed; else printf("-- TestObject3 passed\n");
	if( TestOptimize()                ) goto failed; else printf("-- TestOptimize passed\n");
	if( TestScriptString::Test()      ) goto failed; else printf("-- TestScriptString passed\n");
	if( TestConversion::Test()        ) goto failed; else printf("-- TestConversion passed\n");
	if( TestObject2::Test()           ) goto failed; else printf("-- TestObject2 passed\n");
	if( TestVector3_2::Test()         ) goto failed; else printf("-- TestVector3_2 passed\n");
	if( TestStdVector::Test()         ) goto failed; else printf("-- TestStdVector passed\n");
	if( TestConfigAccess::Test()      ) goto failed; else printf("-- TestConfigAccess passed\n");
	if( TestDebug::Test()             ) goto failed; else printf("-- TestDebug passed\n");
	if( TestImport::Test()            ) goto failed; else printf("-- TestImport passed\n");
	if( TestImport2::Test()           ) goto failed; else printf("-- TestImport2 passed\n");
	if( TestEnumGlobVar()             ) goto failed; else printf("-- TestEnumGlobVar passed\n");
	if( TestStdString()               ) goto failed; else printf("-- TestStdString passed\n");
	if( TestConstObject::Test()       ) goto failed; else printf("-- TestConstObject passed\n");
	if( TestGlobalVar()               ) goto failed; else printf("-- TestGlobalVar passed\n");
	if( TestDynamicConfig::Test()     ) goto failed; else printf("-- TestDynamicConfig passed\n");
	if( TestStack2::Test()            ) goto failed; else printf("-- TestStack2 passed\n");
	if( TestBStr()                    ) goto failed; else printf("-- TestBStr passed\n");
	if( TestScriptStruct::Test()      ) goto failed; else printf("-- TestScriptStruct passed\n");
	if( TestArrayIntf::Test()         ) goto failed; else printf("-- TestArrayIntf passed\n");
	if( TestStructIntf::Test()        ) goto failed; else printf("-- TestStructIntf passed\n");
	if( TestCustomMem::Test()         ) goto failed; else printf("-- TestCustomMem passed\n");
	if( TestObjHandle::Test()         ) goto failed; else printf("-- TestObjHandle passed\n");
	if( TestConstructor()             ) goto failed; else printf("-- TestConstructor passed\n");
	if( TestConstructor2::Test()      ) goto failed; else printf("-- TestConstructor2 passed\n");
	if( TestAutoHandle::Test()        ) goto failed; else printf("-- TestAutoHandle passed\n");
	if( TestExceptionMemory::Test()   ) goto failed; else printf("-- TestExceptionMemory passed\n");
	if( TestObjHandle2::Test()        ) goto failed; else printf("-- TestObjHandle2 passed\n");
	if( TestObjZeroSize::Test()       ) goto failed; else printf("-- TestObjZeroSize passed\n");
	if( TestCondition()               ) goto failed; else printf("-- TestCondition passed\n");
	if( TestFloat::Test()             ) goto failed; else printf("-- TestFloat passed\n");
	if( TestNegateOperator()          ) goto failed; else printf("-- TestNegateOperator passed\n");
	if( TestReturnWithCDeclObjFirst() ) goto failed; else printf("-- TestReturnWithCDeclObjFirst passed\n");
	if( TestVector3()                 ) goto failed; else printf("-- TestVector3 passed\n");
	if( TestInt64()                   ) goto failed; else printf("-- TestInt64 passed\n");
	if( TestCDecl_Class()             ) goto failed; else printf("-- TestCDecl_Class passed\n");
	if( TestNotComplexStdcall()       ) goto failed; else printf("-- TestNotComplexStdcall passed\n");
	if( TestCDecl_ClassA()            ) goto failed; else printf("-- TestCDecl_ClassA passed\n");
	if( TestCDecl_ClassC()            ) goto failed; else printf("-- TestCDecl_ClassC passed\n");
	if( TestCDecl_ClassD()            ) goto failed; else printf("-- TestCDecl_ClassD passed\n");
	if( TestNotComplexThisCall()      ) goto failed; else printf("-- TestNotComplexThisCall passed\n");
	if( TestTempVar()                 ) goto failed; else printf("-- TestTempVar passed\n");
	if( TestGeneric::Test()           ) goto failed; else printf("-- TestGeneric passed\n");
	if( TestMultiAssign::Test()       ) goto failed; else printf("-- TestMultiAssign passed\n");
	if( TestNested()                  ) goto failed; else printf("-- TestNested passed\n");
	if( TestArgRef::Test()            ) goto failed; else printf("-- TestArgRef passed\n");
	if( TestFuncOverload()            ) goto failed; else printf("-- TestFuncOverload passed\n");
	if( TestModuleRef()               ) goto failed; else printf("-- TestModuleRef passed\n");
	if( TestExecuteThis32MixedArgs()  ) goto failed; else printf("-- TestExecuteThis32MixedArgs passed\n");
	if( TestVirtualMethod()           ) goto failed; else printf("-- TestVirtualMethod passed\n");
	if( TestMultipleInheritance()     ) goto failed; else printf("-- TestMultipleInheritance passed\n");
	if( TestExecuteString()           ) goto failed; else printf("-- TestExecuteString passed\n");
	if( TestException()               ) goto failed; else printf("-- TestException passed\n");
	if( TestStack()                   ) goto failed; else printf("-- TestStack passed\n");
	if( TestNotInitialized()          ) goto failed; else printf("-- TestNotInitialized passed\n");
	if( TestCreateEngine()            ) goto failed; else printf("-- TestCreateEngine passed\n");
	if( TestExecute()                 ) goto failed; else printf("-- TestExecute passed\n");
	if( TestExecute1Arg()             ) goto failed; else printf("-- TestExecute1Arg passed\n");
	if( TestExecute2Args()            ) goto failed; else printf("-- TestExecute2Args passed\n");
	if( TestExecute4Args()            ) goto failed; else printf("-- TestExecute4Args passed\n");
	if( TestExecute4Argsf()           ) goto failed; else printf("-- TestExecute4Argsf passed\n");
	if( TestExecute32Args()           ) goto failed; else printf("-- TestExecute32Args passed\n");
	if( TestExecuteMixedArgs()        ) goto failed; else printf("-- TestExecuteMixedArgs passed\n");
	if( TestExecute32MixedArgs()      ) goto failed; else printf("-- TestExecute32MixedArgs passed\n");
	if( TestStdcall4Args()            ) goto failed; else printf("-- TestStdcall4Args passed\n");
	if( TestReturn()                  ) goto failed; else printf("-- TestReturn passed\n");
	if( TestReturnF()                 ) goto failed; else printf("-- TestReturnF passed\n");
	if( TestReturnD()                 ) goto failed; else printf("-- TestReturnD passed\n");
	if( Test2Modules()                ) goto failed; else printf("-- Test2Modules passed\n");
	if( TestBStr2()                   ) goto failed; else printf("-- TestBStr2 passed\n");
	if( TestLongToken()               ) goto failed; else printf("-- TestLongToken passed\n");
	if( TestVirtualInheritance()      ) goto failed; else printf("-- TestVirtualInheritance passed\n");
	if( TestOutput::Test()            ) goto failed; else printf("-- TestOutput passed\n");
	if( Test2Func::Test()             ) goto failed; else printf("-- Test2Func passed\n");
	if( TestDiscard::Test()           ) goto failed; else printf("-- TestDiscard passed\n");
	if( TestCircularImport::Test()    ) goto failed; else printf("-- TestCircularImport passed\n");
	if( TestNeverVisited()            ) goto failed; else printf("-- TestNeverVisited passed\n");
	if( TestConstProperty::Test()     ) goto failed; else printf("-- TestConstProperty passed\n");
	if( TestExecuteScript()           ) goto failed; else printf("-- TestExecuteScript passed\n"); 
#else
	if( TestGeneric::Test()           ) goto failed; else printf("-- TestGeneric passed\n");
	if( TestExecuteScript()           ) goto failed; else printf("-- TestExecuteScript passed\n");
#endif

#ifdef AS_ALLOW_UNSAFE_REFERENCES
	if( TestReturnString::Test()      ) goto failed; else printf("-- TestReturnString passed\n");
	if( TestUnsafeRef::Test()         ) goto failed; else printf("-- TestUnsafeRef passed\n");
#endif

	// No longer valid
//	if( TestPointer::Test()           ) goto failed; else printf("-- TestPointer passed\n");

//succeed:
	printf("--------------------------------------------\n");
	printf("All of the tests passed with success.\n\n");
#ifdef WIN32
	printf("Press any key to quit.\n");
	while(!getch());
#endif
	return 0;

failed:
	printf("--------------------------------------------\n");
	printf("One of the tests failed, see details above.\n\n");
#ifdef WIN32
	printf("Press any key to quit.\n");
	while(!getch());
#endif
	return 0;
}
