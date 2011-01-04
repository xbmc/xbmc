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
// as_scriptfunction.cpp
//
// A container for a compiled script function
//



#include "as_config.h"
#include "as_scriptfunction.h"
#include "as_tokendef.h"
#include "as_scriptengine.h"
#include "as_callfunc.h"
#include "as_bytecode.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

#ifdef AS_MAX_PORTABILITY

static void ScriptFunction_AddRef_Generic(asIScriptGeneric *gen)
{
	asCScriptFunction *self = (asCScriptFunction*)gen->GetObject();
	self->AddRef();
}

static void ScriptFunction_Release_Generic(asIScriptGeneric *gen)
{
	asCScriptFunction *self = (asCScriptFunction*)gen->GetObject();
	self->Release();
}

static void ScriptFunction_GetRefCount_Generic(asIScriptGeneric *gen)
{
	asCScriptFunction *self = (asCScriptFunction*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = self->GetRefCount();
}

static void ScriptFunction_SetFlag_Generic(asIScriptGeneric *gen)
{
	asCScriptFunction *self = (asCScriptFunction*)gen->GetObject();
	self->SetFlag();
}

static void ScriptFunction_GetFlag_Generic(asIScriptGeneric *gen)
{
	asCScriptFunction *self = (asCScriptFunction*)gen->GetObject();
	*(bool*)gen->GetAddressOfReturnLocation() = self->GetFlag();
}

static void ScriptFunction_EnumReferences_Generic(asIScriptGeneric *gen)
{
	asCScriptFunction *self = (asCScriptFunction*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->EnumReferences(engine);
}

static void ScriptFunction_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	asCScriptFunction *self = (asCScriptFunction*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->ReleaseAllHandles(engine);
}

#endif


void RegisterScriptFunction(asCScriptEngine *engine)
{
	// Register the gc behaviours for the script functions
	int r;
	engine->functionBehaviours.engine = engine;
	engine->functionBehaviours.flags = asOBJ_REF | asOBJ_GC;
	engine->functionBehaviours.name = "_builtin_function_";
#ifndef AS_MAX_PORTABILITY
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_ADDREF, "void f()", asMETHOD(asCScriptFunction,AddRef), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_RELEASE, "void f()", asMETHOD(asCScriptFunction,Release), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(asCScriptFunction,GetRefCount), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_SETGCFLAG, "void f()", asMETHOD(asCScriptFunction,SetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(asCScriptFunction,GetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(asCScriptFunction,EnumReferences), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(asCScriptFunction,ReleaseAllHandles), asCALL_THISCALL); asASSERT( r >= 0 );
#else
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptFunction_AddRef_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptFunction_Release_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptFunction_GetRefCount_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptFunction_SetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptFunction_GetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptFunction_EnumReferences_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterBehaviourToObjectType(&engine->functionBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptFunction_ReleaseAllHandles_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
#endif
}

// internal
asCScriptFunction::asCScriptFunction(asCScriptEngine *engine, asCModule *mod, int _funcType)
{
	refCount.set(1);
	this->engine           = engine;
	funcType               = _funcType;
	module                 = mod; 
	objectType             = 0; 
	name                   = ""; 
	isReadOnly             = false;
	stackNeeded            = 0;
	sysFuncIntf            = 0;
	signatureId            = 0;
	scriptSectionIdx       = -1;
	dontCleanUpOnException = false;
	vfTableIdx             = -1;
	jitFunction            = 0;
	gcFlag                 = false;

	// Notify the GC of script functions
	if( funcType == asFUNC_SCRIPT )
		engine->gc.AddScriptObjectToGC(this, &engine->functionBehaviours);
}

// internal
asCScriptFunction::~asCScriptFunction()
{
	// Imported functions are not reference counted, nor are dummy 
	// functions that are allocated on the stack
	asASSERT( funcType == -1              || 
		      funcType == asFUNC_IMPORTED ||
		      refCount.get() == 0         );

	ReleaseReferences();

	// Tell engine to free the function id
	if( funcType != -1 && funcType != asFUNC_IMPORTED && id )
		engine->FreeScriptFunctionId(id);

	for( asUINT n = 0; n < variables.GetLength(); n++ )
	{
		asDELETE(variables[n],asSScriptVariable);
	}

	if( sysFuncIntf )
	{
		asDELETE(sysFuncIntf,asSSystemFunctionInterface);
	}
}

// interface
int asCScriptFunction::GetId() const
{
	return id;
}

// interface
int asCScriptFunction::AddRef()
{
	gcFlag = false;
	asASSERT( funcType != asFUNC_IMPORTED );
	return refCount.atomicInc();
}

// interface
int asCScriptFunction::Release()
{
	gcFlag = false;
	asASSERT( funcType != asFUNC_IMPORTED );
	int r = refCount.atomicDec();
	if( r == 0 && funcType != -1 ) // Dummy functions are allocated on the stack and cannot be deleted
		asDELETE(this,asCScriptFunction);

	return r;
}

// interface
const char *asCScriptFunction::GetModuleName() const
{
	if( module )
	{
		return module->name.AddressOf();
	}

	return 0;
}

// interface
asIObjectType *asCScriptFunction::GetObjectType() const
{
	return objectType;
}

// interface
const char *asCScriptFunction::GetObjectName() const 
{
	if( objectType )
		return objectType->GetName();

	return 0;
}

// interface
const char *asCScriptFunction::GetName() const
{
	return name.AddressOf();
}

// interface
bool asCScriptFunction::IsClassMethod() const
{
	return objectType && objectType->IsInterface() == false;
}

// interface
bool asCScriptFunction::IsInterfaceMethod() const
{
	return objectType && objectType->IsInterface();
}

// interface
bool asCScriptFunction::IsReadOnly() const
{
	return isReadOnly;
}

// internal
int asCScriptFunction::GetSpaceNeededForArguments()
{
	// We need to check the size for each type
	int s = 0;
	for( asUINT n = 0; n < parameterTypes.GetLength(); n++ )
		s += parameterTypes[n].GetSizeOnStackDWords();

	return s;
}

// internal
int asCScriptFunction::GetSpaceNeededForReturnValue()
{
	return returnType.GetSizeOnStackDWords();
}

// internal
asCString asCScriptFunction::GetDeclarationStr(bool includeObjectName) const
{
	asCString str;

	str = returnType.Format();
	str += " ";
	if( objectType && includeObjectName )
	{
		if( objectType->name != "" )
			str += objectType->name + "::";
		else
			str += "_unnamed_type_::";
	}
	asASSERT(name.GetLength() > 0);
	if( name == "" )
		str += "_unnamed_function_(";
	else
		str += name + "(";

	if( parameterTypes.GetLength() > 0 )
	{
		asUINT n;
		for( n = 0; n < parameterTypes.GetLength() - 1; n++ )
		{
			str += parameterTypes[n].Format();
			if( parameterTypes[n].IsReference() && inOutFlags.GetLength() > n )
			{
				if( inOutFlags[n] == asTM_INREF ) str += "in";
				else if( inOutFlags[n] == asTM_OUTREF ) str += "out";
				else if( inOutFlags[n] == asTM_INOUTREF ) str += "inout";
			}
			str += ", ";
		}

		str += parameterTypes[n].Format();
		if( parameterTypes[n].IsReference() && inOutFlags.GetLength() > n )
		{
			if( inOutFlags[n] == asTM_INREF ) str += "in";
			else if( inOutFlags[n] == asTM_OUTREF ) str += "out";
			else if( inOutFlags[n] == asTM_INOUTREF ) str += "inout";
		}
	}

	str += ")";

	if( isReadOnly )
		str += " const";

	return str;
}

// internal
int asCScriptFunction::GetLineNumber(int programPosition)
{
	if( lineNumbers.GetLength() == 0 ) return 0;

	// Do a binary search in the buffer
	int max = (int)lineNumbers.GetLength()/2 - 1;
	int min = 0;
	int i = max/2;

	for(;;)
	{
		if( lineNumbers[i*2] < programPosition )
		{
			// Have we found the largest number < programPosition?
			if( max == i ) return lineNumbers[i*2+1];
			if( lineNumbers[i*2+2] > programPosition ) return lineNumbers[i*2+1];

			min = i + 1;
			i = (max + min)/2; 
		}
		else if( lineNumbers[i*2] > programPosition )
		{
			// Have we found the smallest number > programPosition?
			if( min == i ) return lineNumbers[i*2+1];

			max = i - 1;
			i = (max + min)/2;
		}
		else
		{
			// We found the exact position
			return lineNumbers[i*2+1];
		}
	}
}

// internal
void asCScriptFunction::AddVariable(asCString &name, asCDataType &type, int stackOffset)
{
	asSScriptVariable *var = asNEW(asSScriptVariable);
	var->name        = name;
	var->type        = type;
	var->stackOffset = stackOffset;
	variables.PushLast(var);
}

// internal
void asCScriptFunction::ComputeSignatureId()
{
	// This function will compute the signatureId based on the 
	// function name, return type, and parameter types. The object 
	// type for methods is not used, so that class methods and  
	// interface methods match each other.
	for( asUINT n = 0; n < engine->signatureIds.GetLength(); n++ )
	{
		if( !IsSignatureEqual(engine->signatureIds[n]) ) continue;

		// We don't need to increment the reference counter here, because 
		// asCScriptEngine::FreeScriptFunctionId will maintain the signature
		// id as the function is freed.
		signatureId = engine->signatureIds[n]->signatureId;
		return;
	}

	signatureId = id;
	engine->signatureIds.PushLast(this);
}

// internal
bool asCScriptFunction::IsSignatureEqual(const asCScriptFunction *func) const
{
	if( name              != func->name              ) return false;
	if( returnType        != func->returnType        ) return false;
	if( isReadOnly        != func->isReadOnly        ) return false;
	if( inOutFlags        != func->inOutFlags        ) return false;
	if( parameterTypes    != func->parameterTypes    ) return false;
	if( (objectType != 0) != (func->objectType != 0) ) return false;

	return true;
}

// internal
void asCScriptFunction::AddReferences()
{
	asUINT n;

	// Only count references if there is any bytecode
	if( byteCode.GetLength() ) 
	{
		if( returnType.IsObject() )
			returnType.GetObjectType()->AddRef();

		for( asUINT p = 0; p < parameterTypes.GetLength(); p++ )
			if( parameterTypes[p].IsObject() )
				parameterTypes[p].GetObjectType()->AddRef();
	}

	// TODO: global: The global var address should be stored in the instruction directly
	// Go through the byte code and add references to all resources used by the function
	for( n = 0; n < byteCode.GetLength(); n += asBCTypeSize[asBCInfo[*(asBYTE*)&byteCode[n]].type] )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		// Object types
		case asBC_OBJTYPE:
		case asBC_FREE:
		case asBC_REFCPY:
			{
                asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				objType->AddRef();
			}
			break;

		// Object type and function
		case asBC_ALLOC:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				objType->AddRef();

				int func = asBC_INTARG(&byteCode[n]+AS_PTR_SIZE);
				if( func )
					engine->scriptFunctions[func]->AddRef();
			}
			break;

		// Global variables
		case asBC_LDG:
		case asBC_PGA:
		case asBC_PshG4:
		case asBC_SetG4:
		case asBC_CpyVtoG4:
			// Need to increase the reference for each global variable
			{
				int gvarIdx = asBC_WORDARG0(&byteCode[n]);
				asCConfigGroup *group = GetConfigGroupByGlobalVarPtrIndex(gvarIdx);
				if( group != 0 ) group->AddRef();
			}
			break;

		case asBC_LdGRdR4:
		case asBC_CpyGtoV4:
			// Need to increase the reference for each global variable
			{
				int gvarIdx = asBC_WORDARG1(&byteCode[n]);
				asCConfigGroup *group = GetConfigGroupByGlobalVarPtrIndex(gvarIdx);
				if( group != 0 ) group->AddRef();
			}
			break;

		// System functions
		case asBC_CALLSYS:
			{
				int funcId = asBC_INTARG(&byteCode[n]);
				asCConfigGroup *group = engine->FindConfigGroupForFunction(funcId);
				if( group != 0 ) group->AddRef();

				engine->scriptFunctions[funcId]->AddRef();
			}
			break;

		// Functions
		case asBC_CALL:
		case asBC_CALLINTF:
			{
				int func = asBC_INTARG(&byteCode[n]);
				engine->scriptFunctions[func]->AddRef();
			}
			break;
		}
	}

	// Add reference to the global properties
	for( n = 0; n < globalVarPointers.GetLength(); n++ )
	{
		asCGlobalProperty *prop = GetPropertyByGlobalVarPtrIndex(n);
		prop->AddRef();
	}
}

// internal
void asCScriptFunction::ReleaseReferences()
{
	asUINT n;

	// Only count references if there is any bytecode
	if( byteCode.GetLength() )
	{
		if( returnType.IsObject() )
			returnType.GetObjectType()->Release();

		for( asUINT p = 0; p < parameterTypes.GetLength(); p++ )
			if( parameterTypes[p].IsObject() )
				parameterTypes[p].GetObjectType()->Release();
	}

	// TODO: global: The global var address should be stored in the instruction directly
	// Go through the byte code and release references to all resources used by the function
	for( n = 0; n < byteCode.GetLength(); n += asBCTypeSize[asBCInfo[*(asBYTE*)&byteCode[n]].type] )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		// Object types
		case asBC_OBJTYPE:
		case asBC_FREE:
		case asBC_REFCPY:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				if( objType ) 
					objType->Release();
			}
			break;

		// Object type and function
		case asBC_ALLOC:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				if( objType )
					objType->Release();

				int func = asBC_INTARG(&byteCode[n]+AS_PTR_SIZE);
				if( func )
					engine->scriptFunctions[func]->Release();
			}
			break;

		// Global variables
		case asBC_LDG:
		case asBC_PGA:
		case asBC_PshG4:
		case asBC_SetG4:
		case asBC_CpyVtoG4:
			// Need to decrease the reference for each global variable
			{
				int gvarIdx = asBC_WORDARG0(&byteCode[n]);
				asCConfigGroup *group = GetConfigGroupByGlobalVarPtrIndex(gvarIdx);
				if( group != 0 ) group->Release();
			}
			break;

		case asBC_LdGRdR4:
		case asBC_CpyGtoV4:
			// Need to decrease the reference for each global variable
			{
				int gvarIdx = asBC_WORDARG1(&byteCode[n]);
				asCConfigGroup *group = GetConfigGroupByGlobalVarPtrIndex(gvarIdx);
				if( group != 0 ) group->Release();
			}
			break;

		// System functions
		case asBC_CALLSYS:
			{
				int funcId = asBC_INTARG(&byteCode[n]);
				asCConfigGroup *group = engine->FindConfigGroupForFunction(funcId);
				if( group != 0 ) group->Release();

				if( funcId )
					engine->scriptFunctions[funcId]->Release();
			}
			break;

		// Functions
		case asBC_CALL:
		case asBC_CALLINTF:
			{
				int func = asBC_INTARG(&byteCode[n]);
				if( func )
					engine->scriptFunctions[func]->Release();
			}
			break;
		}
	}

	// Release the global properties
	for( n = 0; n < globalVarPointers.GetLength(); n++ )
	{
		asCGlobalProperty *prop = GetPropertyByGlobalVarPtrIndex(n);
		prop->Release();
	}

	// Release the jit compiled function
	if( jitFunction )
		engine->jitCompiler->ReleaseJITFunction(jitFunction);
	jitFunction = 0;
}

// interface
int asCScriptFunction::GetReturnTypeId() const
{
	return engine->GetTypeIdFromDataType(returnType);
}

// interface
int asCScriptFunction::GetParamCount() const
{
	return (int)parameterTypes.GetLength();
}

// interface
int asCScriptFunction::GetParamTypeId(int index, asDWORD *flags) const
{
	if( index < 0 || (unsigned)index >= parameterTypes.GetLength() )
		return asINVALID_ARG;

	if( flags )
		*flags = inOutFlags[index];

	return engine->GetTypeIdFromDataType(parameterTypes[index]);
}

// interface
asIScriptEngine *asCScriptFunction::GetEngine() const
{
	return engine;
}

// interface
const char *asCScriptFunction::GetDeclaration(bool includeObjectName) const
{
	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = GetDeclarationStr(includeObjectName);
	return tempString->AddressOf();
}

// interface
const char *asCScriptFunction::GetScriptSectionName() const
{
	if( scriptSectionIdx >= 0 )
		return engine->scriptSectionNames[scriptSectionIdx]->AddressOf();
	
	return 0;
}

// interface
const char *asCScriptFunction::GetConfigGroup() const
{
	asCConfigGroup *group = engine->FindConfigGroupForFunction(id);
	if( group == 0 )
		return 0;

	return group->groupName.AddressOf();
}

// internal
void asCScriptFunction::JITCompile()
{
    asIJITCompiler *jit = engine->GetJITCompiler();
    if( !jit )
        return;

	// Release the previous function, if any
    if( jitFunction )
    {
        engine->jitCompiler->ReleaseJITFunction(jitFunction);
        jitFunction = 0;
    }

	// Compile for native system
	int r = jit->CompileFunction(this, &jitFunction);
	if( r < 0 )
	{
		asASSERT( jitFunction == 0 );
	}
}

// interface
asDWORD *asCScriptFunction::GetByteCode(asUINT *length)
{
	if( length )
		*length = (asUINT)byteCode.GetLength();

	if( byteCode.GetLength() )
	{
		return byteCode.AddressOf();
	}

	return 0;
}

// internal
int asCScriptFunction::GetGlobalVarPtrIndex(int gvarId)
{
	void *ptr = engine->globalProperties[gvarId]->GetAddressOfValue();

	// Check if this pointer has been stored already
	for( int n = 0; n < (signed)globalVarPointers.GetLength(); n++ )
		if( globalVarPointers[n] == ptr )
			return n;

	// Add the new variable to the array
	globalVarPointers.PushLast(ptr);
	return (int)globalVarPointers.GetLength()-1;
}

// internal
asCConfigGroup *asCScriptFunction::GetConfigGroupByGlobalVarPtrIndex(int index)
{
	void *gvarPtr = globalVarPointers[index];

	int gvarId = -1;
	for( asUINT g = 0; g < engine->registeredGlobalProps.GetLength(); g++ )
	{
		if( engine->registeredGlobalProps[g] && engine->registeredGlobalProps[g]->GetAddressOfValue() == gvarPtr )
		{
			gvarId = engine->registeredGlobalProps[g]->id;
			break;
		}
	}

	if( gvarId >= 0 )
	{
		// Find the config group from the property id
		return engine->FindConfigGroupForGlobalVar(gvarId);
	}

	return 0;
}

// internal
asCGlobalProperty *asCScriptFunction::GetPropertyByGlobalVarPtrIndex(int index)
{
	void *gvarPtr = globalVarPointers[index];

	for( asUINT g = 0; g < engine->globalProperties.GetLength(); g++ )
		if( engine->globalProperties[g] && engine->globalProperties[g]->GetAddressOfValue() == gvarPtr )
			return engine->globalProperties[g];

	return 0;
}

// internal
int asCScriptFunction::GetRefCount()
{
	return refCount.get();
}

// internal
void asCScriptFunction::SetFlag()
{
	gcFlag = true;
}

// internal
bool asCScriptFunction::GetFlag()
{
	return gcFlag;
}

// internal
void asCScriptFunction::EnumReferences(asIScriptEngine *)
{
	// Notify the GC of all object types used
	if( returnType.IsObject() )
		engine->GCEnumCallback(returnType.GetObjectType());

	for( asUINT p = 0; p < parameterTypes.GetLength(); p++ )
		if( parameterTypes[p].IsObject() )
			engine->GCEnumCallback(parameterTypes[p].GetObjectType());

	// Notify the GC of all script functions that is accessed
	for( asUINT n = 0; n < byteCode.GetLength(); n += asBCTypeSize[asBCInfo[*(asBYTE*)&byteCode[n]].type] )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		case asBC_OBJTYPE:
		case asBC_FREE:
		case asBC_REFCPY:
			{
                asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				engine->GCEnumCallback(objType);
			}
			break;

		case asBC_ALLOC:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				engine->GCEnumCallback(objType);

				int func = asBC_INTARG(&byteCode[n]+AS_PTR_SIZE);
				if( func )
					engine->GCEnumCallback(engine->scriptFunctions[func]);
			}
			break;

		case asBC_CALL:
		case asBC_CALLINTF:
			{
				int func = asBC_INTARG(&byteCode[n]);
				if( func )
					engine->GCEnumCallback(engine->scriptFunctions[func]);
			}
			break;
		}
	}
}

// internal
void asCScriptFunction::ReleaseAllHandles(asIScriptEngine *)
{
	// Release paramaters
	if( byteCode.GetLength() ) 
	{
		if( returnType.IsObject() )
		{
			returnType.GetObjectType()->Release();
			returnType = asCDataType::CreatePrimitive(ttVoid, false);
		}

		for( asUINT p = 0; p < parameterTypes.GetLength(); p++ )
			if( parameterTypes[p].IsObject() )
			{
				parameterTypes[p].GetObjectType()->Release();
				parameterTypes[p] = asCDataType::CreatePrimitive(ttInt, false);
			}
	}

	// Release all script functions
	for( asUINT n = 0; n < byteCode.GetLength(); n += asBCTypeSize[asBCInfo[*(asBYTE*)&byteCode[n]].type] )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		// Object types
		case asBC_OBJTYPE:
		case asBC_FREE:
		case asBC_REFCPY:
			{
                asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				objType->Release();
				*(void**)&byteCode[n+1] = 0;
			}
			break;

		case asBC_ALLOC:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				objType->Release();
				*(void**)&byteCode[n+1] = 0;

				int func = asBC_INTARG(&byteCode[n]+AS_PTR_SIZE);
				if( func )
				{
					engine->scriptFunctions[func]->Release();
					byteCode[n+AS_PTR_SIZE+1] = 0;
				}
			}
			break;

		case asBC_CALL:
		case asBC_CALLINTF:
			{
				int func = asBC_INTARG(&byteCode[n]);
				if( func )
				{
					engine->scriptFunctions[func]->Release();
					byteCode[n+1] = 0;
				}
			}
			break;
		}
	}
}

END_AS_NAMESPACE

