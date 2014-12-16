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
// as_context.h
//
// This class handles the execution of the byte code
//


#ifndef AS_CONTEXT_H
#define AS_CONTEXT_H

#include "as_config.h"
#include "as_atomic.h"
#include "as_array.h"
#include "as_string.h"
#include "as_objecttype.h"
#include "as_callfunc.h"

BEGIN_AS_NAMESPACE

class asCScriptFunction;
class asCScriptEngine;

// TODO: The context should be renamed to something that better describes it, e.g. asIVirtualMachine, asIExecuter, asIProcessor, asIScriptThread, or something like that

class asCContext : public asIScriptContext
{
public:
	// From asIScriptContext
	int  AddRef();
	int  Release();

	asIScriptEngine *GetEngine();

	asEContextState GetState();

	int  Prepare(int functionID);
	int  Unprepare();

	int SetArgByte(asUINT arg, asBYTE value);
	int SetArgWord(asUINT arg, asWORD value);
	int SetArgDWord(asUINT arg, asDWORD value);
	int SetArgQWord(asUINT arg, asQWORD value);
	int SetArgFloat(asUINT arg, float value);
	int SetArgDouble(asUINT arg, double value);
	int SetArgAddress(asUINT arg, void *addr);
	int SetArgObject(asUINT arg, void *obj);
	void *GetAddressOfArg(asUINT arg);

	int SetObject(void *obj);

	asBYTE  GetReturnByte();
	asWORD  GetReturnWord();
	asDWORD GetReturnDWord();
	asQWORD GetReturnQWord();
	float   GetReturnFloat();
	double  GetReturnDouble();
	void   *GetReturnAddress();
	void   *GetReturnObject();
	void   *GetAddressOfReturnValue();

	int  Execute();
	int  Abort();
	int  Suspend();

	int  GetCurrentLineNumber(int *column);
	int  GetCurrentFunction();

	int  SetException(const char *descr);
	int  GetExceptionLineNumber(int *column);
	int  GetExceptionFunction();
	const char *GetExceptionString();

	int  SetLineCallback(asSFuncPtr callback, void *obj, int callConv);
	void ClearLineCallback();
	int  SetExceptionCallback(asSFuncPtr callback, void *obj, int callConv);
	void ClearExceptionCallback();

	int GetCallstackSize();
	int GetCallstackFunction(int index);
	int GetCallstackLineNumber(int index, int *column);

	int         GetVarCount(int stackLevel);
	const char *GetVarName(int varIndex, int stackLevel);
	const char *GetVarDeclaration(int varIndex, int stackLevel);
	int         GetVarTypeId(int varIndex, int stackLevel);
	void       *GetAddressOfVar(int varIndex, int stackLevel);
	int         GetThisTypeId(int stackLevel);
    void       *GetThisPointer(int stackLevel);

	void *SetUserData(void *data);
	void *GetUserData();

public:
	// Internal public functions
	asCContext(asCScriptEngine *engine, bool holdRef);
	virtual ~asCContext();

#ifdef AS_DEPRECATED
// Deprecated since 2009-12-08, 2.18.0
	int  SetExecuteStringFunction(asCScriptFunction *func);
#endif

//protected:
	friend class asCScriptEngine;

	void CallLineCallback();
	void CallExceptionCallback();

	int  CallGeneric(int funcID, void *objectPointer);

	void DetachEngine();

	void ExecuteNext();
	void CleanStack();
	void CleanStackFrame();
	void CleanReturnObject();

	void PushCallState();
	void PopCallState();
	void CallScriptFunction(asCScriptFunction *func);
	void CallInterfaceMethod(asCScriptFunction *func);

	void SetInternalException(const char *descr);

	// Must be protected for multiple accesses
	asCAtomic refCount;

	bool holdEngineRef;
	asCScriptEngine *engine;

	asEContextState status;
	bool doSuspend;
	bool doAbort;
	bool externalSuspendRequest;
	bool isCallingSystemFunction;

	asCScriptFunction *currentFunction;
	bool isStackMemoryNotAllocated;

	asCArray<size_t> callStack;
	asCArray<asDWORD *> stackBlocks;
	int stackBlockSize;
	int stackIndex;

	bool inExceptionHandler;
	asCString exceptionString;
	int exceptionFunction;
	int exceptionLine;
	int exceptionColumn;

	int returnValueSize;
	int argumentsSize;

#ifdef AS_DEPRECATED
// Deprecated since 2009-12-08, 2.18.0
	// String function
	asCScriptFunction *stringFunction;
#endif

	asCScriptFunction *initialFunction;

	// callbacks
	bool lineCallback;
	asSSystemFunctionInterface lineCallbackFunc;
	void *lineCallbackObj;

	bool exceptionCallback;
	asSSystemFunctionInterface exceptionCallbackFunc;
	void *exceptionCallbackObj;

	void *userData;

	// Registers available to JIT compiler functions
	asSVMRegisters regs;
};

END_AS_NAMESPACE

#endif
