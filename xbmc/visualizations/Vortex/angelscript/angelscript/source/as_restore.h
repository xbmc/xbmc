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
// as_restore.h
//
// Functions for saving and restoring module bytecode
// asCRestore was originally written by Dennis Bollyn, dennis@gyrbo.be


// TODO: This should be split in two, so that an application that doesn't compile any 
//       code but only loads precompiled code can link with only the bytecode loader

#ifndef AS_RESTORE_H
#define AS_RESTORE_H

#include "as_scriptengine.h"
#include "as_context.h"
#include "as_map.h"

BEGIN_AS_NAMESPACE

class asCRestore 
{
public:
	asCRestore(asCModule *module, asIBinaryStream *stream, asCScriptEngine *engine);

	int Save();
	int Restore();

protected:
	asCModule *module;
	asIBinaryStream *stream;
	asCScriptEngine *engine;

	void WriteString(asCString *str);
	void WriteFunction(asCScriptFunction *func);
	void WriteFunctionSignature(asCScriptFunction *func);
	void WriteGlobalProperty(asCGlobalProperty *prop);
	void WriteObjectProperty(asCObjectProperty *prop);
	void WriteDataType(const asCDataType *dt);
	void WriteObjectType(asCObjectType *ot);
	void WriteObjectTypeDeclaration(asCObjectType *ot, bool writeProperties);
	void WriteGlobalVarPointers(asCScriptFunction *func);
	void WriteByteCode(asDWORD *bc, int length);

	void ReadString(asCString *str);
	asCScriptFunction *ReadFunction(bool addToModule = true, bool addToEngine = true);
	void ReadFunctionSignature(asCScriptFunction *func);
	void ReadGlobalProperty();
	void ReadObjectProperty(asCObjectProperty *prop);
	void ReadDataType(asCDataType *dt);
	asCObjectType *ReadObjectType();
	void ReadObjectTypeDeclaration(asCObjectType *ot, bool readProperties);
	void ReadGlobalVarPointers(asCScriptFunction *func);
	void ReadByteCode(asDWORD *bc, int length);

	// Helper functions for storing variable data
	int FindObjectTypeIdx(asCObjectType*);
	asCObjectType *FindObjectType(int idx);
	int FindTypeIdIdx(int typeId);
	int FindTypeId(int idx);
	int FindFunctionIndex(asCScriptFunction *func);
	asCScriptFunction *FindFunction(int idx);
	int FindGlobalPropPtrIndex(void *);
	int FindStringConstantIndex(int id);

	// Intermediate data used for storing that which isn't constant, function id's, pointers, etc
	void WriteUsedTypeIds();
	void WriteUsedFunctions();
	void WriteUsedGlobalProps();
	void WriteUsedStringConstants();

	void ReadUsedTypeIds();
	void ReadUsedFunctions();
	void ReadUsedGlobalProps();
	void ReadUsedStringConstants();

	// After loading, each function needs to be translated to update pointers, function ids, etc
	void TranslateFunction(asCScriptFunction *func);

	// Temporary storage for persisting variable data	
	asCArray<int>                usedTypeIds;
	asCArray<asCObjectType*>     usedTypes;
	asCArray<asCScriptFunction*> usedFunctions;
	asCArray<void*>              usedGlobalProperties;
	asCArray<int>                usedStringConstants;

	asCArray<asCScriptFunction*> savedFunctions;
};

END_AS_NAMESPACE

#endif //AS_RESTORE_H
