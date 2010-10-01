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
// as_module.h
//
// A class that holds a script module
//

#ifndef AS_MODULE_H
#define AS_MODULE_H

#include "as_config.h"
#include "as_thread.h"
#include "as_string.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_scriptfunction.h"
#include "as_property.h"

BEGIN_AS_NAMESPACE

const int asFUNC_INIT   = 0xFFFF;
const int asFUNC_STRING = 0xFFFE;

const int FUNC_SYSTEM   = 0x80000000;
const int FUNC_IMPORTED = 0x40000000;

class asCScriptEngine;
class asCCompiler;
class asCBuilder;
class asCContext;
class asCConfigGroup;

struct sBindInfo
{
	asDWORD importFrom;
	int importedFunction;
};

class asCModule
{
public:
	asCModule(const char *name, int id, asCScriptEngine *engine);
	~asCModule();

	int  AddScriptSection(const char *name, const char *code, int codeLength, int lineOffset, bool makeCopy);
	int  Build(asIOutputStream *out);
	void Discard();

	int  ResetGlobalVars();

	int  GetFunctionCount();
	int  GetFunctionIDByName(const char *name);
	int  GetFunctionIDByDecl(const char *decl);

	int  GetGlobalVarCount();
	int  GetGlobalVarIDByName(const char *name);
	int  GetGlobalVarIDByDecl(const char *decl);

	asCString name;

//protected:
	friend class asCScriptEngine;
	friend class asCBuilder;
	friend class asCCompiler;
	friend class asCContext;
	friend class asCRestore;

	void Reset();

	void CallInit();
	void CallExit();
	bool isGlobalVarInitialized;

	bool IsUsed();

	int AddConstantString(const char *str, asUINT length);
	const asCString &GetConstantString(int id);

	int AllocGlobalMemory(int size);

	int GetNextFunctionId();
	int AddScriptFunction(int sectionIdx, int id, const char *name, const asCDataType &returnType, asCDataType *params, int *inOutFlags, int paramCount);
	int AddImportedFunction(int id, const char *name, const asCDataType &returnType, asCDataType *params, int *inOutFlags, int paramCount, int moduleNameStringID);

	bool CanDeleteAllReferences(asCArray<asCModule*> &modules);

	int  GetNextImportedFunctionId();
	int  GetImportedFunctionCount();
	int  GetImportedFunctionIndexByDecl(const char *decl);
	const char *GetImportedFunctionSourceModule(int index);
	int  BindImportedFunction(int index, int sourceID);
	asCScriptFunction *GetImportedFunction(int funcID);

	asCScriptFunction *GetScriptFunction(int funcID);
	asCScriptFunction *GetSpecialFunction(int funcID);

	asCObjectType *GetObjectType(const char *type);
	asCObjectType *RefObjectType(asCObjectType *type);
	void RefConfigGroupForFunction(int funcId);
	void RefConfigGroupForGlobalVar(int gvarId);
	void RefConfigGroupForObjectType(asCObjectType *type);

	int GetGlobalVarIndex(int propIdx);

	asCScriptEngine *engine;
	asCBuilder *builder;
	bool isBuildWithoutErrors;

	int moduleID;
	bool isDiscarded;

	int AddContextRef();
	int ReleaseContextRef();
	int contextCount;

	int AddModuleRef();
	int ReleaseModuleRef();
	int moduleCount;

	int GetScriptSectionIndex(const char *name);

	bool CanDelete();

	asCScriptFunction initFunction;

	asCArray<asCString *> scriptSections;
	asCArray<asCScriptFunction *> scriptFunctions;
	asCArray<asCScriptFunction *> importedFunctions;
	asCArray<sBindInfo> bindInformations;

	asCArray<asCProperty *> scriptGlobals;
	asCArray<asDWORD> globalMem;

	asCArray<void*> globalVarPointers;

	asCArray<asCString*> stringConstants;

	asCArray<asCObjectType*> structTypes;
	asCArray<asCObjectType*> scriptArrayTypes;

	// Reference to all used types
	asCArray<asCObjectType*> usedTypes;
	asCArray<asCConfigGroup*> configGroups;
	
	DECLARECRITICALSECTION(criticalSection);
};

END_AS_NAMESPACE

#endif
