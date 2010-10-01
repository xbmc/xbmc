//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include <vector>
#include "utils.h"

namespace TestSaveLoad
{

#define TESTNAME "TestSaveLoad"


class CBytecodeStream : public asIBinaryStream
{
public:
	CBytecodeStream() {wpointer = 0;rpointer = 0;}

	void Write(const void *ptr, asUINT size) {buffer.resize(buffer.size() + size); memcpy(&buffer[wpointer], ptr, size); wpointer += size;}
	void Read(void *ptr, asUINT size) {memcpy(ptr, &buffer[rpointer], size); rpointer += size;}

	int rpointer;
	int wpointer;
	std::vector<asBYTE> buffer;
};


static const char *script1 =
"import void Test() from \"DynamicModule\";   \n"
"OBJ g_obj;                                   \n"
"A @gHandle;                                  \n"
"void main()                                  \n"
"{                                            \n"
"  Test();                                    \n"
"  TestStruct();                              \n"
"  TestArray();                               \n"
"}                                            \n"
"void TestObj(OBJ &out obj)                   \n"
"{                                            \n"
"}                                            \n"
"void TestStruct()                            \n"
"{                                            \n"
"  A a;                                       \n"
"  a.a = 2;                                   \n"
"  A@ b = @a;                                 \n"
"}                                            \n"
"void TestArray()                             \n"
"{                                            \n"
"  A[] c(3);                                  \n"
"  int[] d(2);                                \n"
"  A[]@[] e(1);                               \n"
"  @e[0] = @c;                                \n"
"}                                            \n"
"struct A                                     \n"
"{                                            \n"
"  int a;                                     \n"
"};                                           \n"
"void TestHandle(string @str)                 \n"
"{                                            \n"
"}                                            \n";

static const char *script2 =
"void Test()                               \n"
"{                                         \n"
"  int[] a(3);                             \n"
"  a[0] = 23;                              \n"
"  a[1] = 13;                              \n"
"  a[2] = 34;                              \n"
"  if( a[0] + a[1] + a[2] == 23+13+34 )    \n"
"    number = 1234567890;                  \n"
"}                                         \n";

bool Test()
{
	bool fail = false;

	int number = 0;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);
	engine->RegisterGlobalProperty("int number", &number);

	engine->RegisterObjectType("OBJ", sizeof(int), asOBJ_PRIMITIVE);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);
	engine->SetCommonMessageStream(&out);
	engine->Build(0);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule");

	// Bind all functions that the module imports
	r = engine->BindAllImportedFunctions(0); assert( r >= 0 );

	// Save the compiled byte code
	CBytecodeStream stream;
	engine->SaveByteCode(0, &stream);

	// Load the compiled byte code into the same module
	engine->LoadByteCode(0, &stream);

	// Verify if handles are properly resolved
	int funcID = engine->GetFunctionIDByDecl(0, "void TestHandle(string @)");
	if( funcID < 0 ) 
	{
		printf("%s: Failed to identify function with handle\n", TESTNAME);
		fail = true;
	}

	// Bind the imported functions again
	r = engine->BindAllImportedFunctions(0); assert( r >= 0 );

	engine->ExecuteString(0, "main()");

	engine->Release();

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		fail = true;
	}

	// Success
	return fail;
}

} // namespace

