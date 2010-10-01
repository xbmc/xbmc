/*
 *      Copyright (C) 2010 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <assert.h>
//#include <sstream>
#include "VortexString.h"
#include <string.h>
#include <New.h>
//using namespace std;

//BEGIN_AS_NAMESPACE

//--------------
// constructors
//--------------

ScriptString_c::ScriptString_c()
{
	// Count the first reference
	refCount = 1;
	myString = NULL;
}

ScriptString_c::ScriptString_c(const char *s)
{
	refCount = 1;
	stringLength = strlen(s);
	myString = new char[stringLength+1];
	strcpy(myString, s);
	//	buffer = s;
}

//ScriptString_c::ScriptString_c(const string &s)
//{
//	refCount = 1;
//	buffer = s;
//}

//ScriptString_c::ScriptString_c(const ScriptString_c &s)
//{
//	refCount = 1;
//	buffer = s.buffer;
//}

ScriptString_c::~ScriptString_c()
{
	delete[] myString;
	assert( refCount == 0 );
}

//--------------------
// reference counting
//--------------------

void ScriptString_c::AddRef()
{
	refCount++;
}

void ScriptString_c::Release()
{
	if( --refCount == 0 )
		delete this;
}

//-----------------
// string = string
//-----------------

//ScriptString_c &ScriptString_c::operator=(const ScriptString_c &other)
//{
//	// Copy only the buffer, not the reference counter
//	buffer = other.buffer;
//
//	// Return a reference to this object
//	return *this;
//}

//------------------
// string += string
//------------------

//ScriptString_c &ScriptString_c::operator+=(const ScriptString_c &other)
//{
//	buffer += other.buffer;
//	return *this;
//}

//-----------------
// string + string
//-----------------

//ScriptString_c *operator+(const ScriptString_c &a, const ScriptString_c &b)
//{
//	// Return a new object as a script handle
//	return new ScriptString_c(a.buffer + b.buffer);
//}

//----------------
// string = value
//----------------

//static ScriptString_c &AssignBitsToString(asDWORD i, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << hex << i;
//	dest.buffer = stream.str(); 
//
//	// Return a reference to the object
//	return dest;
//}

//static ScriptString_c &AssignUIntToString(unsigned int i, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << i;
//	dest.buffer = stream.str(); 
//	return dest;
//}

//static ScriptString_c &AssignIntToString(int i, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << i;
//	dest.buffer = stream.str(); 
//	return dest;
//}
//
//static ScriptString_c &AssignFloatToString(float f, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << f;
//	dest.buffer = stream.str(); 
//	return dest;
//}
//
//static ScriptString_c &AssignDoubleToString(double f, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << f;
//	dest.buffer = stream.str(); 
//	return dest;
//}
//
////-----------------
//// string += value
////-----------------
//
//static ScriptString_c &AddAssignBitsToString(asDWORD i, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << hex << i;
//	dest.buffer += stream.str(); 
//	return dest;
//}
//
//static ScriptString_c &AddAssignUIntToString(unsigned int i, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << i;
//	dest.buffer += stream.str(); 
//	return dest;
//}
//
//static ScriptString_c &AddAssignIntToString(int i, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << i;
//	dest.buffer += stream.str(); 
//	return dest;
//}
//
//static ScriptString_c &AddAssignFloatToString(float f, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << f;
//	dest.buffer += stream.str(); 
//	return dest;
//}
//
//static ScriptString_c &AddAssignDoubleToString(double f, ScriptString_c &dest)
//{
//	ostringstream stream;
//	stream << f;
//	dest.buffer += stream.str(); 
//	return dest;
//}
//
////----------------
//// string + value
////----------------
//
//static ScriptString_c *AddStringBits(const ScriptString_c &str, asDWORD i)
//{
//	ostringstream stream;
//	stream << hex << i;
//	return new ScriptString_c(str.buffer + stream.str()); 
//}
//
//static ScriptString_c *AddStringUInt(const ScriptString_c &str, unsigned int i)
//{
//	ostringstream stream;
//	stream << i;
//	return new ScriptString_c(str.buffer + stream.str());
//}
//
//static ScriptString_c *AddStringInt(const ScriptString_c &str, int i)
//{
//	ostringstream stream;
//	stream << i;
//	return new ScriptString_c(str.buffer + stream.str());
//}
//
//static ScriptString_c *AddStringFloat(const ScriptString_c &str, float f)
//{
//	ostringstream stream;
//	stream << f;
//	return new ScriptString_c(str.buffer + stream.str());
//}
//
//static ScriptString_c *AddStringDouble(const ScriptString_c &str, double f)
//{
//	ostringstream stream;
//	stream << f;
//	return new ScriptString_c(str.buffer + stream.str());
//}
//
////----------------
//// value + string
////----------------
//
//static ScriptString_c *AddBitsString(asDWORD i, const ScriptString_c &str)
//{
//	ostringstream stream;
//	stream << hex << i;
//	return new ScriptString_c(stream.str() + str.buffer);
//}
//
//static ScriptString_c *AddIntString(int i, const ScriptString_c &str)
//{
//	ostringstream stream;
//	stream << i;
//	return new ScriptString_c(stream.str() + str.buffer);
//}
//
//static ScriptString_c *AddUIntString(unsigned int i, const ScriptString_c &str)
//{
//	ostringstream stream;
//	stream << i;
//	return new ScriptString_c(stream.str() + str.buffer);
//}
//
//static ScriptString_c *AddFloatString(float f, const ScriptString_c &str)
//{
//	ostringstream stream;
//	stream << f;
//	return new ScriptString_c(stream.str() + str.buffer);
//}
//
//static ScriptString_c *AddDoubleString(double f, const ScriptString_c &str)
//{
//	ostringstream stream;
//	stream << f;
//	return new ScriptString_c(stream.str() + str.buffer);
//}
//
////----------
//// string[]
////----------
//
//static char *StringCharAt(unsigned int i, ScriptString_c &str)
//{
//	if( i >= str.buffer.size() )
//	{
//		// Set a script exception
//		asIScriptContext *ctx = asGetActiveContext();
//		ctx->SetException("Out of range");
//
//		// Return a null pointer
//		return 0;
//	}
//
//	return &str.buffer[i];
//}

//-----------------------
// AngelScript functions
//-----------------------

// This function allocates memory for the string object
static void *StringAlloc(int)
{
	return new char[sizeof(ScriptString_c)];
}

// This function deallocates the memory for the string object
static void StringFree(void *p)
{
	assert( p );
	delete p;
}

// This is the string factory that creates new strings for the script
static ScriptString_c *StringFactory(asUINT length, const char *s)
{
	// Return a script handle to a new string
	return new ScriptString_c(s);
}

// This is a wrapper for the default ScriptString_c constructor, since
// it is not possible to take the address of the constructor directly
static void ConstructString(ScriptString_c *thisPointer)
{
	// Construct the string in the memory received
	new(thisPointer) ScriptString_c();
}

// This is where we register the string type
void RegisterVortexScriptString(asIScriptEngine *engine)
{
	int r;

	// Register the type
	r = engine->RegisterObjectType("string", sizeof(ScriptString_c), asOBJ_CLASS_CDA); assert( r >= 0 );

	// Register the object operator overloads
	// Note: We don't have to register the destructor, since the object uses reference counting
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADDREF,     "void f()",                    asMETHOD(ScriptString_c,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_RELEASE,    "void f()",                    asMETHOD(ScriptString_c,Release), asCALL_THISCALL); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(const string &in)", asMETHODPR(ScriptString_c, operator =, (const ScriptString_c&), ScriptString_c&), asCALL_THISCALL); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(const string &in)", asMETHODPR(ScriptString_c, operator+=, (const ScriptString_c&), ScriptString_c&), asCALL_THISCALL); assert( r >= 0 );

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
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL,       "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator==, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL,    "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator!=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_LEQUAL,      "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator<=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_GEQUAL,      "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator>=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_LESSTHAN,    "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator <, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_GREATERTHAN, "bool f(const string &in, const string &in)",    asFUNCTIONPR(operator >, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, const string &in)", asFUNCTIONPR(operator +, (const ScriptString_c &, const ScriptString_c &), ScriptString_c*), asCALL_CDECL); assert( r >= 0 );

	// Register the index operator, both as a mutator and as an inspector
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_INDEX, "uint8 &f(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_INDEX, "const uint8 &f(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Register the object methods
	//	r = engine->RegisterObjectMethod("string", "uint length() const", asMETHOD(string,size), asCALL_THISCALL); assert( r >= 0 );

	// Automatic conversion from values
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(double)", asFUNCTION(AssignDoubleToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(double)", asFUNCTION(AddAssignDoubleToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, double)", asFUNCTION(AddStringDouble), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(double, const string &in)", asFUNCTION(AddDoubleString), asCALL_CDECL); assert( r >= 0 );

	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(float)", asFUNCTION(AssignFloatToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(float)", asFUNCTION(AddAssignFloatToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, float)", asFUNCTION(AddStringFloat), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(float, const string &in)", asFUNCTION(AddFloatString), asCALL_CDECL); assert( r >= 0 );

	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(int)", asFUNCTION(AssignIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(int)", asFUNCTION(AddAssignIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, int)", asFUNCTION(AddStringInt), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(int, const string &in)", asFUNCTION(AddIntString), asCALL_CDECL); assert( r >= 0 );

	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(uint)", asFUNCTION(AssignUIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(uint)", asFUNCTION(AddAssignUIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, uint)", asFUNCTION(AddStringUInt), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(uint, const string &in)", asFUNCTION(AddUIntString), asCALL_CDECL); assert( r >= 0 );
	//
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(bits)", asFUNCTION(AssignBitsToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(bits)", asFUNCTION(AddAssignBitsToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(const string &in, bits)", asFUNCTION(AddStringBits), asCALL_CDECL); assert( r >= 0 );
	//	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string@ f(bits, const string &in)", asFUNCTION(AddBitsString), asCALL_CDECL); assert( r >= 0 );
}

//END_AS_NAMESPACE


