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
// as_context.h
//
// This class handles the execution of the byte code
//


#ifndef AS_CONTEXT_H
#define AS_CONTEXT_H

#include "as_config.h"
#include "as_thread.h"
#include "as_array.h"
#include "as_string.h"
#include "as_objecttype.h"
#include "as_callfunc.h"

BEGIN_AS_NAMESPACE

class asCScriptFunction;
class asCScriptEngine;
class asCModule;

class asCContext : public asIScriptContext
{
public:
	asCContext(asCScriptEngine *engine, bool holdRef);
	virtual ~asCContext();

	// Memory management
	int  AddRef();
	int  Release();

	asIScriptEngine *GetEngine();

	int  Prepare(int functionID);
	int  PrepareSpecial(int functionID);

	int  Execute();
	int  Abort();
	int  Suspend();

	int SetArgDWord(asUINT arg, asDWORD value);
	int SetArgQWord(asUINT arg, asQWORD value);
	int SetArgFloat(asUINT arg, float value);
	int SetArgDouble(asUINT arg, double value);
	int SetArgObject(asUINT arg, void *obj);

	asDWORD GetReturnDWord();
	asQWORD GetReturnQWord();
	float   GetReturnFloat();
	double  GetReturnDouble();
	void   *GetReturnObject();

	int  GetState();

	int  GetCurrentLineNumber(int *column);
	int  GetCurrentFunction();

	int  GetExceptionLineNumber(int *column);
	int  GetExceptionFunction();
	const char *GetExceptionString(int *length);

	int  SetLineCallback(asUPtr callback, void *obj, int callConv);
	void ClearLineCallback();
	int  SetExceptionCallback(asUPtr callback, void *obj, int callConv);
	void ClearExceptionCallback();

	int GetCallstackSize();
	int GetCallstackFunction(int index);
	int GetCallstackLineNumber(int index, int *column);

	int GetVarCount(int stackLevel);
	const char *GetVarName(int varIndex, int *length, int stackLevel);
	const char *GetVarDeclaration(int varIndex, int *length, int stackLevel);
	void *GetVarPointer(int varIndex, int stackLevel);

	int  SetException(const char *descr);

	int  SetExecuteStringFunction(asCScriptFunction *func);


//protected:
	friend class asCScriptEngine;
	friend int CallSystemFunction(int id, asCContext *context);

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
	void CallScriptFunction(asCModule *mod, asCScriptFunction *func);

	void SetInternalException(const char *descr);

	// Must be protected for multiple accesses
	int refCount;

	bool holdEngineRef;
	asCScriptEngine *engine;
	asCModule *module;

	int status;
	bool doSuspend;
	bool doAbort;
	bool externalSuspendRequest;
	bool isCallingSystemFunction;
	bool doProcessSuspend;

	asDWORD *byteCode;

	asCScriptFunction *currentFunction;
	asDWORD *stackFramePointer;
	bool isStackMemoryNotAllocated;

	asQWORD register1;

	asCArray<int> callStack;
	asCArray<asDWORD *> stackBlocks;
	asDWORD *stackPointer;
	int stackBlockSize;
	int stackIndex;

	bool inExceptionHandler;
	asCString exceptionString;
	int exceptionFunction;
	int exceptionLine;
	int exceptionColumn;

	int returnValueSize;
	int argumentsSize;

	void          *objectRegister;
	asCObjectType *objectType;

	// String function
	asCScriptFunction *stringFunction;

	asCScriptFunction *initialFunction;

	// callbacks
	bool lineCallback;
	asSSystemFunctionInterface lineCallbackFunc;
	void *lineCallbackObj;

	bool exceptionCallback;
	asSSystemFunctionInterface exceptionCallbackFunc;
	void *exceptionCallbackObj;

	DECLARECRITICALSECTION(criticalSection);
};

enum eContextState
{
	tsUninitialized,
	tsPrepared,
	tsSuspended,
	tsActive,
	tsProgramFinished,
	tsProgramAborted,
	tsUnhandledException
};

END_AS_NAMESPACE

#endif
