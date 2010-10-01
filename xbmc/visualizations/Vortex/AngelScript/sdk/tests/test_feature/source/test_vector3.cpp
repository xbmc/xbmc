#include "utils.h"

#define TESTNAME "TestVector3"

class Vector3
{
public:
	inline Vector3() {}
	inline Vector3( float fX, float fY, float fZ ) : x( fX ), y( fY ), z( fZ ) {}

	float x,y,z;
};

static char *script =
"Vector3 TestVector3()  \n"
"{                      \n"
"  Vector3 v;           \n"
"  v.x=1;               \n"
"  v.y=2;               \n"
"  v.z=3;               \n"
"  return v;            \n"
"}                      \n"
"Vector3 TestVector3Val(Vector3 v)  \n"
"{                                  \n"
"  return v;                        \n"
"}                                  \n"
"void TestVector3Ref(Vector3 &out v)\n"
"{                                  \n"
"  v.x=1;                           \n"
"  v.y=2;                           \n"
"  v.z=3;                           \n"
"}                                  \n";

bool TestVector3()
{
	bool fail = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->RegisterObjectType("Vector3",sizeof(Vector3),asOBJ_CLASS_CDA);assert(r>=0);
	r = engine->RegisterObjectProperty("Vector3","float x",offsetof(Vector3,x));assert(r>=0);
	r = engine->RegisterObjectProperty("Vector3","float y",offsetof(Vector3,y));assert(r>=0);
	r = engine->RegisterObjectProperty("Vector3","float z",offsetof(Vector3,z));assert(r>=0);

	Vector3 v;
	engine->RegisterGlobalProperty("Vector3 v", &v);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script, strlen(script));
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: Failed to build\n", TESTNAME);
		fail = true;
	}
	else
	{
		// Internal return
		r = engine->ExecuteString(0, "v = TestVector3();");
		if( r < 0 )
		{
			printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
			fail = true;
		}
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			fail = true;
		}

		// Manual return
		v.x = 0; v.y = 0; v.z = 0;

		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(engine->GetFunctionIDByDecl(0, "Vector3 TestVector3()"));

		ctx->Execute();
		Vector3 *ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 1 || ret->y != 2 || ret->z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			fail = true;
		}

		ctx->Prepare(engine->GetFunctionIDByDecl(0, "Vector3 TestVector3Val(Vector3)"));
		v.x = 3; v.y = 2; v.z = 1;
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 3 || ret->y != 2 || ret->z != 1 )
		{
			printf("%s: Failed to pass Vector3 by val\n", TESTNAME);
			fail = true;
		}

		ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestVector3Ref(Vector3 &out)"));
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to pass Vector3 by ref\n", TESTNAME);
			fail = true;
		}

		ctx->Release();
	}

	engine->Release();

	return fail;
}
