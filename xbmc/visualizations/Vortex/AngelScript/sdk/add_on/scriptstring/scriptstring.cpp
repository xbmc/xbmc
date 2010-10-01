#include <assert.h>
#include <sstream>
#include "scriptstring.h"
using namespace std;

BEGIN_AS_NAMESPACE

//--------------
// constructors
//--------------

asCScriptString::asCScriptString()
{
	// Count the first reference
	refCount = 1;
}

asCScriptString::asCScriptString(const char *s)
{
	refCount = 1;
	buffer = s;
}

asCScriptString::asCScriptString(const string &s)
{
	refCount = 1;
	buffer = s;
}

asCScriptString::asCScriptString(const asCScriptString &s)
{
	refCount = 1;
	buffer = s.buffer;
}

asCScriptString::~asCScriptString()
{
	assert( refCount == 0 );
}

//--------------------
// reference counting
//--------------------

void asCScriptString::AddRef()
{
	refCount++;
}

void asCScriptString::Release()
{
	if( --refCount == 0 )
		delete this;
}

//-----------------
// string = string
//-----------------

asCScriptString &asCScriptString::operator=(const asCScriptString &other)
{
	// Copy only the buffer, not the reference counter
	buffer = other.buffer;

	// Return a reference to this object
	return *this;
}

//------------------
// string += string
//------------------

asCScriptString &asCScriptString::operator+=(const asCScriptString &other)
{
	buffer += other.buffer;
	return *this;
}

//-----------------
// string + string
//-----------------

asCScriptString *operator+(const asCScriptString &a, const asCScriptString &b)
{
	// Return a new object as a script handle
	return new asCScriptString(a.buffer + b.buffer);
}

//----------------
// string = value
//----------------

static asCScriptString &AssignBitsToString(asDWORD i, asCScriptString &dest)
{
	ostringstream stream;
	stream << hex << i;
	dest.buffer = stream.str(); 

	// Return a reference to the object
	return dest;
}

static asCScriptString &AssignUIntToString(unsigned int i, asCScriptString &dest)
{
	ostringstream stream;
	stream << i;
	dest.buffer = stream.str(); 
	return dest;
}

static asCScriptString &AssignIntToString(int i, asCScriptString &dest)
{
	ostringstream stream;
	stream << i;
	dest.buffer = stream.str(); 
	return dest;
}

static asCScriptString &AssignFloatToString(float f, asCScriptString &dest)
{
	ostringstream stream;
	stream << f;
	dest.buffer = stream.str(); 
	return dest;
}

static asCScriptString &AssignDoubleToString(double f, asCScriptString &dest)
{
	ostringstream stream;
	stream << f;
	dest.buffer = stream.str(); 
	return dest;
}

//-----------------
// string += value
//-----------------

static asCScriptString &AddAssignBitsToString(asDWORD i, asCScriptString &dest)
{
	ostringstream stream;
	stream << hex << i;
	dest.buffer += stream.str(); 
	return dest;
}

static asCScriptString &AddAssignUIntToString(unsigned int i, asCScriptString &dest)
{
	ostringstream stream;
	stream << i;
	dest.buffer += stream.str(); 
	return dest;
}

static asCScriptString &AddAssignIntToString(int i, asCScriptString &dest)
{
	ostringstream stream;
	stream << i;
	dest.buffer += stream.str(); 
	return dest;
}

static asCScriptString &AddAssignFloatToString(float f, asCScriptString &dest)
{
	ostringstream stream;
	stream << f;
	dest.buffer += stream.str(); 
	return dest;
}

static asCScriptString &AddAssignDoubleToString(double f, asCScriptString &dest)
{
	ostringstream stream;
	stream << f;
	dest.buffer += stream.str(); 
	return dest;
}

//----------------
// string + value
//----------------

static asCScriptString *AddStringBits(const asCScriptString &str, asDWORD i)
{
	ostringstream stream;
	stream << hex << i;
	return new asCScriptString(str.buffer + stream.str()); 
}

static asCScriptString *AddStringUInt(const asCScriptString &str, unsigned int i)
{
	ostringstream stream;
	stream << i;
	return new asCScriptString(str.buffer + stream.str());
}

static asCScriptString *AddStringInt(const asCScriptString &str, int i)
{
	ostringstream stream;
	stream << i;
	return new asCScriptString(str.buffer + stream.str());
}

static asCScriptString *AddStringFloat(const asCScriptString &str, float f)
{
	ostringstream stream;
	stream << f;
	return new asCScriptString(str.buffer + stream.str());
}

static asCScriptString *AddStringDouble(const asCScriptString &str, double f)
{
	ostringstream stream;
	stream << f;
	return new asCScriptString(str.buffer + stream.str());
}

//----------------
// value + string
//----------------

static asCScriptString *AddBitsString(asDWORD i, const asCScriptString &str)
{
	ostringstream stream;
	stream << hex << i;
	return new asCScriptString(stream.str() + str.buffer);
}

static asCScriptString *AddIntString(int i, const asCScriptString &str)
{
	ostringstream stream;
	stream << i;
	return new asCScriptString(stream.str() + str.buffer);
}

static asCScriptString *AddUIntString(unsigned int i, const asCScriptString &str)
{
	ostringstream stream;
	stream << i;
	return new asCScriptString(stream.str() + str.buffer);
}

static asCScriptString *AddFloatString(float f, const asCScriptString &str)
{
	ostringstream stream;
	stream << f;
	return new asCScriptString(stream.str() + str.buffer);
}

static asCScriptString *AddDoubleString(double f, const asCScriptString &str)
{
	ostringstream stream;
	stream << f;
	return new asCScriptString(stream.str() + str.buffer);
}

//----------
// string[]
//----------

static char *StringCharAt(unsigned int i, asCScriptString &str)
{
	if( i >= str.buffer.size() )
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException("Out of range");

		// Return a null pointer
		return 0;
	}

	return &str.buffer[i];
}

//-----------------------
// AngelScript functions
//-----------------------

// This function allocates memory for the string object
static void *StringAlloc(int)
{
	return new char[sizeof(asCScriptString)];
}

// This function deallocates the memory for the string object
static void StringFree(void *p)
{
	assert( p );
	delete p;
}

// This is the string factory that creates new strings for the script
static asCScriptString *StringFactory(asUINT length, const char *s)
{
	// Return a script handle to a new string
	return new asCScriptString(s);
}

// This is a wrapper for the default asCScriptString constructor, since
// it is not possible to take the address of the constructor directly
static void ConstructString(asCScriptString *thisPointer)
{
	// Construct the string in the memory received
	new(thisPointer) asCScriptString();
}

// This is where we register the string type
void RegisterScriptString(asIScriptEngine *engine)
{
	int r;

	// Register the type
	r = engine->RegisterObjectType("string", sizeof(asCScriptString), asOBJ_CLASS_CDA); assert( r >= 0 );

	// Register the object operator overloads
	// Note: We don't have to register the destructor, since the object uses reference counting
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADDREF,     "void f()",                    asMETHOD(asCScriptString,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_RELEASE,    "void f()",                    asMETHOD(asCScriptString,Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(const string &in)", asMETHODPR(asCScriptString, operator =, (const asCScriptString&), asCScriptString&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(const string &in)", asMETHODPR(asCScriptString, operator+=, (const asCScriptString&), asCScriptString&), asCALL_THISCALL); assert( r >= 0 );

	// Register the memory allocator routines. This will make all memory allocations for the string 
	// object be made in one place, which is important if for example the script library is used from a dll
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ALLOC, "string &f(uint)", asFUNCTION(StringAlloc), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_FREE, "void f(string &in)", asFUNCTION(StringFree), asCALL_CDECL); assert( r >= 0 );

	// Register the factory to return a handle to a new string
	// Note: We must register the string factory after the basic behaviours, 
	// otherwise the library will not allow the use of object handles for this type
	r = engine->RegisterStringFactory("string@", asFUNCTION(StringFactory), asCALL_CDECL); assert( r >= 0 );

	// Register the global operator overloads
	// Note: We can use std::string's methods directly because the
	// internal std::string is placed at the beginning of the class
	r = engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL,       "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator==, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL,    "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator!=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_LEQUAL,      "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator<=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_GEQUAL,      "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator>=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_LESSTHAN,    "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator <, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_GREATERTHAN, "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator >, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, const string &in)", asFUNCTIONPR(operator +, (const asCScriptString &, const asCScriptString &), asCScriptString*), asCALL_CDECL); assert( r >= 0 );

	// Register the index operator, both as a mutator and as an inspector
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_INDEX, "uint8 &f(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_INDEX, "const uint8 &f(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Register the object methods
	r = engine->RegisterObjectMethod("string", "uint length() const", asMETHOD(string,size), asCALL_THISCALL); assert( r >= 0 );

	// Automatic conversion from values
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(double)", asFUNCTION(AssignDoubleToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(double)", asFUNCTION(AddAssignDoubleToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, double)", asFUNCTION(AddStringDouble), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(double, const string &in)", asFUNCTION(AddDoubleString), asCALL_CDECL); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(float)", asFUNCTION(AssignFloatToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(float)", asFUNCTION(AddAssignFloatToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, float)", asFUNCTION(AddStringFloat), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(float, const string &in)", asFUNCTION(AddFloatString), asCALL_CDECL); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(int)", asFUNCTION(AssignIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(int)", asFUNCTION(AddAssignIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, int)", asFUNCTION(AddStringInt), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(int, const string &in)", asFUNCTION(AddIntString), asCALL_CDECL); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(uint)", asFUNCTION(AssignUIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(uint)", asFUNCTION(AddAssignUIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, uint)", asFUNCTION(AddStringUInt), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(uint, const string &in)", asFUNCTION(AddUIntString), asCALL_CDECL); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(bits)", asFUNCTION(AssignBitsToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(bits)", asFUNCTION(AddAssignBitsToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, bits)", asFUNCTION(AddStringBits), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(bits, const string &in)", asFUNCTION(AddBitsString), asCALL_CDECL); assert( r >= 0 );
}

END_AS_NAMESPACE


