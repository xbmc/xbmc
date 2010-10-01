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
// as_restore.cpp
//
// Functions for saving and restoring module bytecode
// asCRestore was originally written by Dennis Bollyn, dennis@gyrbo.be

#include "as_config.h"
#include "as_restore.h"
#include "as_bytecodedef.h"
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

	// structTypes[]
	count = module->structTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectTypeDeclaration(module->structTypes[i]);
	}

	// usedTypeIndices[]
	count = module->usedTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(module->usedTypes[i]);
	}

	// scriptGlobals[]
	count = module->scriptGlobals.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteProperty(module->scriptGlobals[i]);

	// globalMem size (can restore data using @init())
	count = module->globalMem.GetLength();
	WRITE_NUM(count);
	
	// globalVarPointers[]
	WriteGlobalVarPointers();

	// initFunction
	WriteFunction(&module->initFunction);

	// scriptFunctions[]
	count = module->scriptFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
		WriteFunction(module->scriptFunctions[i]);

	// stringConstants[]
	count = module->stringConstants.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteString(module->stringConstants[i]);

	// importedFunctions[] and bindInformations[]
	count = module->importedFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteFunction(module->importedFunctions[i]);
		WRITE_NUM(module->bindInformations[i].importFrom);
	}

	// usedTypeIds[]
	WriteUsedTypeIds();

	return asSUCCESS;
}

// NEXT: Must go through the bytecode and set the correct objecttype pointer where used
int asCRestore::Restore() 
{
	// Before starting the load, make sure that 
	// any existing resources have been freed
	module->Reset();

	unsigned long i, count;

	asCScriptFunction* func;
	asCProperty* prop;
	asCString *cstr;

	// structTypes[]
	READ_NUM(count);
	module->structTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		asCObjectType *ot = new asCObjectType(engine);
		ReadObjectTypeDeclaration(ot);
		engine->structTypes.PushLast(ot);
		module->structTypes.PushLast(ot);
		ot->refCount++;
	}

	// usedTypes[]
	READ_NUM(count);
	module->usedTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		asCObjectType *ot = ReadObjectType();
		module->usedTypes.PushLast(ot);
		ot->refCount++;		
	}

	// scriptGlobals[]
	READ_NUM(count);
	module->scriptGlobals.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		prop = new asCProperty;
		ReadProperty(prop);
		module->scriptGlobals.PushLast(prop);
	}

	// globalMem size
	READ_NUM(count);
	module->globalMem.SetLength(count);

	// globalVarPointers[]
	ReadGlobalVarPointers();

	// initFunction
	ReadFunction(&module->initFunction);

	// scriptFunctions[]
	READ_NUM(count);
	module->scriptFunctions.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		func = new asCScriptFunction;
		ReadFunction(func);
		module->scriptFunctions.PushLast(func);
	}

	// stringConstants[]
	READ_NUM(count);
	module->stringConstants.Allocate(count, 0);
	for(i=0;i<count;++i) 
	{
		cstr = new asCString();
		ReadString(cstr);
		module->stringConstants.PushLast(cstr);
	}
	
	// importedFunctions[] and bindInformations[]
	READ_NUM(count);
	module->importedFunctions.Allocate(count, 0);
	module->bindInformations.SetLength(count);
	for(i=0;i<count;++i)
	{
		func = new asCScriptFunction;
		ReadFunction(func);
		module->importedFunctions.PushLast(func);

		READ_NUM(module->bindInformations[i].importFrom);
		module->bindInformations[i].importedFunction = -1;
	}
	
	// usedTypeIds[]
	ReadUsedTypeIds();

	// Fake building
	module->isBuildWithoutErrors = true;

	// Init system functions properly
	engine->PrepareEngine();

	module->CallInit();

	return 0;
}

void asCRestore::WriteString(asCString* str) 
{
	unsigned long len = str->GetLength();
	WRITE_NUM(len);
	stream->Write(str->AddressOf(), len);
}

void asCRestore::WriteFunction(asCScriptFunction* func) 
{
	int i, count;

	WriteString(&func->name);

	WriteDataType(&func->returnType);

	count = func->parameterTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteDataType(&func->parameterTypes[i]);

	WRITE_NUM(func->id);
	
	count = func->byteCode.GetLength();
	WRITE_NUM(count);
	WriteByteCode(func->byteCode.AddressOf(), count);

	count = func->objVariablePos.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(func->objVariableTypes[i]);
		WRITE_NUM(func->objVariablePos[i]);
	}

	WRITE_NUM(func->stackNeeded);

	WriteObjectType(func->objectType);

	int length = func->lineNumbers.GetLength();
	WRITE_NUM(length);
	for( i = 0; i < length; ++i )
		WRITE_NUM(func->lineNumbers[i]);
}

void asCRestore::WriteProperty(asCProperty* prop) 
{
	WriteString(&prop->name);
	WriteDataType(&prop->type);
	WRITE_NUM(prop->index);
}

void asCRestore::WriteDataType(const asCDataType *dt) 
{
	if( dt->IsScriptArray() )
	{
		bool b = true;
		WRITE_NUM(b);

		b = dt->IsObjectHandle();
		WRITE_NUM(b);
		b = dt->IsReadOnly();
		WRITE_NUM(b);
		b = dt->IsHandleToConst();
		WRITE_NUM(b);
		b = dt->IsReference();
		WRITE_NUM(b);

		asCDataType sub = dt->GetSubType();
		WriteDataType(&sub);
	}
	else
	{
		bool b = false;
		WRITE_NUM(b);

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
}

void asCRestore::WriteObjectType(asCObjectType* ot) 
{
	char ch;

	// Only write the object type name
	if( ot )
	{
		if( ot->flags & asOBJ_SCRIPT_ARRAY && ot->name != asDEFAULT_ARRAY )
		{
			ch = 'a';
			WRITE_NUM(ch);

			if( ot->subType )
			{
				ch = 's';
				WRITE_NUM(ch);
				WriteObjectType(ot->subType);

				ch = ot->arrayType & 1 ? 'h' : 'o';
				WRITE_NUM(ch);
			}
			else
			{
				ch = 't';
				WRITE_NUM(ch);
				WRITE_NUM(ot->tokenType);
			}
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

void asCRestore::WriteObjectTypeDeclaration(asCObjectType *ot)
{
	// name
	WriteString(&ot->name);
	// size
	int size = ot->size;
	WRITE_NUM(size);
	// properties[]
	size = ot->properties.GetLength();
	WRITE_NUM(size);
	for( asUINT n = 0; n < ot->properties.GetLength(); n++ )
	{
		WriteProperty(ot->properties[n]);
	}

	// TODO:
	// methods[]
	// behaviours
}

void asCRestore::ReadString(asCString* str) 
{
	unsigned long len;
	READ_NUM(len);
	str->SetLength(len);
	stream->Read(str->AddressOf(), len);
}

void asCRestore::ReadFunction(asCScriptFunction* func) 
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

	READ_NUM(func->id);
	
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

	func->objectType = ReadObjectType();

	int length;
	READ_NUM(length);
	func->lineNumbers.SetLength(length);
	for( i = 0; i < length; ++i )
		READ_NUM(func->lineNumbers[i]);
}

void asCRestore::ReadProperty(asCProperty* prop) 
{
	ReadString(&prop->name);
	ReadDataType(&prop->type);
	READ_NUM(prop->index);
}

void asCRestore::ReadDataType(asCDataType *dt) 
{
	bool b;
	READ_NUM(b);
	if( b ) 
	{
		bool isObjectHandle;
		READ_NUM(isObjectHandle);
		bool isReadOnly;
		READ_NUM(isReadOnly);
		bool isHandleToConst;
		READ_NUM(isHandleToConst);
		bool isReference;
		READ_NUM(isReference);

		asCDataType sub;
		ReadDataType(&sub);

		*dt = sub;
		dt->MakeArray(engine);
		if( isObjectHandle )
		{
			dt->MakeReadOnly(isHandleToConst);
			dt->MakeHandle(true);
		}
		dt->MakeReadOnly(isReadOnly);
		dt->MakeReference(isReference);
	}
	else
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
		}
		else
		{
			eTokenType tokenType;
			READ_NUM(tokenType);
			asCDataType dt = asCDataType::CreatePrimitive(tokenType, false);
			dt.MakeArray(engine);
			ot = dt.GetObjectType();
		}
	}
	else
	{
		// Read the object type name
		asCString typeName;
		ReadString(&typeName);

		// Find the object type
		ot = module->GetObjectType(typeName.AddressOf());
		if( !ot )
			ot = engine->GetObjectType(typeName.AddressOf());
	}

	return ot;
}

void asCRestore::ReadObjectTypeDeclaration(asCObjectType *ot)
{
	// name
	ReadString(&ot->name);
	// size
	int size;
	READ_NUM(size);
	ot->size = size;
	// properties[]
	READ_NUM(size);
	ot->properties.Allocate(size,0);
	for( int n = 0; n < size; n++ )
	{
		asCProperty *prop = new asCProperty;
		ReadProperty(prop);
		ot->properties.PushLast(prop);
	}

	// Use the default script struct behaviours
	ot->beh.construct = engine->scriptTypeBehaviours.beh.construct;
	ot->beh.constructors.PushLast(ot->beh.construct);
	ot->beh.addref = engine->scriptTypeBehaviours.beh.addref;
	ot->beh.release = engine->scriptTypeBehaviours.beh.release;
	ot->beh.copy = engine->scriptTypeBehaviours.beh.copy;
	ot->beh.operators.PushLast(ttAssignment);
	ot->beh.operators.PushLast(ot->beh.copy);

	// Some implicit values
	ot->tokenType = ttIdentifier;
	ot->arrayType = 0;
	ot->flags = asOBJ_CLASS_CDA | asOBJ_SCRIPT_STRUCT;

	// TODO: The flag asOBJ_POTENTIAL_CIRCLE must be saved

	// TODO: What about the arrays? the flag must be saved as well

	// TODO:
	// methods[]
}

void asCRestore::WriteByteCode(asDWORD *bc, int length)
{
	while( length )
	{
		asDWORD c = (*bc)&0xFF;
		WRITE_NUM(*bc);
		bc += 1;
		if( c == BC_ALLOC || c == BC_FREE ||
			c == BC_REFCPY || c == BC_OBJTYPE )
		{
			// Translate object type pointers into indices
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindObjectTypeIdx(*(asCObjectType**)tmp);

			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == BC_TYPEID )
		{
			// Translate type ids into indices
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindTypeIdIdx(*(int*)tmp);

			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else
		{
			// Store the bc as is
			for( int n = 1; n < asCByteCode::SizeOfType(bcTypes[c]); n++ )
				WRITE_NUM(*bc++);
		}

		length -= asCByteCode::SizeOfType(bcTypes[c]);
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
	return usedTypeIds.GetLength() - 1;
}

int asCRestore::FindTypeId(int idx)
{
	return usedTypeIds[idx];
}

void asCRestore::WriteUsedTypeIds()
{
	int count = usedTypeIds.GetLength();
	WRITE_NUM(count);
	for( int n = 0; n < count; n++ )
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

	// Translate all the TYPEID bytecodes
	TranslateFunction(&module->initFunction);
	for( n = 0; n < module->scriptFunctions.GetLength(); n++ )
		TranslateFunction(module->scriptFunctions[n]);
}

void asCRestore::TranslateFunction(asCScriptFunction *func)
{
	asDWORD *bc = func->byteCode.AddressOf();
	for( asUINT n = 0; n < func->byteCode.GetLength(); )
	{
		int c = bc[n]&0xFF;
		if( c == BC_TYPEID )
		{
			// Translate the index to the type id
			int *tid = (int*)&bc[n+1];

			*tid = FindTypeId(*tid);
		}

		n += asCByteCode::SizeOfType(bcTypes[c]);
	}
}

int asCRestore::FindObjectTypeIdx(asCObjectType *obj)
{
	asUINT n;
	for( n = 0; n < module->usedTypes.GetLength(); n++ )
	{
		if( module->usedTypes[n] == obj )
			return n;
	}

	assert( false );
	return -1;
}

asCObjectType *asCRestore::FindObjectType(int idx)
{
	return module->usedTypes[idx];
}

void asCRestore::ReadByteCode(asDWORD *bc, int length)
{
	while( length )
	{
		asDWORD c;
		READ_NUM(c);
		*bc = asDWORD(c);
		bc += 1;
		c &= 0xFF;
		if( c == BC_ALLOC || c == BC_FREE ||
			c == BC_REFCPY || c == BC_OBJTYPE )
		{
			// Translate the index to the true object type
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				READ_NUM(tmp[n]);

			*(asCObjectType**)tmp = FindObjectType(*(int*)tmp);

			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				*bc++ = tmp[n];
		}
		else
		{
			// Read the bc as is
			for( int n = 1; n < asCByteCode::SizeOfType(bcTypes[c]); n++ )
				READ_NUM(*bc++);
		}

		length -= asCByteCode::SizeOfType(bcTypes[c]);
	}
}

void asCRestore::WriteGlobalVarPointers()
{
	int c = module->globalVarPointers.GetLength();
	WRITE_NUM(c);

	for( int n = 0; n < c; n++ )
	{
		asDWORD *p = (asDWORD*)module->globalVarPointers[n];
		int idx = 0;
		
		// Is it a module global or engine global?
		if( p >= module->globalMem.AddressOf() && p <= module->globalMem.AddressOf() + module->globalMem.GetLength() )
			idx = (asDWORD(p) - asDWORD(module->globalMem.AddressOf()))/4;
		else
		{
			for( int i = 0; i < (signed)engine->globalPropAddresses.GetLength(); i++ )
			{
				if( engine->globalPropAddresses[i] == p )
				{
					idx = -i - 1;
					break;
				}
			}
			assert( idx != 0 );
		}

		WRITE_NUM(idx);
	}
}

void asCRestore::ReadGlobalVarPointers()
{
	int c;
	READ_NUM(c);

	module->globalVarPointers.SetLength(c);

	for( int n = 0; n < c; n++ )
	{
		int idx;
		READ_NUM(idx);

		if( idx < 0 ) 
			module->globalVarPointers[n] = (void*)(engine->globalPropAddresses[-idx - 1]);
		else
			module->globalVarPointers[n] = (void*)(module->globalMem.AddressOf() + idx);
	}
}

END_AS_NAMESPACE

