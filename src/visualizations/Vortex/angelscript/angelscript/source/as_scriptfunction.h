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
// as_scriptfunction.h
//
// A container for a compiled script function
//



#ifndef AS_SCRIPTFUNCTION_H
#define AS_SCRIPTFUNCTION_H

#include "as_config.h"
#include "as_string.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_atomic.h"

BEGIN_AS_NAMESPACE

class asCScriptEngine;
class asCModule;
class asCConfigGroup;
class asCGlobalProperty;

struct asSScriptVariable
{
	asCString name;
	asCDataType type;
	int stackOffset;
};

const int asFUNC_SYSTEM    = 0;
const int asFUNC_SCRIPT    = 1;
const int asFUNC_INTERFACE = 2;
const int asFUNC_IMPORTED  = 3;
const int asFUNC_VIRTUAL   = 4;

struct asSSystemFunctionInterface;

// TODO: Need a method for obtaining the function type, so that the application can differenciate between the types
//       This should replace the IsClassMethod and IsInterfaceMethod

// TODO: GetModuleName should be removed. A function won't belong to a specific module anymore
//       as the function can be removed from the module, but still remain alive. For example
//       for dynamically generated functions held by a function pointer.

// TODO: Might be interesting to allow enumeration of accessed global variables, and 
//       also functions/methods that are being called.

void RegisterScriptFunction(asCScriptEngine *engine);

class asCScriptFunction : public asIScriptFunction
{
public:
	// From asIScriptFunction
	asIScriptEngine     *GetEngine() const;

	// Memory management
	int AddRef();
	int Release();

	int                  GetId() const;
	const char          *GetModuleName() const;
	asIObjectType       *GetObjectType() const;
	const char          *GetObjectName() const;
	const char          *GetName() const;
	const char          *GetDeclaration(bool includeObjectName = true) const;
	const char          *GetScriptSectionName() const;
	const char          *GetConfigGroup() const;

	bool                 IsClassMethod() const;
	bool                 IsInterfaceMethod() const;
	bool                 IsReadOnly() const;

	int                  GetParamCount() const;
	int                  GetParamTypeId(int index, asDWORD *flags = 0) const;
	int                  GetReturnTypeId() const;

	// For JIT compilation
	asDWORD             *GetByteCode(asUINT *length = 0);

public:
	//-----------------------------------
	// Internal methods

	asCScriptFunction(asCScriptEngine *engine, asCModule *mod, int funcType);
	~asCScriptFunction();

	void      AddVariable(asCString &name, asCDataType &type, int stackOffset);

	int       GetSpaceNeededForArguments();
	int       GetSpaceNeededForReturnValue();
	asCString GetDeclarationStr(bool includeObjectName = true) const;
	int       GetLineNumber(int programPosition);
	void      ComputeSignatureId();
	bool      IsSignatureEqual(const asCScriptFunction *func) const;

    void      JITCompile();

	void      AddReferences();
	void      ReleaseReferences();

	int                GetGlobalVarPtrIndex(int gvarId);
	asCConfigGroup    *GetConfigGroupByGlobalVarPtrIndex(int index);
	asCGlobalProperty *GetPropertyByGlobalVarPtrIndex(int index);

	// GC methods
	int  GetRefCount();
	void SetFlag();
	bool GetFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllHandles(asIScriptEngine *engine);

public:
	//-----------------------------------
	// Properties

	asCAtomic                    refCount;
	bool                         gcFlag;
	asCScriptEngine             *engine;
	asCModule                   *module;

	// Function signature
	asCString                    name;
	asCDataType                  returnType;
	asCArray<asCDataType>        parameterTypes;
	asCArray<asETypeModifiers>   inOutFlags;
	bool                         isReadOnly;
	asCObjectType               *objectType;
	int                          signatureId;

	int                          id;

	int                          funcType;

	// Used by asFUNC_SCRIPT
	asCArray<asDWORD>            byteCode;
	asCArray<asCObjectType*>     objVariableTypes;
	asCArray<int>	             objVariablePos;
	int                          stackNeeded;
	asCArray<int>                lineNumbers;      // debug info
	asCArray<asSScriptVariable*> variables;        // debug info
	int                          scriptSectionIdx; // debug info
	bool                         dontCleanUpOnException;   // Stub functions don't own the object and parameters

	// This array holds pointers to all global variables that the function access.
	// The byte code holds an index into this table to refer to a global variable.
	asCArray<void*>              globalVarPointers;

	// Used by asFUNC_VIRTUAL
	int                          vfTableIdx;

	// Used by asFUNC_SYSTEM
	asSSystemFunctionInterface  *sysFuncIntf;

    // JIT compiled code of this function
    asJITFunction                jitFunction;
};

END_AS_NAMESPACE

#endif
