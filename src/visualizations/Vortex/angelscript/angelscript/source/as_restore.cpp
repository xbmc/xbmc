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
// as_restore.cpp
//
// Functions for saving and restoring module bytecode
// asCRestore was originally written by Dennis Bollyn, dennis@gyrbo.be

#include "as_config.h"
#include "as_restore.h"
#include "as_bytecode.h"
#include "as_arrayobject.h"

BEGIN_AS_NAMESPACE

#define WRITE_NUM(N) stream->Write(&(N), sizeof(N))
#define READ_NUM(N) stream->Read(&(N), sizeof(N))

asCRestore::asCRestore(asCModule* _module, asIBinaryStream* _stream, asCScriptEngine* _engine)
 : module(_module), stream(_stream), engine(_engine)
{
}

int asCRestore::Save() 
{
	unsigned long i, count;

	// Store everything in the same order that the builder parses scripts
	
	// Store enums
	count = (asUINT)module->enumTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		WriteObjectTypeDeclaration(module->enumTypes[i], false);
		WriteObjectTypeDeclaration(module->enumTypes[i], true);
	}

	// Store type declarations first
	count = (asUINT)module->classTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		// Store only the name of the class/interface types
		WriteObjectTypeDeclaration(module->classTypes[i], false);
	}

	// Now store all interface methods
	for( i = 0; i < count; i++ )
	{
		if( module->classTypes[i]->IsInterface() )
			WriteObjectTypeDeclaration(module->classTypes[i], true);
	}

	// Then store the class methods, properties, and behaviours
	for( i = 0; i < count; ++i )
	{
		if( !module->classTypes[i]->IsInterface() )
			WriteObjectTypeDeclaration(module->classTypes[i], true);
	}

	// Store typedefs
	count = (asUINT)module->typeDefs.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		WriteObjectTypeDeclaration(module->typeDefs[i], false);
		WriteObjectTypeDeclaration(module->typeDefs[i], true);
	}

	// scriptGlobals[]
	count = (asUINT)module->scriptGlobals.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteGlobalProperty(module->scriptGlobals[i]);

	// scriptFunctions[]
	count = 0;
	for( i = 0; i < module->scriptFunctions.GetLength(); i++ )
		if( module->scriptFunctions[i]->objectType == 0 )
			count++;
	WRITE_NUM(count);
	for( i = 0; i < module->scriptFunctions.GetLength(); ++i )
		if( module->scriptFunctions[i]->objectType == 0 )
			WriteFunction(module->scriptFunctions[i]);

	// globalFunctions[]
	count = (int)module->globalFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		WriteFunction(module->globalFunctions[i]);
	}

	// bindInformations[]
	count = (asUINT)module->bindInformations.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteFunction(module->bindInformations[i]->importedFunctionSignature);
		WriteString(&module->bindInformations[i]->importFromModule);
	}

	// usedTypes[]
	count = (asUINT)usedTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(usedTypes[i]);
	}

	// usedTypeIds[]
	WriteUsedTypeIds();

	// usedFunctions[]
	WriteUsedFunctions();

	// usedGlobalProperties[]
	WriteUsedGlobalProps();

	// usedStringConstants[]
	WriteUsedStringConstants();

	// TODO: Store script section names

	return asSUCCESS;
}

int asCRestore::Restore() 
{
	// Before starting the load, make sure that 
	// any existing resources have been freed
	module->InternalReset();

	unsigned long i, count;

	asCScriptFunction* func;

	// Read enums
	READ_NUM(count);
	module->enumTypes.Allocate(count, 0);
	for( i = 0; i < count; i++ )
	{
		asCObjectType *ot = asNEW(asCObjectType)(engine);
		ReadObjectTypeDeclaration(ot, false);
		engine->classTypes.PushLast(ot);
		module->enumTypes.PushLast(ot);
		ot->AddRef();
		ReadObjectTypeDeclaration(ot, true);
	}

	// structTypes[]
	// First restore the structure names, then the properties
	READ_NUM(count);
	module->classTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		asCObjectType *ot = asNEW(asCObjectType)(engine);
		ReadObjectTypeDeclaration(ot, false);
		engine->classTypes.PushLast(ot);
		module->classTypes.PushLast(ot);
		ot->AddRef();

		// Add script classes to the GC
		if( (ot->GetFlags() & asOBJ_SCRIPT_OBJECT) && ot->GetSize() > 0 )
			engine->gc.AddScriptObjectToGC(ot, &engine->objectTypeBehaviours);
	}

	// Read interface methods
	for( i = 0; i < module->classTypes.GetLength(); i++ )
	{
		if( module->classTypes[i]->IsInterface() )
			ReadObjectTypeDeclaration(module->classTypes[i], true);
	}

	module->ResolveInterfaceIds();

	// Read class methods, properties, and behaviours
	for( i = 0; i < module->classTypes.GetLength(); ++i )
	{
		if( !module->classTypes[i]->IsInterface() )
			ReadObjectTypeDeclaration(module->classTypes[i], true);
	}

	// Read typedefs
	READ_NUM(count);
	module->typeDefs.Allocate(count, 0);
	for( i = 0; i < count; i++ )
	{
		asCObjectType *ot = asNEW(asCObjectType)(engine);
		ReadObjectTypeDeclaration(ot, false);
		engine->classTypes.PushLast(ot);
		module->typeDefs.PushLast(ot);
		ot->AddRef();
		ReadObjectTypeDeclaration(ot, true);
	}

	// scriptGlobals[]
	READ_NUM(count);
	module->scriptGlobals.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		ReadGlobalProperty();
	}

	// scriptFunctions[]
	READ_NUM(count);
	for( i = 0; i < count; ++i ) 
	{
		func = ReadFunction();
	}

	// globalFunctions[]
	READ_NUM(count);
	for( i = 0; i < count; ++i )
	{
		func = ReadFunction(false, false);

		module->globalFunctions.PushLast(func);
		func->AddRef();
	}

	// bindInformations[]
	READ_NUM(count);
	module->bindInformations.SetLength(count);
	for(i=0;i<count;++i)
	{
		sBindInfo *info = asNEW(sBindInfo);
		info->importedFunctionSignature = ReadFunction(false, false);
		info->importedFunctionSignature->id = int(FUNC_IMPORTED + engine->importedFunctions.GetLength());
		engine->importedFunctions.PushLast(info);
		ReadString(&info->importFromModule);
		info->boundFunctionId = -1;
		module->bindInformations[i] = info;
	}
	
	// usedTypes[]
	READ_NUM(count);
	usedTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		asCObjectType *ot = ReadObjectType();
		usedTypes.PushLast(ot);
	}

	// usedTypeIds[]
	ReadUsedTypeIds();

	// usedFunctions[]
	ReadUsedFunctions();

	// usedGlobalProperties[]
	ReadUsedGlobalProps();

	// usedStringConstants[]
	ReadUsedStringConstants();

	for( i = 0; i < module->scriptFunctions.GetLength(); i++ )
		TranslateFunction(module->scriptFunctions[i]);
	for( i = 0; i < module->scriptGlobals.GetLength(); i++ )
		if( module->scriptGlobals[i]->initFunc )
			TranslateFunction(module->scriptGlobals[i]->initFunc);

	// Init system functions properly
	engine->PrepareEngine();

	// Add references for all functions
	for( i = 0; i < module->scriptFunctions.GetLength(); i++ )
		module->scriptFunctions[i]->AddReferences();
	for( i = 0; i < module->scriptGlobals.GetLength(); i++ )
		if( module->scriptGlobals[i]->initFunc )
			module->scriptGlobals[i]->initFunc->AddReferences();

	module->CallInit();

	return 0;
}

int asCRestore::FindStringConstantIndex(int id)
{
	for( asUINT i = 0; i < usedStringConstants.GetLength(); i++ )
		if( usedStringConstants[i] == id )
			return i;

	usedStringConstants.PushLast(id);
	return int(usedStringConstants.GetLength() - 1);
}

void asCRestore::WriteUsedStringConstants()
{
	asUINT count = (asUINT)usedStringConstants.GetLength();
	WRITE_NUM(count);
	for( asUINT i = 0; i < count; ++i )
		WriteString(engine->stringConstants[i]);
}

void asCRestore::ReadUsedStringConstants()
{
	asCString str;

	asUINT count;
	READ_NUM(count);
	usedStringConstants.SetLength(count);
	for( asUINT i = 0; i < count; ++i ) 
	{
		ReadString(&str);
		usedStringConstants[i] = engine->AddConstantString(str.AddressOf(), str.GetLength());
	}
}

void asCRestore::WriteUsedFunctions()
{
	asUINT count = (asUINT)usedFunctions.GetLength();
	WRITE_NUM(count);

	for( asUINT n = 0; n < usedFunctions.GetLength(); n++ )
	{
		char c;

		// Write enough data to be able to uniquely identify the function upon load

		// Is the function from the module or the application?
		c = usedFunctions[n]->module ? 'm' : 'a';
		WRITE_NUM(c);

		WriteFunctionSignature(usedFunctions[n]);
	}
}

void asCRestore::ReadUsedFunctions()
{
	asUINT count;
	READ_NUM(count);
	usedFunctions.SetLength(count);

	for( asUINT n = 0; n < usedFunctions.GetLength(); n++ )
	{
		char c;

		// Read the data to be able to uniquely identify the function

		// Is the function from the module or the application?
		READ_NUM(c);

		asCScriptFunction func(engine, c == 'm' ? module : 0, -1);
		ReadFunctionSignature(&func);

		// Find the correct function
		if( c == 'm' )
		{
			for( asUINT i = 0; i < module->scriptFunctions.GetLength(); i++ )
			{
				asCScriptFunction *f = module->scriptFunctions[i];
				if( !func.IsSignatureEqual(f) ||
					func.objectType != f->objectType ||
					func.funcType != f->funcType )
					continue;

				usedFunctions[n] = f;
				break;
			}
		}
		else
		{
			for( asUINT i = 0; i < engine->scriptFunctions.GetLength(); i++ )
			{
				asCScriptFunction *f = engine->scriptFunctions[i];
				if( f == 0 ||
					!func.IsSignatureEqual(f) ||
					func.objectType != f->objectType )
					continue;

				usedFunctions[n] = f;
				break;
			}
		}

		// Set the type to dummy so it won't try to release the id
		func.funcType = -1;
	}
}

void asCRestore::WriteFunctionSignature(asCScriptFunction *func)
{
	asUINT i, count;

	WriteString(&func->name);
	WriteDataType(&func->returnType);
	count = (asUINT)func->parameterTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteDataType(&func->parameterTypes[i]);

	count = (asUINT)func->inOutFlags.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
		WRITE_NUM(func->inOutFlags[i]);

	WRITE_NUM(func->funcType);

	WriteObjectType(func->objectType);

	WRITE_NUM(func->isReadOnly);
}

void asCRestore::ReadFunctionSignature(asCScriptFunction *func)
{
	int i, count;
	asCDataType dt;
	int num;

	ReadString(&func->name);
	ReadDataType(&func->returnType);
	READ_NUM(count);
	func->parameterTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		ReadDataType(&dt);
		func->parameterTypes.PushLast(dt);
	}

	READ_NUM(count);
	func->inOutFlags.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		READ_NUM(num);
		func->inOutFlags.PushLast(static_cast<asETypeModifiers>(num));
	}

	READ_NUM(func->funcType);

	func->objectType = ReadObjectType();

	READ_NUM(func->isReadOnly);
}

void asCRestore::WriteFunction(asCScriptFunction* func) 
{
	char c;

	// If there is no function, then store a null char
	if( func == 0 )
	{
		c = '\0';
		WRITE_NUM(c);
		return;
	}

	// First check if the function has been saved already
	for( asUINT f = 0; f < savedFunctions.GetLength(); f++ )
	{
		if( savedFunctions[f] == func )
		{
			c = 'r';
			WRITE_NUM(c);
			WRITE_NUM(f);
			return;
		}
	}

	// Keep a reference to the function in the list
	savedFunctions.PushLast(func);

	c = 'f';
	WRITE_NUM(c);

	asUINT i, count;

	WriteFunctionSignature(func);

	count = (asUINT)func->byteCode.GetLength();
	WRITE_NUM(count);
	WriteByteCode(func->byteCode.AddressOf(), count);

	count = (asUINT)func->objVariablePos.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(func->objVariableTypes[i]);
		WRITE_NUM(func->objVariablePos[i]);
	}

	WRITE_NUM(func->stackNeeded);

	asUINT length = (asUINT)func->lineNumbers.GetLength();
	WRITE_NUM(length);
	for( i = 0; i < length; ++i )
		WRITE_NUM(func->lineNumbers[i]);

	WRITE_NUM(func->vfTableIdx);

	WriteGlobalVarPointers(func);

	// TODO: Write variables

	// TODO: Store script section index
}

asCScriptFunction *asCRestore::ReadFunction(bool addToModule, bool addToEngine) 
{
	char c;
	READ_NUM(c);

	if( c == '\0' )
	{
		// There is no function, so return a null pointer
		return 0;
	}

	if( c == 'r' )
	{
		// This is a reference to a previously saved function
		int index;
		READ_NUM(index);

		return savedFunctions[index];
	}

	// Load the new function
	asCScriptFunction *func = asNEW(asCScriptFunction)(engine,module,-1);
	savedFunctions.PushLast(func);

	int i, count;
	asCDataType dt;
	int num;

	ReadFunctionSignature(func);

	if( func->funcType == asFUNC_SCRIPT )
		engine->gc.AddScriptObjectToGC(func, &engine->functionBehaviours);

	func->id = engine->GetNextScriptFunctionId();
	
	READ_NUM(count);
	func->byteCode.Allocate(count, 0);
	ReadByteCode(func->byteCode.AddressOf(), count);
	func->byteCode.SetLength(count);

	READ_NUM(count);
	func->objVariablePos.Allocate(count, 0);
	func->objVariableTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		func->objVariableTypes.PushLast(ReadObjectType());
		READ_NUM(num);
		func->objVariablePos.PushLast(num);
	}

	READ_NUM(func->stackNeeded);

	int length;
	READ_NUM(length);
	func->lineNumbers.SetLength(length);
	for( i = 0; i < length; ++i )
		READ_NUM(func->lineNumbers[i]);

	READ_NUM(func->vfTableIdx);

	if( addToModule )
	{
		// The refCount is already 1
		module->scriptFunctions.PushLast(func);
	}
	if( addToEngine )
		engine->SetScriptFunction(func);
	if( func->objectType )
		func->ComputeSignatureId();

	ReadGlobalVarPointers(func);

	return func;
}

void asCRestore::WriteObjectTypeDeclaration(asCObjectType *ot, bool writeProperties)
{
	if( !writeProperties )
	{
		// name
		WriteString(&ot->name);
		// size
		int size = ot->size;
		WRITE_NUM(size);
		// flags
		asDWORD flags = ot->flags;
		WRITE_NUM(flags);
	}
	else
	{	
		if( ot->flags & asOBJ_ENUM )
		{
			// enumValues[]
			int size = (int)ot->enumValues.GetLength();
			WRITE_NUM(size);

			for( int n = 0; n < size; n++ )
			{
				WriteString(&ot->enumValues[n]->name);
				WRITE_NUM(ot->enumValues[n]->value);
			}
		}
		else if( ot->flags & asOBJ_TYPEDEF )
		{
			eTokenType t = ot->templateSubType.GetTokenType();
			WRITE_NUM(t);
		}
		else
		{
			WriteObjectType(ot->derivedFrom);

			// interfaces[]
			int size = (asUINT)ot->interfaces.GetLength();
			WRITE_NUM(size);
			asUINT n;
			for( n = 0; n < ot->interfaces.GetLength(); n++ )
			{
				WriteObjectType(ot->interfaces[n]);
			}

			// properties[]
			size = (asUINT)ot->properties.GetLength();
			WRITE_NUM(size);
			for( n = 0; n < ot->properties.GetLength(); n++ )
			{
				WriteObjectProperty(ot->properties[n]);
			}

			// behaviours
			if( !ot->IsInterface() && ot->flags != asOBJ_TYPEDEF && ot->flags != asOBJ_ENUM )
			{
				WriteFunction(engine->scriptFunctions[ot->beh.construct]);
				WriteFunction(engine->scriptFunctions[ot->beh.destruct]);
				WriteFunction(engine->scriptFunctions[ot->beh.factory]);
				size = (int)ot->beh.constructors.GetLength() - 1;
				WRITE_NUM(size);
				for( n = 1; n < ot->beh.constructors.GetLength(); n++ )
				{
					WriteFunction(engine->scriptFunctions[ot->beh.constructors[n]]);
					WriteFunction(engine->scriptFunctions[ot->beh.factories[n]]);
				}
			}

			// methods[]
			size = (int)ot->methods.GetLength();
			WRITE_NUM(size);
			for( n = 0; n < ot->methods.GetLength(); n++ )
			{
				WriteFunction(engine->scriptFunctions[ot->methods[n]]);
			}

			// virtualFunctionTable[]
			size = (int)ot->virtualFunctionTable.GetLength();
			WRITE_NUM(size);
			for( n = 0; n < (asUINT)size; n++ )
			{
				WriteFunction(ot->virtualFunctionTable[n]);
			}
		}
	}
}

void asCRestore::ReadObjectTypeDeclaration(asCObjectType *ot, bool readProperties)
{
	if( !readProperties )
	{
		// name
		ReadString(&ot->name);
		// size
		int size;
		READ_NUM(size);
		ot->size = size;
		// flags
		asDWORD flags;
		READ_NUM(flags);
		ot->flags = flags;

		// Use the default script class behaviours
		ot->beh = engine->scriptTypeBehaviours.beh;
		engine->scriptFunctions[ot->beh.addref]->AddRef();
		engine->scriptFunctions[ot->beh.release]->AddRef();
		engine->scriptFunctions[ot->beh.gcEnumReferences]->AddRef();
		engine->scriptFunctions[ot->beh.gcGetFlag]->AddRef();
		engine->scriptFunctions[ot->beh.gcGetRefCount]->AddRef();
		engine->scriptFunctions[ot->beh.gcReleaseAllReferences]->AddRef();
		engine->scriptFunctions[ot->beh.gcSetFlag]->AddRef();
		engine->scriptFunctions[ot->beh.copy]->AddRef();
		engine->scriptFunctions[ot->beh.factory]->AddRef();
		engine->scriptFunctions[ot->beh.construct]->AddRef();
		for( asUINT i = 1; i < ot->beh.operators.GetLength(); i += 2 )
			engine->scriptFunctions[ot->beh.operators[i]]->AddRef();
	}
	else
	{	
		if( ot->flags & asOBJ_ENUM )
		{
			int count;
			READ_NUM(count);
			ot->enumValues.Allocate(count, 0);
			for( int n = 0; n < count; n++ )
			{
				asSEnumValue *e = asNEW(asSEnumValue);
				ReadString(&e->name);
				READ_NUM(e->value);
				ot->enumValues.PushLast(e);
			}
		}
		else if( ot->flags & asOBJ_TYPEDEF )
		{
			eTokenType t;
			READ_NUM(t);
			ot->templateSubType = asCDataType::CreatePrimitive(t, false);
		}
		else
		{
			ot->derivedFrom = ReadObjectType();
			if( ot->derivedFrom )
				ot->derivedFrom->AddRef();

			// interfaces[]
			int size;
			READ_NUM(size);
			ot->interfaces.Allocate(size,0);
			int n;
			for( n = 0; n < size; n++ )
			{
				asCObjectType *intf = ReadObjectType();
				ot->interfaces.PushLast(intf);
			}

			// properties[]
			READ_NUM(size);
			ot->properties.Allocate(size,0);
			for( n = 0; n < size; n++ )
			{
				asCObjectProperty *prop = asNEW(asCObjectProperty);
				ReadObjectProperty(prop);
				ot->properties.PushLast(prop);
			}

			// behaviours
			if( !ot->IsInterface() && ot->flags != asOBJ_TYPEDEF && ot->flags != asOBJ_ENUM )
			{
				asCScriptFunction *func = ReadFunction();
				engine->scriptFunctions[ot->beh.construct]->Release();
				ot->beh.construct = func->id;
				ot->beh.constructors[0] = func->id;
				func->AddRef();

				func = ReadFunction();
				if( func )
				{
					ot->beh.destruct = func->id;
					func->AddRef();
				}

				func = ReadFunction();
				engine->scriptFunctions[ot->beh.factory]->Release();
				ot->beh.factory = func->id;
				ot->beh.factories[0] = func->id;
				func->AddRef();

				READ_NUM(size);
				for( n = 0; n < size; n++ )
				{
					asCScriptFunction *func = ReadFunction();
					ot->beh.constructors.PushLast(func->id);
					func->AddRef();

					func = ReadFunction();
					ot->beh.factories.PushLast(func->id);
					func->AddRef();
				}
			}

			// methods[]
			READ_NUM(size);
			for( n = 0; n < size; n++ ) 
			{
				asCScriptFunction *func = ReadFunction();
				ot->methods.PushLast(func->id);
				func->AddRef();
			}

			// virtualFunctionTable[]
			READ_NUM(size);
			for( n = 0; n < size; n++ )
			{
				asCScriptFunction *func = ReadFunction();
				ot->virtualFunctionTable.PushLast(func);
				func->AddRef();
			}
		}
	}
}

void asCRestore::WriteString(asCString* str) 
{
	asUINT len = (asUINT)str->GetLength();
	WRITE_NUM(len);
	stream->Write(str->AddressOf(), (asUINT)len);
}

void asCRestore::ReadString(asCString* str) 
{
	asUINT len;
	READ_NUM(len);
	str->SetLength(len);
	stream->Read(str->AddressOf(), len);
}

void asCRestore::WriteGlobalProperty(asCGlobalProperty* prop) 
{
	// TODO: We might be able to avoid storing the name and type of the global 
	//       properties twice if we merge this with the WriteUsedGlobalProperties. 
	WriteString(&prop->name);
	WriteDataType(&prop->type);

	// Store the initialization function
	if( prop->initFunc )
	{
		bool f = true;
		WRITE_NUM(f);

		WriteFunction(prop->initFunc);
	}
	else
	{
		bool f = false;
		WRITE_NUM(f);
	}
}

void asCRestore::ReadGlobalProperty() 
{
	asCString name;
	asCDataType type;

	ReadString(&name);
	ReadDataType(&type);

	asCGlobalProperty *prop = module->AllocateGlobalProperty(name.AddressOf(), type);

	// Read the initialization function
	bool f;
	READ_NUM(f);
	if( f )
	{
		asCScriptFunction *func = ReadFunction(false, true);

		// refCount was already set to 1
		prop->initFunc = func;
	}
}

void asCRestore::WriteObjectProperty(asCObjectProperty* prop) 
{
	WriteString(&prop->name);
	WriteDataType(&prop->type);
	WRITE_NUM(prop->byteOffset);
}

void asCRestore::ReadObjectProperty(asCObjectProperty* prop) 
{
	ReadString(&prop->name);
	ReadDataType(&prop->type);
	READ_NUM(prop->byteOffset);
}

void asCRestore::WriteDataType(const asCDataType *dt) 
{
	bool b;
	int t = dt->GetTokenType();
	WRITE_NUM(t);
	WriteObjectType(dt->GetObjectType());
	b = dt->IsObjectHandle();
	WRITE_NUM(b);
	b = dt->IsReadOnly();
	WRITE_NUM(b);
	b = dt->IsHandleToConst();
	WRITE_NUM(b);
	b = dt->IsReference();
	WRITE_NUM(b);
}

void asCRestore::ReadDataType(asCDataType *dt) 
{
	eTokenType tokenType;
	READ_NUM(tokenType);
	asCObjectType *objType = ReadObjectType();
	bool isObjectHandle;
	READ_NUM(isObjectHandle);
	bool isReadOnly;
	READ_NUM(isReadOnly);
	bool isHandleToConst;
	READ_NUM(isHandleToConst);
	bool isReference;
	READ_NUM(isReference);

	if( tokenType == ttIdentifier )
		*dt = asCDataType::CreateObject(objType, false);
	else
		*dt = asCDataType::CreatePrimitive(tokenType, false);
	if( isObjectHandle )
	{
		dt->MakeReadOnly(isHandleToConst);
		dt->MakeHandle(true);
	}
	dt->MakeReadOnly(isReadOnly);
	dt->MakeReference(isReference);
}

void asCRestore::WriteObjectType(asCObjectType* ot) 
{
	char ch;

	// Only write the object type name
	if( ot )
	{
		// Check for template instances/specializations
		if( ot->templateSubType.GetTokenType() != ttUnrecognizedToken &&
			ot != engine->defaultArrayObjectType )
		{
			ch = 'a';
			WRITE_NUM(ch);

			if( ot->templateSubType.IsObject() )
			{
				ch = 's';
				WRITE_NUM(ch);
				WriteObjectType(ot->templateSubType.GetObjectType());

				if( ot->templateSubType.IsObjectHandle() )
					ch = 'h';
				else
					ch = 'o';
				WRITE_NUM(ch);
			}
			else
			{
				ch = 't';
				WRITE_NUM(ch);
				eTokenType t = ot->templateSubType.GetTokenType();
				WRITE_NUM(t);
			}
		}
		else if( ot->flags & asOBJ_TEMPLATE_SUBTYPE )
		{
			ch = 's';
			WRITE_NUM(ch);
			WriteString(&ot->name);
		}
		else
		{
			ch = 'o';
			WRITE_NUM(ch);
			WriteString(&ot->name);
		}
	}
	else
	{
		ch = '\0';
		WRITE_NUM(ch);
		// Write a null string
		asDWORD null = 0;
		WRITE_NUM(null);
	}
}

asCObjectType* asCRestore::ReadObjectType() 
{
	asCObjectType *ot;
	char ch;
	READ_NUM(ch);
	if( ch == 'a' )
	{
		READ_NUM(ch);
		if( ch == 's' )
		{
			ot = ReadObjectType();
			asCDataType dt = asCDataType::CreateObject(ot, false);

			READ_NUM(ch);
			if( ch == 'h' )
				dt.MakeHandle(true);

			dt.MakeArray(engine);
			ot = dt.GetObjectType();
			
			asASSERT(ot);
		}
		else
		{
			eTokenType tokenType;
			READ_NUM(tokenType);
			asCDataType dt = asCDataType::CreatePrimitive(tokenType, false);
			dt.MakeArray(engine);
			ot = dt.GetObjectType();
			
			asASSERT(ot);
		}
	}
	else if( ch == 's' )
	{
		// Read the name of the template subtype
		asCString typeName;
		ReadString(&typeName);

		// Find the template subtype
		for( asUINT n = 0; n < engine->templateSubTypes.GetLength(); n++ )
		{
			if( engine->templateSubTypes[n] && engine->templateSubTypes[n]->name == typeName )
			{
				ot = engine->templateSubTypes[n];
				break;
			}
		}

		// TODO: Should give a friendly error in case the template type isn't found
		asASSERT(ot);
	}
	else
	{
		// Read the object type name
		asCString typeName;
		ReadString(&typeName);

		if( typeName.GetLength() && typeName != "_builtin_object_" )
		{
			// Find the object type
			ot = module->GetObjectType(typeName.AddressOf());
			if( !ot )
				ot = engine->GetObjectType(typeName.AddressOf());
			
			asASSERT(ot);
		}
		else if( typeName == "_builtin_object_" )
		{
			ot = &engine->scriptTypeBehaviours;
		}
		else
			ot = 0;
	}

	return ot;
}

void asCRestore::WriteByteCode(asDWORD *bc, int length)
{
	while( length )
	{
		asDWORD c = *(asBYTE*)bc;

		if( c == asBC_ALLOC )
		{
			WRITE_NUM(*bc++);
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				tmp[n] = *bc++;

			// Translate the object type 
			asCObjectType *ot = *(asCObjectType**)tmp;
			*(int*)tmp = FindObjectTypeIdx(ot);

			// Translate the constructor func id, if it is a script class
			if( ot->flags & asOBJ_SCRIPT_OBJECT )
				*(int*)&tmp[AS_PTR_SIZE] = FindFunctionIndex(engine->scriptFunctions[*(int*)&tmp[AS_PTR_SIZE]]);

			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == asBC_FREE   ||
			     c == asBC_REFCPY || 
				 c == asBC_OBJTYPE )
		{
			WRITE_NUM(*bc++);
			// Translate object type pointers into indices
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindObjectTypeIdx(*(asCObjectType**)tmp);

			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == asBC_TYPEID )
		{
			WRITE_NUM(*bc++);

			// Translate type ids into indices
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindTypeIdIdx(*(int*)tmp);

			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == asBC_CALL ||
				 c == asBC_CALLINTF || 
				 c == asBC_CALLSYS )
		{
			WRITE_NUM(*bc++);

			// Translate the function id
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindFunctionIndex(engine->scriptFunctions[*(int*)tmp]);

			for( n = 0; n < asBCTypeSize[asBCInfo[c].type]-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == asBC_STR )
		{
			asDWORD tmp = *bc++;

			// Translate the string constant id
			asWORD *arg = ((asWORD*)&tmp)+1;
			*arg = FindStringConstantIndex(*arg);
			WRITE_NUM(tmp);
		}
		else if( c == asBC_CALLBND )
		{
			WRITE_NUM(*bc++);

			// Translate the function id
			int funcId = *bc++;
			for( asUINT n = 0; n < module->bindInformations.GetLength(); n++ )
				if( module->bindInformations[n]->importedFunctionSignature->id == funcId )
				{
					funcId = n;
					break;
				}

			WRITE_NUM(funcId);
		}
		else
		{
			// Store the bc as is
			for( int n = 0; n < asBCTypeSize[asBCInfo[c].type]; n++ )
				WRITE_NUM(*bc++);
		}

		length -= asBCTypeSize[asBCInfo[c].type];
	}
}

void asCRestore::ReadByteCode(asDWORD *bc, int length)
{
	while( length )
	{
		asDWORD c;
		READ_NUM(c);
		*bc = c;
		bc += 1;
		c = *(asBYTE*)&c;

		// Read the bc as is
		for( int n = 1; n < asBCTypeSize[asBCInfo[c].type]; n++ )
			READ_NUM(*bc++);

		length -= asBCTypeSize[asBCInfo[c].type];
	}
}

void asCRestore::WriteUsedTypeIds()
{
	asUINT count = (asUINT)usedTypeIds.GetLength();
	WRITE_NUM(count);
	for( asUINT n = 0; n < count; n++ )
		WriteDataType(engine->GetDataTypeFromTypeId(usedTypeIds[n]));
}

void asCRestore::ReadUsedTypeIds()
{
	asUINT n;
	asUINT count;
	READ_NUM(count);
	usedTypeIds.SetLength(count);
	for( n = 0; n < count; n++ )
	{
		asCDataType dt;
		ReadDataType(&dt);
		usedTypeIds[n] = engine->GetTypeIdFromDataType(dt);
	}
}

int asCRestore::FindGlobalPropPtrIndex(void *ptr)
{
	int i = usedGlobalProperties.IndexOf(ptr);
	if( i >= 0 ) return i;

	usedGlobalProperties.PushLast(ptr);
	return (int)usedGlobalProperties.GetLength()-1;
}

void asCRestore::WriteUsedGlobalProps()
{
	int c = (int)usedGlobalProperties.GetLength();
	WRITE_NUM(c);

	for( int n = 0; n < c; n++ )
	{
		size_t *p = (size_t*)usedGlobalProperties[n];
		
		// First search for the global in the module
		char moduleProp = 0;
		asCGlobalProperty *prop = 0;
		for( int i = 0; i < (signed)module->scriptGlobals.GetLength(); i++ )
		{
			if( p == module->scriptGlobals[i]->GetAddressOfValue() )
			{
				prop = module->scriptGlobals[i];
				moduleProp = 1;
				break;
			}
		}

		// If it is not in the module, it must be an application registered property
		if( !prop )
		{
			for( int i = 0; i < (signed)engine->registeredGlobalProps.GetLength(); i++ )
			{
				if( engine->registeredGlobalProps[i]->GetAddressOfValue() == p )
				{
					prop = engine->registeredGlobalProps[i];
					break;
				}
			}
		}

		asASSERT(prop);

		// Store the name and type of the property so we can find it again on loading
		WriteString(&prop->name);
		WriteDataType(&prop->type);

		// Also store whether the property is a module property or a registered property
		WRITE_NUM(moduleProp);
	}
}

void asCRestore::ReadUsedGlobalProps()
{
	int c;
	READ_NUM(c);

	usedGlobalProperties.SetLength(c);

	for( int n = 0; n < c; n++ )
	{
		asCString name;
		asCDataType type;
		char moduleProp;

		ReadString(&name);
		ReadDataType(&type);
		READ_NUM(moduleProp);

		// Find the real property
		void *prop = 0;
		if( moduleProp )
		{
			for( asUINT p = 0; p < module->scriptGlobals.GetLength(); p++ )
			{
				if( module->scriptGlobals[p]->name == name &&
					module->scriptGlobals[p]->type == type )
				{
					prop = module->scriptGlobals[p]->GetAddressOfValue();
					break;
				}
			}
		}
		else
		{
			for( asUINT p = 0; p < engine->registeredGlobalProps.GetLength(); p++ )
			{
				if( engine->registeredGlobalProps[p] &&
					engine->registeredGlobalProps[p]->name == name &&
					engine->registeredGlobalProps[p]->type == type )
				{
					prop = engine->registeredGlobalProps[p]->GetAddressOfValue();
					break;
				}
			}
		}

		// TODO: If the property isn't found, we must give an error
		asASSERT(prop);

		usedGlobalProperties[n] = prop;
	}
}

void asCRestore::WriteGlobalVarPointers(asCScriptFunction *func)
{
	int c = (int)func->globalVarPointers.GetLength();
	WRITE_NUM(c);

	for( int n = 0; n < c; n++ )
	{
		void *p = (void*)func->globalVarPointers[n];

		int i = FindGlobalPropPtrIndex(p);
		WRITE_NUM(i);
	}
}

void asCRestore::ReadGlobalVarPointers(asCScriptFunction *func)
{
	int c;
	READ_NUM(c);

	func->globalVarPointers.SetLength(c);

	for( int n = 0; n < c; n++ )
	{
		int idx;
		READ_NUM(idx);

		// This will later on be translated into the true pointer
		func->globalVarPointers[n] = (void*)(size_t)idx;
	}
}

//---------------------------------------------------------------------------------------------------
// Miscellaneous
//---------------------------------------------------------------------------------------------------

int asCRestore::FindFunctionIndex(asCScriptFunction *func)
{
	asUINT n;
	for( n = 0; n < usedFunctions.GetLength(); n++ )
	{
		if( usedFunctions[n] == func )
			return n;
	}

	usedFunctions.PushLast(func);
	return (int)usedFunctions.GetLength() - 1;
}

asCScriptFunction *asCRestore::FindFunction(int idx)
{
	return usedFunctions[idx];
}

void asCRestore::TranslateFunction(asCScriptFunction *func)
{
	asUINT n;
	asDWORD *bc = func->byteCode.AddressOf();
	for( n = 0; n < func->byteCode.GetLength(); )
	{
		int c = *(asBYTE*)&bc[n];
		if( c == asBC_FREE ||
			c == asBC_REFCPY || c == asBC_OBJTYPE )
		{
			// Translate the index to the true object type
			asPTRWORD *ot = (asPTRWORD*)&bc[n+1];
			*(asCObjectType**)ot = FindObjectType(*(int*)ot);
		}
		else  if( c == asBC_TYPEID )
		{
			// Translate the index to the type id
			int *tid = (int*)&bc[n+1];
			*tid = FindTypeId(*tid);
		}
		else if( c == asBC_CALL ||
				 c == asBC_CALLINTF ||
				 c == asBC_CALLSYS )
		{
			// Translate the index to the func id
			int *fid = (int*)&bc[n+1];
			*fid = FindFunction(*fid)->id;
		}
		else if( c == asBC_ALLOC )
		{
			// Translate the index to the true object type
			asPTRWORD *arg = (asPTRWORD*)&bc[n+1];
			*(asCObjectType**)arg = FindObjectType(*(int*)arg);

			// If the object type is a script class then the constructor id must be translated
			asCObjectType *ot = *(asCObjectType**)arg;
			if( ot->flags & asOBJ_SCRIPT_OBJECT )
			{
				int *fid = (int*)&bc[n+1+AS_PTR_SIZE];
				*fid = FindFunction(*fid)->id;
			}
		}
		else if( c == asBC_STR )
		{
			// Translate the index to the true string id
			asWORD *arg = ((asWORD*)&bc[n])+1;

			*arg = usedStringConstants[*arg];
		}
		else if( c == asBC_CALLBND )
		{
			// Translate the function id
			int *fid = (int*)&bc[n+1];
			*fid = module->bindInformations[*fid]->importedFunctionSignature->id;
		}

		n += asBCTypeSize[asBCInfo[c].type];
	}

	// Translate the globalVarPointers
	for( n = 0; n < func->globalVarPointers.GetLength(); n++ )
	{
		func->globalVarPointers[n] = usedGlobalProperties[(int)(size_t)func->globalVarPointers[n]];
	}
}

int asCRestore::FindTypeIdIdx(int typeId)
{
	asUINT n;
	for( n = 0; n < usedTypeIds.GetLength(); n++ )
	{
		if( usedTypeIds[n] == typeId )
			return n;
	}

	usedTypeIds.PushLast(typeId);
	return (int)usedTypeIds.GetLength() - 1;
}

int asCRestore::FindTypeId(int idx)
{
	return usedTypeIds[idx];
}

int asCRestore::FindObjectTypeIdx(asCObjectType *obj)
{
	asUINT n;
	for( n = 0; n < usedTypes.GetLength(); n++ )
	{
		if( usedTypes[n] == obj )
			return n;
	}

	usedTypes.PushLast(obj);
	return (int)usedTypes.GetLength() - 1;
}

asCObjectType *asCRestore::FindObjectType(int idx)
{
	return usedTypes[idx];
}

END_AS_NAMESPACE

