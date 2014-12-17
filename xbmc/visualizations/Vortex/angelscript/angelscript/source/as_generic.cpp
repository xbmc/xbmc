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
// as_generic.cpp
//
// This class handles the call to a function registered with asCALL_GENERIC
//

#include "as_generic.h"
#include "as_scriptfunction.h"
#include "as_objecttype.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

// TODO: optimize: The access to the arguments should be optimized so that code 
//                 doesn't have to count the position of the argument with every call

// internal
asCGeneric::asCGeneric(asCScriptEngine *engine, asCScriptFunction *sysFunction, void *currentObject, asDWORD *stackPointer)
{
	this->engine = engine;
	this->sysFunction = sysFunction;
	this->currentObject = currentObject;
	this->stackPointer = stackPointer;
	
	objectRegister = 0;
	returnVal = 0;
}

// internal
asCGeneric::~asCGeneric()
{
}

// interface
asIScriptEngine *asCGeneric::GetEngine()
{
	return (asIScriptEngine*)engine;
}

// interface
int asCGeneric::GetFunctionId()
{
	return sysFunction->id;
}

// interface
void *asCGeneric::GetObject()
{
	return currentObject;
}

// interface
int asCGeneric::GetObjectTypeId()
{
	asCDataType dt = asCDataType::CreateObject(sysFunction->objectType, false);
	return engine->GetTypeIdFromDataType(dt);
}

// interface
int asCGeneric::GetArgCount()
{
	return (int)sysFunction->parameterTypes.GetLength();
}

// interface
asBYTE asCGeneric::GetArgByte(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
		return 0;

	if( dt->GetSizeInMemoryBytes() != 1 )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return *(asBYTE*)&stackPointer[offset];
}

// interface
asWORD asCGeneric::GetArgWord(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
		return 0;

	if( dt->GetSizeInMemoryBytes() != 2 )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return *(asWORD*)&stackPointer[offset];
}

// interface
asDWORD asCGeneric::GetArgDWord(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
		return 0;

	if( dt->GetSizeInMemoryBytes() != 4 )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return *(asDWORD*)&stackPointer[offset];
}

// interface
asQWORD asCGeneric::GetArgQWord(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
		return 0;

	if( dt->GetSizeInMemoryBytes() != 8 )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return *(asQWORD*)(&stackPointer[offset]);
}

// interface
float asCGeneric::GetArgFloat(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
		return 0;

	if( dt->GetSizeInMemoryBytes() != 4 )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return *(float*)(&stackPointer[offset]);
}

// interface
double asCGeneric::GetArgDouble(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
		return 0;

	if( dt->GetSizeInMemoryBytes() != 8 )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return *(double*)(&stackPointer[offset]);
}

// interface
void *asCGeneric::GetArgAddress(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( !dt->IsReference() && !dt->IsObjectHandle() )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return (void*)*(size_t*)(&stackPointer[offset]);
}

// interface
void *asCGeneric::GetArgObject(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Verify that the type is correct
	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( !dt->IsObject() )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Get the value
	return *(void**)(&stackPointer[offset]);
}

// interface
void *asCGeneric::GetAddressOfArg(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

	// For object variables it's necessary to dereference the pointer to get the address of the value
	if( !sysFunction->parameterTypes[arg].IsReference() && 
		sysFunction->parameterTypes[arg].IsObject() && 
		!sysFunction->parameterTypes[arg].IsObjectHandle() )
		return *(void**)&stackPointer[offset];

	// Get the address of the value
	return &stackPointer[offset];
}

// interface
int asCGeneric::GetArgTypeId(asUINT arg)
{
	if( arg >= (unsigned)sysFunction->parameterTypes.GetLength() )
		return 0;

	asCDataType *dt = &sysFunction->parameterTypes[arg];
	if( dt->GetTokenType() != ttQuestion )
		return engine->GetTypeIdFromDataType(*dt);
	else
	{
		int offset = 0;
		for( asUINT n = 0; n < arg; n++ )
			offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();

		// Skip the actual value to get to the type id
		offset += AS_PTR_SIZE;

		// Get the value
		return stackPointer[offset];
	}
}

// interface
int asCGeneric::SetReturnByte(asBYTE val)
{
	// Verify the type of the return value
	if( sysFunction->returnType.IsObject() || sysFunction->returnType.IsReference() )
		return asINVALID_TYPE;

	if( sysFunction->returnType.GetSizeInMemoryBytes() != 1 )
		return asINVALID_TYPE;

    // Store the value
    *(asBYTE*)&returnVal = val;

	return 0;
}

// interface
int asCGeneric::SetReturnWord(asWORD val)
{
	// Verify the type of the return value
	if( sysFunction->returnType.IsObject() || sysFunction->returnType.IsReference() )
		return asINVALID_TYPE;

	if( sysFunction->returnType.GetSizeInMemoryBytes() != 2 )
		return asINVALID_TYPE;

    // Store the value
    *(asWORD*)&returnVal = val;

	return 0;
}

// interface
int asCGeneric::SetReturnDWord(asDWORD val)
{
	// Verify the type of the return value
	if( sysFunction->returnType.IsObject() || sysFunction->returnType.IsReference() )
		return asINVALID_TYPE;

	if( sysFunction->returnType.GetSizeInMemoryBytes() != 4 )
		return asINVALID_TYPE;

    // Store the value
    *(asDWORD*)&returnVal = val;

	return 0;
}

// interface
int asCGeneric::SetReturnQWord(asQWORD val)
{
	// Verify the type of the return value
	if( sysFunction->returnType.IsObject() || sysFunction->returnType.IsReference() )
		return asINVALID_TYPE;

	if( sysFunction->returnType.GetSizeOnStackDWords() != 2 )
		return asINVALID_TYPE;

	// Store the value
	returnVal = val;

	return 0;
}

// interface
int asCGeneric::SetReturnFloat(float val)
{
	// Verify the type of the return value
	if( sysFunction->returnType.IsObject() || sysFunction->returnType.IsReference() )
		return asINVALID_TYPE;

	if( sysFunction->returnType.GetSizeOnStackDWords() != 1 )
		return asINVALID_TYPE;

	// Store the value
	*(float*)&returnVal = val;

	return 0;
}

// interface
int asCGeneric::SetReturnDouble(double val)
{
	// Verify the type of the return value
	if( sysFunction->returnType.IsObject() || sysFunction->returnType.IsReference() )
		return asINVALID_TYPE;

	if( sysFunction->returnType.GetSizeOnStackDWords() != 2 )
		return asINVALID_TYPE;

	// Store the value
	*(double*)&returnVal = val;

	return 0;
}

// interface
int asCGeneric::SetReturnAddress(void *val)
{
	// Verify the type of the return value
	if( sysFunction->returnType.IsReference() )
	{
		// Store the value
		*(void**)&returnVal = val;
		return 0;
	}
	else if( sysFunction->returnType.IsObjectHandle() )
	{
		// Store the handle without increasing reference
		objectRegister = val;
		return 0;
	}

	return asINVALID_TYPE;
}

// interface
int asCGeneric::SetReturnObject(void *obj)
{
	asCDataType *dt = &sysFunction->returnType;
	if( !dt->IsObject() )
		return asINVALID_TYPE;

	if( dt->IsReference() )
	{
		*(void**)&returnVal = obj;
		return 0;
	}

	if( dt->IsObjectHandle() )
	{
		// Increase the reference counter
		asSTypeBehaviour *beh = &dt->GetObjectType()->beh;
		if( obj && beh->addref )
			engine->CallObjectMethod(obj, beh->addref);
	}
	else
	{
		obj = engine->CreateScriptObjectCopy(obj, engine->GetTypeIdFromDataType(*dt));
	}

	objectRegister = obj;

	return 0;
}

// internal
void *asCGeneric::GetReturnPointer()
{
	asCDataType &dt = sysFunction->returnType;

	if( dt.IsObject() && !dt.IsReference() )
		return &objectRegister;

	return &returnVal;
}

// interface
void *asCGeneric::GetAddressOfReturnLocation()
{
	asCDataType &dt = sysFunction->returnType;

	if( dt.IsObject() && !dt.IsReference() )
	{
		if( dt.GetObjectType()->flags & asOBJ_VALUE )
		{
			// Allocate the necessary memory for this object, 
			// but do not initialize it, as the caller will do that.
			objectRegister = engine->CallAlloc(dt.GetObjectType());

			// TODO: How will we know if the initialization was successful?

			return objectRegister;
		}

		// Reference types store the handle in the objectReference
		return &objectRegister;
	}

	// Primitive types and references are stored in the returnVal property
	return &returnVal;
}

// interface
int asCGeneric::GetReturnTypeId()
{
	return sysFunction->GetReturnTypeId();
}

END_AS_NAMESPACE
