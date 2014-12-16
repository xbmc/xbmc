/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

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

   Andreas Jonsson
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

	// _builtin_array_<T> represents the default array
	dt.objectType       = engine->defaultArrayObjectType;
	dt.tokenType        = ttIdentifier;

	return dt;
}

asCDataType asCDataType::CreateNullHandle()
{
	asCDataType dt;

	dt.tokenType = ttUnrecognizedToken;
	dt.isReadOnly = true;
	dt.isObjectHandle = true;
	dt.isConstHandle = true;

	return dt;
}

bool asCDataType::IsNullHandle() const
{
	if( tokenType == ttUnrecognizedToken &&
		objectType == 0 &&
		isObjectHandle  )
		return true;

	return false;
}

asCString asCDataType::Format() const
{
	if( IsNullHandle() )
		return "<null handle>";

	asCString str;

	if( isReadOnly )
		str = "const ";

	if( tokenType != ttIdentifier )
	{
		str += asGetTokenDefinition(tokenType);
	}
	else if( IsArrayType() )
	{
		str += objectType->templateSubType.Format();
		str += "[]";
	}
	else if( objectType )
	{
		str += objectType->name;
		if( objectType->flags & asOBJ_TEMPLATE )
		{
			str += "<";
			str += objectType->templateSubType.Format();
			str += ">";
		}
	}
	else
	{
		str = "<unknown>";
	}

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

int asCDataType::MakeHandle(bool b, bool acceptHandleForScope)
{
	if( !b )
	{
		isObjectHandle = b;
		isConstHandle = false;
	}
	else if( b && !isObjectHandle )
	{
		// Only reference types are allowed to be handles, 
		// but not nohandle reference types, and not scoped references (except when returned from registered function)
		if( !objectType || 
			!((objectType->flags & asOBJ_REF) || (objectType->flags & asOBJ_TEMPLATE_SUBTYPE)) || 
			(objectType->flags & asOBJ_NOHANDLE) || 
			((objectType->flags & asOBJ_SCOPED) && !acceptHandleForScope) )
			return -1;

		isObjectHandle = b;
		isConstHandle = false;
	}

	return 0;
}

int asCDataType::MakeArray(asCScriptEngine *engine)
{
	bool tmpIsReadOnly = isReadOnly;
	isReadOnly = false;
	asCObjectType *at = engine->GetTemplateInstanceType(engine->defaultArrayObjectType, *this);
	isReadOnly = tmpIsReadOnly;

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

bool asCDataType::SupportHandles() const
{
	if( objectType &&
		(objectType->flags & asOBJ_REF) && 
		!(objectType->flags & asOBJ_NOHANDLE) &&
		!isObjectHandle )
		return true;

	return false;
}

bool asCDataType::CanBeInstanciated() const
{
	if( GetSizeOnStackDWords() == 0 ||
		(IsObject() && 
		 (objectType->flags & asOBJ_REF) &&        // It's a ref type and
		 ((objectType->flags & asOBJ_NOHANDLE) ||  // the ref type doesn't support handles or
		  (!IsObjectHandle() &&                    // it's not a handle and
		   objectType->beh.factories.GetLength() == 0))) ) // the ref type cannot be instanciated
		return false;

	return true;
}

bool asCDataType::CanBeCopied() const
{
	// All primitives can be copied
	if( IsPrimitive() ) return true;

	// Plain-old-data structures can always be copied
	if( objectType->flags & asOBJ_POD ) return true;

	// It must be possible to instanciate the type
	if( !CanBeInstanciated() ) return false;

	// It must have a default constructor or factory
	if( objectType->beh.construct == 0 &&
		objectType->beh.factory   == 0 ) return false;

	// It must be possible to copy the type
	if( objectType->beh.copy == 0 ) return false;

	return true;
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
	return objectType ? (objectType->name == objectType->engine->defaultArrayObjectType->name) : false;
}

bool asCDataType::IsTemplate() const
{
	if( objectType && (objectType->flags & asOBJ_TEMPLATE) )
		return true;

	return false;
}

bool asCDataType::IsScriptObject() const
{
	if( objectType && (objectType->flags & asOBJ_SCRIPT_OBJECT) )
		return true;

	return false;
}

asCDataType asCDataType::GetSubType() const
{
	asASSERT(objectType);
	return objectType->templateSubType;
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

bool asCDataType::IsEqualExceptInterfaceType(const asCDataType &dt) const
{
	if( tokenType != dt.tokenType )           return false;
	if( isReference != dt.isReference )       return false;
	if( isObjectHandle != dt.isObjectHandle ) return false;
	if( isReadOnly != dt.isReadOnly )         return false;
	if( isConstHandle != dt.isConstHandle )   return false;

	if( objectType != dt.objectType )
	{
		if( !objectType || !dt.objectType ) return false;
		if( !objectType->IsInterface() || !dt.objectType->IsInterface() ) return false;
	}

	return true;
}

bool asCDataType::IsPrimitive() const
{
	//	Enumerations are primitives
	if( IsEnumType() )
		return true;

	// A registered object is never a primitive neither is a pointer, nor an array
	if( objectType )
		return false;

	// Null handle doesn't have an objectType, but it is not a primitive
	if( tokenType == ttUnrecognizedToken )
		return false;

	return true;
}

bool asCDataType::IsSamePrimitiveBaseType(const asCDataType &dt) const
{
	if( !IsPrimitive() || !dt.IsPrimitive() ) return false;
	
	if( IsIntegerType()  && dt.IsIntegerType()  ) return true;
	if( IsUnsignedType() && dt.IsUnsignedType() ) return true;
	if( IsFloatType()    && dt.IsFloatType()    ) return true;
	if( IsDoubleType()   && dt.IsDoubleType()   ) return true;
	if( IsBooleanType()  && dt.IsBooleanType()  ) return true;
	if( IsFloatType()    && dt.IsDoubleType()   ) return true;
	if( IsDoubleType()   && dt.IsFloatType()    ) return true;

	return false;
}

bool asCDataType::IsIntegerType() const
{
	if( tokenType == ttInt ||
		tokenType == ttInt8 ||
		tokenType == ttInt16 ||
		tokenType == ttInt64 )
		return true;

	return false;
}

bool asCDataType::IsUnsignedType() const
{
	if( tokenType == ttUInt ||
		tokenType == ttUInt8 ||
		tokenType == ttUInt16 ||
		tokenType == ttUInt64 )
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

bool asCDataType::IsBooleanType() const
{
	if( tokenType == ttBool )
		return true;

	return false;
}

bool asCDataType::IsObject() const
{
	//	Enumerations are not objects, even though they are described with an objectType.
	if( IsEnumType() )
		return false;

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
		tokenType == ttUInt8 )
		return 1;

	if( tokenType == ttInt16 ||
		tokenType == ttUInt16 )
		return 2;

	if( tokenType == ttDouble ||
		tokenType == ttInt64 ||
		tokenType == ttUInt64 )
		return 8;

	if( tokenType == ttBool )
		return AS_SIZEOF_BOOL;

	// null handle
	if( tokenType == ttUnrecognizedToken )
		return 4*AS_PTR_SIZE;

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
	int size = tokenType == ttQuestion ? 1 : 0;

	if( isReference ) return AS_PTR_SIZE + size;
	if( objectType ) return AS_PTR_SIZE + size;

	return GetSizeInMemoryDWords() + size;
}

asSTypeBehaviour *asCDataType::GetBehaviour() const
{ 
	return objectType ? &objectType->beh : 0; 
}

bool asCDataType::IsEnumType() const
{
	if( objectType && (objectType->flags & asOBJ_ENUM) )
		return true;

	return false;
}

END_AS_NAMESPACE

