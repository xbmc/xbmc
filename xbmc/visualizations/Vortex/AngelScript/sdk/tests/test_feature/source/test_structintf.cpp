#include "utils.h"

namespace TestStructIntf
{

#define TESTNAME "TestStructIntf"

// Normal structure
static const char *script1 =
"struct MyStruct              \n"
"{                            \n"
"   float a;                  \n"
"   string b;                 \n"
"   string @c;                \n"
"};                           \n"
"void Test()                  \n"
"{                            \n"
"   MyStruct s;               \n"
"   s.a = 3.141592f;          \n"
"   s.b = \"test\";           \n"
"   @s.c = \"test2\";         \n"
"   g_any.store(@s);          \n"
"}                            \n";



asIScriptAny *any = 0;

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	any = (asIScriptAny*)engine->CreateScriptObject(engine->GetTypeIdByDecl(0, "any"));
	engine->RegisterGlobalProperty("any g_any", any);

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	// Try retrieving the type Id for the structure
	int typeId = engine->GetTypeIdByDecl(0, "MyStruct");
	if( typeId < 0 )
	{
		printf("%s: Failed to retrieve the type id for the script struct\n", TESTNAME);
		fail = true;
	}

	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED ) 
		fail = true;
	else
	{		
		asIScriptStruct *s = 0;
		typeId = any->GetTypeId();
		any->Retrieve(&s, typeId);

		if( (typeId & asTYPEID_MASK_OBJECT) != asTYPEID_SCRIPTSTRUCT )
			fail = true;

		if( strcmp(engine->GetTypeDeclaration(typeId), "MyStruct@") )
			fail = true;

		typeId = s->GetStructTypeId();
		if( strcmp(engine->GetTypeDeclaration(typeId), "MyStruct") )
			fail = true;

		if( s->GetPropertyCount() != 3 )
			fail = true;

		if( strcmp(s->GetPropertyName(0), "a") )
			fail = true;

		if( s->GetPropertyTypeId(0) != engine->GetTypeIdByDecl(0, "float") )
			fail = true;

		if( *(float*)s->GetPropertyPointer(0) != 3.141592f )
			fail = true;

		if( strcmp(s->GetPropertyName(1), "b") )
			fail = true;

		if( s->GetPropertyTypeId(1) != engine->GetTypeIdByDecl(0, "string") )
			fail = true;

		if( ((asCScriptString*)s->GetPropertyPointer(1))->buffer != "test" )
			fail = true;

		if( strcmp(s->GetPropertyName(2), "c") )
			fail = true;

		if( s->GetPropertyTypeId(2) != engine->GetTypeIdByDecl(0, "string@") )
			fail = true;

		if( (*(asCScriptString**)s->GetPropertyPointer(2))->buffer != "test2" )
			fail = true;

		if( s )
			s->Release();
	}

	if( any )
		any->Release();

	// The type id is valid for as long as the type exists
	if( strcmp(engine->GetTypeDeclaration(typeId), "MyStruct") )
		fail = true;

	// Make sure the type is not used anywhere
	engine->Discard(0);
	engine->GarbageCollect();

	// The type id is no longer valid
	if( engine->GetTypeDeclaration(typeId) != 0 )
		fail = true;

	engine->Release();

	// Success
	return fail;
}

} // namespace

