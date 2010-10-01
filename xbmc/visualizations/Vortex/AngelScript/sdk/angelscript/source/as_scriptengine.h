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
// as_scriptengine.h
//
// The implementation of the script engine interface
//



#ifndef AS_SCRIPTENGINE_H
#define AS_SCRIPTENGINE_H

#include "as_config.h"
#include "as_thread.h"
#include "as_scriptfunction.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_objecttype.h"
#include "as_module.h"
#include "as_restore.h"
#include "as_callfunc.h"
#include "as_configgroup.h"

BEGIN_AS_NAMESPACE

#define EXECUTESTRINGID 0x7FFFFFFFul

class asCBuilder;
class asCContext;
class asCGCObject;

class asCFunctionStream : public asIOutputStream
{
public:
	asCFunctionStream();

	void Write(const char *text); 

	asOUTPUTFUNC_t func;
	void          *param;
};

class asCScriptEngine : public asIScriptEngine
{
public:
	asCScriptEngine();
	virtual ~asCScriptEngine();

	// Memory management
	int AddRef();
	int Release();

	// Script building
	void SetCommonMessageStream(asIOutputStream *out);
    void SetCommonMessageStream(asOUTPUTFUNC_t outFunc, void *outParam);
	int SetCommonObjectMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);

	int RegisterObjectType(const char *objname, int byteSize, asDWORD flags);
	int RegisterObjectProperty(const char *objname, const char *declaration, int byteOffset);
	int RegisterObjectMethod(const char *objname, const char *declaration, const asUPtr &funcPointer, asDWORD callConv);
	int RegisterObjectBehaviour(const char *objname, asDWORD behaviour, const char *decl, const asUPtr &funcPointer, asDWORD callConv);

	int RegisterGlobalProperty(const char *declaration, void *pointer);
	int RegisterGlobalFunction(const char *declaration, const asUPtr &funcPointer, asDWORD callConv);
	int RegisterGlobalBehaviour(asDWORD behaviour, const char *decl, const asUPtr &funcPointer, asDWORD callConv);

	int RegisterStringFactory(const char *datatype, const asUPtr &factoryFunc, asDWORD callConv);

	int BeginConfigGroup(const char *groupName);
	int EndConfigGroup();
	int RemoveConfigGroup(const char *groupName);
	int SetConfigGroupModuleAccess(const char *groupName, const char *module, bool haveAccess);

	int AddScriptSection(const char *module, const char *name, const char *code, int codeLength, int lineOffset, bool makeCopy);
#ifdef AS_DEPRECATED
	int Build(const char *module, asIOutputStream *out);
#endif
	int Build(const char *module);
	int Discard(const char *module);
	int ResetModule(const char *module);
	int GetModuleIndex(const char *module);
	const char *GetModuleNameFromIndex(int index, int *length);

	int GetFunctionCount(const char *module);
	int GetFunctionIDByIndex(const char *module, int index);
	int GetFunctionIDByName(const char *module, const char *name);
	int GetFunctionIDByDecl(const char *module, const char *decl);
	const char *GetFunctionDeclaration(int funcID, int *length);
	const char *GetFunctionName(int funcID, int *length);
	const char *GetFunctionSection(int funcID, int *length);

	int GetGlobalVarCount(const char *module);
	int GetGlobalVarIDByIndex(const char *module, int index);
	int GetGlobalVarIDByName(const char *module, const char *name);
	int GetGlobalVarIDByDecl(const char *module, const char *decl);
	const char *GetGlobalVarDeclaration(int gvarID, int *length);
	const char *GetGlobalVarName(int gvarID, int *length);
	void *GetGlobalVarPointer(int gvarID);
#ifdef AS_DEPRECATED
	int GetGlobalVarPointer(int gvarID, void **ptr);
#endif

	// Dynamic binding between modules
	int GetImportedFunctionCount(const char *module);
	int GetImportedFunctionIndexByDecl(const char *module, const char *decl);
	const char *GetImportedFunctionDeclaration(const char *module, int index, int *length);
	const char *GetImportedFunctionSourceModule(const char *module, int index, int *length);
	int BindImportedFunction(const char *module, int index, int funcID);
	int UnbindImportedFunction(const char *module, int index);

	int BindAllImportedFunctions(const char *module);
	int UnbindAllImportedFunctions(const char *module);

	// Type identification
	int GetTypeIdByDecl(const char *module, const char *decl);
	const char *GetTypeDeclaration(int typeId, int *length);

	// Script execution
	int SetDefaultContextStackSize(asUINT initial, asUINT maximum);
	asIScriptContext *CreateContext();
#ifdef AS_DEPRECATED
	int CreateContext(asIScriptContext **ctx);
#endif
	void *CreateScriptObject(int typeId);

	// String interpretation
#ifdef AS_DEPRECATED
	int ExecuteString(const char *module, const char *script, asIOutputStream *out, asIScriptContext **ctx, asDWORD flags);
#endif
	int ExecuteString(const char *module, const char *script, asIScriptContext **ctx, asDWORD flags);

	// Bytecode Saving/Restoring
	int SaveByteCode(const char *module, asIBinaryStream *out);
	int LoadByteCode(const char *module, asIBinaryStream *in);


	asCObjectType *GetArrayTypeFromSubType(asCDataType &subType);

	void AddScriptObjectToGC(asCGCObject *obj);
	int GarbageCollect(bool doFullCycle);
	int GetObjectsInGarbageCollectorCount();

//protected:
	friend class asCBuilder;
	friend class asCCompiler;
	friend class asCContext;
	friend class asCDataType;
	friend class asCModule;
	friend class asCRestore;
	friend int CallSystemFunction(int id, asCContext *context);
	friend int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *engine);

	int RegisterSpecialObjectType(const char *objname, int byteSize, asDWORD flags);
	int RegisterSpecialObjectMethod(const char *objname, const char *declaration, const asUPtr &funcPointer, int callConv);
	int RegisterSpecialObjectBehaviour(asCObjectType *objType, asDWORD behaviour, const char *decl, const asUPtr &funcPointer, int callConv);

	void *CallAlloc(asCObjectType *objType);
	void CallFree(asCObjectType *objType, void *obj);
	void CallObjectMethod(void *obj, int func);
	void CallObjectMethod(void *obj, void *param, int func);
	void CallObjectMethod(void *obj, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void CallObjectMethod(void *obj, void *param, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void CallGlobalFunction(void *param1, void *param2, asSSystemFunctionInterface *func, asCScriptFunction *desc);

	void ClearUnusedTypes();
	void RemoveArrayType(asCObjectType *t);

	asCConfigGroup *FindConfigGroup(asCObjectType *ot);
	asCConfigGroup *FindConfigGroupForFunction(int funcId);
	asCConfigGroup *FindConfigGroupForGlobalVar(int gvarId);
	asCConfigGroup *FindConfigGroupForObjectType(asCObjectType *type);

	void Reset();
	void PrepareEngine();
	bool isPrepared;

	int CreateContext(asIScriptContext **context, bool isInternal);

	asCObjectType *GetObjectType(const char *type, int arrayType = 0);

	int AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal);

	asCString GetFunctionDeclaration(int funcID);

	asCScriptFunction *GetScriptFunction(int funcID);

	int GCInternal();

	int initialContextStackSize;
	int maximumContextStackSize;

	// Information registered by host
	asSTypeBehaviour globalBehaviours;
	asCObjectType *defaultArrayObjectType;
	asCObjectType *anyObjectType;
	asCObjectType scriptTypeBehaviours;

	// Store information about registered object types
	asCArray<asCObjectType *> objectTypes;
	// Store information about registered array types
	asCArray<asCObjectType *> arrayTypes;
	asCArray<asCProperty *> globalProps;
	asCArray<void *> globalPropAddresses;
	asCArray<asCScriptFunction *> systemFunctions;
	asCArray<asSSystemFunctionInterface *> systemFunctionInterfaces;
	asCScriptFunction *stringFactory;

	int ConfigError(int err);
	bool configFailed;

	// Script modules
	asCModule *GetModule(const char *name, bool create);
	asCModule *GetModule(int id);

	// These resources must be protected for multiple accesses
	int refCount;
	asCArray<asCModule *> scriptModules;
	asCModule *lastModule;

	asCArray<asCObjectType *> structTypes;
	asCArray<asCObjectType *> scriptArrayTypes;

	// Type identifiers
	int typeIdSeqNbr;
	asCMap<int, asCDataType*> mapTypeIdToDataType;
	int GetTypeIdFromDataType(const asCDataType &dt);
	const asCDataType *GetDataTypeFromTypeId(int typeId);
	void RemoveFromTypeIdMap(asCObjectType *type);

	// Garbage collector
	asCArray<asCGCObject*> gcObjects;
	asCArray<asCGCObject*> unmarked;
	int gcState;
	asUINT gcIdx;

	asCConfigGroup defaultGroup;
	asCArray<asCConfigGroup*> configGroups;
	asCConfigGroup *currentGroup;

	asALLOCFUNC_t global_alloc;
	asFREEFUNC_t  global_free;

	asIOutputStream *outStream;
	asCFunctionStream outStreamFunc;

	// Critical sections for threads
	DECLARECRITICALSECTION(engineCritical);
	DECLARECRITICALSECTION(moduleCritical);
};

END_AS_NAMESPACE

#endif
