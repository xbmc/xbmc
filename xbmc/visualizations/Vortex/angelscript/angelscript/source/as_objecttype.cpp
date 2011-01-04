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
// as_objecttype.cpp
//
// A class for storing object type information
//


#include <stdio.h>

#include "as_config.h"
#include "as_objecttype.h"
#include "as_configgroup.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

#ifdef AS_MAX_PORTABILITY

static void ObjectType_AddRef_Generic(asIScriptGeneric *gen)
{
	asCObjectType *self = (asCObjectType*)gen->GetObject();
	self->AddRef();
}

static void ObjectType_Release_Generic(asIScriptGeneric *gen)
{
	asCObjectType *self = (asCObjectType*)gen->GetObject();
	self->Release();
}

static void ObjectType_GetRefCount_Generic(asIScriptGeneric *gen)
{
	asCObjectType *self = (asCObjectType*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = self->GetRefCount();
}

static void ObjectType_SetGCFlag_Generic(asIScriptGeneric *gen)
{
	asCObjectType *self = (asCObjectType*)gen->GetObject();
	self->SetGCFlag();
}

static void ObjectType_GetGCFlag_Generic(asIScriptGeneric *gen)
{
	asCObjectType *self = (asCObjectType*)gen->GetObject();
	*(bool*)gen->GetAddressOfReturnLocation() = self->GetGCFlag();
}

static void ObjectType_EnumReferences_Generic(asIScriptGeneric *gen)
{
	asCObjectType *self = (asCObjectType*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->EnumReferences(engine);
}

static void ObjectType_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	asCObjectType *self = (asCObjectType*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->ReleaseAllHandles(engine);
}

#endif


void RegisterObjectTypeGCBehaviours(asCScriptEngine *engine)
{
	// Register the gc behaviours for the object types
	int r;
	engine->objectTypeBehaviours.engine = engine;
	engine->objectTypeBehaviours.flags = asOBJ_REF | asOBJ_GC;
	engine->objectTypeBehaviours.name = "_builtin_objecttype_";
#ifndef AS_MAX_PORTABILITY
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_ADDREF, "void f()", asMETHOD(asCObjectType,AddRef), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_RELEASE, "void f()", asMETHOD(asCObjectType,Release), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(asCObjectType,GetRefCount), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_SETGCFLAG, "void f()", asMETHOD(asCObjectType,SetGCFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(asCObjectType,GetGCFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(asCObjectType,EnumReferences), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(asCObjectType,ReleaseAllHandles), asCALL_THISCALL); asASSERT( r >= 0 );
#else
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_ADDREF, "void f()", asFUNCTION(ObjectType_AddRef_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_RELEASE, "void f()", asFUNCTION(ObjectType_Release_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ObjectType_GetRefCount_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ObjectType_SetGCFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ObjectType_GetGCFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ObjectType_EnumReferences_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->objectTypeBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ObjectType_ReleaseAllHandles_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
#endif
}

asCObjectType::asCObjectType()
{
	engine      = 0; 
	refCount.set(0); 
	derivedFrom = 0;

	acceptValueSubType = true;
	acceptRefSubType = true;
}

asCObjectType::asCObjectType(asCScriptEngine *engine) 
{
	this->engine = engine; 
	refCount.set(0); 
	derivedFrom  = 0;

	acceptValueSubType = true;
	acceptRefSubType = true;
}

int asCObjectType::AddRef()
{
	gcFlag = false;
	return refCount.atomicInc();
}

int asCObjectType::Release()
{
	gcFlag = false;
	return refCount.atomicDec();
}

int asCObjectType::GetRefCount()
{
	return refCount.get();
}

bool asCObjectType::GetGCFlag()
{
	return gcFlag;
}

void asCObjectType::SetGCFlag()
{
	gcFlag = true;
}

asCObjectType::~asCObjectType()
{
	// Release the object type held by the templateSubType
	if( templateSubType.GetObjectType() )
		templateSubType.GetObjectType()->Release();

	if( derivedFrom )
		derivedFrom->Release();

	asUINT n;
	for( n = 0; n < properties.GetLength(); n++ )
		if( properties[n] ) 
		{
			if( flags & asOBJ_SCRIPT_OBJECT )
			{
				// Release the config group for script classes that are being destroyed
				asCConfigGroup *group = engine->FindConfigGroupForObjectType(properties[n]->type.GetObjectType());
				if( group != 0 ) group->Release();
			}

			asDELETE(properties[n],asCObjectProperty);
		}

	properties.SetLength(0);

	ReleaseAllFunctions();

	for( n = 0; n < enumValues.GetLength(); n++ )
	{
		if( enumValues[n] )
			asDELETE(enumValues[n],asSEnumValue);
	}

	enumValues.SetLength(0);
}

bool asCObjectType::Implements(const asCObjectType *objType) const
{
	if( this == objType )
		return true;

	for( asUINT n = 0; n < interfaces.GetLength(); n++ )
		if( interfaces[n] == objType ) return true;

	return false;
}

bool asCObjectType::DerivesFrom(const asCObjectType *objType) const
{
	if( this == objType )
		return true;

	asCObjectType *base = derivedFrom;
	while( base )
	{
		if( base == objType )
			return true;

		base = base->derivedFrom;
	}

	return false;
}

// interface
const char *asCObjectType::GetName() const
{
	return name.AddressOf();
}

// interface
asDWORD asCObjectType::GetFlags() const
{
	return flags;
}

// interface
asUINT asCObjectType::GetSize() const
{
	return size;
}

// interface
int asCObjectType::GetTypeId() const
{
	// We need a non const pointer to create the asCDataType object.
	// We're not breaking anything here because this function is not
	// modifying the object, so this const cast is safe.
	asCObjectType *ot = const_cast<asCObjectType*>(this);

	return engine->GetTypeIdFromDataType(asCDataType::CreateObject(ot, false));
}

// interface
int asCObjectType::GetSubTypeId() const
{
	// TODO: template: This method should allow indexing multiple template subtypes

	if( flags & asOBJ_TEMPLATE )
	{
		return engine->GetTypeIdFromDataType(templateSubType);
	}

	// Only template types have sub types
	return asERROR;
}

int asCObjectType::GetInterfaceCount() const
{
	return (int)interfaces.GetLength();
}

asIObjectType *asCObjectType::GetInterface(asUINT index) const
{
	assert(index < interfaces.GetLength());

	return interfaces[index];
}

// internal
bool asCObjectType::IsInterface() const
{
	if( (flags & asOBJ_SCRIPT_OBJECT) && size == 0 )
		return true;

	return false;
}

asIScriptEngine *asCObjectType::GetEngine() const
{
	return engine;
}

int asCObjectType::GetFactoryCount() const
{
	return (int)beh.factories.GetLength();
}

int asCObjectType::GetFactoryIdByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= beh.factories.GetLength() )
		return asINVALID_ARG;

	return beh.factories[index];
}

int asCObjectType::GetFactoryIdByDecl(const char *decl) const
{
	if( beh.factories.GetLength() == 0 )
		return asNO_FUNCTION;

	// Let the engine parse the string and find the appropriate factory function
	return engine->GetFactoryIdByDecl(this, decl);
}

int asCObjectType::GetMethodCount() const
{
	return (int)methods.GetLength();
}

int asCObjectType::GetMethodIdByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= methods.GetLength() )
		return asINVALID_ARG;

	return methods[index];
}

int asCObjectType::GetMethodIdByName(const char *name) const
{
	int id = -1;
	for( size_t n = 0; n < methods.GetLength(); n++ )
	{
		if( engine->scriptFunctions[methods[n]]->name == name )
		{
			if( id == -1 )
				id = methods[n];
			else
				return asMULTIPLE_FUNCTIONS;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

int asCObjectType::GetMethodIdByDecl(const char *decl) const
{
	// Get the module from one of the methods
	if( methods.GetLength() == 0 )
		return asNO_FUNCTION;

	asCModule *mod = engine->scriptFunctions[methods[0]]->module;
	if( mod == 0 )
	{
		if( engine->scriptFunctions[methods[0]]->funcType == asFUNC_INTERFACE )
			return engine->GetMethodIdByDecl(this, decl, 0);

		return asNO_MODULE;
	}

	return engine->GetMethodIdByDecl(this, decl, mod);
}

asIScriptFunction *asCObjectType::GetMethodDescriptorByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= methods.GetLength() ) 
		return 0;

	return engine->scriptFunctions[methods[index]];
}

int asCObjectType::GetPropertyCount() const
{
	return (int)properties.GetLength();
}

int asCObjectType::GetPropertyTypeId(asUINT prop) const
{
	if( prop >= properties.GetLength() )
		return asINVALID_ARG;

	return engine->GetTypeIdFromDataType(properties[prop]->type);
}

const char *asCObjectType::GetPropertyName(asUINT prop) const
{
	if( prop >= properties.GetLength() )
		return 0;

	return properties[prop]->name.AddressOf();
}

asIObjectType *asCObjectType::GetBaseType() const
{
	return derivedFrom; 
}

int asCObjectType::GetPropertyOffset(asUINT prop) const
{
	if( prop >= properties.GetLength() )
		return 0;

	return properties[prop]->byteOffset;
}

int asCObjectType::GetBehaviourCount() const
{
	// Count the number of behaviours (except factory functions)
	int count = 0;
	
	if( beh.destruct )               count++;
	if( beh.addref )                 count++;
	if( beh.release )                count++;
	if( beh.gcGetRefCount )          count++;
	if( beh.gcSetFlag )              count++;
	if( beh.gcGetFlag )              count++;
	if( beh.gcEnumReferences )       count++;
	if( beh.gcReleaseAllReferences ) count++; 
	if( beh.templateCallback )       count++;

	count += (int)beh.constructors.GetLength();
	count += (int)beh.operators.GetLength() / 2;

	return count;
}

int asCObjectType::GetBehaviourByIndex(asUINT index, asEBehaviours *outBehaviour) const
{
	// Find the correct behaviour
	int count = 0;

	if( beh.destruct && count++ == (int)index ) // only increase count if the behaviour is registered
	{ 
		if( outBehaviour ) *outBehaviour = asBEHAVE_DESTRUCT;
		return beh.destruct;
	}

	if( beh.addref && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_ADDREF;
		return beh.addref;
	}

	if( beh.release && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_RELEASE;
		return beh.release;
	}

	if( beh.gcGetRefCount && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_GETREFCOUNT;
		return beh.gcGetRefCount;
	}

	if( beh.gcSetFlag && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_SETGCFLAG;
		return beh.gcSetFlag;
	}

	if( beh.gcGetFlag && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_GETGCFLAG;
		return beh.gcGetFlag;
	}

	if( beh.gcEnumReferences && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_ENUMREFS;
		return beh.gcEnumReferences;
	}

	if( beh.gcReleaseAllReferences && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_RELEASEREFS;
		return beh.gcReleaseAllReferences;
	}

	if( beh.templateCallback && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_TEMPLATE_CALLBACK;
		return beh.templateCallback;
	}

	if( index - count < beh.constructors.GetLength() )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_CONSTRUCT;
		return beh.constructors[index - count];
	}
	else 
		count += (int)beh.constructors.GetLength();

	if( index - count < beh.operators.GetLength() / 2 )
	{
		index = 2*(index - count);

		if( outBehaviour ) *outBehaviour = static_cast<asEBehaviours>(beh.operators[index]);
		return beh.operators[index + 1];
	}

	return asINVALID_ARG;
}

// interface
const char *asCObjectType::GetConfigGroup() const
{
	asCConfigGroup *group = engine->FindConfigGroupForObjectType(this);
	if( group == 0 )
		return 0;

	return group->groupName.AddressOf();
}

// internal
void asCObjectType::ReleaseAllHandles(asIScriptEngine *)
{
	ReleaseAllFunctions();
}

// internal
void asCObjectType::ReleaseAllFunctions()
{
	beh.factory   = 0;
	for( asUINT a = 0; a < beh.factories.GetLength(); a++ )
	{
		if( engine->scriptFunctions[beh.factories[a]] ) 
			engine->scriptFunctions[beh.factories[a]]->Release();
	}
	beh.factories.SetLength(0);

	beh.construct = 0;
	for( asUINT b = 0; b < beh.constructors.GetLength(); b++ )
	{
		if( engine->scriptFunctions[beh.constructors[b]] ) 
			engine->scriptFunctions[beh.constructors[b]]->Release();
	}
	beh.constructors.SetLength(0);

	if( beh.templateCallback )
		engine->scriptFunctions[beh.templateCallback]->Release();
	beh.templateCallback = 0;

	if( beh.destruct )
		engine->scriptFunctions[beh.destruct]->Release();
	beh.destruct  = 0;

	if( beh.addref )
		engine->scriptFunctions[beh.addref]->Release();
	beh.addref = 0;

	if( beh.release )
		engine->scriptFunctions[beh.release]->Release();
	beh.release = 0;

	if( beh.copy )
		engine->scriptFunctions[beh.copy]->Release();
	beh.copy = 0;

	if( beh.gcEnumReferences )
		engine->scriptFunctions[beh.gcEnumReferences]->Release();
	beh.gcEnumReferences = 0;

	if( beh.gcGetFlag )
		engine->scriptFunctions[beh.gcGetFlag]->Release();
	beh.gcGetFlag = 0;

	if( beh.gcGetRefCount )
		engine->scriptFunctions[beh.gcGetRefCount]->Release();
	beh.gcGetRefCount = 0;

	if( beh.gcReleaseAllReferences )
		engine->scriptFunctions[beh.gcReleaseAllReferences]->Release();
	beh.gcReleaseAllReferences = 0;

	if( beh.gcSetFlag )
		engine->scriptFunctions[beh.gcSetFlag]->Release();
	beh.gcSetFlag = 0;

	for( asUINT e = 1; e < beh.operators.GetLength(); e += 2 )
	{
		if( engine->scriptFunctions[beh.operators[e]] )
			engine->scriptFunctions[beh.operators[e]]->Release();
	}
	beh.operators.SetLength(0);

	for( asUINT c = 0; c < methods.GetLength(); c++ )
	{
		if( engine->scriptFunctions[methods[c]] ) 
			engine->scriptFunctions[methods[c]]->Release();
	}
	methods.SetLength(0);

	for( asUINT d = 0; d < virtualFunctionTable.GetLength(); d++ )
	{
		if( virtualFunctionTable[d] )
			virtualFunctionTable[d]->Release();
	}
	virtualFunctionTable.SetLength(0);
}

// internal
void asCObjectType::EnumReferences(asIScriptEngine *)
{
	for( asUINT a = 0; a < beh.factories.GetLength(); a++ )
		if( engine->scriptFunctions[beh.factories[a]] ) 
			engine->GCEnumCallback(engine->scriptFunctions[beh.factories[a]]);

	for( asUINT b = 0; b < beh.constructors.GetLength(); b++ )
		if( engine->scriptFunctions[beh.constructors[b]] ) 
			engine->GCEnumCallback(engine->scriptFunctions[beh.constructors[b]]);

	if( beh.templateCallback )
		engine->GCEnumCallback(engine->scriptFunctions[beh.templateCallback]);

	if( beh.destruct )
		engine->GCEnumCallback(engine->scriptFunctions[beh.destruct]);

	if( beh.addref )
		engine->GCEnumCallback(engine->scriptFunctions[beh.addref]);

	if( beh.release )
		engine->GCEnumCallback(engine->scriptFunctions[beh.release]);

	if( beh.copy )
		engine->GCEnumCallback(engine->scriptFunctions[beh.copy]);

	if( beh.gcEnumReferences )
		engine->GCEnumCallback(engine->scriptFunctions[beh.gcEnumReferences]);

	if( beh.gcGetFlag )
		engine->GCEnumCallback(engine->scriptFunctions[beh.gcGetFlag]);

	if( beh.gcGetRefCount )
		engine->GCEnumCallback(engine->scriptFunctions[beh.gcGetRefCount]);

	if( beh.gcReleaseAllReferences )
		engine->GCEnumCallback(engine->scriptFunctions[beh.gcReleaseAllReferences]);

	if( beh.gcSetFlag )
		engine->GCEnumCallback(engine->scriptFunctions[beh.gcSetFlag]);

	for( asUINT e = 1; e < beh.operators.GetLength(); e += 2 )
		if( engine->scriptFunctions[beh.operators[e]] )
			engine->GCEnumCallback(engine->scriptFunctions[beh.operators[e]]);

	for( asUINT c = 0; c < methods.GetLength(); c++ )
		if( engine->scriptFunctions[methods[c]] ) 
			engine->GCEnumCallback(engine->scriptFunctions[methods[c]]);

	for( asUINT d = 0; d < virtualFunctionTable.GetLength(); d++ )
		if( virtualFunctionTable[d] )
			engine->GCEnumCallback(virtualFunctionTable[d]);
}

END_AS_NAMESPACE



