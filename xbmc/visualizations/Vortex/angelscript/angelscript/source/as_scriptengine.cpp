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
// as_scriptengine.cpp
//
// The implementation of the script engine interface
//


#include <stdlib.h>

#include "as_config.h"
#include "as_scriptengine.h"
#include "as_builder.h"
#include "as_context.h"
#include "as_string_util.h"
#include "as_tokenizer.h"
#include "as_texts.h"
#include "as_module.h"
#include "as_callfunc.h"
#include "as_arrayobject.h"
#include "as_generic.h"
#include "as_scriptobject.h"
#include "as_compiler.h"

BEGIN_AS_NAMESPACE

extern "C"
{

AS_API const char * asGetLibraryVersion()
{
#ifdef _DEBUG
	return ANGELSCRIPT_VERSION_STRING " DEBUG";
#else
	return ANGELSCRIPT_VERSION_STRING;
#endif
}

AS_API const char * asGetLibraryOptions()
{
	const char *string = " "

	// Options
#ifdef AS_MAX_PORTABILITY
		"AS_MAX_PORTABILITY "
#endif
#ifdef AS_DEBUG
		"AS_DEBUG "
#endif
#ifdef AS_NO_CLASS_METHODS
		"AS_NO_CLASS_METHODS "
#endif
#ifdef AS_USE_DOUBLE_AS_FLOAT
		"AS_USE_DOUBLE_AS_FLOAT "
#endif
#ifdef AS_64BIT_PTR
		"AS_64BIT_PTR "
#endif
#ifdef AS_NO_THREADS
		"AS_NO_THREADS "
#endif
#ifdef AS_NO_ATOMIC
		"AS_NO_ATOMIC "
#endif

	// Target system
#ifdef AS_WIN
		"AS_WIN "
#endif
#ifdef AS_LINUX
		"AS_LINUX "
#endif
#ifdef AS_MAC
		"AS_MAC "
#endif
#ifdef AS_BSD
		"AS_BSD "
#endif
#ifdef AS_XBOX
		"AS_XBOX "
#endif
#ifdef AS_XBOX360
		"AS_XBOX360 "
#endif
#ifdef AS_PSP
		"AS_PSP "
#endif
#ifdef AS_PS2
		"AS_PS2 "
#endif
#ifdef AS_PS3
		"AS_PS3 "
#endif
#ifdef AS_DC
		"AS_DC "
#endif
#ifdef AS_GC
		"AS_GC "
#endif
#ifdef AS_WII
		"AS_WII "
#endif
#ifdef AS_IPHONE
		"AS_IPHONE "
#endif
#ifdef AS_ANDROID
		"AS_ANDROID "
#endif

	// CPU family
#ifdef AS_PPC
		"AS_PPC "
#endif
#ifdef AS_PPC_64
		"AS_PPC_64 "
#endif
#ifdef AS_X86
		"AS_X86 "
#endif
#ifdef AS_MIPS
		"AS_MIPS "
#endif
#ifdef AS_SH4
		"AS_SH4 "
#endif
#ifdef AS_XENON
		"AS_XENON "
#endif
#ifdef AS_ARM
		"AS_ARM "
#endif
	;

	return string;
}

AS_API asIScriptEngine *asCreateScriptEngine(asDWORD version)
{
	// Verify the version that the application expects
	if( (version/10000) != (ANGELSCRIPT_VERSION/10000) )
		return 0;

	if( (version/100)%100 != (ANGELSCRIPT_VERSION/100)%100 )
		return 0;

	if( (version%100) > (ANGELSCRIPT_VERSION%100) )
		return 0;

	// Verify the size of the types
	asASSERT( sizeof(asBYTE)  == 1 );
	asASSERT( sizeof(asWORD)  == 2 );
	asASSERT( sizeof(asDWORD) == 4 );
	asASSERT( sizeof(asQWORD) == 8 );
	asASSERT( sizeof(asPWORD) == sizeof(void*) );

	// Verify the boolean type
	asASSERT( sizeof(bool) == AS_SIZEOF_BOOL );
	asASSERT( true == VALUE_OF_BOOLEAN_TRUE );

	// Verify endianess
#ifdef AS_BIG_ENDIAN
	asASSERT( *(asDWORD*)"\x00\x01\x02\x03" == 0x00010203 );
	asASSERT( *(asQWORD*)"\x00\x01\x02\x03\x04\x05\x06\x07" == I64(0x0001020304050607) );
#else
	asASSERT( *(asDWORD*)"\x00\x01\x02\x03" == 0x03020100 );
	asASSERT( *(asQWORD*)"\x00\x01\x02\x03\x04\x05\x06\x07" == I64(0x0706050403020100) );
#endif

	return asNEW(asCScriptEngine)();
}

int asCScriptEngine::SetEngineProperty(asEEngineProp property, asPWORD value)
{
	switch( property )
	{
	case asEP_ALLOW_UNSAFE_REFERENCES:
		ep.allowUnsafeReferences = value ? true : false;
		break;

	case asEP_OPTIMIZE_BYTECODE:
		ep.optimizeByteCode = value ? true : false;
		break;

	case asEP_COPY_SCRIPT_SECTIONS:
		ep.copyScriptSections = value ? true : false;
		break;

	case asEP_MAX_STACK_SIZE:
		// The size is given in bytes, but we only store dwords
		ep.maximumContextStackSize = (int)value/4;
		if( initialContextStackSize > ep.maximumContextStackSize )
			initialContextStackSize = ep.maximumContextStackSize;
		break;

	case asEP_USE_CHARACTER_LITERALS:
		ep.useCharacterLiterals = value ? true : false;
		break;

	case asEP_ALLOW_MULTILINE_STRINGS:
		ep.allowMultilineStrings = value ? true : false;
		break;

	case asEP_ALLOW_IMPLICIT_HANDLE_TYPES:
		ep.allowImplicitHandleTypes = value ? true : false;
		break;

	case asEP_BUILD_WITHOUT_LINE_CUES:
		ep.buildWithoutLineCues = value ? true : false;
		break;

	case asEP_INIT_GLOBAL_VARS_AFTER_BUILD:
		ep.initGlobalVarsAfterBuild = value ? true : false;
		break;

	case asEP_REQUIRE_ENUM_SCOPE:
		ep.requireEnumScope = value ? true : false;
		break;

	case asEP_SCRIPT_SCANNER:
		if( value <= 1 )
			ep.scanner = (int)value;
		else
			return asINVALID_ARG;
		break;

	case asEP_INCLUDE_JIT_INSTRUCTIONS:
		ep.includeJitInstructions = value ? true : false;
		break;

	case asEP_STRING_ENCODING:
		if( value <= 1 )
			ep.stringEncoding = (int)value;
		else
			return asINVALID_ARG;
		break;

	default:
		return asINVALID_ARG;
	}

	return asSUCCESS;
}

asPWORD asCScriptEngine::GetEngineProperty(asEEngineProp property)
{
	switch( property )
	{
	case asEP_ALLOW_UNSAFE_REFERENCES:
		return ep.allowUnsafeReferences;

	case asEP_OPTIMIZE_BYTECODE:
		return ep.optimizeByteCode;

	case asEP_COPY_SCRIPT_SECTIONS:
		return ep.copyScriptSections;

	case asEP_MAX_STACK_SIZE:
		return ep.maximumContextStackSize*4;

	case asEP_USE_CHARACTER_LITERALS:
		return ep.useCharacterLiterals;

	case asEP_ALLOW_MULTILINE_STRINGS:
		return ep.allowMultilineStrings;

	case asEP_ALLOW_IMPLICIT_HANDLE_TYPES:
		return ep.allowImplicitHandleTypes;

	case asEP_BUILD_WITHOUT_LINE_CUES:
		return ep.buildWithoutLineCues;

	case asEP_INIT_GLOBAL_VARS_AFTER_BUILD:
		return ep.initGlobalVarsAfterBuild;

	case asEP_REQUIRE_ENUM_SCOPE:
		return ep.requireEnumScope;

	case asEP_SCRIPT_SCANNER:
		return ep.scanner;

	case asEP_INCLUDE_JIT_INSTRUCTIONS:
		return ep.includeJitInstructions;

	case asEP_STRING_ENCODING:
		return ep.stringEncoding;
	}

	return 0;
}

} // extern "C"





asCScriptEngine::asCScriptEngine()
{
	// Instanciate the thread manager
	if( threadManager == 0 )
		threadManager = asNEW(asCThreadManager);
	else
		threadManager->AddRef();

	// Engine properties
	ep.allowUnsafeReferences    = false;
	ep.optimizeByteCode         = true;
	ep.copyScriptSections       = true;
	ep.maximumContextStackSize  = 0;         // no limit
	ep.useCharacterLiterals     = false;
	ep.allowMultilineStrings    = false;
	ep.allowImplicitHandleTypes = false;
	ep.buildWithoutLineCues     = false;
	ep.initGlobalVarsAfterBuild = true;
	ep.requireEnumScope         = false;
	ep.scanner                  = 1;         // utf8. 0 = ascii
	ep.includeJitInstructions   = false;
	ep.stringEncoding           = 0;         // utf8. 1 = utf16

	gc.engine = this;

	refCount.set(1);
	stringFactory = 0;
	configFailed = false;
	isPrepared = false;
	isBuilding = false;
	lastModule = 0;


	userData = 0;

	initialContextStackSize = 1024;      // 1 KB


	typeIdSeqNbr = 0;
	currentGroup = &defaultGroup;

	msgCallback = 0;
    jitCompiler = 0;

	// Reserve function id 0 for no function
	scriptFunctions.PushLast(0);

	// Make sure typeId for the built-in primitives are defined according to asETypeIdFlags
	int id;
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttVoid,   false)); asASSERT( id == asTYPEID_VOID   );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttBool,   false)); asASSERT( id == asTYPEID_BOOL   );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttInt8,   false)); asASSERT( id == asTYPEID_INT8   );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttInt16,  false)); asASSERT( id == asTYPEID_INT16  );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttInt,    false)); asASSERT( id == asTYPEID_INT32  );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttInt64,  false)); asASSERT( id == asTYPEID_INT64  );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttUInt8,  false)); asASSERT( id == asTYPEID_UINT8  );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttUInt16, false)); asASSERT( id == asTYPEID_UINT16 );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttUInt,   false)); asASSERT( id == asTYPEID_UINT32 );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttUInt64, false)); asASSERT( id == asTYPEID_UINT64 );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttFloat,  false)); asASSERT( id == asTYPEID_FLOAT  );
	id = GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttDouble, false)); asASSERT( id == asTYPEID_DOUBLE );

	defaultArrayObjectType = 0;

	RegisterArrayObject(this);
	RegisterScriptObject(this);
	RegisterScriptFunction(this);
	RegisterObjectTypeGCBehaviours(this);
}

asCScriptEngine::~asCScriptEngine()
{
	asASSERT(refCount.get() == 0);
	asUINT n;

	// The modules must be deleted first, as they may use
	// object types from the config groups
	for( n = (asUINT)scriptModules.GetLength(); n-- > 0; )
	{
		if( scriptModules[n] )
		{
			asDELETE(scriptModules[n],asCModule);
		}
	}
	scriptModules.SetLength(0);

	GarbageCollect(asGC_FULL_CYCLE);

	// Delete the functions for template types that may references object types
	for( n = 0; n < templateTypes.GetLength(); n++ )
	{
		if( templateTypes[n] )
		{
			asUINT f;

			// Delete the factory stubs first
			for( f = 0; f < templateTypes[n]->beh.factories.GetLength(); f++ )
			{
				scriptFunctions[templateTypes[n]->beh.factories[f]]->Release();
			}
			templateTypes[n]->beh.factories.Allocate(0, false);

			// Delete the specialized functions
			for( f = 1; f < templateTypes[n]->beh.operators.GetLength(); f += 2 )
			{
				if( scriptFunctions[templateTypes[n]->beh.operators[f]]->objectType == templateTypes[n] )
				{
					scriptFunctions[templateTypes[n]->beh.operators[f]]->Release();
					templateTypes[n]->beh.operators[f] = 0;
				}
			}
		}
	}

	// Do one more garbage collect to free gc objects that were global variables
	GarbageCollect(asGC_FULL_CYCLE);
	FreeUnusedGlobalProperties();
	ClearUnusedTypes();

	// Break all relationship between remaining class types and functions
	for( n = 0; n < classTypes.GetLength(); n++ )
	{
		if( classTypes[n] )
			classTypes[n]->ReleaseAllFunctions();

		if( classTypes[n]->derivedFrom )
		{
			classTypes[n]->derivedFrom->Release();
			classTypes[n]->derivedFrom = 0;
		}
	}

	GarbageCollect(asGC_FULL_CYCLE);
	FreeUnusedGlobalProperties();
	ClearUnusedTypes();

	asSMapNode<int,asCDataType*> *cursor = 0;
	while( mapTypeIdToDataType.MoveFirst(&cursor) )
	{
		asDELETE(mapTypeIdToDataType.GetValue(cursor),asCDataType);
		mapTypeIdToDataType.Erase(cursor);
	}

	defaultGroup.RemoveConfiguration(this);
	while( configGroups.GetLength() )
	{
		// Delete config groups in the right order
		asCConfigGroup *grp = configGroups.PopLast();
		if( grp )
		{
			asDELETE(grp,asCConfigGroup);
		}
	}

	for( n = 0; n < registeredGlobalProps.GetLength(); n++ )
	{
		if( registeredGlobalProps[n] )
		{
			asDELETE(registeredGlobalProps[n],asCGlobalProperty);
		}
	}
	registeredGlobalProps.SetLength(0);
	FreeUnusedGlobalProperties();

	for( n = 0; n < templateTypes.GetLength(); n++ )
	{
		if( templateTypes[n] )
		{
			// Clear the sub type before deleting the template type so that the sub type isn't freed to soon
			templateTypes[n]->templateSubType = asCDataType::CreateNullHandle();
			asDELETE(templateTypes[n],asCObjectType);
		}
	}
	templateTypes.SetLength(0);

	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] )
		{
			// Clear the sub type before deleting the template type so that the sub type isn't freed to soon
			objectTypes[n]->templateSubType = asCDataType::CreateNullHandle();
			asDELETE(objectTypes[n],asCObjectType);
		}
	}
	objectTypes.SetLength(0);
	for( n = 0; n < templateSubTypes.GetLength(); n++ )
	{
		if( templateSubTypes[n] )
		{
			asDELETE(templateSubTypes[n], asCObjectType);
		}
	}
	templateSubTypes.SetLength(0);
	registeredTypeDefs.SetLength(0);
	registeredEnums.SetLength(0);
	registeredObjTypes.SetLength(0);

	for( n = 0; n < registeredGlobalFuncs.GetLength(); n++ )
	{
		if( registeredGlobalFuncs[n] )
			registeredGlobalFuncs[n]->Release();
	}
	registeredGlobalFuncs.SetLength(0);

	scriptTypeBehaviours.ReleaseAllFunctions();
	functionBehaviours.ReleaseAllFunctions();
	objectTypeBehaviours.ReleaseAllFunctions();

	// Free string constants
	for( n = 0; n < stringConstants.GetLength(); n++ )
	{
		asDELETE(stringConstants[n],asCString);
	}
	stringConstants.SetLength(0);

	// Free the script section names
	for( n = 0; n < scriptSectionNames.GetLength(); n++ )
	{
		asDELETE(scriptSectionNames[n],asCString);
	}
	scriptSectionNames.SetLength(0);

	// Release the thread manager
	threadManager->Release();
}

// interface
int asCScriptEngine::AddRef()
{
	return refCount.atomicInc();
}

// interface
int asCScriptEngine::Release()
{
	int r = refCount.atomicDec();

	if( r == 0 )
	{
		asDELETE(this,asCScriptEngine);
		return 0;
	}

	return r;
}

// interface
void *asCScriptEngine::SetUserData(void *data)
{
	void *old = userData;
	userData = data;
	return old;
}

// interface
void *asCScriptEngine::GetUserData()
{
	return userData;
}

// interface
int asCScriptEngine::SetMessageCallback(const asSFuncPtr &callback, void *obj, asDWORD callConv)
{
	msgCallback = true;
	msgCallbackObj = obj;
	bool isObj = false;
	if( (unsigned)callConv == asCALL_GENERIC )
	{
		msgCallback = false;
		return asNOT_SUPPORTED;
	}
	if( (unsigned)callConv >= asCALL_THISCALL )
	{
		isObj = true;
		if( obj == 0 )
		{
			msgCallback = false;
			return asINVALID_ARG;
		}
	}
	int r = DetectCallingConvention(isObj, callback, callConv, &msgCallbackFunc);
	if( r < 0 ) msgCallback = false;
	return r;
}

// interface
int asCScriptEngine::ClearMessageCallback()
{
	msgCallback = false;
	return 0;
}

// interface
int asCScriptEngine::WriteMessage(const char *section, int row, int col, asEMsgType type, const char *message)
{
	// Validate input parameters
	if( section == 0 ||
		message == 0 )
		return asINVALID_ARG;

	// If there is no callback then there's nothing to do
	if( !msgCallback )
		return 0;

	asSMessageInfo msg;
	msg.section = section;
	msg.row     = row;
	msg.col     = col;
	msg.type    = type;
	msg.message = message;

	if( msgCallbackFunc.callConv < ICC_THISCALL )
		CallGlobalFunction(&msg, msgCallbackObj, &msgCallbackFunc, 0);
	else
		CallObjectMethod(msgCallbackObj, &msg, &msgCallbackFunc, 0);

	return 0;
}

int asCScriptEngine::SetJITCompiler(asIJITCompiler *compiler)
{
    jitCompiler = compiler;
    return asSUCCESS;
}

asIJITCompiler *asCScriptEngine::GetJITCompiler()
{
    return jitCompiler;
}

// interface
asETokenClass asCScriptEngine::ParseToken(const char *string, size_t stringLength, int *tokenLength)
{
	if( stringLength == 0 )
		stringLength = strlen(string);

	size_t len;
	asCTokenizer t;
	asETokenClass tc;
	t.GetToken(string, stringLength, &len, &tc);

	if( tokenLength )
		*tokenLength = (int)len;

	return tc;
}

// interface
asIScriptModule *asCScriptEngine::GetModule(const char *module, asEGMFlags flag)
{
	asCModule *mod = GetModule(module, false);

	if( flag == asGM_ALWAYS_CREATE )
	{
		if( mod != 0 )
		{
			asDELETE(mod, asCModule);
		}
		return GetModule(module, true);
	}

	if( mod == 0 && flag == asGM_CREATE_IF_NOT_EXISTS )
	{
		return GetModule(module, true);
	}

	return mod;
}

// interface
int asCScriptEngine::DiscardModule(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	asDELETE(mod, asCModule);

	FreeUnusedGlobalProperties();
	ClearUnusedTypes();

	return 0;
}

void asCScriptEngine::ClearUnusedTypes()
{
	// Build a list of all types to check for
	asCArray<asCObjectType*> types;
	types = classTypes;
	types.Concatenate(templateInstanceTypes);

	// Go through all modules
	asUINT n;
	for( n = 0; n < scriptModules.GetLength() && types.GetLength(); n++ )
	{
		asCModule *mod = scriptModules[n];
		if( mod )
		{
			// Functions/Methods/Globals are handled after this

			// Go through all type declarations
			asUINT m;
			for( m = 0; m < mod->classTypes.GetLength() && types.GetLength(); m++ )
				RemoveTypeAndRelatedFromList(types, mod->classTypes[m]);
			for( m = 0; m < mod->enumTypes.GetLength() && types.GetLength(); m++ )
				RemoveTypeAndRelatedFromList(types, mod->enumTypes[m]);
			for( m = 0; m < mod->typeDefs.GetLength() && types.GetLength(); m++ )
				RemoveTypeAndRelatedFromList(types, mod->typeDefs[m]);
		}
	}

	// Go through all function parameters and remove used types
	for( n = 0; n < scriptFunctions.GetLength() && types.GetLength(); n++ )
	{
		asCScriptFunction *func = scriptFunctions[n];
		if( func )
		{
			// Ignore factory stubs
			if( func->name == "factstub" )
				continue;

			asCObjectType *ot = func->returnType.GetObjectType();
			if( ot != 0 && ot != func->objectType )
				if( func->name != ot->name )
					RemoveTypeAndRelatedFromList(types, ot);

			for( asUINT p = 0; p < func->parameterTypes.GetLength(); p++ )
			{
				ot = func->parameterTypes[p].GetObjectType();
				if( ot != 0 && ot != func->objectType )
					if( func->name != ot->name )
						RemoveTypeAndRelatedFromList(types, ot);
			}
		}
	}

	// Go through all global properties
	for( n = 0; n < globalProperties.GetLength() && types.GetLength(); n++ )
	{
		if( globalProperties[n] && globalProperties[n]->type.GetObjectType() )
			RemoveTypeAndRelatedFromList(types, globalProperties[n]->type.GetObjectType());
	}

	// All that remains in the list after this can be discarded, since they are no longer used
	for(;;)
	{
		bool didClearTemplateInstanceType = false;

		for( n = 0; n < types.GetLength(); n++ )
		{
			// Template types and script classes will have two references for each factory stub
			int refCount = ((types[n]->flags & asOBJ_TEMPLATE) || (types[n]->flags & asOBJ_SCRIPT_OBJECT)) ? 2*(int)types[n]->beh.factories.GetLength() : 0;

			if( types[n]->GetRefCount() == refCount )
			{
				if( types[n]->flags & asOBJ_TEMPLATE )
				{
					didClearTemplateInstanceType = true;
					RemoveTemplateInstanceType(types[n]);
				}
				else
				{
					RemoveFromTypeIdMap(types[n]);
					asDELETE(types[n],asCObjectType);

					int i = classTypes.IndexOf(types[n]);
					if( i == (signed)classTypes.GetLength() - 1 )
						classTypes.PopLast();
					else
						classTypes[i] = classTypes.PopLast();
				}

				// Remove the type from the array
				if( n < types.GetLength() - 1 )
					types[n] = types.PopLast();
				else
					types.PopLast();
				n--;
			}
		}

		if( didClearTemplateInstanceType == false )
			break;
	}
}

void asCScriptEngine::RemoveTypeAndRelatedFromList(asCArray<asCObjectType*> &types, asCObjectType *ot)
{
	// Remove the type from the list
	int i = types.IndexOf(ot);
	if( i == -1 ) return;

	if( i == (signed)types.GetLength() - 1 )
		types.PopLast();
	else
		types[i] = types.PopLast();

	// If the type is an template type, then remove all sub types as well
	if( ot->templateSubType.GetObjectType() )
	{
		while( ot->templateSubType.GetObjectType() )
		{
			ot = ot->templateSubType.GetObjectType();
			RemoveTypeAndRelatedFromList(types, ot);
		}
		return;
	}

	// If the type is a class, then remove all properties types as well
	if( ot->properties.GetLength() )
	{
		for( asUINT n = 0; n < ot->properties.GetLength(); n++ )
			RemoveTypeAndRelatedFromList(types, ot->properties[n]->type.GetObjectType());
	}
}


// internal
int asCScriptEngine::GetFactoryIdByDecl(const asCObjectType *ot, const char *decl)
{
	asCModule *mod = 0;

	// Is this a script class?
	if( ot->flags & asOBJ_SCRIPT_OBJECT && ot->size > 0 )
		mod = scriptFunctions[ot->beh.factory]->module;

	asCBuilder bld(this, mod);

	asCScriptFunction func(this, mod,-1);
	int r = bld.ParseFunctionDeclaration(0, decl, &func, false);
	if( r < 0 )
		return asINVALID_DECLARATION;

	// Search for matching factory function
	int id = -1;
	for( size_t n = 0; n < ot->beh.factories.GetLength(); n++ )
	{
		asCScriptFunction *f = scriptFunctions[ot->beh.factories[n]];
		if( f->IsSignatureEqual(&func) )
		{
			id = ot->beh.factories[n];
			break;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}


// internal
int asCScriptEngine::GetMethodIdByDecl(const asCObjectType *ot, const char *decl, asCModule *mod)
{
	asCBuilder bld(this, mod);

	asCScriptFunction func(this, mod, -1);
	int r = bld.ParseFunctionDeclaration(0, decl, &func, false);
	if( r < 0 )
		return asINVALID_DECLARATION;

	// Set the object type so that the signature can be properly compared
	// This cast is OK, it will only be used for comparison
	func.objectType = const_cast<asCObjectType*>(ot);

	// Search script functions for matching interface
	int id = -1;
	for( size_t n = 0; n < ot->methods.GetLength(); ++n )
	{
		if( func.IsSignatureEqual(scriptFunctions[ot->methods[n]]) )
		{
			if( id == -1 )
				id = ot->methods[n];
			else
				return asMULTIPLE_FUNCTIONS;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}


// Internal
asCString asCScriptEngine::GetFunctionDeclaration(int funcID)
{
	asCString str;
	asCScriptFunction *func = GetScriptFunction(funcID);
	if( func )
		str = func->GetDeclarationStr();

	return str;
}

asCScriptFunction *asCScriptEngine::GetScriptFunction(int funcId)
{
	if( funcId < 0 || funcId >= (int)scriptFunctions.GetLength() )
		return 0;

	return scriptFunctions[funcId];
}



asIScriptContext *asCScriptEngine::CreateContext()
{
	asIScriptContext *ctx = 0;
	CreateContext(&ctx, false);
	return ctx;
}

int asCScriptEngine::CreateContext(asIScriptContext **context, bool isInternal)
{
	*context = asNEW(asCContext)(this, !isInternal);

	// We need to make sure the engine has been 
	// prepared before any context is executed
	PrepareEngine();

	return 0;
}


int asCScriptEngine::RegisterObjectProperty(const char *obj, const char *declaration, int byteOffset)
{
	int r;
	asCDataType dt;
	asCBuilder bld(this, 0);
	r = bld.ParseDataType(obj, &dt);
	if( r < 0 )
		return ConfigError(r);

	// Verify that the correct config group is used
	if( currentGroup->FindType(dt.GetObjectType()->name.AddressOf()) == 0 )
		return ConfigError(asWRONG_CONFIG_GROUP);

	asCDataType type;
	asCString name;

	if( (r = bld.VerifyProperty(&dt, declaration, name, type)) < 0 )
		return ConfigError(r);

	// Store the property info
	if( dt.GetObjectType() == 0 )
		return ConfigError(asINVALID_OBJECT);

	asCObjectProperty *prop = asNEW(asCObjectProperty);
	prop->name       = name;
	prop->type       = type;
	prop->byteOffset = byteOffset;

	dt.GetObjectType()->properties.PushLast(prop);

	currentGroup->RefConfigGroup(FindConfigGroupForObjectType(type.GetObjectType()));

	return asSUCCESS;
}

int asCScriptEngine::RegisterInterface(const char *name)
{
	if( name == 0 ) return ConfigError(asINVALID_NAME);

	// Verify if the name has been registered as a type already
	asUINT n;
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && objectTypes[n]->name == name )
			return asALREADY_REGISTERED;
	}

	// Use builder to parse the datatype
	asCDataType dt;
	asCBuilder bld(this, 0);
	bool oldMsgCallback = msgCallback; msgCallback = false;
	int r = bld.ParseDataType(name, &dt);
	msgCallback = oldMsgCallback;
	if( r >= 0 ) return ConfigError(asERROR);

	// Make sure the name is not a reserved keyword
	asCTokenizer t;
	size_t tokenLen;
	int token = t.GetToken(name, strlen(name), &tokenLen);
	if( token != ttIdentifier || strlen(name) != tokenLen )
		return ConfigError(asINVALID_NAME);

	r = bld.CheckNameConflict(name, 0, 0);
	if( r < 0 )
		return ConfigError(asNAME_TAKEN);

	// Don't have to check against members of object
	// types as they are allowed to use the names

	// Register the object type for the interface
	asCObjectType *st = asNEW(asCObjectType)(this);
	st->flags = asOBJ_REF | asOBJ_SCRIPT_OBJECT;
	st->size = 0; // Cannot be instanciated
	st->name = name;

	// Use the default script class behaviours
	st->beh.factory = 0;
	st->beh.addref = scriptTypeBehaviours.beh.addref;
	scriptFunctions[st->beh.addref]->AddRef();
	st->beh.release = scriptTypeBehaviours.beh.release;
	scriptFunctions[st->beh.release]->AddRef();
	st->beh.copy = 0;

	objectTypes.PushLast(st);
	registeredObjTypes.PushLast(st);

	currentGroup->objTypes.PushLast(st);

	return asSUCCESS;
}

int asCScriptEngine::RegisterInterfaceMethod(const char *intf, const char *declaration)
{
	// Verify that the correct config group is set.
	if( currentGroup->FindType(intf) == 0 )
		return ConfigError(asWRONG_CONFIG_GROUP);

	asCDataType dt;
	asCBuilder bld(this, 0);
	int r = bld.ParseDataType(intf, &dt);
	if( r < 0 )
		return ConfigError(r);

	asCScriptFunction *func = asNEW(asCScriptFunction)(this, 0, asFUNC_INTERFACE);
	func->objectType = dt.GetObjectType();

	r = bld.ParseFunctionDeclaration(func->objectType, declaration, func, false);
	if( r < 0 )
	{
		asDELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	r = bld.CheckNameConflictMember(dt, func->name.AddressOf(), 0, 0);
	if( r < 0 )
	{
		asDELETE(func,asCScriptFunction);
		return ConfigError(asNAME_TAKEN);
	}

	func->id = GetNextScriptFunctionId();
	SetScriptFunction(func);
	func->objectType->methods.PushLast(func->id);
	// The refCount was already set to 1

	func->ComputeSignatureId();

	// If parameter type from other groups are used, add references
	// TODO: The code for adding references to config groups is repeated in a lot of places
	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(func->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( asUINT n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroupForObjectType(func->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

	// Return function id as success
	return func->id;
}

int asCScriptEngine::RegisterObjectType(const char *name, int byteSize, asDWORD flags)
{
	int r;

	isPrepared = false;

	// Verify flags
	//   Must have either asOBJ_REF or asOBJ_VALUE
	if( flags & asOBJ_REF )
	{
		// Can optionally have the asOBJ_GC, asOBJ_NOHANDLE, asOBJ_SCOPED, or asOBJ_TEMPLATE flag set, but nothing else
		if( flags & ~(asOBJ_REF | asOBJ_GC | asOBJ_NOHANDLE | asOBJ_SCOPED | asOBJ_TEMPLATE) )
			return ConfigError(asINVALID_ARG);

		// flags are exclusive
		if( (flags & asOBJ_GC) && (flags & (asOBJ_NOHANDLE|asOBJ_SCOPED)) )
			return ConfigError(asINVALID_ARG);
		if( (flags & asOBJ_NOHANDLE) && (flags & (asOBJ_GC|asOBJ_SCOPED)) )
			return ConfigError(asINVALID_ARG);
		if( (flags & asOBJ_SCOPED) && (flags & (asOBJ_GC|asOBJ_NOHANDLE)) )
			return ConfigError(asINVALID_ARG);
	}
	else if( flags & asOBJ_VALUE )
	{
		// Cannot use reference flags
		// TODO: template: Should be possible to register a value type as template type
		if( flags & (asOBJ_REF | asOBJ_GC | asOBJ_SCOPED) )
			return ConfigError(asINVALID_ARG);

		// If the app type is given, we must validate the flags
		if( flags & asOBJ_APP_CLASS )
		{
			// Must not set the primitive or float flag
			if( flags & (asOBJ_APP_PRIMITIVE |
				         asOBJ_APP_FLOAT) )
				return ConfigError(asINVALID_ARG);
		}
		else if( flags & asOBJ_APP_PRIMITIVE )
		{
			// Must not set the class flags nor the float flag
			if( flags & (asOBJ_APP_CLASS             |
				         asOBJ_APP_CLASS_CONSTRUCTOR |
						 asOBJ_APP_CLASS_DESTRUCTOR  |
						 asOBJ_APP_CLASS_ASSIGNMENT  |
						 asOBJ_APP_FLOAT) )
				return ConfigError(asINVALID_ARG);
		}
		else if( flags & asOBJ_APP_FLOAT )
		{
			// Must not set the class flags nor the primitive flag
			if( flags & (asOBJ_APP_CLASS             |
				         asOBJ_APP_CLASS_CONSTRUCTOR |
						 asOBJ_APP_CLASS_DESTRUCTOR  |
						 asOBJ_APP_CLASS_ASSIGNMENT  |
						 asOBJ_APP_PRIMITIVE) )
				return ConfigError(asINVALID_ARG);
		}
		else if( flags & (asOBJ_APP_CLASS_CONSTRUCTOR |
		                  asOBJ_APP_CLASS_DESTRUCTOR  |
		                  asOBJ_APP_CLASS_ASSIGNMENT) )
		{
			// Must not set the class properties, without the class flag
			return ConfigError(asINVALID_ARG);
		}
	}
	else
		return ConfigError(asINVALID_ARG);

	// Don't allow anything else than the defined flags
	if( flags - (flags & asOBJ_MASK_VALID_FLAGS) )
		return ConfigError(asINVALID_ARG);

	// Value types must have a defined size
	if( (flags & asOBJ_VALUE) && byteSize == 0 )
	{
		WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_VALUE_TYPE_MUST_HAVE_SIZE);
		return ConfigError(asINVALID_ARG);
	}

	// Verify type name
	if( name == 0 )
		return ConfigError(asINVALID_NAME);

	asCString typeName;
	asCBuilder bld(this, 0);
	if( flags & asOBJ_TEMPLATE )
	{
		asCString subtypeName;
		r = bld.ParseTemplateDecl(name, &typeName, &subtypeName);
		if( r < 0 )
			return r;

		// Verify that the template name hasn't been registered as a type already
		asUINT n;
		for( n = 0; n < objectTypes.GetLength(); n++ )
		{
			if( objectTypes[n] && objectTypes[n]->name == typeName )
				return asALREADY_REGISTERED;
		}

		asCObjectType *type = asNEW(asCObjectType)(this);
		type->name      = typeName;
		type->size      = byteSize;
		type->flags     = flags;

		// Store it in the object types
		objectTypes.PushLast(type);

		// Define a template subtype
		asCObjectType *subtype = 0;
		for( n = 0; n < templateSubTypes.GetLength(); n++ )
		{
			if( templateSubTypes[n]->name == subtypeName )
			{
				subtype = templateSubTypes[n];
				break;
			}
		}
		if( subtype == 0 )
		{
			// Create the new subtype if not already existing
			subtype = asNEW(asCObjectType)(this);
			subtype->name      = subtypeName;
			subtype->size      = 0;
			subtype->flags     = asOBJ_TEMPLATE_SUBTYPE;
			templateSubTypes.PushLast(subtype);
			subtype->AddRef();
		}
		type->templateSubType = asCDataType::CreateObject(subtype, false);
		subtype->AddRef();

		currentGroup->objTypes.PushLast(type);

		if( defaultArrayObjectType == 0 )
		{
			// TODO: The default array object type should be defined by the application
			// The default array object type is registered by the engine itself
			defaultArrayObjectType = type;
			type->AddRef();
		}
		else
		{
			registeredObjTypes.PushLast(type);
		}
	}
	else
	{
		typeName = name;

		// Verify if the name has been registered as a type already
		asUINT n;
		for( n = 0; n < objectTypes.GetLength(); n++ )
		{
			if( objectTypes[n] && objectTypes[n]->name == typeName )
				return asALREADY_REGISTERED;
		}

		for( n = 0; n < templateTypes.GetLength(); n++ )
		{
			if( templateTypes[n] && templateTypes[n]->name == typeName )
				return asALREADY_REGISTERED;
		}

		// Verify the most recently created template instance type
		asCObjectType *mostRecentTemplateInstanceType = 0;
		if( templateInstanceTypes.GetLength() )
			mostRecentTemplateInstanceType = templateInstanceTypes[templateInstanceTypes.GetLength()-1];

		// Use builder to parse the datatype
		asCDataType dt;
		bool oldMsgCallback = msgCallback; msgCallback = false;
		r = bld.ParseDataType(name, &dt);
		msgCallback = oldMsgCallback;

		// If the builder fails, then the type name
		// is new and it should be registered
		if( r < 0 )
		{
			// Make sure the name is not a reserved keyword
			asCTokenizer t;
			size_t tokenLen;
			int token = t.GetToken(name, typeName.GetLength(), &tokenLen);
			if( token != ttIdentifier || typeName.GetLength() != tokenLen )
				return ConfigError(asINVALID_NAME);

			int r = bld.CheckNameConflict(name, 0, 0);
			if( r < 0 )
				return ConfigError(asNAME_TAKEN);

			// Don't have to check against members of object
			// types as they are allowed to use the names

			// Put the data type in the list
			asCObjectType *type = asNEW(asCObjectType)(this);
			type->name      = typeName;
			type->size      = byteSize;
			type->flags     = flags;

			objectTypes.PushLast(type);
			registeredObjTypes.PushLast(type);

			currentGroup->objTypes.PushLast(type);
		}
		else
		{
			// The application is registering a template specialization so we
			// need to replace the template instance type with the new type.

			// TODO: Template: We don't require the lower dimensions to be registered first for registered template types
			// int[][] must not be allowed to be registered
			// if int[] hasn't been registered first
			if( dt.GetSubType().IsTemplate() )
				return ConfigError(asLOWER_ARRAY_DIMENSION_NOT_REGISTERED);

			if( dt.IsReadOnly() ||
				dt.IsReference() )
				return ConfigError(asINVALID_TYPE);

			// Was the template instance type created before?
			if( templateInstanceTypes[templateInstanceTypes.GetLength()-1] == mostRecentTemplateInstanceType ||
				mostRecentTemplateInstanceType == dt.GetObjectType() )
				// TODO: Should have a better error message
				return ConfigError(asNOT_SUPPORTED);

			// TODO: Add this again. The type is used by the factory stubs so we need to discount that
			// Is the template instance type already being used?
//			if( dt.GetObjectType()->GetRefCount() > 1 )
//				return ConfigError(asNOT_SUPPORTED);

			// Put the data type in the list
			asCObjectType *type = asNEW(asCObjectType)(this);
			type->name      = dt.GetObjectType()->name;
			type->templateSubType = dt.GetSubType();
			if( type->templateSubType.GetObjectType() ) type->templateSubType.GetObjectType()->AddRef();
			type->size      = byteSize;
			type->flags     = flags;

			templateTypes.PushLast(type);

			currentGroup->objTypes.PushLast(type);

			// Remove the template instance type, which will no longer be used.
			RemoveTemplateInstanceType(dt.GetObjectType());
		}
	}

	return asSUCCESS;
}

// interface
int asCScriptEngine::RegisterObjectBehaviour(const char *datatype, asEBehaviours behaviour, const char *decl, const asSFuncPtr &funcPointer, asDWORD callConv)
{
	if( datatype == 0 ) return ConfigError(asINVALID_ARG);

	// Determine the object type
	asCBuilder bld(this, 0);
	asCDataType type;
	int r = bld.ParseDataType(datatype, &type);
	if( r < 0 )
		return ConfigError(r);

	if( type.GetObjectType() == 0 )
		return ConfigError(asINVALID_TYPE);

	if( type.IsReadOnly() || type.IsReference() )
		return ConfigError(asINVALID_TYPE);

	return RegisterBehaviourToObjectType(type.GetObjectType(), behaviour, decl, funcPointer, callConv);
}

// internal
int asCScriptEngine::RegisterBehaviourToObjectType(asCObjectType *objectType, asEBehaviours behaviour, const char *decl, const asSFuncPtr &funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	if( behaviour == asBEHAVE_FACTORY ||
		behaviour == asBEHAVE_TEMPLATE_CALLBACK )
	{
#ifdef AS_MAX_PORTABILITY
		if( callConv != asCALL_GENERIC )
			return ConfigError(asNOT_SUPPORTED);
#endif
		int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
		if( r < 0 )
			return ConfigError(r);
	}
	else
	{
#ifdef AS_MAX_PORTABILITY
		if( callConv != asCALL_GENERIC )
			return ConfigError(asNOT_SUPPORTED);
#else
		if( callConv != asCALL_THISCALL &&
			callConv != asCALL_CDECL_OBJLAST &&
			callConv != asCALL_CDECL_OBJFIRST &&
			callConv != asCALL_GENERIC )
			return ConfigError(asNOT_SUPPORTED);
#endif

		int r = DetectCallingConvention(true, funcPointer, callConv, &internal);
		if( r < 0 )
			return ConfigError(r);
	}

	isPrepared = false;

	asSTypeBehaviour *beh = &objectType->beh;

	// Verify function declaration
	asCScriptFunction func(this, 0, -1);

	asCBuilder bld(this, 0);
	int r = bld.ParseFunctionDeclaration(objectType, decl, &func, true, &internal.paramAutoHandles, &internal.returnAutoHandle);
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);
	func.name.Format("_beh_%d_", behaviour);

	if( behaviour != asBEHAVE_FACTORY )
		func.objectType = objectType;

	// Check if the method restricts that use of the template to value types or reference types
	if( objectType->flags & asOBJ_TEMPLATE )
	{
		if( func.returnType.GetObjectType() == objectType->templateSubType.GetObjectType() )
		{
			if( func.returnType.IsObjectHandle() )
				objectType->acceptValueSubType = false;
			else if( !func.returnType.IsReference() )
				objectType->acceptRefSubType = false;
		}

		for( asUINT n = 0; n < func.parameterTypes.GetLength(); n++ )
		{
			if( func.parameterTypes[n].GetObjectType() == objectType->templateSubType.GetObjectType() )
			{
				// TODO: If unsafe references are allowed, then inout references allow value types
				if( func.parameterTypes[n].IsObjectHandle() || (func.parameterTypes[n].IsReference() && func.inOutFlags[n] == asTM_INOUTREF) )
					objectType->acceptValueSubType = false;
				else if( !func.parameterTypes[n].IsReference() )
					objectType->acceptRefSubType = false;
			}
		}
	}

	if( behaviour == asBEHAVE_CONSTRUCT )
	{
		// TODO: Add asBEHAVE_IMPLICIT_CONSTRUCT

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( objectType->flags & asOBJ_SCRIPT_OBJECT )
		{
			// The script object is a special case
			asASSERT(func.parameterTypes.GetLength() == 1);

			beh->construct = AddBehaviourFunction(func, internal);
			beh->factory   = beh->construct;
			scriptFunctions[beh->factory]->AddRef();
			beh->constructors.PushLast(beh->construct);
			beh->factories.PushLast(beh->factory);
			func.id = beh->construct;
		}
		else
		{
			// Verify that it is a value type
			if( !(func.objectType->flags & asOBJ_VALUE) )
			{
				WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
				return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
			}

			// Implicit constructors must take one and only one parameter
	/*		if( behaviour == asBEHAVE_IMPLICIT_CONSTRUCT &&
				func.parameterTypes.GetLength() != 1 )
				return ConfigError(asINVALID_DECLARATION);
	*/
			// TODO: Verify that the same constructor hasn't been registered already

			// Store all constructors in a list
			if( func.parameterTypes.GetLength() == 0 )
			{
				func.id = beh->construct = AddBehaviourFunction(func, internal);
				beh->constructors.PushLast(beh->construct);
			}
			else
			{
				func.id = AddBehaviourFunction(func, internal);
				beh->constructors.PushLast(func.id);
	/*
				if( behaviour == asBEHAVE_IMPLICIT_CONSTRUCT )
				{
					beh->operators.PushLast(behaviour);
					beh->operators.PushLast(func.id);
				}
	*/		}
		}
	}
	else if( behaviour == asBEHAVE_DESTRUCT )
	{
		// Must be a value type
		if( !(func.objectType->flags & asOBJ_VALUE) )
		{
			WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		if( beh->destruct )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		func.id = beh->destruct = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_FACTORY )
	{
		// TODO: Add asBEHAVE_IMPLICIT_FACTORY

		// Must be a ref type and must not have asOBJ_NOHANDLE
		if( !(objectType->flags & asOBJ_REF) || (objectType->flags & asOBJ_NOHANDLE) )
		{
			WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		// Verify that the return type is a handle to the type
		if( func.returnType != asCDataType::CreateObjectHandle(objectType, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Implicit factories must take one and only one parameter
/*		if( behaviour == asBEHAVE_IMPLICIT_FACTORY &&
			func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);
*/
		// TODO: Verify that the same factory function hasn't been registered already

		// The templates take a hidden parameter with the object type
		if( (objectType->flags & asOBJ_TEMPLATE) &&
			(func.parameterTypes.GetLength() == 0 ||
			 !func.parameterTypes[0].IsReference()) )
		{
			return ConfigError(asINVALID_DECLARATION);
		}

		// Store all factory functions in a list
		if( (func.parameterTypes.GetLength() == 0) ||
			(func.parameterTypes.GetLength() == 1 && (objectType->flags & asOBJ_TEMPLATE)) )
		{
			func.id = beh->factory = AddBehaviourFunction(func, internal);
			beh->factories.PushLast(beh->factory);
		}
		else
		{
			func.id = AddBehaviourFunction(func, internal);
			beh->factories.PushLast(func.id);
/*
			if( behaviour == asBEHAVE_IMPLICIT_FACTORY )
			{
				beh->operators.PushLast(behaviour);
				beh->operators.PushLast(func.id);
			}
*/		}
	}
	else if( behaviour == asBEHAVE_ADDREF )
	{
		// Must be a ref type and must not have asOBJ_NOHANDLE, nor asOBJ_SCOPED
		if( !(func.objectType->flags & asOBJ_REF) ||
			(func.objectType->flags & asOBJ_NOHANDLE) ||
			(func.objectType->flags & asOBJ_SCOPED) )
		{
			WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		if( beh->addref )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		func.id = beh->addref = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_RELEASE )
	{
		// Must be a ref type and must not have asOBJ_NOHANDLE
		if( !(func.objectType->flags & asOBJ_REF) || (func.objectType->flags & asOBJ_NOHANDLE) )
		{
			WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		if( beh->release )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		func.id = beh->release = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_TEMPLATE_CALLBACK )
	{
		// Must be a template type 
		if( !(func.objectType->flags & asOBJ_TEMPLATE) )
		{
			WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		if( beh->templateCallback )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is bool
		if( func.returnType != asCDataType::CreatePrimitive(ttBool, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there is one parameters
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		func.id = beh->templateCallback = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_INDEX )
	{
		// Verify that the var type is not used
		if( VerifyVarTypeNotInFunction(&func) < 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there is only one parameter
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is not void
		if( func.returnType.GetTokenType() == ttVoid )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the operator hasn't been registered already

		beh->operators.PushLast(behaviour);
		func.id = AddBehaviourFunction(func, internal);
		beh->operators.PushLast(func.id);
	}
	else if( behaviour >= asBEHAVE_FIRST_GC &&
		     behaviour <= asBEHAVE_LAST_GC )
	{
		// Only allow GC behaviours for types registered to be garbage collected
		if( !(func.objectType->flags & asOBJ_GC) )
		{
			WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		// Verify parameter count
		if( (behaviour == asBEHAVE_GETREFCOUNT ||
			 behaviour == asBEHAVE_SETGCFLAG   ||
			 behaviour == asBEHAVE_GETGCFLAG) &&
			func.parameterTypes.GetLength() != 0 )
			return ConfigError(asINVALID_DECLARATION);

		if( (behaviour == asBEHAVE_ENUMREFS ||
			 behaviour == asBEHAVE_RELEASEREFS) &&
			func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify return type
		if( behaviour == asBEHAVE_GETREFCOUNT &&
			func.returnType != asCDataType::CreatePrimitive(ttInt, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( behaviour == asBEHAVE_GETGCFLAG &&
			func.returnType != asCDataType::CreatePrimitive(ttBool, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( (behaviour == asBEHAVE_SETGCFLAG ||
			 behaviour == asBEHAVE_ENUMREFS  ||
			 behaviour == asBEHAVE_RELEASEREFS) &&
			func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( behaviour == asBEHAVE_GETREFCOUNT )
			func.id = beh->gcGetRefCount = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_SETGCFLAG )
			func.id = beh->gcSetFlag = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_GETGCFLAG )
			func.id = beh->gcGetFlag = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_ENUMREFS )
			func.id = beh->gcEnumReferences = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_RELEASEREFS )
			func.id = beh->gcReleaseAllReferences = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_IMPLICIT_VALUE_CAST ||
		     behaviour == asBEHAVE_VALUE_CAST )
	{
		// Verify parameter count
		if( func.parameterTypes.GetLength() != 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify return type
		if( func.returnType.IsEqualExceptRefAndConst(asCDataType::CreatePrimitive(ttBool, false)) )
			return ConfigError(asNOT_SUPPORTED);

		if( func.returnType.IsEqualExceptRefAndConst(asCDataType::CreatePrimitive(ttVoid, false)) )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: verify that the same cast is not registered already (const or non-const is treated the same for the return type)

		beh->operators.PushLast(behaviour);
		func.id = AddBehaviourFunction(func, internal);
		beh->operators.PushLast(func.id);
	}
	else if( behaviour == asBEHAVE_REF_CAST ||
	         behaviour == asBEHAVE_IMPLICIT_REF_CAST )
	{
		// Verify parameter count
		if( func.parameterTypes.GetLength() != 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify return type
		if( !func.returnType.IsObjectHandle() )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: verify that the same cast is not registered already (cosnt or non-const is treated the same for the return type)

		beh->operators.PushLast(behaviour);
		func.id = AddBehaviourFunction(func, internal);
		beh->operators.PushLast(func.id);
	}
	else
	{
		asASSERT(false);

		return ConfigError(asINVALID_ARG);
	}

	// Return function id as success
	return func.id;
}


int asCScriptEngine::VerifyVarTypeNotInFunction(asCScriptFunction *func)
{
	// Don't allow var type in this function
	if( func->returnType.GetTokenType() == ttQuestion )
		return asINVALID_DECLARATION;

	for( unsigned int n = 0; n < func->parameterTypes.GetLength(); n++ )
		if( func->parameterTypes[n].GetTokenType() == ttQuestion )
			return asINVALID_DECLARATION;

	return 0;
}

int asCScriptEngine::AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal)
{
	asUINT n;

	int id = GetNextScriptFunctionId();

	asSSystemFunctionInterface *newInterface = asNEW(asSSystemFunctionInterface);
	newInterface->func               = internal.func;
	newInterface->baseOffset         = internal.baseOffset;
	newInterface->callConv           = internal.callConv;
	newInterface->scriptReturnSize   = internal.scriptReturnSize;
	newInterface->hostReturnInMemory = internal.hostReturnInMemory;
	newInterface->hostReturnFloat    = internal.hostReturnFloat;
	newInterface->hostReturnSize     = internal.hostReturnSize;
	newInterface->paramSize          = internal.paramSize;
	newInterface->takesObjByVal      = internal.takesObjByVal;
	newInterface->paramAutoHandles   = internal.paramAutoHandles;
	newInterface->returnAutoHandle   = internal.returnAutoHandle;
	newInterface->hasAutoHandles     = internal.hasAutoHandles;

	asCScriptFunction *f = asNEW(asCScriptFunction)(this, 0, asFUNC_SYSTEM);
	asASSERT(func.name != "" && func.name != "f");
	f->name        = func.name;
	f->sysFuncIntf = newInterface;
	f->returnType  = func.returnType;
	f->objectType  = func.objectType;
	f->id          = id;
	f->isReadOnly  = func.isReadOnly;
	for( n = 0; n < func.parameterTypes.GetLength(); n++ )
	{
		f->parameterTypes.PushLast(func.parameterTypes[n]);
		f->inOutFlags.PushLast(func.inOutFlags[n]);
	}

	SetScriptFunction(f);

	// If parameter type from other groups are used, add references
	if( f->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(f->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( n = 0; n < f->parameterTypes.GetLength(); n++ )
	{
		if( f->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroupForObjectType(f->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

	return id;
}

// interface
int asCScriptEngine::RegisterGlobalProperty(const char *declaration, void *pointer)
{
	asCDataType type;
	asCString name;

	int r;
	asCBuilder bld(this, 0);
	if( (r = bld.VerifyProperty(0, declaration, name, type)) < 0 )
		return ConfigError(r);

	// Don't allow registering references as global properties
	if( type.IsReference() )
		return ConfigError(asINVALID_TYPE);

	// Store the property info
	asCGlobalProperty *prop = AllocateGlobalProperty();
	prop->name        = name;
	prop->type        = type;

	prop->SetRegisteredAddress(pointer);
	
	registeredGlobalProps.PushLast(prop);
	currentGroup->globalProps.PushLast(prop);

	// If from another group add reference
	if( type.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(type.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}

	return asSUCCESS;
}

// internal
asCGlobalProperty *asCScriptEngine::AllocateGlobalProperty()
{
	asCGlobalProperty *prop = asNEW(asCGlobalProperty);

	// First check the availability of a free slot
	if( freeGlobalPropertyIds.GetLength() )
	{
		prop->id = freeGlobalPropertyIds.PopLast();
		globalProperties[prop->id] = prop;
		return prop;
	}

	prop->id = (asUINT)globalProperties.GetLength();
	globalProperties.PushLast(prop);
	return prop;
}

// internal
void asCScriptEngine::FreeUnusedGlobalProperties()
{
	for( asUINT n = 0; n < globalProperties.GetLength(); n++ )
	{
		if( globalProperties[n] && globalProperties[n]->refCount.get() == 0 )
		{
			freeGlobalPropertyIds.PushLast(n);
			asDELETE(globalProperties[n], asCGlobalProperty);
			globalProperties[n] = 0;
		}
	}
}

// interface
int asCScriptEngine::GetGlobalPropertyCount()
{
	return (int)registeredGlobalProps.GetLength();
}

// interface
// TODO: If the typeId ever encodes the const flag, then the isConst parameter should be removed
int asCScriptEngine::GetGlobalPropertyByIndex(asUINT index, const char **name, int *typeId, bool *isConst, const char **configGroup, void **pointer)
{
	if( index >= registeredGlobalProps.GetLength() )
		return asINVALID_ARG;

	if( name )
		*name = registeredGlobalProps[index]->name.AddressOf();

	if( configGroup )
	{
		asCConfigGroup *group = FindConfigGroupForGlobalVar(index);
		if( group )
			*configGroup = group->groupName.AddressOf();
		else
			*configGroup = 0;
	}

	if( typeId )
		*typeId = GetTypeIdFromDataType(registeredGlobalProps[index]->type);

	if( isConst )
		*isConst = registeredGlobalProps[index]->type.IsReadOnly();

	if( pointer )
		*pointer = registeredGlobalProps[index]->realAddress;

	return asSUCCESS;
}

// interface
int asCScriptEngine::RegisterObjectMethod(const char *obj, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv)
{
	if( obj == 0 )
		return ConfigError(asINVALID_ARG);

	// Determine the object type
	asCDataType dt;
	asCBuilder bld(this, 0);
	int r = bld.ParseDataType(obj, &dt);
	if( r < 0 )
		return ConfigError(r);

	if( dt.GetObjectType() == 0 )
		return ConfigError(asINVALID_ARG);

	return RegisterMethodToObjectType(dt.GetObjectType(), declaration, funcPointer, callConv);
}

// internal
int asCScriptEngine::RegisterMethodToObjectType(asCObjectType *objectType, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(true, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	// We only support these calling conventions for object methods
#ifdef AS_MAX_PORTABILITY
	if( callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#else
	if( callConv != asCALL_THISCALL &&
		callConv != asCALL_CDECL_OBJLAST &&
		callConv != asCALL_CDECL_OBJFIRST &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#endif

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = asNEW(asSSystemFunctionInterface)(internal);

	asCScriptFunction *func = asNEW(asCScriptFunction)(this, 0, asFUNC_SYSTEM);
	func->sysFuncIntf = newInterface;
	func->objectType  = objectType;

	asCBuilder bld(this, 0);
	r = bld.ParseFunctionDeclaration(func->objectType, declaration, func, true, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
	if( r < 0 )
	{
		// Set as dummy function before deleting
		func->funcType = -1;
		asDELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	asCDataType x = asCDataType::CreateObject(objectType, false);
	r = bld.CheckNameConflictMember(x, func->name.AddressOf(), 0, 0);
	if( r < 0 )
	{
		asDELETE(func,asCScriptFunction);
		return ConfigError(asNAME_TAKEN);
	}

	func->id = GetNextScriptFunctionId();
	func->objectType->methods.PushLast(func->id);
	SetScriptFunction(func);

	// TODO: This code is repeated in many places
	// If parameter type from other groups are used, add references
	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(func->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( asUINT n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroupForObjectType(func->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

	// Check if the method restricts that use of the template to value types or reference types
	if( func->objectType->flags & asOBJ_TEMPLATE )
	{
		if( func->returnType.GetObjectType() == func->objectType->templateSubType.GetObjectType() )
		{
			if( func->returnType.IsObjectHandle() )
				func->objectType->acceptValueSubType = false;
			else if( !func->returnType.IsReference() )
				func->objectType->acceptRefSubType = false;
		}

		for( asUINT n = 0; n < func->parameterTypes.GetLength(); n++ )
		{
			if( func->parameterTypes[n].GetObjectType() == func->objectType->templateSubType.GetObjectType() )
			{
				// TODO: If unsafe references are allowed, then inout references allow value types
				if( func->parameterTypes[n].IsObjectHandle() || (func->parameterTypes[n].IsReference() && func->inOutFlags[n] == asTM_INOUTREF) )
					func->objectType->acceptValueSubType = false;
				else if( !func->parameterTypes[n].IsReference() )
					func->objectType->acceptRefSubType = false;
			}
		}
	}

	// TODO: beh.copy member will be removed, so this is not necessary
	// Is this the default copy behaviour?
	if( func->name == "opAssign" && func->parameterTypes.GetLength() == 1 && func->isReadOnly == false &&
		(objectType->flags & asOBJ_SCRIPT_OBJECT || func->parameterTypes[0].IsEqualExceptRefAndConst(asCDataType::CreateObject(func->objectType, false))) )
	{
		func->objectType->beh.copy = func->id;
		func->AddRef();
	}

	// Return the function id as success
	return func->id;
}

// interface
int asCScriptEngine::RegisterGlobalFunction(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

#ifdef AS_MAX_PORTABILITY
	if( callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#else
	if( callConv != asCALL_CDECL &&
		callConv != asCALL_STDCALL &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#endif

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = asNEW(asSSystemFunctionInterface)(internal);

	asCScriptFunction *func = asNEW(asCScriptFunction)(this, 0, asFUNC_SYSTEM);
	func->sysFuncIntf = newInterface;

	asCBuilder bld(this, 0);
	r = bld.ParseFunctionDeclaration(0, declaration, func, true, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
	if( r < 0 )
	{
		// Set as dummy function before deleting
		func->funcType = -1;
		asDELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	r = bld.CheckNameConflict(func->name.AddressOf(), 0, 0);
	if( r < 0 )
	{
		asDELETE(func,asCScriptFunction);
		return ConfigError(asNAME_TAKEN);
	}

	func->id = GetNextScriptFunctionId();
	SetScriptFunction(func);

	currentGroup->scriptFunctions.PushLast(func);
	registeredGlobalFuncs.PushLast(func);

	// If parameter type from other groups are used, add references
	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(func->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( asUINT n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroupForObjectType(func->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

	// Return the function id as success
	return func->id;
}

// interface
int asCScriptEngine::GetGlobalFunctionCount()
{
	return (int)registeredGlobalFuncs.GetLength();
}

// interface
int asCScriptEngine::GetGlobalFunctionIdByIndex(asUINT index)
{
	if( index >= registeredGlobalFuncs.GetLength() )
		return asINVALID_ARG;

	return registeredGlobalFuncs[index]->id;
}





asCObjectType *asCScriptEngine::GetObjectType(const char *type)
{
	// TODO: optimize: Improve linear search
	for( asUINT n = 0; n < objectTypes.GetLength(); n++ )
		if( objectTypes[n] &&
			objectTypes[n]->name == type ) // TODO: template: Should we check the subtype in case of template instances?
			return objectTypes[n];

	return 0;
}




void asCScriptEngine::PrepareEngine()
{
	if( isPrepared ) return;
	if( configFailed ) return;

	asUINT n;
	for( n = 0; n < scriptFunctions.GetLength(); n++ )
	{
		// Determine the host application interface
		if( scriptFunctions[n] && scriptFunctions[n]->funcType == asFUNC_SYSTEM )
		{
			if( scriptFunctions[n]->sysFuncIntf->callConv == ICC_GENERIC_FUNC ||
				scriptFunctions[n]->sysFuncIntf->callConv == ICC_GENERIC_METHOD )
				PrepareSystemFunctionGeneric(scriptFunctions[n], scriptFunctions[n]->sysFuncIntf, this);
			else
				PrepareSystemFunction(scriptFunctions[n], scriptFunctions[n]->sysFuncIntf, this);
		}
	}

	// Validate object type registrations
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && !(objectTypes[n]->flags & asOBJ_SCRIPT_OBJECT) )
		{
			bool missingBehaviour = false;
			const char *infoMsg = 0;

			// Verify that GC types have all behaviours
			if( objectTypes[n]->flags & asOBJ_GC )
			{
				if( objectTypes[n]->beh.addref                 == 0 ||
					objectTypes[n]->beh.release                == 0 ||
					objectTypes[n]->beh.gcGetRefCount          == 0 ||
					objectTypes[n]->beh.gcSetFlag              == 0 ||
					objectTypes[n]->beh.gcGetFlag              == 0 ||
					objectTypes[n]->beh.gcEnumReferences       == 0 ||
					objectTypes[n]->beh.gcReleaseAllReferences == 0 )
				{
					infoMsg = TXT_GC_REQUIRE_ADD_REL_GC_BEHAVIOUR;
					missingBehaviour = true;
				}
			}
			// Verify that scoped ref types have the release behaviour
			else if( objectTypes[n]->flags & asOBJ_SCOPED )
			{
				if( objectTypes[n]->beh.release == 0 )
				{
					infoMsg = TXT_SCOPE_REQUIRE_REL_BEHAVIOUR;
					missingBehaviour = true;
				}
			}
			// Verify that ref types have add ref and release behaviours
			else if( (objectTypes[n]->flags & asOBJ_REF) &&
				     !(objectTypes[n]->flags & asOBJ_NOHANDLE) )
			{
				if( objectTypes[n]->beh.addref  == 0 ||
					objectTypes[n]->beh.release == 0 )
				{
					infoMsg = TXT_REF_REQUIRE_ADD_REL_BEHAVIOUR;
					missingBehaviour = true;
				}
			}
			// Verify that non-pod value types have the constructor and destructor registered
			else if( (objectTypes[n]->flags & asOBJ_VALUE) &&
				     !(objectTypes[n]->flags & asOBJ_POD) )
			{
				if( objectTypes[n]->beh.construct == 0 ||
					objectTypes[n]->beh.destruct  == 0 )
				{
					infoMsg = TXT_NON_POD_REQUIRE_CONSTR_DESTR_BEHAVIOUR;
					missingBehaviour = true;
				}
			}

			if( missingBehaviour )
			{
				asCString str;
				str.Format(TXT_TYPE_s_IS_MISSING_BEHAVIOURS, objectTypes[n]->name.AddressOf());
				WriteMessage("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
				WriteMessage("", 0, 0, asMSGTYPE_INFORMATION, infoMsg);
				ConfigError(asINVALID_CONFIGURATION);
			}
		}
	}

	isPrepared = true;
}

int asCScriptEngine::ConfigError(int err)
{
	configFailed = true;
	return err;
}


// interface
int asCScriptEngine::RegisterStringFactory(const char *datatype, const asSFuncPtr &funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

#ifdef AS_MAX_PORTABILITY
	if( callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#else
	if( callConv != asCALL_CDECL &&
		callConv != asCALL_STDCALL &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#endif

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = asNEW(asSSystemFunctionInterface)(internal);

	asCScriptFunction *func = asNEW(asCScriptFunction)(this, 0, asFUNC_SYSTEM);
	func->name        = "_string_factory_";
	func->sysFuncIntf = newInterface;

	asCBuilder bld(this, 0);

	asCDataType dt;
	r = bld.ParseDataType(datatype, &dt);
	if( r < 0 )
	{
		// Set as dummy before deleting
		func->funcType = -1;
		asDELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_TYPE);
	}

	func->returnType = dt;
	func->parameterTypes.PushLast(asCDataType::CreatePrimitive(ttInt, true));
	asCDataType parm1 = asCDataType::CreatePrimitive(ttUInt8, true);
	parm1.MakeReference(true);
	func->parameterTypes.PushLast(parm1);
	func->id = GetNextScriptFunctionId();
	SetScriptFunction(func);

	stringFactory = func;

	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(func->returnType.GetObjectType());
		if( group == 0 ) group = &defaultGroup;
		group->scriptFunctions.PushLast(func);
	}

	// Register function id as success
	return func->id;
}

// interface
int asCScriptEngine::GetStringFactoryReturnTypeId()
{
	if( stringFactory == 0 )
		return asNO_FUNCTION;

	return GetTypeIdFromDataType(stringFactory->returnType);
}

// interface
asCModule *asCScriptEngine::GetModule(const char *_name, bool create)
{
	// Accept null as well as zero-length string
	const char *name = "";
	if( _name != 0 ) name = _name;

	if( lastModule && lastModule->name == name )
		return lastModule;

	// TODO: optimize: Improve linear search
	for( asUINT n = 0; n < scriptModules.GetLength(); ++n )
		if( scriptModules[n] && scriptModules[n]->name == name )
		{
			lastModule = scriptModules[n];
			return lastModule;
		}

	if( create )
	{
		asCModule *module = asNEW(asCModule)(name, this);

		scriptModules.PushLast(module);

		lastModule = module;

		return lastModule;
	}

	return 0;
}

asCModule *asCScriptEngine::GetModuleFromFuncId(int id)
{
	if( id < 0 ) return 0;
	if( id >= (int)scriptFunctions.GetLength() ) return 0;
	asCScriptFunction *func = scriptFunctions[id];
	if( func == 0 ) return 0;
	return func->module;
}

// internal
int asCScriptEngine::RequestBuild()
{
	ENTERCRITICALSECTION(engineCritical);
	if( isBuilding )
	{
		LEAVECRITICALSECTION(engineCritical);
		return asBUILD_IN_PROGRESS;
	}
	isBuilding = true;
	LEAVECRITICALSECTION(engineCritical);

	return 0;
}

// internal
void asCScriptEngine::BuildCompleted()
{
	// Always free up pooled memory after a completed build
	memoryMgr.FreeUnusedMemory();

	isBuilding = false;
}

#ifdef AS_DEPRECATED
// Deprecated since 2009-12-08, 2.18.0
// interface
int asCScriptEngine::ExecuteString(const char *module, const char *script, asIScriptContext **ctx, asDWORD flags)
{
	int r;
	if( (r = RequestBuild()) < 0 )
		return r;

	PrepareEngine();

	// Make sure the config worked
	if( configFailed )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
			*ctx = 0;

		WriteMessage("",0,0,asMSGTYPE_ERROR,TXT_INVALID_CONFIGURATION);
		isBuilding = false;
		return asINVALID_CONFIGURATION;
	}

	asIScriptContext *exec = 0;
	if( !(flags & asEXECSTRING_USE_MY_CONTEXT) )
	{
		int r = CreateContext(&exec, false);
		if( r < 0 )
		{
			if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
				*ctx = 0;
			isBuilding = false;
			return r;
		}
		if( ctx )
		{
			*ctx = exec;
			exec->AddRef();
		}
	}
	else
	{
		if( *ctx == 0 )
		{
			isBuilding = false;
			return asINVALID_ARG;
		}
		exec = *ctx;
		exec->AddRef();
	}

	// Make sure the context isn't holding a reference to the previous ExecuteString function()
	exec->Unprepare();

	// Get the module to compile the string in
	asCModule *mod = GetModule(module, true);

	// Compile string function
	asCBuilder builder(this, mod);
	asCString str = script;
	str = "void ExecuteString(){\n" + str + "\n;}";

	r = builder.BuildString(str.AddressOf(), (asCContext*)exec);
	
	BuildCompleted();

	if( r < 0 )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
		{
			(*ctx)->Release();
			*ctx = 0;
		}
		exec->Release();
		return asERROR;
	}

	// Prepare and execute the context
	r = ((asCContext*)exec)->Prepare(((asCContext*)exec)->stringFunction->id);
	if( r < 0 )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
		{
			(*ctx)->Release();
			*ctx = 0;
		}
		exec->Release();
		return r;
	}

	if( flags & asEXECSTRING_ONLY_PREPARE )
		r = asEXECUTION_PREPARED;
	else
		r = exec->Execute();

	exec->Release();
	return r;
}
#endif

void asCScriptEngine::RemoveTemplateInstanceType(asCObjectType *t)
{
	int n;

	// Destroy the factory stubs
	for( n = 0; n < (int)t->beh.factories.GetLength(); n++ )
	{
		// Make sure the factory stub isn't referencing this object anymore
		scriptFunctions[t->beh.factories[n]]->ReleaseAllHandles(this);
		scriptFunctions[t->beh.factories[n]]->Release();
	}
	t->beh.factories.SetLength(0);

	// Destroy the specialized functions
	for( n = 1; n < (int)t->beh.operators.GetLength(); n += 2 )
	{
		if( t->beh.operators[n] && scriptFunctions[t->beh.operators[n]]->objectType == t )
		{
			scriptFunctions[t->beh.operators[n]]->Release();
		}
	}
	t->beh.operators.SetLength(0);

	// Start searching from the end of the list, as most of
	// the time it will be the last two types
	for( n = (int)templateTypes.GetLength()-1; n >= 0; n-- )
	{
		if( templateTypes[n] == t )
		{
			if( n == (signed)templateTypes.GetLength()-1 )
				templateTypes.PopLast();
			else
				templateTypes[n] = templateTypes.PopLast();
		}
	}

	for( n = (int)templateInstanceTypes.GetLength()-1; n >= 0; n-- )
	{
		if( templateInstanceTypes[n] == t )
		{
			if( n == (signed)templateInstanceTypes.GetLength()-1 )
				templateInstanceTypes.PopLast();
			else
				templateInstanceTypes[n] = templateInstanceTypes.PopLast();
		}
	}

	asDELETE(t,asCObjectType);
}

asCObjectType *asCScriptEngine::GetTemplateInstanceType(asCObjectType *templateType, asCDataType &subType)
{
	asUINT n;

	// Is there any template instance type or template specialization already with this subtype?
	for( n = 0; n < templateTypes.GetLength(); n++ )
	{
		if( templateTypes[n] &&
			templateTypes[n]->name == templateType->name &&
			templateTypes[n]->templateSubType == subType )
			return templateTypes[n];
	}

	// No previous template instance exists

	// Make sure this template supports the subtype
	if( !templateType->acceptValueSubType && (subType.IsPrimitive() || (subType.GetObjectType()->flags & asOBJ_VALUE)) )
		return 0;

	if( !templateType->acceptRefSubType && (subType.IsObject() && (subType.GetObjectType()->flags & asOBJ_REF)) )
		return 0;

	// Create a new template instance type based on the templateType
	asCObjectType *ot = asNEW(asCObjectType)(this);
	ot->templateSubType = subType;
	ot->flags     = templateType->flags;
	ot->size      = templateType->size;
	ot->name      = templateType->name;

	// Before filling in the methods, call the template instance callback behaviour to validate the type
	if( templateType->beh.templateCallback )
	{
		asCScriptFunction *callback = scriptFunctions[templateType->beh.templateCallback];
		if( !CallGlobalFunctionRetBool(ot, 0, callback->sysFuncIntf, callback) )
		{
			// The type cannot be instanciated
			ot->templateSubType = asCDataType();
			asDELETE(ot, asCObjectType);
			return 0;
		}
	}

	ot->methods   = templateType->methods;
	for( n = 0; n < ot->methods.GetLength(); n++ )
		scriptFunctions[ot->methods[n]]->AddRef();
	// Store the real factory in the constructor
	ot->beh.construct = templateType->beh.factory;
	ot->beh.constructors = templateType->beh.factories;
	for( n = 0; n < ot->beh.constructors.GetLength(); n++ )
		scriptFunctions[ot->beh.constructors[n]]->AddRef();

	// Generate factory stubs for each of the factories
	for( n = 0; n < templateType->beh.factories.GetLength(); n++ )
	{
		int factoryId = templateType->beh.factories[n];
		asCScriptFunction *factory = scriptFunctions[factoryId];

		asCScriptFunction *func = asNEW(asCScriptFunction)(this, 0, asFUNC_SCRIPT);
		func->name             = "factstub";
		func->id               = GetNextScriptFunctionId();
		func->returnType       = asCDataType::CreateObjectHandle(ot, false);

		// Skip the first parameter as this is the object type pointer that the stub will add
		for( asUINT p = 1; p < factory->parameterTypes.GetLength(); p++ )
		{
			func->parameterTypes.PushLast(factory->parameterTypes[p]);
			func->inOutFlags.PushLast(factory->inOutFlags[p]);
		}

		SetScriptFunction(func);

		asCBuilder builder(this, 0);
		asCCompiler compiler(this);
		compiler.CompileTemplateFactoryStub(&builder, factoryId, ot, func);

		// The function's refCount was already initialized to 1
		ot->beh.factories.PushLast(func->id);
	}
	if( ot->beh.factories.GetLength() )
		ot->beh.factory = ot->beh.factories[0];
	else
	{
		asASSERT(false);
		ot->beh.factory = templateType->beh.factory;
	}



	ot->beh.addref                 = templateType->beh.addref;
	if( scriptFunctions[ot->beh.addref] ) scriptFunctions[ot->beh.addref]->AddRef();
	ot->beh.release                = templateType->beh.release;
	if( scriptFunctions[ot->beh.release] ) scriptFunctions[ot->beh.release]->AddRef();
	ot->beh.copy                   = templateType->beh.copy;
	if( scriptFunctions[ot->beh.copy] ) scriptFunctions[ot->beh.copy]->AddRef();
	ot->beh.operators              = templateType->beh.operators;
	for( n = 1; n < ot->beh.operators.GetLength(); n += 2 )
	{
		scriptFunctions[ot->beh.operators[n]]->AddRef();
	}
	ot->beh.gcGetRefCount          = templateType->beh.gcGetRefCount;
	if( scriptFunctions[ot->beh.gcGetRefCount] ) scriptFunctions[ot->beh.gcGetRefCount]->AddRef();
	ot->beh.gcSetFlag              = templateType->beh.gcSetFlag;
	if( scriptFunctions[ot->beh.gcSetFlag] ) scriptFunctions[ot->beh.gcSetFlag]->AddRef();
	ot->beh.gcGetFlag              = templateType->beh.gcGetFlag;
	if( scriptFunctions[ot->beh.gcGetFlag] ) scriptFunctions[ot->beh.gcGetFlag]->AddRef();
	ot->beh.gcEnumReferences       = templateType->beh.gcEnumReferences;
	if( scriptFunctions[ot->beh.gcEnumReferences] ) scriptFunctions[ot->beh.gcEnumReferences]->AddRef();
	ot->beh.gcReleaseAllReferences = templateType->beh.gcReleaseAllReferences;
	if( scriptFunctions[ot->beh.gcReleaseAllReferences] ) scriptFunctions[ot->beh.gcReleaseAllReferences]->AddRef();

	// As the new template type is instanciated, the engine should
	// generate new functions to substitute the ones with the template subtype.
	for( n = 1; n < ot->beh.operators.GetLength(); n += 2 )
	{
		int funcId = ot->beh.operators[n];
		asCScriptFunction *func = scriptFunctions[funcId];

		if( GenerateNewTemplateFunction(templateType, ot, subType, func, &func) )
		{
			// Release the old function, the new one already has its ref count set to 1
			scriptFunctions[ot->beh.operators[n]]->Release();
			ot->beh.operators[n] = func->id;
		}
	}

	// As the new template type is instanciated, the engine should
	// generate new functions to substitute the ones with the template subtype.
	for( n = 0; n < ot->methods.GetLength(); n++ )
	{
		int funcId = ot->methods[n];
		asCScriptFunction *func = scriptFunctions[funcId];

		if( GenerateNewTemplateFunction(templateType, ot, subType, func, &func) )
		{
			// Release the old function, the new one already has its ref count set to 1
			scriptFunctions[ot->methods[n]]->Release();
			ot->methods[n] = func->id;
		}
	}

	// Increase ref counter for sub type if it is an object type
	if( ot->templateSubType.GetObjectType() ) ot->templateSubType.GetObjectType()->AddRef();

	// Verify if the subtype contains a garbage collected object, in which case this template is a potential circular reference
	// TODO: We may be a bit smarter here. If we can guarantee that the array type cannot be part of the
	//       potential circular reference then we don't need to set the flag

	if( ot->templateSubType.GetObjectType() && (ot->templateSubType.GetObjectType()->flags & asOBJ_GC) )
		ot->flags |= asOBJ_GC;
	else if( ot->name == defaultArrayObjectType->name )
		ot->flags &= ~asOBJ_GC;

	templateTypes.PushLast(ot);

	// We need to store the object type somewhere for clean-up later
	// TODO: Why do we need both templateTypes and templateInstanceTypes? It is possible to differ between template instance and template specialization by checking for the asOBJ_TEMPLATE flag
	templateInstanceTypes.PushLast(ot);

	return ot;
}

bool asCScriptEngine::GenerateNewTemplateFunction(asCObjectType *templateType, asCObjectType *ot, asCDataType &subType, asCScriptFunction *func, asCScriptFunction **newFunc)
{
	bool needNewFunc = false;
	if( func->returnType.GetObjectType() == templateType->templateSubType.GetObjectType() ||
		func->returnType.GetObjectType() == templateType )
		needNewFunc = true;
	else
	{
		for( asUINT p = 0; p < func->parameterTypes.GetLength(); p++ )
		{
			if( func->parameterTypes[p].GetObjectType() == templateType->templateSubType.GetObjectType() ||
				func->parameterTypes[p].GetObjectType() == templateType )
			{
				needNewFunc = true;
				break;
			}
		}
	}

	if( needNewFunc )
	{
		asCScriptFunction *func2 = asNEW(asCScriptFunction)(this, 0, func->funcType);
		func2->name     = func->name;
		func2->id       = GetNextScriptFunctionId();

		if( func->returnType.GetObjectType() == templateType->templateSubType.GetObjectType() )
		{
			func2->returnType = subType;
			if( func->returnType.IsObjectHandle() )
				func2->returnType.MakeHandle(true, true);
			func2->returnType.MakeReference(func->returnType.IsReference());
			func2->returnType.MakeReadOnly(func->returnType.IsReadOnly());
		}
		else if( func->returnType.GetObjectType() == templateType )
		{
			if( func2->returnType.IsObjectHandle() )
				func2->returnType = asCDataType::CreateObjectHandle(ot, false);
			else
				func2->returnType = asCDataType::CreateObject(ot, false);

			func2->returnType.MakeReference(func->returnType.IsReference());
			func2->returnType.MakeReadOnly(func->returnType.IsReadOnly());
		}
		else
			func2->returnType = func->returnType;

		func2->parameterTypes.SetLength(func->parameterTypes.GetLength());
		for( asUINT p = 0; p < func->parameterTypes.GetLength(); p++ )
		{
			if( func->parameterTypes[p].GetObjectType() == templateType->templateSubType.GetObjectType() )
			{
				func2->parameterTypes[p] = subType;
				if( func->parameterTypes[p].IsObjectHandle() )
					func2->parameterTypes[p].MakeHandle(true);
				func2->parameterTypes[p].MakeReference(func->parameterTypes[p].IsReference());
				func2->parameterTypes[p].MakeReadOnly(func->parameterTypes[p].IsReference());
			}
			else if( func->parameterTypes[p].GetObjectType() == templateType )
			{
				if( func2->parameterTypes[p].IsObjectHandle() )
					func2->parameterTypes[p] = asCDataType::CreateObjectHandle(ot, false);
				else
					func2->parameterTypes[p] = asCDataType::CreateObject(ot, false);

				func2->parameterTypes[p].MakeReference(func->parameterTypes[p].IsReference());
				func2->parameterTypes[p].MakeReadOnly(func->parameterTypes[p].IsReadOnly());
			}
			else
				func2->parameterTypes[p] = func->parameterTypes[p];
		}

		// TODO: template: Must be careful when instanciating templates for garbage collected types
		//                 If the template hasn't been registered with the behaviours, it shouldn't
		//                 permit instanciation of garbage collected types that in turn may refer to
		//                 this instance.

		func2->inOutFlags = func->inOutFlags;
		func2->isReadOnly = func->isReadOnly;
		func2->objectType = ot;
		func2->stackNeeded = func->stackNeeded;
		func2->sysFuncIntf = asNEW(asSSystemFunctionInterface)(*func->sysFuncIntf);

		SetScriptFunction(func2);

		// Return the new function
		*newFunc = func2;
	}

	return needNewFunc;
}

void asCScriptEngine::CallObjectMethod(void *obj, int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	CallObjectMethod(obj, s->sysFuncIntf, s);
}

void asCScriptEngine::CallObjectMethod(void *obj, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
#ifdef __GNUC__
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else if( i->callConv == ICC_VIRTUAL_THISCALL )
	{
		// For virtual thiscalls we must call the method as a true class method
		// so that the compiler will lookup the function address in the vftable
		union
		{
			asSIMPLEMETHOD_t mthd;
			struct
			{
				asFUNCTION_t func;
				asPWORD baseOffset;  // Same size as the pointer
			} f;
		} p;
		p.f.func = (void (*)())(i->func);
		p.f.baseOffset = asPWORD(i->baseOffset);
		void (asCSimpleDummy::*f)() = p.mthd;
		(((asCSimpleDummy*)obj)->*f)();
	}
	else /*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		// Non-virtual thiscall can be called just like any global function, passing the object as the first parameter
		void (*f)(void *) = (void (*)(void *))(i->func);
		f(obj);
	}
#else
#ifndef AS_NO_CLASS_METHODS
	if( i->callConv == ICC_THISCALL )
	{
		union
		{
			asSIMPLEMETHOD_t mthd;
			asFUNCTION_t func;
		} p;
		p.func = (void (*)())(i->func);
		void (asCSimpleDummy::*f)() = p.mthd;
		(((asCSimpleDummy*)obj)->*f)();
	}
	else
#endif
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		void (*f)(void *) = (void (*)(void *))(i->func);
		f(obj);
	}
#endif
}

bool asCScriptEngine::CallObjectMethodRetBool(void *obj, int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	asSSystemFunctionInterface *i = s->sysFuncIntf;

#ifdef __GNUC__
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(bool*)gen.GetReturnPointer();
	}
	else if( i->callConv == ICC_VIRTUAL_THISCALL )
	{
		// For virtual thiscalls we must call the method as a true class method so that the compiler will lookup the function address in the vftable
		union
		{
			asSIMPLEMETHOD_t mthd;
			struct
			{
				asFUNCTION_t func;
				asDWORD baseOffset;
			} f;
		} p;
		p.f.func = (void (*)())(i->func);
		p.f.baseOffset = i->baseOffset;
		bool (asCSimpleDummy::*f)() = (bool (asCSimpleDummy::*)())(p.mthd);
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else /*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		// Non-virtual thiscall can be called just like any global function, passing the object as the first parameter
		bool (*f)(void *) = (bool (*)(void *))(i->func);
		return f(obj);
	}
#else
#ifndef AS_NO_CLASS_METHODS
	if( i->callConv == ICC_THISCALL )
	{
		union
		{
			asSIMPLEMETHOD_t mthd;
			asFUNCTION_t func;
		} p;
		p.func = (void (*)())(i->func);
		bool (asCSimpleDummy::*f)() = (bool (asCSimpleDummy::*)())p.mthd;
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else
#endif
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(bool*)gen.GetReturnPointer();
	}
	else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		bool (*f)(void *) = (bool (*)(void *))(i->func);
		return f(obj);
	}
#endif
}

int asCScriptEngine::CallObjectMethodRetInt(void *obj, int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	asSSystemFunctionInterface *i = s->sysFuncIntf;

#ifdef __GNUC__
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(int*)gen.GetReturnPointer();
	}
	else if( i->callConv == ICC_VIRTUAL_THISCALL )
	{
		// For virtual thiscalls we must call the method as a true class method so that the compiler will lookup the function address in the vftable
		union
		{
			asSIMPLEMETHOD_t mthd;
			struct
			{
				asFUNCTION_t func;
				asDWORD baseOffset;
			} f;
		} p;
		p.f.func = (void (*)())(i->func);
		p.f.baseOffset = i->baseOffset;
		int (asCSimpleDummy::*f)() = (int (asCSimpleDummy::*)())(p.mthd);
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else /*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		// Non-virtual thiscall can be called just like any global function, passing the object as the first parameter
		int (*f)(void *) = (int (*)(void *))(i->func);
		return f(obj);
	}
#else
#ifndef AS_NO_CLASS_METHODS
	if( i->callConv == ICC_THISCALL )
	{
		union
		{
			asSIMPLEMETHOD_t mthd;
			asFUNCTION_t func;
		} p;
		p.func = (void (*)())(i->func);
		int (asCSimpleDummy::*f)() = (int (asCSimpleDummy::*)())p.mthd;
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else
#endif
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(int*)gen.GetReturnPointer();
	}
	else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		int (*f)(void *) = (int (*)(void *))(i->func);
		return f(obj);
	}
#endif
}

void *asCScriptEngine::CallGlobalFunctionRetPtr(int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	return CallGlobalFunctionRetPtr(s->sysFuncIntf, s);
}

void *asCScriptEngine::CallGlobalFunctionRetPtr(int func, void *param1)
{
	asCScriptFunction *s = scriptFunctions[func];
	return CallGlobalFunctionRetPtr(s->sysFuncIntf, s, param1);
}

void *asCScriptEngine::CallGlobalFunctionRetPtr(asSSystemFunctionInterface *i, asCScriptFunction *s)
{
	if( i->callConv == ICC_CDECL )
	{
		void *(*f)() = (void *(*)())(i->func);
		return f();
	}
	else if( i->callConv == ICC_STDCALL )
	{
		void *(STDCALL *f)() = (void *(STDCALL *)())(i->func);
		return f();
	}
	else
	{
		asCGeneric gen(this, s, 0, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(void**)gen.GetReturnPointer();
	}
}

void *asCScriptEngine::CallGlobalFunctionRetPtr(asSSystemFunctionInterface *i, asCScriptFunction *s, void *param1)
{
	if( i->callConv == ICC_CDECL )
	{
		void *(*f)(void *) = (void *(*)(void *))(i->func);
		return f(param1);
	}
	else if( i->callConv == ICC_STDCALL )
	{
		void *(STDCALL *f)(void *) = (void *(STDCALL *)(void *))(i->func);
		return f(param1);
	}
	else
	{
		asCGeneric gen(this, s, 0, (asDWORD*)&param1);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(void**)gen.GetReturnPointer();
	}
}

void asCScriptEngine::CallObjectMethod(void *obj, void *param, int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	CallObjectMethod(obj, param, s->sysFuncIntf, s);
}

void asCScriptEngine::CallObjectMethod(void *obj, void *param, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
#ifdef __GNUC__
	if( i->callConv == ICC_CDECL_OBJLAST )
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(param, obj);
	}
	else if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, (asDWORD*)&param);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else /*if( i->callConv == ICC_CDECL_OBJFIRST || i->callConv == ICC_THISCALL )*/
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(obj, param);
	}
#else
#ifndef AS_NO_CLASS_METHODS
	if( i->callConv == ICC_THISCALL )
	{
		union
		{
			asSIMPLEMETHOD_t mthd;
			asFUNCTION_t func;
		} p;
		p.func = (void (*)())(i->func);
		void (asCSimpleDummy::*f)(void *) = (void (asCSimpleDummy::*)(void *))(p.mthd);
		(((asCSimpleDummy*)obj)->*f)(param);
	}
	else
#endif
	if( i->callConv == ICC_CDECL_OBJLAST )
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(param, obj);
	}
	else if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, (asDWORD*)&param);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else /*if( i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(obj, param);
	}
#endif
}

void asCScriptEngine::CallGlobalFunction(void *param1, void *param2, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
	if( i->callConv == ICC_CDECL )
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(param1, param2);
	}
	else if( i->callConv == ICC_STDCALL )
	{
		void (STDCALL *f)(void *, void *) = (void (STDCALL *)(void *, void *))(i->func);
		f(param1, param2);
	}
	else
	{
		asCGeneric gen(this, s, 0, (asDWORD*)&param1);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
}

bool asCScriptEngine::CallGlobalFunctionRetBool(void *param1, void *param2, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
	if( i->callConv == ICC_CDECL )
	{
		bool (*f)(void *, void *) = (bool (*)(void *, void *))(i->func);
		return f(param1, param2);
	}
	else if( i->callConv == ICC_STDCALL )
	{
		bool (STDCALL *f)(void *, void *) = (bool (STDCALL *)(void *, void *))(i->func);
		return f(param1, param2);
	}
	else
	{
		// TODO: When simulating a 64bit environment by defining AS_64BIT_PTR on a 32bit platform this code
		//       fails, because the stack given to asCGeneric is not prepared with two 64bit arguments.
		asCGeneric gen(this, s, 0, (asDWORD*)&param1);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(bool*)gen.GetReturnPointer();
	}
}

void *asCScriptEngine::CallAlloc(asCObjectType *type)
{
    // Allocate 4 bytes as the smallest size. Otherwise CallSystemFunction may try to
    // copy a DWORD onto a smaller memory block, in case the object type is return in registers.
#if defined(AS_DEBUG)
	return ((asALLOCFUNCDEBUG_t)(userAlloc))(type->size < 4 ? 4 : type->size, __FILE__, __LINE__);
#else
	return userAlloc(type->size < 4 ? 4 : type->size);
#endif
}

void asCScriptEngine::CallFree(void *obj)
{
	userFree(obj);
}

// interface
void asCScriptEngine::NotifyGarbageCollectorOfNewObject(void *obj, int typeId)
{
	asCObjectType *objType = GetObjectTypeFromTypeId(typeId);
	gc.AddScriptObjectToGC(obj, objType);
}

// interface
int asCScriptEngine::GarbageCollect(asDWORD flags)
{
	return gc.GarbageCollect(flags);
}

// interface
void asCScriptEngine::GetGCStatistics(asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected)
{
	gc.GetStatistics(currentSize, totalDestroyed, totalDetected);
}

// interface
void asCScriptEngine::GCEnumCallback(void *reference)
{
	gc.GCEnumCallback(reference);
}


// TODO: multithread: The mapTypeIdToDataType must be protected with critical sections in all functions that access it
int asCScriptEngine::GetTypeIdFromDataType(const asCDataType &dt)
{
	if( dt.IsNullHandle() ) return 0;

	// Find the existing type id
	asSMapNode<int,asCDataType*> *cursor = 0;
	mapTypeIdToDataType.MoveFirst(&cursor);
	while( cursor )
	{
		if( mapTypeIdToDataType.GetValue(cursor)->IsEqualExceptRefAndConst(dt) )
			return mapTypeIdToDataType.GetKey(cursor);

		mapTypeIdToDataType.MoveNext(&cursor, cursor);
	}

	// The type id doesn't exist, create it

	// Setup the basic type id
	int typeId = typeIdSeqNbr++;
	if( dt.GetObjectType() )
	{
		if( dt.GetObjectType()->flags & asOBJ_SCRIPT_OBJECT ) typeId |= asTYPEID_SCRIPTOBJECT;
		else if( dt.GetObjectType()->flags & asOBJ_TEMPLATE ) typeId |= asTYPEID_SCRIPTARRAY; // TODO: Should be asTYPEID_TEMPLATE
		else if( dt.GetObjectType()->flags & asOBJ_ENUM ); // TODO: Should we have a specific bit for this?
		else typeId |= asTYPEID_APPOBJECT;
	}

	// Insert the basic object type
	asCDataType *newDt = asNEW(asCDataType)(dt);
	newDt->MakeReference(false);
	newDt->MakeReadOnly(false);
	newDt->MakeHandle(false);

	mapTypeIdToDataType.Insert(typeId, newDt);

	// If the object type supports object handles then register those types as well
	// Note: Don't check for addref, as asOBJ_SCOPED don't have this
	if( dt.IsObject() && dt.GetObjectType()->beh.release )
	{
		newDt = asNEW(asCDataType)(dt);
		newDt->MakeReference(false);
		newDt->MakeReadOnly(false);
		newDt->MakeHandle(true);
		newDt->MakeHandleToConst(false);

		mapTypeIdToDataType.Insert(typeId | asTYPEID_OBJHANDLE, newDt);

		newDt = asNEW(asCDataType)(dt);
		newDt->MakeReference(false);
		newDt->MakeReadOnly(false);
		newDt->MakeHandle(true);
		newDt->MakeHandleToConst(true);

		mapTypeIdToDataType.Insert(typeId | asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST, newDt);
	}

	// Call the method recursively to get the correct type id
	return GetTypeIdFromDataType(dt);
}

const asCDataType *asCScriptEngine::GetDataTypeFromTypeId(int typeId)
{
	asSMapNode<int,asCDataType*> *cursor = 0;
	if( mapTypeIdToDataType.MoveTo(&cursor, typeId) )
		return mapTypeIdToDataType.GetValue(cursor);

	return 0;
}

asCObjectType *asCScriptEngine::GetObjectTypeFromTypeId(int typeId)
{
	asSMapNode<int,asCDataType*> *cursor = 0;
	if( mapTypeIdToDataType.MoveTo(&cursor, typeId) )
		return mapTypeIdToDataType.GetValue(cursor)->GetObjectType();

	return 0;
}

void asCScriptEngine::RemoveFromTypeIdMap(asCObjectType *type)
{
	asSMapNode<int,asCDataType*> *cursor = 0;
	mapTypeIdToDataType.MoveFirst(&cursor);
	while( cursor )
	{
		asCDataType *dt = mapTypeIdToDataType.GetValue(cursor);
		asSMapNode<int,asCDataType*> *old = cursor;
		mapTypeIdToDataType.MoveNext(&cursor, cursor);
		if( dt->GetObjectType() == type )
		{
			asDELETE(dt,asCDataType);
			mapTypeIdToDataType.Erase(old);
		}
	}
}

// interface
int asCScriptEngine::GetTypeIdByDecl(const char *decl)
{
	asCDataType dt;
	asCBuilder bld(this, 0);
	int r = bld.ParseDataType(decl, &dt);
	if( r < 0 )
		return asINVALID_TYPE;

	return GetTypeIdFromDataType(dt);
}



const char *asCScriptEngine::GetTypeDeclaration(int typeId)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return 0;

	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = dt->Format();

	return tempString->AddressOf();
}

int asCScriptEngine::GetSizeOfPrimitiveType(int typeId)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return 0;
	if( !dt->IsPrimitive() ) return 0;

	return dt->GetSizeInMemoryBytes();
}

void *asCScriptEngine::CreateScriptObject(int typeId)
{
	// Make sure the type id is for an object type, and not a primitive or a handle
	if( (typeId & (asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) != typeId ) return 0;
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return 0;

	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return 0;

	// Allocate the memory
	asCObjectType *objType = dt->GetObjectType();
	void *ptr = 0;

	// Construct the object
	if( objType->flags & asOBJ_SCRIPT_OBJECT )
		ptr = ScriptObjectFactory(objType, this);
	else if( objType->flags & asOBJ_TEMPLATE )
		// The registered factory is moved to the construct behaviour when the type is instanciated
		ptr = CallGlobalFunctionRetPtr(objType->beh.construct, objType);
	else if( objType->flags & asOBJ_REF )
		ptr = CallGlobalFunctionRetPtr(objType->beh.factory);
	else
	{
		ptr = CallAlloc(objType);
		int funcIndex = objType->beh.construct;
		if( funcIndex )
			CallObjectMethod(ptr, funcIndex);
	}

	return ptr;
}

void *asCScriptEngine::CreateScriptObjectCopy(void *origObj, int typeId)
{
	void *newObj = CreateScriptObject(typeId);
	if( newObj == 0 ) return 0;

	CopyScriptObject(newObj, origObj, typeId);

	return newObj;
}

void asCScriptEngine::CopyScriptObject(void *dstObj, void *srcObj, int typeId)
{
	// Make sure the type id is for an object type, and not a primitive or a handle
	if( (typeId & (asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) != typeId ) return;
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return;

	// Copy the contents from the original object, using the assignment operator
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return;

	asCObjectType *objType = dt->GetObjectType();
	// TODO: beh.copy will be removed, so we need to find the default opAssign method instead
	if( objType->beh.copy )
	{
		CallObjectMethod(dstObj, srcObj, objType->beh.copy);
	}
	else if( objType->size )
	{
		memcpy(dstObj, srcObj, objType->size);
	}
}

void asCScriptEngine::AddRefScriptObject(void *obj, int typeId)
{
	// Make sure it is not a null pointer
	if( obj == 0 ) return;

	// Make sure the type id is for an object type or a handle
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return;

	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return;

	asCObjectType *objType = dt->GetObjectType();

	if( objType->beh.addref )
	{
		// Call the addref behaviour
		CallObjectMethod(obj, objType->beh.addref);
	}
}

void asCScriptEngine::ReleaseScriptObject(void *obj, int typeId)
{
	// Make sure it is not a null pointer
	if( obj == 0 ) return;

	// Make sure the type id is for an object type or a handle
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return;

	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return;

	asCObjectType *objType = dt->GetObjectType();

	if( objType->beh.release )
	{
		// Call the release behaviour
		CallObjectMethod(obj, objType->beh.release);
	}
	else
	{
		// Call the destructor
		if( objType->beh.destruct )
			CallObjectMethod(obj, objType->beh.destruct);

		// Then free the memory
		CallFree(obj);
	}
}

bool asCScriptEngine::IsHandleCompatibleWithObject(void *obj, int objTypeId, int handleTypeId)
{
	// if equal, then it is obvious they are compatible
	if( objTypeId == handleTypeId )
		return true;

	// Get the actual data types from the type ids
	const asCDataType *objDt = GetDataTypeFromTypeId(objTypeId);
	const asCDataType *hdlDt = GetDataTypeFromTypeId(handleTypeId);

	// A handle to const cannot be passed to a handle that is not referencing a const object
	if( objDt->IsHandleToConst() && !hdlDt->IsHandleToConst() )
		return false;

	if( objDt->GetObjectType() == hdlDt->GetObjectType() )
	{
		// The object type is equal
		return true;
	}
	else if( objDt->IsScriptObject() && obj )
	{
		// There's still a chance the object implements the requested interface
		asCObjectType *objType = ((asCScriptObject*)obj)->objType;
		if( objType->Implements(hdlDt->GetObjectType()) )
			return true;
	}

	return false;
}


int asCScriptEngine::BeginConfigGroup(const char *groupName)
{
	// Make sure the group name doesn't already exist
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		if( configGroups[n]->groupName == groupName )
			return asNAME_TAKEN;
	}

	if( currentGroup != &defaultGroup )
		return asNOT_SUPPORTED;

	asCConfigGroup *group = asNEW(asCConfigGroup)();
	group->groupName = groupName;

	configGroups.PushLast(group);
	currentGroup = group;

	return 0;
}

int asCScriptEngine::EndConfigGroup()
{
	// Raise error if trying to end the default config
	if( currentGroup == &defaultGroup )
		return asNOT_SUPPORTED;

	currentGroup = &defaultGroup;

	return 0;
}

int asCScriptEngine::RemoveConfigGroup(const char *groupName)
{
	// It is not allowed to remove a group that is still in use. 

	// It would be possible to change the code in such a way that
	// the group could be removed even though it was still in use,
	// but that would cause severe negative impact on runtime 
	// performance, since the VM would then have to be able handle
	// situations where the types, functions, and global variables
	// can be removed at any time.

	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		if( configGroups[n]->groupName == groupName )
		{
			asCConfigGroup *group = configGroups[n];

			// Make sure the group isn't referenced by anyone
			if( group->refCount > 0 )
				return asCONFIG_GROUP_IS_IN_USE;

			// Verify if any objects registered in this group is still alive
			if( group->HasLiveObjects() )
				return asCONFIG_GROUP_IS_IN_USE;

			// Remove the group from the list
			if( n == configGroups.GetLength() - 1 )
				configGroups.PopLast();
			else
				configGroups[n] = configGroups.PopLast();

			// Remove the configurations registered with this group
			group->RemoveConfiguration(this);

			asDELETE(group,asCConfigGroup);
		}
	}

	return 0;
}

asCConfigGroup *asCScriptEngine::FindConfigGroupForFunction(int funcId)
{
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		// Check global functions
		asUINT m;
		for( m = 0; m < configGroups[n]->scriptFunctions.GetLength(); m++ )
		{
			if( configGroups[n]->scriptFunctions[m]->id == funcId )
				return configGroups[n];
		}
	}

	return 0;
}


asCConfigGroup *asCScriptEngine::FindConfigGroupForGlobalVar(int gvarId)
{
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		for( asUINT m = 0; m < configGroups[n]->globalProps.GetLength(); m++ )
		{
			if( configGroups[n]->globalProps[m]->id == gvarId )
				return configGroups[n];
		}
	}

	return 0;
}

asCConfigGroup *asCScriptEngine::FindConfigGroupForObjectType(const asCObjectType *objType)
{
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		for( asUINT m = 0; m < configGroups[n]->objTypes.GetLength(); m++ )
		{
			if( configGroups[n]->objTypes[m] == objType )
				return configGroups[n];
		}
	}

	return 0;
}

int asCScriptEngine::SetConfigGroupModuleAccess(const char *groupName, const char *module, bool hasAccess)
{
	asCConfigGroup *group = 0;

	// Make sure the group name doesn't already exist
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		if( configGroups[n]->groupName == groupName )
		{
			group = configGroups[n];
			break;
		}
	}

	if( group == 0 )
		return asWRONG_CONFIG_GROUP;

	return group->SetModuleAccess(module, hasAccess);
}

int asCScriptEngine::GetNextScriptFunctionId()
{
	if( freeScriptFunctionIds.GetLength() )
		return freeScriptFunctionIds.PopLast();

	int id = (int)scriptFunctions.GetLength();
	scriptFunctions.PushLast(0);
	return id;
}

void asCScriptEngine::SetScriptFunction(asCScriptFunction *func)
{
	scriptFunctions[func->id] = func;
}

void asCScriptEngine::FreeScriptFunctionId(int id)
{
	if( id < 0 ) return;
	id &= 0xFFFF;
	if( id >= (int)scriptFunctions.GetLength() ) return;

	if( scriptFunctions[id] )
	{
		asCScriptFunction *func = scriptFunctions[id];

		// Remove the function from the list of script functions
		if( id == (int)scriptFunctions.GetLength() - 1 )
		{
			scriptFunctions.PopLast();
		}
		else
		{
			scriptFunctions[id] = 0;
			freeScriptFunctionIds.PushLast(id);
		}

		// Is the function used as signature id?
		if( func->signatureId == id )
		{
			// Remove the signature id
			signatureIds.RemoveValue(func);

			// Update all functions using the signature id
			int newSigId = 0;
			for( asUINT n = 0; n < scriptFunctions.GetLength(); n++ )
			{
				if( scriptFunctions[n] && scriptFunctions[n]->signatureId == id )
				{
					if( newSigId == 0 )
					{
						newSigId = scriptFunctions[n]->id;
						signatureIds.PushLast(scriptFunctions[n]);
					}

					scriptFunctions[n]->signatureId = newSigId;
				}
			}
		}
	}
}

// interface
// TODO: typedef: Accept complex types for the typedefs
int asCScriptEngine::RegisterTypedef(const char *type, const char *decl)
{
	if( type == 0 ) return ConfigError(asINVALID_NAME);

	// Verify if the name has been registered as a type already
	asUINT n;
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && objectTypes[n]->name == type )
			return asALREADY_REGISTERED;
	}

	// Grab the data type
	asCTokenizer t;
	size_t tokenLen;
	eTokenType token;
	asCDataType dataType;

	//	Create the data type
	token = t.GetToken(decl, strlen(decl), &tokenLen);
	switch(token)
	{
	case ttBool:
	case ttInt:
	case ttInt8:
	case ttInt16:
	case ttInt64:
	case ttUInt:
	case ttUInt8:
	case ttUInt16:
	case ttUInt64:
	case ttFloat:
	case ttDouble:
		if( strlen(decl) != tokenLen )
		{
			return ConfigError(asINVALID_TYPE);
		}
		break;

	default:
		return ConfigError(asINVALID_TYPE);
	}

	dataType = asCDataType::CreatePrimitive(token, false);

	// Make sure the name is not a reserved keyword
	token = t.GetToken(type, strlen(type), &tokenLen);
	if( token != ttIdentifier || strlen(type) != tokenLen )
		return ConfigError(asINVALID_NAME);

	asCBuilder bld(this, 0);
	int r = bld.CheckNameConflict(type, 0, 0);
	if( r < 0 )
		return ConfigError(asNAME_TAKEN);

	// Don't have to check against members of object
	// types as they are allowed to use the names

	// Put the data type in the list
	asCObjectType *object= asNEW(asCObjectType)(this);
	object->flags = asOBJ_TYPEDEF;
	object->size = dataType.GetSizeInMemoryBytes();
	object->name = type;
	object->templateSubType = dataType;

	objectTypes.PushLast(object);
	registeredTypeDefs.PushLast(object);

	currentGroup->objTypes.PushLast(object);

	return asSUCCESS;
}

// interface
int asCScriptEngine::GetTypedefCount()
{
	return (int)registeredTypeDefs.GetLength();
}

// interface
const char *asCScriptEngine::GetTypedefByIndex(asUINT index, int *typeId, const char **configGroup)
{
	if( index >= registeredTypeDefs.GetLength() )
		return 0;

	if( typeId )
		*typeId = GetTypeIdByDecl(registeredTypeDefs[index]->name.AddressOf());

	if( configGroup )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(registeredTypeDefs[index]);
		if( group )
			*configGroup = group->groupName.AddressOf();
		else
			*configGroup = 0;
	}

	return registeredTypeDefs[index]->name.AddressOf();
}

// interface
int asCScriptEngine::RegisterEnum(const char *name)
{
	//	Check the name
	if( NULL == name )
		return ConfigError(asINVALID_NAME);

	// Verify if the name has been registered as a type already
	asUINT n;
	for( n = 0; n < objectTypes.GetLength(); n++ )
		if( objectTypes[n] && objectTypes[n]->name == name )
			return asALREADY_REGISTERED;

	// Use builder to parse the datatype
	asCDataType dt;
	asCBuilder bld(this, 0);
	bool oldMsgCallback = msgCallback; msgCallback = false;
	int r = bld.ParseDataType(name, &dt);
	msgCallback = oldMsgCallback;
	if( r >= 0 )
		return ConfigError(asERROR);

	// Make sure the name is not a reserved keyword
	asCTokenizer t;
	size_t tokenLen;
	int token = t.GetToken(name, strlen(name), &tokenLen);
	if( token != ttIdentifier || strlen(name) != tokenLen )
		return ConfigError(asINVALID_NAME);

	r = bld.CheckNameConflict(name, 0, 0);
	if( r < 0 )
		return ConfigError(asNAME_TAKEN);

	asCObjectType *st = asNEW(asCObjectType)(this);

	asCDataType dataType;
	dataType.CreatePrimitive(ttInt, false);

	st->flags = asOBJ_ENUM;
	st->size = dataType.GetSizeInMemoryBytes();
	st->name = name;

	objectTypes.PushLast(st);
	registeredEnums.PushLast(st);

	currentGroup->objTypes.PushLast(st);

	return asSUCCESS;
}

// interface
int asCScriptEngine::RegisterEnumValue(const char *typeName, const char *valueName, int value)
{
	// Verify that the correct config group is used
	if( currentGroup->FindType(typeName) == 0 )
		return asWRONG_CONFIG_GROUP;

	asCDataType dt;
	int r;
	asCBuilder bld(this, 0);
	r = bld.ParseDataType(typeName, &dt);
	if( r < 0 )
		return ConfigError(r);

	// Store the enum value
	asCObjectType *ot = dt.GetObjectType();
	if( ot == 0 || !(ot->flags & asOBJ_ENUM) )
		return ConfigError(asINVALID_TYPE);

	if( NULL == valueName )
		return ConfigError(asINVALID_NAME);

	for( unsigned int n = 0; n < ot->enumValues.GetLength(); n++ )
	{
		if( ot->enumValues[n]->name == valueName )
			return ConfigError(asALREADY_REGISTERED);
	}

	asSEnumValue *e = asNEW(asSEnumValue);
	e->name = valueName;
	e->value = value;

	ot->enumValues.PushLast(e);

	return asSUCCESS;
}

// interface
int asCScriptEngine::GetEnumCount()
{
	return (int)registeredEnums.GetLength();
}

// interface
const char *asCScriptEngine::GetEnumByIndex(asUINT index, int *enumTypeId, const char **configGroup)
{
	if( index >= registeredEnums.GetLength() )
		return 0;

	if( configGroup )
	{
		asCConfigGroup *group = FindConfigGroupForObjectType(registeredEnums[index]);
		if( group )
			*configGroup = group->groupName.AddressOf();
		else
			*configGroup = 0;
	}

	if( enumTypeId )
		*enumTypeId = GetTypeIdByDecl(registeredEnums[index]->name.AddressOf());

	return registeredEnums[index]->name.AddressOf();
}

// interface
int asCScriptEngine::GetEnumValueCount(int enumTypeId)
{
	const asCDataType *dt = GetDataTypeFromTypeId(enumTypeId);
	asCObjectType *t = dt->GetObjectType();
	if( t == 0 || !(t->GetFlags() & asOBJ_ENUM) )
		return asINVALID_TYPE;

	return (int)t->enumValues.GetLength();
}

// interface
const char *asCScriptEngine::GetEnumValueByIndex(int enumTypeId, asUINT index, int *outValue)
{
	// TODO: This same function is implemented in as_module.cpp as well. Perhaps it should be moved to asCObjectType?
	const asCDataType *dt = GetDataTypeFromTypeId(enumTypeId);
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
int asCScriptEngine::GetObjectTypeCount()
{
	return (int)registeredObjTypes.GetLength();
}

// interface
asIObjectType *asCScriptEngine::GetObjectTypeByIndex(asUINT index)
{
	if( index >= registeredObjTypes.GetLength() )
		return 0;

	return registeredObjTypes[index];
}

// interface
asIObjectType *asCScriptEngine::GetObjectTypeById(int typeId)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return 0;

	// Enum types are not objects, so we shouldn't return an object type for them
	if( dt->GetObjectType() && dt->GetObjectType()->GetFlags() & asOBJ_ENUM )
		return 0;

	return dt->GetObjectType();
}


asIScriptFunction *asCScriptEngine::GetFunctionDescriptorById(int funcId)
{
	return GetScriptFunction(funcId);
}


// internal
bool asCScriptEngine::IsTemplateType(const char *name)
{
	// TODO: optimize: Improve linear search
	for( unsigned int n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && objectTypes[n]->name == name )
		{
			return objectTypes[n]->flags & asOBJ_TEMPLATE ? true : false;
		}
	}

	return false;
}

// internal
int asCScriptEngine::AddConstantString(const char *str, size_t len)
{
	// The str may contain null chars, so we cannot use strlen, or strcmp, or strcpy

	// TODO: optimize: Improve linear search
	// Has the string been registered before?
	for( size_t n = 0; n < stringConstants.GetLength(); n++ )
	{
		if( stringConstants[n]->Compare(str, len) == 0 )
		{
			return (int)n;
		}
	}

	// No match was found, add the string
	asCString *cstr = asNEW(asCString)(str, len);
	stringConstants.PushLast(cstr);

	// The VM currently doesn't handle string ids larger than 65535
	asASSERT(stringConstants.GetLength() <= 65536);

	return (int)stringConstants.GetLength() - 1;
}

// internal
const asCString &asCScriptEngine::GetConstantString(int id)
{
	return *stringConstants[id];
}

// internal
int asCScriptEngine::GetScriptSectionNameIndex(const char *name)
{
	// TODO: These names are only released when the engine is freed. The assumption is that
	//       the same script section names will be reused instead of there always being new
	//       names. Is this assumption valid? Do we need to add reference counting?

	// Store the script section names for future reference
	for( asUINT n = 0; n < scriptSectionNames.GetLength(); n++ )
	{
		if( scriptSectionNames[n]->Compare(name) == 0 )
			return n;
	}

	scriptSectionNames.PushLast(asNEW(asCString)(name));
	return int(scriptSectionNames.GetLength()-1);
}


END_AS_NAMESPACE

