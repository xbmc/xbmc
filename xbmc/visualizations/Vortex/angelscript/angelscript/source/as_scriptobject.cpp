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


#include <new>

#include "as_config.h"

#include "as_scriptengine.h"

#include "as_scriptobject.h"
#include "as_arrayobject.h"

BEGIN_AS_NAMESPACE

// This helper function will call the default factory, that is a script function
asIScriptObject *ScriptObjectFactory(asCObjectType *objType, asCScriptEngine *engine)
{
	asIScriptContext *ctx;
	int r = engine->CreateContext(&ctx, true);
	if( r < 0 )
		return 0;

	r = ctx->Prepare(objType->beh.factory);
	if( r < 0 )
	{
		ctx->Release();
		return 0;
	}

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		// TODO: Verify that the memory for the structure have been released already
		ctx->Release();
		return 0;
	}

	asIScriptObject *ptr = (asIScriptObject*)ctx->GetReturnAddress();

	// Increase the reference, because the context will release it's pointer
	ptr->AddRef();

	ctx->Release();

	return ptr;
}

#ifdef AS_MAX_PORTABILITY

static void ScriptObject_AddRef_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();
	self->AddRef();
}

static void ScriptObject_Release_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();
	self->Release();
}

static void ScriptObject_GetRefCount_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = self->GetRefCount();
}

static void ScriptObject_SetFlag_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();
	self->SetFlag();
}

static void ScriptObject_GetFlag_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();
	*(bool*)gen->GetAddressOfReturnLocation() = self->GetFlag();
}

static void ScriptObject_EnumReferences_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->EnumReferences(engine);
}

static void ScriptObject_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->ReleaseAllHandles(engine);
}

#endif

void RegisterScriptObject(asCScriptEngine *engine)
{
	// Register the default script class behaviours
	int r;
	engine->scriptTypeBehaviours.engine = engine;
	engine->scriptTypeBehaviours.flags = asOBJ_SCRIPT_OBJECT | asOBJ_REF | asOBJ_GC;
	engine->scriptTypeBehaviours.name = "_builtin_object_";
#ifndef AS_MAX_PORTABILITY
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(ScriptObject_Construct), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_ADDREF, "void f()", asMETHOD(asCScriptObject,AddRef), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_RELEASE, "void f()", asMETHOD(asCScriptObject,Release), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterMethodToObjectType(&engine->scriptTypeBehaviours, "int &opAssign(int &in)", asFUNCTION(ScriptObject_Assignment), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(asCScriptObject,GetRefCount), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_SETGCFLAG, "void f()", asMETHOD(asCScriptObject,SetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(asCScriptObject,GetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(asCScriptObject,EnumReferences), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(asCScriptObject,ReleaseAllHandles), asCALL_THISCALL); asASSERT( r >= 0 );
#else
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(ScriptObject_Construct_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptObject_AddRef_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptObject_Release_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterMethodToObjectType(&engine->scriptTypeBehaviours, "int &opAssign(int &in)", asFUNCTION(ScriptObject_Assignment_Generic), asCALL_GENERIC); asASSERT( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptObject_GetRefCount_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptObject_SetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptObject_GetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptObject_EnumReferences_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->scriptTypeBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptObject_ReleaseAllHandles_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
#endif
}

void ScriptObject_Construct_Generic(asIScriptGeneric *gen)
{
	asCObjectType *objType = *(asCObjectType**)gen->GetAddressOfArg(0);
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();

	ScriptObject_Construct(objType, self);
}

void ScriptObject_Construct(asCObjectType *objType, asCScriptObject *self)
{
	new(self) asCScriptObject(objType);
}

asCScriptObject::asCScriptObject(asCObjectType *ot)
{
	refCount.set(1);
	objType          = ot;
	objType->AddRef();
	isDestructCalled = false;

	// Notify the garbage collector of this object
	if( objType->flags & asOBJ_GC )
		objType->engine->gc.AddScriptObjectToGC(this, objType);

	// Construct all properties
	asCScriptEngine *engine = objType->engine;
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCObjectProperty *prop = objType->properties[n];
		if( prop->type.IsObject() )
		{
			size_t *ptr = (size_t*)(((char*)this) + prop->byteOffset);

			if( prop->type.IsObjectHandle() )
				*ptr = 0;
			else
			{
				// Allocate the object and call it's constructor
				*ptr = (size_t)AllocateObject(prop->type.GetObjectType(), engine);
			}
		}
	}
}

void asCScriptObject::Destruct()
{
	// Call the destructor, which will also call the GCObject's destructor
	this->~asCScriptObject();

	// Free the memory
	userFree(this);
}

asCScriptObject::~asCScriptObject()
{
	objType->Release();

	// The engine pointer should be available from the objectType
	asCScriptEngine *engine = objType->engine;

	// Destroy all properties
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCObjectProperty *prop = objType->properties[n];
		if( prop->type.IsObject() )
		{
			// Destroy the object
			void **ptr = (void**)(((char*)this) + prop->byteOffset);
			if( *ptr )
			{
				FreeObject(*ptr, prop->type.GetObjectType(), engine);
				*(asDWORD*)ptr = 0;
			}
		}
	}
}

asIScriptEngine *asCScriptObject::GetEngine() const
{
	return objType->engine;
}

int asCScriptObject::AddRef()
{
	// Increase counter and clear flag set by GC
	gcFlag = false;
	return refCount.atomicInc();
}

int asCScriptObject::Release()
{
	// Clear the flag set by the GC
	gcFlag = false;

	// Call the script destructor behaviour if the reference counter is 1.
	if( refCount.get() == 1 && !isDestructCalled )
	{
		CallDestructor();
	}

	// Now do the actual releasing
	int r = refCount.atomicDec();
	if( r == 0 )
	{
		Destruct();
		return 0;
	}

	return r;
}

void asCScriptObject::CallDestructor()
{
	// Make sure the destructor is called once only, even if the  
	// reference count is increased and then decreased again
	isDestructCalled = true;

	asIScriptContext *ctx = 0;

	// Call the destructor for this class and all the super classes
	asCObjectType *ot = objType;
	while( ot )
	{
		int funcIndex = ot->beh.destruct;
		if( funcIndex )
		{
			if( ctx == 0 )
			{
				// Setup a context for calling the default constructor
				asCScriptEngine *engine = objType->engine;
				int r = engine->CreateContext(&ctx, true);
				if( r < 0 ) return;
			}

			int r = ctx->Prepare(funcIndex);
			if( r >= 0 )
			{
				ctx->SetObject(this);
				ctx->Execute();

				// There's not much to do if the execution doesn't 
				// finish, so we just ignore the result
			}
		}

		ot = ot->derivedFrom;
	}

	if( ctx )
	{
		ctx->Release();
	}
}

asIObjectType *asCScriptObject::GetObjectType() const
{
	return objType;
}

int asCScriptObject::GetRefCount()
{
	return refCount.get();
}

void asCScriptObject::SetFlag()
{
	gcFlag = true;
}

bool asCScriptObject::GetFlag()
{
	return gcFlag;
}

// interface
int asCScriptObject::GetTypeId() const
{
	asCDataType dt = asCDataType::CreateObject(objType, false);
	return objType->engine->GetTypeIdFromDataType(dt);
}

int asCScriptObject::GetPropertyCount() const
{
	return (int)objType->properties.GetLength();
}

int asCScriptObject::GetPropertyTypeId(asUINT prop) const
{
	if( prop >= objType->properties.GetLength() )
		return asINVALID_ARG;

	return objType->engine->GetTypeIdFromDataType(objType->properties[prop]->type);
}

const char *asCScriptObject::GetPropertyName(asUINT prop) const
{
	if( prop >= objType->properties.GetLength() )
		return 0;

	return objType->properties[prop]->name.AddressOf();
}

void *asCScriptObject::GetAddressOfProperty(asUINT prop)
{
	if( prop >= objType->properties.GetLength() )
		return 0;

	// Objects are stored by reference, so this must be dereferenced
	asCDataType *dt = &objType->properties[prop]->type;
	if( dt->IsObject() && !dt->IsObjectHandle() )
		return *(void**)(((char*)this) + objType->properties[prop]->byteOffset);

	return (void*)(((char*)this) + objType->properties[prop]->byteOffset);
}

void asCScriptObject::EnumReferences(asIScriptEngine *engine)
{
	// We'll notify the GC of all object handles that we're holding
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCObjectProperty *prop = objType->properties[n];
		if( prop->type.IsObject() )
		{
			void *ptr = *(void**)(((char*)this) + prop->byteOffset);
			if( ptr )
				((asCScriptEngine*)engine)->GCEnumCallback(ptr);
		}
	}
}

void asCScriptObject::ReleaseAllHandles(asIScriptEngine *engine)
{
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCObjectProperty *prop = objType->properties[n];
		if( prop->type.IsObject() && prop->type.IsObjectHandle() )
		{
			void **ptr = (void**)(((char*)this) + prop->byteOffset);
			if( *ptr )
			{
				((asCScriptEngine*)engine)->CallObjectMethod(*ptr, prop->type.GetBehaviour()->release);
				*ptr = 0;
			}
		}
	}
}

void ScriptObject_Assignment_Generic(asIScriptGeneric *gen)
{
	asCScriptObject *other = *(asCScriptObject**)gen->GetAddressOfArg(0);
	asCScriptObject *self = (asCScriptObject*)gen->GetObject();

	*self = *other;

	*(asCScriptObject**)gen->GetAddressOfReturnLocation() = self;
}

asCScriptObject &ScriptObject_Assignment(asCScriptObject *other, asCScriptObject *self)
{
	return (*self = *other);
}

asCScriptObject &asCScriptObject::operator=(const asCScriptObject &other)
{
	if( &other != this )
	{
		asASSERT( other.objType->DerivesFrom(objType) );

		asCScriptEngine *engine = objType->engine;

		// Copy all properties
		for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
		{
			asCObjectProperty *prop = objType->properties[n];
			if( prop->type.IsObject() )
			{
				void **dst = (void**)(((char*)this) + prop->byteOffset);
				void **src = (void**)(((char*)&other) + prop->byteOffset);
				if( !prop->type.IsObjectHandle() )
					CopyObject(*src, *dst, prop->type.GetObjectType(), engine);
				else
					CopyHandle((asDWORD*)src, (asDWORD*)dst, prop->type.GetObjectType(), engine);
			}
			else
			{
				void *dst = ((char*)this) + prop->byteOffset;
				void *src = ((char*)&other) + prop->byteOffset;
				memcpy(dst, src, prop->type.GetSizeInMemoryBytes());
			}
		}
	}

	return *this;
}

int asCScriptObject::CopyFrom(asIScriptObject *other)
{
	if( other == 0 ) return asINVALID_ARG;

	if( GetTypeId() != other->GetTypeId() )
		return asINVALID_TYPE;

	*this = *(asCScriptObject*)other;

	return 0;
}

void *asCScriptObject::AllocateObject(asCObjectType *objType, asCScriptEngine *engine)
{
	void *ptr = 0;

	if( objType->flags & asOBJ_SCRIPT_OBJECT )
	{
		ptr = ScriptObjectFactory(objType, engine);
	}
	else if( objType->flags & asOBJ_TEMPLATE )
	{
		ptr = ArrayObjectFactory(objType);
	}
	else if( objType->flags & asOBJ_REF )
	{
		ptr = engine->CallGlobalFunctionRetPtr(objType->beh.factory);
	}
	else
	{
		ptr = engine->CallAlloc(objType);
		int funcIndex = objType->beh.construct;
		if( funcIndex )
			engine->CallObjectMethod(ptr, funcIndex);
	}

	return ptr;
}

void asCScriptObject::FreeObject(void *ptr, asCObjectType *objType, asCScriptEngine *engine)
{
	if( !objType->beh.release )
	{
		if( objType->beh.destruct )
			engine->CallObjectMethod(ptr, objType->beh.destruct);

		engine->CallFree(ptr);
	}
	else
	{
		engine->CallObjectMethod(ptr, objType->beh.release);
	}
}

void asCScriptObject::CopyObject(void *src, void *dst, asCObjectType *objType, asCScriptEngine *engine)
{
	int funcIndex = objType->beh.copy;

	if( funcIndex )
		engine->CallObjectMethod(dst, src, funcIndex);
	else
		memcpy(dst, src, objType->size);
}

void asCScriptObject::CopyHandle(asDWORD *src, asDWORD *dst, asCObjectType *objType, asCScriptEngine *engine)
{
	if( *dst )
		engine->CallObjectMethod(*(void**)dst, objType->beh.release);
	*dst = *src;
	if( *dst )
		engine->CallObjectMethod(*(void**)dst, objType->beh.addref);
}

END_AS_NAMESPACE

