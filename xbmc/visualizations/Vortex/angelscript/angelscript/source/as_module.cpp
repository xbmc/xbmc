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
// as_module.cpp
//
// A class that holds a script module
//

#include "as_config.h"
#include "as_module.h"
#include "as_builder.h"
#include "as_context.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

// internal
asCModule::asCModule(const char *name, asCScriptEngine *engine)
{
	this->name     = name;
	this->engine   = engine;

	builder = 0;
	isGlobalVarInitialized = false;
}

// internal
asCModule::~asCModule()
{
	InternalReset();

	if( builder ) 
	{
		asDELETE(builder,asCBuilder);
		builder = 0;
	}

	// Remove the module from the engine
	if( engine )
	{
		if( engine->lastModule == this )
			engine->lastModule = 0;

		engine->scriptModules.RemoveValue(this);
	}
}

// interface
asIScriptEngine *asCModule::GetEngine()
{
	return engine;
}

// interface
void asCModule::SetName(const char *name)
{
	this->name = name;
}

// interface
const char *asCModule::GetName()
{
	return name.AddressOf();
}

// interface
int asCModule::AddScriptSection(const char *name, const char *code, size_t codeLength, int lineOffset)
{
	if( !builder )
		builder = asNEW(asCBuilder)(engine, this);

	builder->AddCode(name, code, (int)codeLength, lineOffset, (int)engine->GetScriptSectionNameIndex(name ? name : ""), engine->ep.copyScriptSections);

	return asSUCCESS;
}

// internal
void asCModule::JITCompile()
{
    for (unsigned int i = 0; i < scriptFunctions.GetLength(); i++)
    {
        scriptFunctions[i]->JITCompile();
    }
}

// interface
int asCModule::Build()
{
	// Only one thread may build at one time
	int r = engine->RequestBuild();
	if( r < 0 )
		return r;

	engine->PrepareEngine();
	if( engine->configFailed )
	{
		engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_INVALID_CONFIGURATION);
		engine->BuildCompleted();
		return asINVALID_CONFIGURATION;
	}

 	InternalReset();

	if( !builder )
	{
		engine->BuildCompleted();
		return asSUCCESS;
	}

	// Compile the script
	r = builder->Build();
	asDELETE(builder,asCBuilder);
	builder = 0;
	
	if( r < 0 )
	{
		// Reset module again
		InternalReset();

		engine->BuildCompleted();
		return r;
	}

    JITCompile();

 	engine->PrepareEngine();
	engine->BuildCompleted();

	// Initialize global variables
	if( r >= 0 && engine->ep.initGlobalVarsAfterBuild )
		r = ResetGlobalVars();

	return r;
}

// interface
int asCModule::ResetGlobalVars()
{
	if( isGlobalVarInitialized ) 
		CallExit();

	// TODO: The application really should do this manually through a context
	//       otherwise it cannot properly handle script exceptions that may be
	//       thrown by object initializations.
	return CallInit();
}

// interface
int asCModule::GetFunctionIdByIndex(int index)
{
	if( index < 0 || index >= (int)scriptFunctions.GetLength() )
		return asNO_FUNCTION;

	return scriptFunctions[index]->id;
}

// internal
int asCModule::CallInit()
{
	if( isGlobalVarInitialized ) 
		return asERROR;

	// Each global variable needs to be cleared individually
	asUINT n;
	for( n = 0; n < scriptGlobals.GetLength(); n++ )
	{
		if( scriptGlobals[n] )
		{
			memset(scriptGlobals[n]->GetAddressOfValue(), 0, sizeof(asDWORD)*scriptGlobals[n]->type.GetSizeOnStackDWords());
		}
	}

	// Call the init function for each of the global variables
	asIScriptContext *ctx = 0;
	int r = 0;
	for( n = 0; n < scriptGlobals.GetLength() && r == 0; n++ )
	{
		if( scriptGlobals[n]->initFunc )
		{
			if( ctx == 0 )
			{
				r = engine->CreateContext(&ctx, true);
				if( r < 0 )
					break;
			}

			r = ctx->Prepare(scriptGlobals[n]->initFunc->id);
			if( r >= 0 )
				r = ctx->Execute();
		}
	}

	if( ctx )
	{
		ctx->Release();
		ctx = 0;
	}

	// Even if the initialization failed we need to set the 
	// flag that the variables have been initialized, otherwise
	// the module won't free those variables that really were 
	// initialized.
	isGlobalVarInitialized = true;

	if( r != asEXECUTION_FINISHED )
		return asINIT_GLOBAL_VARS_FAILED;

	return asSUCCESS;
}

// internal
void asCModule::CallExit()
{
	if( !isGlobalVarInitialized ) return;

	for( size_t n = 0; n < scriptGlobals.GetLength(); n++ )
	{
		if( scriptGlobals[n]->type.IsObject() )
		{
			void *obj = *(void**)scriptGlobals[n]->GetAddressOfValue();
			if( obj )
			{
				asCObjectType *ot = scriptGlobals[n]->type.GetObjectType();

				if( ot->beh.release )
					engine->CallObjectMethod(obj, ot->beh.release);
				else
				{
					if( ot->beh.destruct )
						engine->CallObjectMethod(obj, ot->beh.destruct);

					engine->CallFree(obj);
				}
			}
		}
	}

	isGlobalVarInitialized = false;
}

// internal
void asCModule::InternalReset()
{
	CallExit();

	size_t n;

	// Release all global functions
	for( n = 0; n < globalFunctions.GetLength(); n++ )
	{
		if( globalFunctions[n] )
			globalFunctions[n]->Release();
	}
	globalFunctions.SetLength(0);

	// First release all compiled functions
	for( n = 0; n < scriptFunctions.GetLength(); n++ )
	{
		if( scriptFunctions[n] )
		{
			// Remove the module reference in the functions
			scriptFunctions[n]->module = 0;
			scriptFunctions[n]->Release();
		}
	}
	scriptFunctions.SetLength(0);

	// Release the global properties declared in the module
	for( n = 0; n < scriptGlobals.GetLength(); n++ )
		scriptGlobals[n]->Release();
	scriptGlobals.SetLength(0);

	UnbindAllImportedFunctions();

	// Free bind information
	for( n = 0; n < bindInformations.GetLength(); n++ )
	{
		engine->importedFunctions[bindInformations[n]->importedFunctionSignature->id & 0xFFFF] = 0 ;

		asDELETE(bindInformations[n]->importedFunctionSignature, asCScriptFunction);
		asDELETE(bindInformations[n], sBindInfo);
	}
	bindInformations.SetLength(0);

	// Free declared types, including classes, typedefs, and enums
	for( n = 0; n < classTypes.GetLength(); n++ )
		classTypes[n]->Release();
	classTypes.SetLength(0);
	for( n = 0; n < enumTypes.GetLength(); n++ )
		enumTypes[n]->Release();
	enumTypes.SetLength(0);
	for( n = 0; n < typeDefs.GetLength(); n++ )
		typeDefs[n]->Release();
	typeDefs.SetLength(0);
}

// interface
int asCModule::GetFunctionIdByName(const char *name)
{
	// TODO: optimize: Improve linear search
	// Find the function id
	int id = -1;
	for( size_t n = 0; n < globalFunctions.GetLength(); n++ )
	{
		if( globalFunctions[n]->name == name )
		{
			if( id == -1 )
				id = globalFunctions[n]->id;
			else
				return asMULTIPLE_FUNCTIONS;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

// interface
int asCModule::GetImportedFunctionCount()
{
	return (int)bindInformations.GetLength();
}

// interface
int asCModule::GetImportedFunctionIndexByDecl(const char *decl)
{
	asCBuilder bld(engine, this);

	asCScriptFunction func(engine, this, -1);
	bld.ParseFunctionDeclaration(0, decl, &func, false);

	// TODO: optimize: Improve linear search
	// Search script functions for matching interface
	int id = -1;
	for( asUINT n = 0; n < bindInformations.GetLength(); ++n )
	{
		if( func.name == bindInformations[n]->importedFunctionSignature->name && 
			func.returnType == bindInformations[n]->importedFunctionSignature->returnType &&
			func.parameterTypes.GetLength() == bindInformations[n]->importedFunctionSignature->parameterTypes.GetLength() )
		{
			bool match = true;
			for( asUINT p = 0; p < func.parameterTypes.GetLength(); ++p )
			{
				if( func.parameterTypes[p] != bindInformations[n]->importedFunctionSignature->parameterTypes[p] )
				{
					match = false;
					break;
				}
			}

			if( match )
			{
				if( id == -1 )
					id = n;
				else
					return asMULTIPLE_FUNCTIONS;
			}
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

// interface
int asCModule::GetFunctionCount()
{
	return (int)globalFunctions.GetLength();
}

// interface
int asCModule::GetFunctionIdByDecl(const char *decl)
{
	asCBuilder bld(engine, this);

	asCScriptFunction func(engine, this, -1);
	int r = bld.ParseFunctionDeclaration(0, decl, &func, false);
	if( r < 0 )
		return asINVALID_DECLARATION;

	// TODO: optimize: Improve linear search
	// Search script functions for matching interface
	int id = -1;
	for( size_t n = 0; n < globalFunctions.GetLength(); ++n )
	{
		if( globalFunctions[n]->objectType == 0 && 
			func.name == globalFunctions[n]->name && 
			func.returnType == globalFunctions[n]->returnType &&
			func.parameterTypes.GetLength() == globalFunctions[n]->parameterTypes.GetLength() )
		{
			bool match = true;
			for( size_t p = 0; p < func.parameterTypes.GetLength(); ++p )
			{
				if( func.parameterTypes[p] != globalFunctions[n]->parameterTypes[p] )
				{
					match = false;
					break;
				}
			}

			if( match )
			{
				if( id == -1 )
					id = globalFunctions[n]->id;
				else
					return asMULTIPLE_FUNCTIONS;
			}
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

// interface
int asCModule::GetGlobalVarCount()
{
	return (int)scriptGlobals.GetLength();
}

// interface
int asCModule::GetGlobalVarIndexByName(const char *name)
{
	// Find the global var id
	int id = -1;
	for( size_t n = 0; n < scriptGlobals.GetLength(); n++ )
	{
		if( scriptGlobals[n]->name == name )
		{
			id = (int)n;
			break;
		}
	}

	if( id == -1 ) return asNO_GLOBAL_VAR;

	return id;
}

// interface
int asCModule::RemoveGlobalVar(int index)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return asINVALID_ARG;

	scriptGlobals[index]->Release();
	scriptGlobals.RemoveIndex(index);

	return 0;
}

// interface
asIScriptFunction *asCModule::GetFunctionDescriptorByIndex(int index)
{
	if( index < 0 || index >= (int)globalFunctions.GetLength() )
		return 0;

	return globalFunctions[index];
}

// interface
asIScriptFunction *asCModule::GetFunctionDescriptorById(int funcId)
{
	return engine->GetFunctionDescriptorById(funcId);
}

// interface
int asCModule::GetGlobalVarIndexByDecl(const char *decl)
{
	asCBuilder bld(engine, this);

	asCObjectProperty gvar;
	bld.ParseVariableDeclaration(decl, &gvar);

	// TODO: optimize: Improve linear search
	// Search script functions for matching interface
	int id = -1;
	for( size_t n = 0; n < scriptGlobals.GetLength(); ++n )
	{
		if( gvar.name == scriptGlobals[n]->name && 
			gvar.type == scriptGlobals[n]->type )
		{
			id = (int)n;
			break;
		}
	}

	if( id == -1 ) return asNO_GLOBAL_VAR;

	return id;
}

// interface
void *asCModule::GetAddressOfGlobalVar(int index)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return 0;

	// TODO: value types shouldn't need dereferencing
	// For object variables it's necessary to dereference the pointer to get the address of the value
	if( scriptGlobals[index]->type.IsObject() && !scriptGlobals[index]->type.IsObjectHandle() )
		return *(void**)(scriptGlobals[index]->GetAddressOfValue());

	return (void*)(scriptGlobals[index]->GetAddressOfValue());
}

// interface
const char *asCModule::GetGlobalVarDeclaration(int index)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return 0;

	asCGlobalProperty *prop = scriptGlobals[index];

	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = prop->type.Format();
	*tempString += " " + prop->name;

	return tempString->AddressOf();
}

// interface
const char *asCModule::GetGlobalVarName(int index)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return 0;

	return scriptGlobals[index]->name.AddressOf();
}

// interface
// TODO: If the typeId ever encodes the const flag, then the isConst parameter should be removed
int asCModule::GetGlobalVarTypeId(int index, bool *isConst)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return asINVALID_ARG;

	if( isConst )
		*isConst = scriptGlobals[index]->type.IsReadOnly();

	return engine->GetTypeIdFromDataType(scriptGlobals[index]->type);
}

// interface
int asCModule::GetObjectTypeCount()
{
	return (int)classTypes.GetLength();
}

// interface 
asIObjectType *asCModule::GetObjectTypeByIndex(asUINT index)
{
	if( index >= classTypes.GetLength() ) 
		return 0;

	return classTypes[index];
}

// interface
int asCModule::GetTypeIdByDecl(const char *decl)
{
	asCDataType dt;
	asCBuilder bld(engine, this);
	int r = bld.ParseDataType(decl, &dt);
	if( r < 0 )
		return asINVALID_TYPE;

	return engine->GetTypeIdFromDataType(dt);
}

// interface
int asCModule::GetEnumCount()
{
	return (int)enumTypes.GetLength();
}

// interface
const char *asCModule::GetEnumByIndex(asUINT index, int *enumTypeId)
{
	if( index >= enumTypes.GetLength() )
		return 0;

	if( enumTypeId )
		*enumTypeId = GetTypeIdByDecl(enumTypes[index]->name.AddressOf());

	return enumTypes[index]->name.AddressOf();
}

// interface
int asCModule::GetEnumValueCount(int enumTypeId)
{
	const asCDataType *dt = engine->GetDataTypeFromTypeId(enumTypeId);
	asCObjectType *t = dt->GetObjectType();
	if( t == 0 || !(t->GetFlags() & asOBJ_ENUM) ) 
		return asINVALID_TYPE;

	return (int)t->enumValues.GetLength();
}

// interface
const char *asCModule::GetEnumValueByIndex(int enumTypeId, asUINT index, int *outValue)
{
	const asCDataType *dt = engine->GetDataTypeFromTypeId(enumTypeId);
	asCObjectType *t = dt->GetObjectType();
	if( t == 0 || !(t->GetFlags() & asOBJ_ENUM) ) 
		return 0;

	if( index >= t->enumValues.GetLength() )
		return 0;

	if( outValue )
		*outValue = t->enumValues[index]->value;

	return t->enumValues[index]->name.AddressOf();
}

// interface
int asCModule::GetTypedefCount()
{
	return (int)typeDefs.GetLength();
}

// interface
const char *asCModule::GetTypedefByIndex(asUINT index, int *typeId)
{
	if( index >= typeDefs.GetLength() )
		return 0;

	if( typeId )
		*typeId = GetTypeIdByDecl(typeDefs[index]->name.AddressOf());

	return typeDefs[index]->name.AddressOf();
}


// internal
int asCModule::GetNextImportedFunctionId()
{
	return FUNC_IMPORTED | (asUINT)engine->importedFunctions.GetLength();
}

// internal
int asCModule::AddScriptFunction(int sectionIdx, int id, const char *name, const asCDataType &returnType, asCDataType *params, asETypeModifiers *inOutFlags, int paramCount, bool isInterface, asCObjectType *objType, bool isConstMethod, bool isGlobalFunction)
{
	asASSERT(id >= 0);

	// Store the function information
	asCScriptFunction *func = asNEW(asCScriptFunction)(engine, this, isInterface ? asFUNC_INTERFACE : asFUNC_SCRIPT);
	func->name       = name;
	func->id         = id;
	func->returnType = returnType;
	func->scriptSectionIdx = sectionIdx;
	for( int n = 0; n < paramCount; n++ )
	{
		func->parameterTypes.PushLast(params[n]);
		func->inOutFlags.PushLast(inOutFlags[n]);
	}
	func->objectType = objType;
	func->isReadOnly = isConstMethod;

	// The script function's refCount was initialized to 1
	scriptFunctions.PushLast(func);
	engine->SetScriptFunction(func);

	// Compute the signature id
	if( objType )
		func->ComputeSignatureId();

	// Add reference
	if( isGlobalFunction )
	{
		globalFunctions.PushLast(func);
		func->AddRef();
	}

	return 0;
}

// internal 
int asCModule::AddScriptFunction(asCScriptFunction *func)
{
	scriptFunctions.PushLast(func);
	func->AddRef();
	engine->SetScriptFunction(func);

	return 0;
}




// internal
int asCModule::AddImportedFunction(int id, const char *name, const asCDataType &returnType, asCDataType *params, asETypeModifiers *inOutFlags, int paramCount, const asCString &moduleName)
{
	asASSERT(id >= 0);

	// Store the function information
	asCScriptFunction *func = asNEW(asCScriptFunction)(engine, this, asFUNC_IMPORTED);
	func->name       = name;
	func->id         = id;
	func->returnType = returnType;
	for( int n = 0; n < paramCount; n++ )
	{
		func->parameterTypes.PushLast(params[n]);
		func->inOutFlags.PushLast(inOutFlags[n]);
	}
	func->objectType = 0;

	sBindInfo *info = asNEW(sBindInfo);
	info->importedFunctionSignature = func;
	info->boundFunctionId = -1;
	info->importFromModule = moduleName;
	bindInformations.PushLast(info);

	// Add the info to the array in the engine
	engine->importedFunctions.PushLast(info);

	return 0;
}

// internal
asCScriptFunction *asCModule::GetImportedFunction(int index)
{
	return bindInformations[index]->importedFunctionSignature;
}

// interface
int asCModule::BindImportedFunction(int index, int sourceId)
{
	// First unbind the old function
	int r = UnbindImportedFunction(index);
	if( r < 0 ) return r;

	// Must verify that the interfaces are equal
	asCScriptFunction *dst = GetImportedFunction(index);
	if( dst == 0 ) return asNO_FUNCTION;

	asCScriptFunction *src = engine->GetScriptFunction(sourceId);
	if( src == 0 ) 
		return asNO_FUNCTION;

	// Verify return type
	if( dst->returnType != src->returnType )
		return asINVALID_INTERFACE;

	if( dst->parameterTypes.GetLength() != src->parameterTypes.GetLength() )
		return asINVALID_INTERFACE;

	for( size_t n = 0; n < dst->parameterTypes.GetLength(); ++n )
	{
		if( dst->parameterTypes[n] != src->parameterTypes[n] )
			return asINVALID_INTERFACE;
	}

	bindInformations[index]->boundFunctionId = sourceId;
	engine->scriptFunctions[sourceId]->AddRef();

	return asSUCCESS;
}

// interface
int asCModule::UnbindImportedFunction(int index)
{
	if( index < 0 || index > (int)bindInformations.GetLength() )
		return asINVALID_ARG;

	// Remove reference to old module
	int oldFuncID = bindInformations[index]->boundFunctionId;
	if( oldFuncID != -1 )
	{
		bindInformations[index]->boundFunctionId = -1;
		engine->scriptFunctions[oldFuncID]->Release();
	}

	return asSUCCESS;
}

// interface
const char *asCModule::GetImportedFunctionDeclaration(int index)
{
	asCScriptFunction *func = GetImportedFunction(index);
	if( func == 0 ) return 0;

	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = func->GetDeclarationStr();

	return tempString->AddressOf();
}

// interface
const char *asCModule::GetImportedFunctionSourceModule(int index)
{
	if( index >= (int)bindInformations.GetLength() )
		return 0;

	return bindInformations[index]->importFromModule.AddressOf();
}

// inteface
int asCModule::BindAllImportedFunctions()
{
	bool notAllFunctionsWereBound = false;

	// Bind imported functions
	int c = GetImportedFunctionCount();
	for( int n = 0; n < c; ++n )
	{
		asCScriptFunction *func = GetImportedFunction(n);
		if( func == 0 ) return asERROR;

		asCString str = func->GetDeclarationStr();

		// Get module name from where the function should be imported
		const char *moduleName = GetImportedFunctionSourceModule(n);
		if( moduleName == 0 ) return asERROR;

		asCModule *srcMod = engine->GetModule(moduleName, false);
		int funcId = -1;
		if( srcMod )
			funcId = srcMod->GetFunctionIdByDecl(str.AddressOf());

		if( funcId < 0 )
			notAllFunctionsWereBound = true;
		else
		{
			if( BindImportedFunction(n, funcId) < 0 )
				notAllFunctionsWereBound = true;
		}
	}

	if( notAllFunctionsWereBound )
		return asCANT_BIND_ALL_FUNCTIONS;

	return asSUCCESS;
}

// interface
int asCModule::UnbindAllImportedFunctions()
{
	int c = GetImportedFunctionCount();
	for( int n = 0; n < c; ++n )
		UnbindImportedFunction(n);

	return asSUCCESS;
}

// internal
asCObjectType *asCModule::GetObjectType(const char *type)
{
	size_t n;

	// TODO: optimize: Improve linear search
	for( n = 0; n < classTypes.GetLength(); n++ )
		if( classTypes[n]->name == type )
			return classTypes[n];

	for( n = 0; n < enumTypes.GetLength(); n++ )
		if( enumTypes[n]->name == type )
			return enumTypes[n];

	for( n = 0; n < typeDefs.GetLength(); n++ )
		if( typeDefs[n]->name == type )
			return typeDefs[n];

	return 0;
}

// internal
asCGlobalProperty *asCModule::AllocateGlobalProperty(const char *name, const asCDataType &dt)
{
	asCGlobalProperty *prop = engine->AllocateGlobalProperty();
	prop->name = name;

	// Allocate the memory for this property based on its type
	prop->type = dt;
	prop->AllocateMemory();

	// Store the variable in the module scope (the reference count is already set to 1)
	scriptGlobals.PushLast(prop);

	return prop;
}

// internal
void asCModule::ResolveInterfaceIds()
{
	// For each of the interfaces declared in the script find identical interface in the engine.
	// If an identical interface was found then substitute the current id for the identical interface's id, 
	// then remove this interface declaration. If an interface was modified by the declaration, then 
	// retry the detection of identical interface for it since it may now match another.

	// For an interface to be equal to another the name and methods must match. If the interface
	// references another interface, then that must be checked as well, which can lead to circular references.

	// Example:
	//
	// interface A { void f(B@); }
	// interface B { void f(A@); void f(C@); }
	// interface C { void f(A@); }
	//
	// A1 equals A2 if and only if B1 equals B2
	// B1 equals B2 if and only if A1 equals A2 and C1 equals C2
	// C1 equals C2 if and only if A1 equals A2


	unsigned int i;

	// The interface can only be equal to interfaces declared in other modules. 
	// Interfaces registered by the application will conflict with this one if it has the same name.
	// This means that we only need to look for the interfaces in the engine->classTypes, but not in engine->objectTypes.
	asCArray<sObjectTypePair> equals;
	for( i = 0; i < classTypes.GetLength(); i++ )
	{
		asCObjectType *intf1 = classTypes[i];
		if( !intf1->IsInterface() )
			continue;

		// The interface may have been determined to be equal to another already
		bool found = false;
		for( unsigned int e = 0; e < equals.GetLength(); e++ )
		{
			if( equals[e].a == intf1 )
			{
				found = true;
				break;
			}
		}
		
		if( found )
			break;

		for( unsigned int n = 0; n < engine->classTypes.GetLength(); n++ )
		{
			// Don't compare against self
			if( engine->classTypes[n] == intf1 )
				continue;
	
			asCObjectType *intf2 = engine->classTypes[n];

			// Assume the interface are equal, then validate this
			sObjectTypePair pair = {intf1,intf2};
			equals.PushLast(pair);

			if( AreInterfacesEqual(intf1, intf2, equals) )
				break;
			
			// Since they are not equal, remove them from the list again
			equals.PopLast();
		}
	}

	// For each of the interfaces that have been found to be equal we need to 
	// remove the new declaration and instead have the module use the existing one.
	for( i = 0; i < equals.GetLength(); i++ )
	{
		// Substitute the old object type from the module's class types
		unsigned int c;
		for( c = 0; c < classTypes.GetLength(); c++ )
		{
			if( classTypes[c] == equals[i].a )
			{
				classTypes[c] = equals[i].b;
				equals[i].b->AddRef();
				break;
			}
		}

		// Remove the old object type from the engine's class types
		for( c = 0; c < engine->classTypes.GetLength(); c++ )
		{
			if( engine->classTypes[c] == equals[i].a )
			{
				engine->classTypes[c] = engine->classTypes[engine->classTypes.GetLength()-1];
				engine->classTypes.PopLast();
				break;
			}
		}

		// Substitute all uses of this object type
		// Only interfaces in the module is using the type so far
		for( c = 0; c < classTypes.GetLength(); c++ )
		{
			if( classTypes[c]->IsInterface() )
			{
				asCObjectType *intf = classTypes[c];
				for( int m = 0; m < intf->GetMethodCount(); m++ )
				{
					asCScriptFunction *func = engine->GetScriptFunction(intf->methods[m]);
					if( func )
					{
						if( func->returnType.GetObjectType() == equals[i].a )
							func->returnType.SetObjectType(equals[i].b);

						for( int p = 0; p < func->GetParamCount(); p++ )
						{
							if( func->parameterTypes[p].GetObjectType() == equals[i].a )
								func->parameterTypes[p].SetObjectType(equals[i].b);
						}
					}
				}
			}
		}

		// Substitute all interface methods in the module. Delete all methods for the old interface
		for( unsigned int m = 0; m < equals[i].a->methods.GetLength(); m++ )
		{
			for( c = 0; c < scriptFunctions.GetLength(); c++ )
			{
				if( scriptFunctions[c]->id == equals[i].a->methods[m] )
				{
					scriptFunctions[c]->Release();

					scriptFunctions[c] = engine->GetScriptFunction(equals[i].b->methods[m]);
					scriptFunctions[c]->AddRef();
				}
			}
		}

		// Deallocate the object type
		asDELETE(equals[i].a, asCObjectType);
	}
}

// internal
bool asCModule::AreInterfacesEqual(asCObjectType *a, asCObjectType *b, asCArray<sObjectTypePair> &equals)
{
	// An interface is considered equal to another if the following criterias apply:
	//
	// - The interface names are equal
	// - The number of methods is equal
	// - All the methods are equal
	// - The order of the methods is equal
	// - If a method returns or takes an interface by handle or reference, both interfaces must be equal


	// ------------
	// TODO: Study the possiblity of allowing interfaces where methods are declared in different orders to
	// be considered equal. The compiler and VM can handle this, but it complicates the comparison of interfaces
	// where multiple methods take different interfaces as parameters (or return values). Example:
	// 
	// interface A
	// {
	//    void f(B, C);
	//    void f(B);
	//    void f(C);
	// }
	//
	// If 'void f(B)' in module A is compared against 'void f(C)' in module B, then the code will assume
	// interface B in module A equals interface C in module B. Thus 'void f(B, C)' in module A won't match
	// 'void f(C, B)' in module B.
	// ------------


	// Are both interfaces?
	if( !a->IsInterface() || !b->IsInterface() )
		return false;

	// Are the names equal?
	if( a->name != b->name )
		return false;

	// Are the number of methods equal?
	if( a->methods.GetLength() != b->methods.GetLength() )
		return false;

	// Keep the number of equals in the list so we can restore it later if necessary
	int prevEquals = (int)equals.GetLength();

	// Are the methods equal to each other?
	bool match = true;
	for( unsigned int n = 0; n < a->methods.GetLength(); n++ )
	{
		match = false;

		asCScriptFunction *funcA = (asCScriptFunction*)engine->GetFunctionDescriptorById(a->methods[n]);
		asCScriptFunction *funcB = (asCScriptFunction*)engine->GetFunctionDescriptorById(b->methods[n]);

		// funcB can be null if the module that created the interface has been  
		// discarded but the type has not yet been released by the engine.
		if( funcB == 0 )
			break;

		// The methods must have the same name and the same number of parameters
		if( funcA->name != funcB->name ||
			funcA->parameterTypes.GetLength() != funcB->parameterTypes.GetLength() )
			break;
		
		// The return types must be equal. If the return type is an interface the interfaces must match.
		if( !AreTypesEqual(funcA->returnType, funcB->returnType, equals) )
			break;

		match = true;
		for( unsigned int p = 0; p < funcA->parameterTypes.GetLength(); p++ )
		{
			if( !AreTypesEqual(funcA->parameterTypes[p], funcB->parameterTypes[p], equals) ||
				funcA->inOutFlags[p] != funcB->inOutFlags[p] )
			{
				match = false;
				break;
			}
		}

		if( !match )
			break;
	}

	// For each of the new interfaces that we're assuming to be equal, we need to validate this
	if( match )
	{
		for( unsigned int n = prevEquals; n < equals.GetLength(); n++ )
		{
			if( !AreInterfacesEqual(equals[n].a, equals[n].b, equals) )
			{
				match = false;
				break;
			}
		}
	}

	if( !match )
	{
		// The interfaces doesn't match. 
		// Restore the list of previous equals before we go on, so  
		// the caller can continue comparing with another interface
		equals.SetLength(prevEquals);
	}

	return match;
}

// internal
bool asCModule::AreTypesEqual(const asCDataType &a, const asCDataType &b, asCArray<sObjectTypePair> &equals)
{
	if( !a.IsEqualExceptInterfaceType(b) )
		return false;

	asCObjectType *ai = a.GetObjectType();
	asCObjectType *bi = b.GetObjectType();

	if( ai && ai->IsInterface() )
	{
		// If the interface is in the equals list, then the pair must match the pair in the list
		bool found = false;
		unsigned int e;
		for( e = 0; e < equals.GetLength(); e++ )
		{
			if( equals[e].a == ai )
			{
				found = true;
				break;
			}
		}

		if( found )
		{
			// Do the pairs match?
			if( equals[e].b != bi )
				return false;
		}
		else
		{
			// Assume they are equal from now on
			sObjectTypePair pair = {ai, bi};
			equals.PushLast(pair);
		}
	}

	return true;
}

// interface
int asCModule::SaveByteCode(asIBinaryStream *out)
{
	if( out == 0 ) return asINVALID_ARG;

	asCRestore rest(this, out, engine);
	return rest.Save();
}

// interface
int asCModule::LoadByteCode(asIBinaryStream *in)
{
	if( in == 0 ) return asINVALID_ARG;

	// Only permit loading bytecode if no other thread is currently compiling
	int r = engine->RequestBuild();
	if( r < 0 )
		return r;

	asCRestore rest(this, in, engine);
	r = rest.Restore();

    JITCompile();

	engine->BuildCompleted();

	return r;
}

// interface
int asCModule::CompileGlobalVar(const char *sectionName, const char *code, int lineOffset)
{
	// Validate arguments
	if( code == 0 )
		return asINVALID_ARG;

	// Only one thread may build at one time
	int r = engine->RequestBuild();
	if( r < 0 )
		return r;

	// Prepare the engine
	engine->PrepareEngine();
	if( engine->configFailed )
	{
		engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_INVALID_CONFIGURATION);
		engine->BuildCompleted();
		return asINVALID_CONFIGURATION;
	}

	// Compile the global variable and add it to the module scope
	asCBuilder builder(engine, this);
	asCString str = code;
	r = builder.CompileGlobalVar(sectionName, str.AddressOf(), lineOffset);

	engine->BuildCompleted();

	// Initialize the variable
	if( r >= 0 && engine->ep.initGlobalVarsAfterBuild )
	{
		// Clear the memory 
		asCGlobalProperty *prop = scriptGlobals[scriptGlobals.GetLength()-1];
		memset(prop->GetAddressOfValue(), 0, sizeof(asDWORD)*prop->type.GetSizeOnStackDWords());

		if( prop->initFunc )
		{
			// Call the init function for the global variable
			asIScriptContext *ctx = 0;
			int r = engine->CreateContext(&ctx, true);
			if( r < 0 )
				return r;

			r = ctx->Prepare(prop->initFunc->id);
			if( r >= 0 )
				r = ctx->Execute();

			ctx->Release();
		}
	}

	return r;
}

// interface
int asCModule::CompileFunction(const char *sectionName, const char *code, int lineOffset, asDWORD compileFlags, asIScriptFunction **outFunc)
{
	asASSERT(outFunc == 0 || *outFunc == 0);

	// Validate arguments
	if( code == 0 || 
		(compileFlags != 0 && compileFlags != asCOMP_ADD_TO_MODULE) )
		return asINVALID_ARG;

	// Only one thread may build at one time
	int r = engine->RequestBuild();
	if( r < 0 )
		return r;

	// Prepare the engine
	engine->PrepareEngine();
	if( engine->configFailed )
	{
		engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_INVALID_CONFIGURATION);
		engine->BuildCompleted();
		return asINVALID_CONFIGURATION;
	}

	// Compile the single function
	asCBuilder builder(engine, this);
	asCString str = code;
	asCScriptFunction *func = 0;
	r = builder.CompileFunction(sectionName, str.AddressOf(), lineOffset, compileFlags, &func);

	engine->BuildCompleted();

	if( r >= 0 && outFunc )
	{
		// Return the function to the caller
		*outFunc = func;
		func->AddRef();
	}

	// Release our reference to the function
	if( func )
		func->Release();

	return r;
}

// interface
int asCModule::RemoveFunction(int funcId)
{
	// Find the global function
	for( asUINT n = 0; n < globalFunctions.GetLength(); n++ )
	{
		if( globalFunctions[n] && globalFunctions[n]->id == funcId )
		{
			asCScriptFunction *func = globalFunctions[n];
			globalFunctions.RemoveIndex(n);
			func->Release();

			scriptFunctions.RemoveValue(func);
			func->Release();

			return 0;
		}
	}

	return asNO_FUNCTION;
}

END_AS_NAMESPACE

