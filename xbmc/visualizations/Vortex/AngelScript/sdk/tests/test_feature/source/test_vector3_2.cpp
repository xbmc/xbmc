#include "utils.h"

namespace TestVector3_2
{

#define TESTNAME "TestVector3_2"

class csVector3
{
public:
	inline csVector3() {}
	inline csVector3( float fX, float fY, float fZ ) : x( fX ), y( fY ), z( fZ ) {}

	/// Multiply this vector by a scalar.
	inline csVector3& operator*= (float f)
	{ x *= f; y *= f; z *= f; return *this; }

	/// Divide this vector by a scalar.
	inline csVector3& operator/= (float f)
	{ f = 1.0f / f; x *= f; y *= f; z *= f; return *this; }

	/// Add another vector to this vector.
	inline csVector3& operator+= (const csVector3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	/// Subtract another vector from this vector.
	inline csVector3& operator-= (const csVector3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	float x,y,z;
};

/// Add two vectors.
inline csVector3 operator+(const csVector3& v1, const csVector3& v2)
{ return csVector3(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z); }

/// Subtract two vectors.
inline csVector3 operator-(const csVector3& v1, const csVector3& v2)
{ return csVector3(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z); }

/// Multiply a vector and a scalar.
inline csVector3 operator* (const csVector3& v, float f)
{ return csVector3(v.x*f, v.y*f, v.z*f); }

/// Multiply a vector and a scalar.
inline csVector3 operator* (float f, const csVector3& v)
{ return csVector3(v.x*f, v.y*f, v.z*f); }

/// Divide a vector by a scalar int.
inline csVector3 operator/ (const csVector3& v, int f)
{ return v / (float)f; }

void ConstructVector3(csVector3*o)
{
	new(o) csVector3;
}

void ConstructVector3(float a, float b, float c, csVector3*o)
{
	new(o) csVector3(a,b,c);
}


const char *script1 =
"void func() {\n"
"  Vector3 meshOrigin, sideVector, frontVector;\n"
"  float stepLength = 0;\n"
"  if(true) {\n"
"    Vector3 newPos1 = meshOrigin + sideVector*0.1f + frontVector*stepLength;\n"
"  }\n"
"}\n";



bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream(&out);

    r = engine->RegisterObjectType ("Vector3", sizeof (csVector3), asOBJ_CLASS_C); assert( r >= 0 );
    r = engine->RegisterObjectProperty ("Vector3", "float x", offsetof(csVector3, x)); assert( r >= 0 );
    r = engine->RegisterObjectProperty ("Vector3", "float y", offsetof(csVector3, y)); assert( r >= 0 );
    r = engine->RegisterObjectProperty ("Vector3", "float z", offsetof(csVector3, z)); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour ("Vector3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructVector3, (csVector3*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour ("Vector3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTIONPR(ConstructVector3, (float, float, float, csVector3*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
//    r = engine->RegisterObjectMethod ("Vector3", "float Length()", asMETHODPR(csVector3, Norm, (void) const, float), asCALL_THISCALL); assert( r >= 0 );
//    r = engine->RegisterObjectMethod ("Vector3", "void Normalize()", asMETHOD(csVector3, Normalize), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD, "Vector3 f(Vector3 &in, Vector3 &in)", asFUNCTIONPR(operator+, (const csVector3&, const csVector3&), csVector3), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterGlobalBehaviour (asBEHAVE_SUBTRACT, "Vector3 f(Vector3 &in, Vector3 &in)", asFUNCTIONPR(operator-, (const csVector3&, const csVector3&), csVector3), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterGlobalBehaviour (asBEHAVE_MULTIPLY, "Vector3 f(Vector3 &in, float)", asFUNCTIONPR(operator*, (const csVector3&, float), csVector3), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterGlobalBehaviour (asBEHAVE_MULTIPLY, "Vector3 f(float, Vector3 &in)", asFUNCTIONPR(operator*, (float, const csVector3&), csVector3), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterGlobalBehaviour (asBEHAVE_DIVIDE, "Vector3 f(Vector3 &in, float)", asFUNCTIONPR(operator/, (const csVector3&, float), csVector3), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour ("Vector3", asBEHAVE_ADD_ASSIGN, "Vector3 &f(Vector3 &in)", asMETHODPR(csVector3, operator+=, (const csVector3&), csVector3&), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour ("Vector3", asBEHAVE_SUB_ASSIGN, "Vector3 &f(Vector3 &in)", asMETHODPR(csVector3, operator+=, (const csVector3&), csVector3&), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour ("Vector3", asBEHAVE_MUL_ASSIGN, "Vector3 &f(float)", asMETHODPR(csVector3, operator*=, (float), csVector3&), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour ("Vector3", asBEHAVE_DIV_ASSIGN, "Vector3 &f(float)", asMETHODPR(csVector3, operator/=, (float), csVector3&), asCALL_THISCALL); assert( r >= 0 );


	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1));
	r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: Failed to build\n", TESTNAME);
		fail = true;
	}
	else
	{
		// Internal return
		r = engine->ExecuteString(0, "func()");
		if( r < 0 )
		{
			printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
			fail = true;
		}
	}

	engine->Release();

	return fail;
}

}
