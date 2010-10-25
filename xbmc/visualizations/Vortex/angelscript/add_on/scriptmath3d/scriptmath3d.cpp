#include <assert.h>
#include <string.h> // strstr
#include <new> // new()
#include <math.h>
#include "scriptmath3d.h"

BEGIN_AS_NAMESPACE

Vector3::Vector3()
{
	x = 0;
	y = 0;
	z = 0;
}

Vector3::Vector3(const Vector3 &other)
{
	x = other.x;
	y = other.y;
	z = other.z;
}

Vector3::Vector3(float _x, float _y, float _z)
{
	x = _x;
	y = _y;
	z = _z;
}

bool operator==(const Vector3 &a, const Vector3 &b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

bool operator!=(const Vector3 &a, const Vector3 &b)
{
	return !(a == b);
}

Vector3 &Vector3::operator=(const Vector3 &other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	return *this;
}

Vector3 &Vector3::operator+=(const Vector3 &other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
}

Vector3 &Vector3::operator-=(const Vector3 &other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
}

Vector3 &Vector3::operator*=(float s)
{
	x *= s;
	y *= s;
	z *= s;
	return *this;
}

Vector3 &Vector3::operator/=(float s)
{
	x /= s;
	y /= s;
	z /= s;
	return *this;
}

float Vector3::length() const
{
	return sqrtf(x*x + y*y + z*z);
}

Vector3 operator+(const Vector3 &a, const Vector3 &b)
{
	// Return a new object as a script handle
	Vector3 res(a.x + b.x, a.y + b.y, a.z + b.z);
	return res;
}

Vector3 operator-(const Vector3 &a, const Vector3 &b)
{
	// Return a new object as a script handle
	Vector3 res(a.x - b.x, a.y - b.y, a.z - b.z);
	return res;
}

Vector3 operator*(float s, const Vector3 &v)
{
	// Return a new object as a script handle
	Vector3 res(v.x * s, v.y * s, v.z * s);
	return res;
}

Vector3 operator*(const Vector3 &v, float s)
{
	// Return a new object as a script handle
	Vector3 res(v.x * s, v.y * s, v.z * s);
	return res;
}

Vector3 operator/(const Vector3 &v, float s)
{
	// Return a new object as a script handle
	Vector3 res(v.x / s, v.y / s, v.z / s);
	return res;
}

//-----------------------
// AngelScript functions
//-----------------------

static void Vector3DefaultConstructor(Vector3 *self)
{
	new(self) Vector3();
}

static void Vector3CopyConstructor(const Vector3 &other, Vector3 *self)
{
	new(self) Vector3(other);
}

static void Vector3InitConstructor(float x, float y, float z, Vector3 *self)
{
	new(self) Vector3(x,y,z);
}

//-----------------------
// Generic calling convention
//-----------------------

static void Vector3DefaultConstructor_Generic(asIScriptGeneric *gen)
{
	Vector3 *self = (Vector3*)gen->GetObject();
	new(self) Vector3();
}

static void Vector3CopyConstructor_Generic(asIScriptGeneric *gen)
{
	Vector3 *other = (Vector3*)gen->GetArgObject(0);
	Vector3 *self = (Vector3*)gen->GetObject();
	Vector3CopyConstructor(*other, self);
}

static void Vector3InitConstructor_Generic(asIScriptGeneric *gen)
{
	float x = gen->GetArgFloat(0);
	float y = gen->GetArgFloat(1);
	float z = gen->GetArgFloat(2);
	Vector3 *self = (Vector3*)gen->GetObject();
	Vector3InitConstructor(x,y,z,self);
}

static void Vector3Equal_Generic(asIScriptGeneric *gen)
{
	Vector3 *a = (Vector3*)gen->GetObject();
	Vector3 *b = (Vector3*)gen->GetArgAddress(0);
	bool r = *a == *b;
    *(bool*)gen->GetAddressOfReturnLocation() = r;
}

static void Vector3Length_Generic(asIScriptGeneric *gen)
{
	Vector3 *s = (Vector3*)gen->GetObject();
	gen->SetReturnFloat(s->length());
}

static void Vector3AddAssign_Generic(asIScriptGeneric *gen)
{
	Vector3 *a = (Vector3*)gen->GetArgAddress(0);
	Vector3 *thisPointer = (Vector3*)gen->GetObject();
	*thisPointer += *a;
	gen->SetReturnAddress(thisPointer);
}

static void Vector3SubAssign_Generic(asIScriptGeneric *gen)
{
	Vector3 *a = (Vector3*)gen->GetArgAddress(0);
	Vector3 *thisPointer = (Vector3*)gen->GetObject();
	*thisPointer -= *a;
	gen->SetReturnAddress(thisPointer);
}

static void Vector3MulAssign_Generic(asIScriptGeneric *gen)
{
	float s = gen->GetArgFloat(0);
	Vector3 *thisPointer = (Vector3*)gen->GetObject();
	*thisPointer *= s;
	gen->SetReturnAddress(thisPointer);
}

static void Vector3DivAssign_Generic(asIScriptGeneric *gen)
{
	float s = gen->GetArgFloat(0);
	Vector3 *thisPointer = (Vector3*)gen->GetObject();
	*thisPointer /= s;
	gen->SetReturnAddress(thisPointer);
}

static void Vector3Add_Generic(asIScriptGeneric *gen)
{
	Vector3 *a = (Vector3*)gen->GetObject();
	Vector3 *b = (Vector3*)gen->GetArgAddress(0);
	Vector3 res = *a + *b;
	gen->SetReturnObject(&res);
}

static void Vector3Sub_Generic(asIScriptGeneric *gen)
{
	Vector3 *a = (Vector3*)gen->GetObject();
	Vector3 *b = (Vector3*)gen->GetArgAddress(0);
	Vector3 res = *a - *b;
	gen->SetReturnObject(&res);
}

static void Vector3FloatMulVector3_Generic(asIScriptGeneric *gen)
{
	float s = gen->GetArgFloat(0);
	Vector3 *v = (Vector3*)gen->GetObject();
	Vector3 res = s * *v;
	gen->SetReturnObject(&res);
}

static void Vector3Vector3MulFloat_Generic(asIScriptGeneric *gen)
{
	Vector3 *v = (Vector3*)gen->GetObject();
	float s = gen->GetArgFloat(0);
	Vector3 res = *v * s;
	gen->SetReturnObject(&res);
}

static void Vector3Vector3DivFloat_Generic(asIScriptGeneric *gen)
{
	Vector3 *v = (Vector3*)gen->GetObject();
	float s = gen->GetArgFloat(0);
	Vector3 res = *v / s;
	gen->SetReturnObject(&res);
}

//--------------------------------
// Registration
//-------------------------------------

void RegisterScriptMath3D_Native(asIScriptEngine *engine)
{
	int r;

	// Register the type
	r = engine->RegisterObjectType("vector3", sizeof(Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA); assert( r >= 0 );

	// Register the object properties
	r = engine->RegisterObjectProperty("vector3", "float x", offsetof(Vector3, x)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("vector3", "float y", offsetof(Vector3, y)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("vector3", "float z", offsetof(Vector3, z)); assert( r >= 0 );

	// Register the constructors
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,  "void f()",                     asFUNCTION(Vector3DefaultConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,  "void f(const vector3 &in)",       asFUNCTION(Vector3CopyConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,  "void f(float, float, float)",  asFUNCTION(Vector3InitConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Register the operator overloads
	r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(const vector3 &in)", asMETHODPR(Vector3, operator+=, (const Vector3 &), Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(const vector3 &in)", asMETHODPR(Vector3, operator-=, (const Vector3 &), Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(float)", asMETHODPR(Vector3, operator*=, (float), Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(float)", asMETHODPR(Vector3, operator/=, (float), Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "bool opEquals(const vector3 &in) const", asFUNCTIONPR(operator==, (const Vector3&, const Vector3&), bool), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opAdd(const vector3 &in) const", asFUNCTIONPR(operator+, (const Vector3&, const Vector3&), Vector3), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opSub(const vector3 &in) const", asFUNCTIONPR(operator-, (const Vector3&, const Vector3&), Vector3), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opMul(float) const", asFUNCTIONPR(operator*, (const Vector3&, float), Vector3), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const Vector3&), Vector3), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(float) const", asFUNCTIONPR(operator/, (const Vector3&, float), Vector3), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

	// Register the object methods
	r = engine->RegisterObjectMethod("vector3", "float length() const", asMETHOD(Vector3,length), asCALL_THISCALL); assert( r >= 0 );
}

void RegisterScriptMath3D_Generic(asIScriptEngine *engine)
{
	int r;

	// Register the type
	r = engine->RegisterObjectType("vector3", sizeof(Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA); assert( r >= 0 );

	// Register the object properties
	r = engine->RegisterObjectProperty("vector3", "float x", offsetof(Vector3, x)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("vector3", "float y", offsetof(Vector3, y)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("vector3", "float z", offsetof(Vector3, z)); assert( r >= 0 );

	// Register the constructors
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,    "void f()",                     asFUNCTION(Vector3DefaultConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,    "void f(const vector3 &in)",       asFUNCTION(Vector3CopyConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,    "void f(float, float, float)", asFUNCTION(Vector3InitConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );

	// Register the operator overloads
	r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(const vector3 &in)", asFUNCTION(Vector3AddAssign_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(const vector3 &in)", asFUNCTION(Vector3SubAssign_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(float)", asFUNCTION(Vector3MulAssign_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(float)", asFUNCTION(Vector3DivAssign_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "bool opEquals(const vector3 &in) const", asFUNCTION(Vector3Equal_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opAdd(const vector3 &in) const", asFUNCTION(Vector3Add_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opSub(const vector3 &in) const", asFUNCTION(Vector3Sub_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opMul_r(float) const", asFUNCTION(Vector3FloatMulVector3_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opMul(float) const", asFUNCTION(Vector3Vector3MulFloat_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(float) const", asFUNCTION(Vector3Vector3DivFloat_Generic), asCALL_GENERIC); assert( r >= 0 );

	// Register the object methods
	r = engine->RegisterObjectMethod("vector3", "float length() const", asFUNCTION(Vector3Length_Generic), asCALL_GENERIC); assert( r >= 0 );
}

void RegisterScriptMath3D(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptMath3D_Generic(engine);
	else
		RegisterScriptMath3D_Native(engine);
}

END_AS_NAMESPACE


