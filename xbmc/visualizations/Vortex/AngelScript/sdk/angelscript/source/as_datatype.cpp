/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/


//
// as_datatype.cpp
//
// This class describes the datatype for expressions during compilation
//

#include "as_config.h"
#include "as_datatype.h"
#include "as_tokendef.h"
#include "as_objecttype.h"
#include "as_scriptengine.h"
#include "as_arrayobject.h"
#include "as_tokenizer.h"

BEGIN_AS_NAMESPACE

asCDataType::asCDataType()
{
	tokenType      = ttUnrecognizedToken;
	objectType     = 0;
	isReference    = false;
	isReadOnly     = false;
	isObjectHandle = false;
	isConstHandle  = false;
}

asCDataType::asCDataType(const asCDataType &dt)
{
	tokenType      = dt.tokenType;
	objectType     = dt.objectType;
	isReference    = dt.isReference;
	isReadOnly     = dt.isReadOnly;
	isObjectHandle = dt.isObjectHandle;
	isConstHandle  = dt.isConstHandle;
}

asCDataType::~asCDataType()
{
}

asCDataType asCDataType::CreateObject(asCObjectType *ot, bool isConst)
{
	asCDataType dt;

	dt.tokenType        = ttIdentifier;
	dt.objectType       = ot;
	dt.isReadOnly       = isConst;

	return dt;
}

asCDataType asCDataType::CreateObjectHandle(asCObjectType *ot, bool isConst)
{
	asCDataType dt;

	dt.tokenType        = ttIdentifier;
	dt.objectType       = ot;
	dt.isObjectHandle   = true;
	dt.isConstHandle    = isConst;

	return dt;
}

asCDataType asCDataType::CreatePrimitive(eTokenType tt, bool isConst)
{
	asCDataType dt;

	dt.tokenType        = tt;
	dt.isReadOnly       = isConst;

	return dt;
}

asCDataType asCDataType::CreateDefaultArray(asCScriptEngine *engine)
{
	asCDataType dt;

	// void[] represents the default array
	dt.objectType       = engine->defaultArrayObjectType;
	dt.tokenType        = ttVoid;

	return dt;
}

asCDataType asCDataType::CreateNullHandle()
{
	asCDataType dt;

	dt.tokenType = ttInt;
	dt.isReadOnly = true;
	dt.isObjectHandle = true;
	dt.isConstHandle = true;

	return dt;
}

asCString asCDataType::Format() const
{
	asCString str;

	if( isReadOnly )
		str = "const ";

	if( tokenType != ttIdentifier )
		str += asGetTokenDefinition(tokenType);
	else
	{
		// find the baseType by following the objectType
		asCObjectType *baseType = objectType;
		if( baseType ) 
			while( baseType->subType ) 
				baseType = baseType->subType;
		if( baseType == 0 )
			str += "<unknown>";
		else
		{
			if( baseType->tokenType == ttIdentifier )
				str += baseType->name;
			else
				str += asGetTokenDefinition(baseType->tokenType);
		}
	}


	asCString append;
	int at = objectType ? objectType->arrayType : 0;
	while( at )
	{
		if( (at & 3) == 3 ) 
			append = "@[]" + append;
		else 
			append = "[]" + append;
		at >>= 2;
	}
	str += append;

	if( isObjectHandle )
	{
		str += "@";
		if( isConstHandle )
			str += "const";
	}

    if( isReference )
		str += "&";

	return str;
}


asCDataType &asCDataType::operator =(const asCDataType &dt)
{
	tokenType        = dt.tokenType;
	isReference      = dt.isReference;
	objectType       = dt.objectType;
	isReadOnly       = dt.isReadOnly;
	isObjectHandle   = dt.isObjectHandle;
	isConstHandle    = dt.isConstHandle;

	return (asCDataType &)*this;
}

int asCDataType::MakeHandle(bool b)
{
	if( !objectType || objectType->beh.addref == 0 || objectType->beh.release == 0 )
		return -1;

	if( !b || (b && !isObjectHandle) )
	{
		isObjectHandle = b;
		isConstHandle = false;
	}

	return 0;
}

int asCDataType::MakeArray(asCScriptEngine *engine)
{
	asCObjectType *at = engine->GetArrayTypeFromSubType(*this);

	isObjectHandle = false;
	isConstHandle = false;
	
	objectType = at;
	tokenType = ttIdentifier;

	return 0;
}

int asCDataType::MakeReference(bool b)
{
	isReference = b;

	return 0;
}

int asCDataType::MakeReadOnly(bool b)
{
	if( isObjectHandle )
	{
		isConstHandle = b;
		return 0;
	}

	isReadOnly = b;
	return 0;
}

int asCDataType::MakeHandleToConst(bool b)
{
	if( !isObjectHandle ) return -1;

	isReadOnly = b;
	return 0;
}

bool asCDataType::IsReadOnly() const
{
	if( isObjectHandle )
		return isConstHandle;

	return isReadOnly;
}

bool asCDataType::IsHandleToConst() const
{
	if( !isObjectHandle ) return false;
	return isReadOnly;
}

bool asCDataType::IsArrayType() const
{
	return objectType ? (objectType->arrayType != 0) : false;
}

bool asCDataType::IsScriptArray() const
{
	if( objectType && (objectType->flags & asOBJ_SCRIPT_ARRAY) )
		return true;

	return false;
}

bool asCDataType::IsScriptStruct() const
{
	if( objectType && (objectType->flags & asOBJ_SCRIPT_STRUCT) )
		return true;

	return false;
}

bool asCDataType::IsScriptAny() const
{
	if( objectType && (objectType->flags & asOBJ_SCRIPT_ANY) )
		return true;

	return false;
}

asCDataType asCDataType::GetSubType() const
{
	asCDataType dt(*this);

	int arrayType = GetArrayType();

	dt.isReadOnly = false;
	dt.isConstHandle = false;
	dt.isReference = false;
	if( objectType )
	{
		dt.objectType = objectType->subType;
		if( dt.objectType == 0 )
			dt.tokenType = objectType->tokenType;
	}
	else
		dt.objectType = 0;

	dt.MakeHandle((arrayType & 1) ? true : false);
	
	if( IsReadOnly() )
		dt.MakeReadOnly(false);

	return dt;
}


bool asCDataType::operator !=(const asCDataType &dt) const
{
	return !(*this == dt);
}

bool asCDataType::operator ==(const asCDataType &dt) const
{
	if( !IsEqualExceptRefAndConst(dt) ) return false;
	if( isReference != dt.isReference ) return false;
	if( isReadOnly != dt.isReadOnly ) return false;
	if( isConstHandle != dt.isConstHandle ) return false;

	return true;
}

bool asCDataType::IsEqualExceptRef(const asCDataType &dt) const
{
	if( !IsEqualExceptRefAndConst(dt) ) return false;
	if( isReadOnly != dt.isReadOnly ) return false;
	if( isConstHandle != dt.isConstHandle ) return false;

	return true;
}

bool asCDataType::IsEqualExceptRefAndConst(const asCDataType &dt) const
{
	// Check base type
	if( tokenType != dt.tokenType ) return false;
	if( objectType != dt.objectType ) return false;
	if( isObjectHandle != dt.isObjectHandle ) return false;
	if( isObjectHandle )
		if( isReadOnly != dt.isReadOnly ) return false;

	return true;
}

bool asCDataType::IsEqualExceptConst(const asCDataType &dt) const
{
	if( !IsEqualExceptRefAndConst(dt) ) return false;
	if( isReference != dt.isReference ) return false;

	return true;
}

bool asCDataType::IsPrimitive() const
{
	// A registered object is never a primitive neither is a pointer, nor an array
	if( objectType )
		return false;

	return true;
}

bool asCDataType::IsSamePrimitiveBaseType(const asCDataType &dt) const
{
	if( !IsPrimitive() || !dt.IsPrimitive() ) return false;
	
	if( IsIntegerType() && dt.IsIntegerType() ) return true;
	if( IsUnsignedType() && dt.IsUnsignedType() ) return true;
	if( IsFloatType() && dt.IsFloatType() ) return true;
	if( IsBitVectorType() && dt.IsBitVectorType() ) return true;
	if( IsDoubleType() && dt.IsDoubleType() ) return true;
	if( IsBooleanType() && dt.IsBooleanType() ) return true;

	return false;
}

bool asCDataType::IsIntegerType() const
{
	if( tokenType == ttInt ||
		tokenType == ttInt8 ||
		tokenType == ttInt16 )
		return true;

	return false;
}

bool asCDataType::IsUnsignedType() const
{
	if( tokenType == ttUInt ||
		tokenType == ttUInt8 ||
		tokenType == ttUInt16 )
		return true;

	return false;
}

bool asCDataType::IsFloatType() const
{
	if( tokenType == ttFloat )
		return true;

	return false;
}

bool asCDataType::IsDoubleType() const
{
	if( tokenType == ttDouble )
		return true;

	return false;
}

bool asCDataType::IsBitVectorType() const
{
	if( tokenType == ttBits ||
		tokenType == ttBits8 ||
		tokenType == ttBits16 )
		return true;

	return false;
}

bool asCDataType::IsBooleanType() const
{
	if( tokenType == ttBool )
		return true;

	return false;
}

bool asCDataType::IsObject() const
{
	if( objectType ) return true;

	return false;
}

int asCDataType::GetSizeInMemoryBytes() const
{
	if( objectType != 0 )
		return objectType->size;

	if( tokenType == ttVoid )
		return 0;

	if( tokenType == ttInt8 ||
		tokenType == ttUInt8 ||
		tokenType == ttBits8 ||
		tokenType == ttBool )
		return 1;

	if( tokenType == ttInt16 ||
		tokenType == ttUInt16 ||
		tokenType == ttBits16 )
		return 2;

	if( tokenType == ttDouble )
		return 8;

	return 4;
}

int asCDataType::GetSizeInMemoryDWords() const
{
	int s = GetSizeInMemoryBytes();
	if( s == 0 ) return 0;
	if( s <= 4 ) return 1;
	
	return s/4;
}

int asCDataType::GetSizeOnStackDWords() const
{
	if( isReference ) return 1;

	// Objects are stored with a pointer to dynamic memory
	if( objectType ) return 1;

	return GetSizeInMemoryDWords();
}

int asCDataType::GetArrayType() const
{
	return objectType ? objectType->arrayType : 0;
}

asSTypeBehaviour *asCDataType::GetBehaviour()
{ 
	return objectType ? &objectType->beh : 0; 
}

END_AS_NAMESPACE

