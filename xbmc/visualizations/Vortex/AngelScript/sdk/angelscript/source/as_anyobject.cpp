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


#include <assert.h>
#include <new>

#include "as_config.h"
#include "as_anyobject.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

void AnyObjectConstructor(asCObjectType *ot, asCAnyObject *self)
{
	new(self) asCAnyObject(ot);
}

#ifndef AS_MAX_PORTABILITY

void AnyObjectConstructor2(void *ref, int refTypeId, asCObjectType *ot, asCAnyObject *self)
{
	new(self) asCAnyObject(ref,refTypeId,ot);
}

static asCAnyObject &AnyObjectAssignment(asCAnyObject *other, asCAnyObject *self)
{
	return *self = *other;
}

static void AnyObject_Store(void *ref, int refTypeId, asCAnyObject *self)
{
	self->Store(ref, refTypeId);
}

static void AnyObject_Retrieve(void *ref, int refTypeId, asCAnyObject *self)
{
	self->Retrieve(ref, refTypeId);
}

#else

static void AnyObjectConstructor_Generic(asIScriptGeneric *gen)
{
	asCObjectType *ot = (asCObjectType*)gen->GetArgDWord(0);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	new(self) asCAnyObject(ot);
}

static void AnyObjectConstructor2_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgDWord(0);
	int refType = gen->GetArgDWord(1);
	asCObjectType *ot = (asCObjectType*)gen->GetArgDWord(2);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	new(self) asCAnyObject(ref,refType,ot);
}

static void AnyObjectAssignment_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *other = (asCAnyObject*)gen->GetArgObject(0);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void AnyObject_Store_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgObject(0);
	int refTypeId = gen->GetArgDWord(1);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	self->Store(ref, refTypeId);
}

static void AnyObject_Retrieve_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgObject(0);
	int refTypeId = gen->GetArgDWord(1);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	self->Retrieve(ref, refTypeId);
}

#endif


void RegisterAnyObject(asCScriptEngine *engine)
{
	int r;
	r = engine->RegisterSpecialObjectType("any", sizeof(asCAnyObject), asOBJ_CLASS_CDA | asOBJ_SCRIPT_ANY | asOBJ_POTENTIAL_CIRCLE | asOBJ_CONTAINS_ANY); assert( r >= 0 );
#ifndef AS_MAX_PORTABILITY
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(AnyObjectConstructor, (asCObjectType*, asCAnyObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int&in, int, int)", asFUNCTIONPR(AnyObjectConstructor2, (void*, asCObjectType*, asCObjectType*, asCAnyObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ADDREF, "void f()", asFUNCTION(GCObject_AddRef), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_RELEASE, "void f()", asFUNCTION(GCObject_Release), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(AnyObjectAssignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod("any", "void store(int&in, int)", asFUNCTION(AnyObject_Store), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod("any", "void retrieve(int&out, int) const", asFUNCTION(AnyObject_Retrieve), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(AnyObjectConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int&in, int, int)", asFUNCTION(AnyObjectConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ADDREF, "void f()", asFUNCTION(GCObject_AddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_RELEASE, "void f()", asFUNCTION(GCObject_Release_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(AnyObjectAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod("any", "void store(int&in, int)", asFUNCTION(AnyObject_Store_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod("any", "void retrieve(int&out, int) const", asFUNCTION(AnyObject_Retrieve_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif 
}

int asCAnyObject::AddRef()
{
	return gc.AddRef();
}

int asCAnyObject::Release()
{
	return gc.Release();
}

asCAnyObject &asCAnyObject::operator=(asCAnyObject &other)
{
	FreeObject();

	// TODO: Need to know if it is a handle or not
	valueTypeId = other.valueTypeId;
	value = other.value;
	if( valueTypeId && value )
	{
		const asCDataType *valueType = gc.objType->engine->GetDataTypeFromTypeId(valueTypeId);

		asCObjectType *ot = valueType->GetObjectType();
		ot->engine->CallObjectMethod(value, ot->beh.addref);
		ot->refCount++;
	}

	return *this;
}

int asCAnyObject::CopyFrom(asIScriptAny *other)
{
	if( other == 0 ) return asINVALID_ARG;

	*this = *(asCAnyObject*)other;

	return 0;
}

asCAnyObject::asCAnyObject(asCObjectType *ot)
{
	gc.Init(ot);

	valueTypeId = 0;
	value = 0;
}

asCAnyObject::asCAnyObject(void *ref, int refTypeId, asCObjectType *ot)
{
	gc.Init(ot);

	valueTypeId = 0;
	value = 0;

	Store(ref, refTypeId);
}

asCAnyObject::~asCAnyObject()
{
	FreeObject();

	// The GCObject's destructor will be called after this
}

void asCAnyObject::Store(void *ref, int refTypeId)
{
	FreeObject();

	valueTypeId = refTypeId;

	const asCDataType *dt = gc.objType->engine->GetDataTypeFromTypeId(valueTypeId);

	value = *(void**)ref; // We receive a reference to the handle
	asCObjectType *ot = dt->GetObjectType();
	if( ot && value )
	{
		ot->engine->CallObjectMethod(value, ot->beh.addref);
		ot->refCount++;
	}
}

int asCAnyObject::Retrieve(void *ref, int refTypeId)
{
	// Verify if the value is compatible with the requested type
	bool isCompatible = false;
	if( valueTypeId == refTypeId ) 
		isCompatible = true;
	// Allow obj@ to be copied to const obj@
	else if( ((valueTypeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) == (refTypeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR))) && // Handle to the same object type
	 	     ((refTypeId & (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) == (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) )			   // Handle to const object
		isCompatible = true;

	const asCDataType *refType   = gc.objType->engine->GetDataTypeFromTypeId(refTypeId);
	asCObjectType *ot = refType->GetObjectType();
	
	// Release the old value held by the reference
	if( *(void**)ref && ot && ot->beh.release )
	{
		ot->engine->CallObjectMethod(*(void**)ref, ot->beh.release);
		*(void**)ref = 0;
	}

	if( isCompatible )
	{
		// Set the reference to the object handle
		*(void**)ref = value;
		if( value )
			ot->engine->CallObjectMethod(value, ot->beh.addref);

		return 0;
	}

	return -1;
}

int asCAnyObject::GetTypeId()
{
	return valueTypeId;
}

void asCAnyObject::FreeObject()
{
	if( value && (valueTypeId & asTYPEID_OBJHANDLE) )
	{
		const asCDataType *valueType = gc.objType->engine->GetDataTypeFromTypeId(valueTypeId);
		asCObjectType *ot = valueType->GetObjectType();

		if( !ot->beh.release )
		{
			if( ot->beh.destruct )
				ot->engine->CallObjectMethod(value, ot->beh.destruct);

			ot->engine->CallFree(ot, value);
		}
		else
		{
			ot->engine->CallObjectMethod(value, ot->beh.release);
		}
		ot->refCount--;
	}

	value = 0;
	valueTypeId = 0;
}

void asCAnyObject::Destruct()
{
	// Call the destructor, which will also call the GCObject's destructor
	this->~asCAnyObject();

	// Free the memory
	gc.objType->engine->global_free(this);
}

void asCAnyObject::CountReferences()
{
	const asCDataType *valueType = gc.objType->engine->GetDataTypeFromTypeId(valueTypeId);

	if( value && valueType && valueType->GetObjectType()->flags & asOBJ_POTENTIAL_CIRCLE )
		((asCGCObject*)value)->gc.gcCount--;
}

void asCAnyObject::AddUnmarkedReferences(asCArray<asCGCObject*> &unmarked)
{
	const asCDataType *valueType = gc.objType->engine->GetDataTypeFromTypeId(valueTypeId);

	if( value && valueType && valueType->GetObjectType()->flags & asOBJ_POTENTIAL_CIRCLE )
		unmarked.PushLast((asCGCObject*)value);
}

void asCAnyObject::ReleaseAllHandles()
{
	const asCDataType *valueType = gc.objType->engine->GetDataTypeFromTypeId(valueTypeId);

	if( value && valueType && valueType->GetObjectType()->flags & asOBJ_POTENTIAL_CIRCLE )
	{
		((asCGCObject*)value)->Release();
		value = 0;
		valueType->GetObjectType()->refCount--;
	}
}


END_AS_NAMESPACE
