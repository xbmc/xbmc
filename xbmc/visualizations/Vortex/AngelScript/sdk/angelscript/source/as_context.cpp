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
// as_context.cpp
//
// This class handles the execution of the byte code
//

#include <math.h> // fmodf()

#include "as_config.h"
#include "as_context.h"
#include "as_scriptengine.h"
#include "as_tokendef.h"
#include "as_bytecodedef.h"
#include "as_texts.h"
#include "as_callfunc.h"
#include "as_module.h"
#include "as_generic.h"
#include "as_debug.h" // mkdir()
#include "as_bytecode.h"

BEGIN_AS_NAMESPACE

// We need at least 2 DWORDs reserved for exception handling
// We need at least 1 DWORD reserved for calling system functions
const int RESERVE_STACK = 2;

// For each script function call we push 6 DWORDs on the call stack
const int CALLSTACK_FRAME_SIZE = 6;


#ifdef AS_DEBUG
// Instruction statistics
int instrCount[256];

int instrCount2[256][256];
int lastBC;

class asCDebugStats
{
public:
	asCDebugStats() 
	{
		memset(instrCount, 0, sizeof(instrCount)); 
	}

	~asCDebugStats() 
	{
		mkdir("AS_DEBUG"); 
		FILE *f = fopen("AS_DEBUG/total.txt", "at");
		if( f )
		{
			// Output instruction statistics
			fprintf(f, "\nTotal count\n");
			int n;
			for( n = 0; n < BC_MAXBYTECODE; n++ )
			{
				if( bcName[n].name && instrCount[n] > 0 )
					fprintf(f, "%-10.10s : %.0f\n", bcName[n].name, instrCount[n]);
			}

			fprintf(f, "\nNever executed\n");
			for( n = 0; n < BC_MAXBYTECODE; n++ )
			{
				if( bcName[n].name && instrCount[n] == 0 )
					fprintf(f, "%-10.10s\n", bcName[n].name);
			}

			fclose(f);
		}
	}

	double instrCount[256];
} stats;
#endif

AS_API asIScriptContext *asGetActiveContext()
{
	asCThreadLocalData *tld = threadManager.GetLocalData();
	if( tld->activeContexts.GetLength() == 0 )
		return 0;
	return tld->activeContexts[tld->activeContexts.GetLength()-1];
}

void asPushActiveContext(asIScriptContext *ctx)
{
	asCThreadLocalData *tld = threadManager.GetLocalData();
	tld->activeContexts.PushLast(ctx);
}

void asPopActiveContext(asIScriptContext *ctx)
{
	asCThreadLocalData *tld = threadManager.GetLocalData();

	assert(tld->activeContexts.GetLength() > 0);
	assert(tld->activeContexts[tld->activeContexts.GetLength()-1] == ctx);

	tld->activeContexts.PopLast();
}

asCContext::asCContext(asCScriptEngine *engine, bool holdRef)
{
#ifdef AS_DEBUG
	memset(instrCount, 0, sizeof(instrCount));

	memset(instrCount2, 0, sizeof(instrCount2));

	lastBC = 255;
#endif
	
	holdEngineRef = holdRef;
	if( holdRef )
		engine->AddRef();
	this->engine = engine;

	status = tsUninitialized;
	stackBlockSize = 0;
	refCount = 1;
	module = 0;
	inExceptionHandler = false;
	isStackMemoryNotAllocated = false;

	stringFunction = 0;
	currentFunction = 0;
	objectRegister = 0;
	initialFunction = 0;

	lineCallback = false;
	exceptionCallback = false;

	doProcessSuspend = false;
}

asCContext::~asCContext()
{
	DetachEngine();

	for( asUINT n = 0; n < stackBlocks.GetLength(); n++ )
	{
		if( stackBlocks[n] )
			delete[] stackBlocks[n];
	}
	stackBlocks.SetLength(0);

	if( stringFunction )
		delete stringFunction;
}

int asCContext::AddRef()
{
	ENTERCRITICALSECTION(criticalSection);
	int r = ++refCount;
	LEAVECRITICALSECTION(criticalSection);

	return r;
}

int asCContext::Release()
{
	ENTERCRITICALSECTION(criticalSection);
	int r = --refCount;

	if( refCount == 0 )
	{
		LEAVECRITICALSECTION(criticalSection);
		delete this;
		return 0;
	}
	LEAVECRITICALSECTION(criticalSection);

	return r;
}

void asCContext::DetachEngine()
{
	if( engine == 0 ) return;

	// Abort any execution
	Abort();

	// Release module
	if( module )
	{
		module->ReleaseContextRef();
		module = 0;
	}

	// Clear engine pointer
	if( holdEngineRef )
		engine->Release();
	engine = 0;
}

asIScriptEngine *asCContext::GetEngine()
{
	return engine;
}

int asCContext::Prepare(int funcID)
{
	if( status == tsActive || status == tsSuspended )
		return asCONTEXT_ACTIVE;

	// Release the returned object (if any)
	CleanReturnObject();
		
	if( funcID == -1 )
	{
		// Use the previously prepared function
		if( initialFunction == 0 )
			return asNO_FUNCTION;

		currentFunction = initialFunction;
	}
	else
	{
		// Check engine pointer
		if( engine == 0 ) return asERROR;

		if( status == tsActive || status == tsSuspended )
			return asCONTEXT_ACTIVE;

		initialFunction = engine->GetScriptFunction(funcID);
		currentFunction = initialFunction;
		if( currentFunction == 0 )
			return asNO_FUNCTION;

		// Remove reference to previous module. Add reference to new module
		if( module ) module->ReleaseContextRef();
		module = engine->GetModule(funcID);
		if( module ) 
			module->AddContextRef(); 
		else 
			return asNO_MODULE;

		// Determine the minimum stack size needed
		int stackSize = currentFunction->GetSpaceNeededForArguments() + currentFunction->stackNeeded + RESERVE_STACK;

		stackSize = stackSize > engine->initialContextStackSize ? stackSize : engine->initialContextStackSize;

		if( stackSize != stackBlockSize )
		{
			for( asUINT n = 0; n < stackBlocks.GetLength(); n++ )
				if( stackBlocks[n] )
					delete[] stackBlocks[n];
			stackBlocks.SetLength(0);

			stackBlockSize = stackSize;

			asDWORD *stack = new asDWORD[stackBlockSize];
			stackBlocks.PushLast(stack);
		}

		// Reserve space for the arguments and return value
		returnValueSize = currentFunction->GetSpaceNeededForReturnValue();
		argumentsSize = currentFunction->GetSpaceNeededForArguments();
	}

	byteCode = currentFunction->byteCode.AddressOf();

	// Reset state
	exceptionLine = -1;
	exceptionFunction = 0;
	isCallingSystemFunction = false;
	doAbort = false;
	doSuspend = false;
	doProcessSuspend = lineCallback;
	externalSuspendRequest = false;
	status = tsPrepared;

	assert(objectRegister == 0);
	objectRegister = 0;

	// Reserve space for the arguments and return value
	stackFramePointer = stackBlocks[0] + stackBlockSize - argumentsSize;
	stackPointer = stackFramePointer;
	stackIndex = 0;
	
	// Set arguments to 0
	memset(stackPointer, 0, 4*argumentsSize);

	// Set all object variables to 0
	for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
	{
		int pos = currentFunction->objVariablePos[n];
		stackFramePointer[-pos] = 0;
	}

	return asSUCCESS;
}

int asCContext::SetExecuteStringFunction(asCScriptFunction *func)
{
	// TODO: Make thread safe

	// TODO: Verify that the context isn't running

	if( stringFunction )
		delete stringFunction;

	stringFunction = func;

	return 0;
}

int asCContext::PrepareSpecial(int funcID)
{
	// Check engine pointer
	if( engine == 0 ) return asERROR;

	if( status == tsActive || status == tsSuspended )
		return asCONTEXT_ACTIVE;

	exceptionLine = -1;
	exceptionFunction = 0;

	isCallingSystemFunction = false;

	if( module ) module->ReleaseContextRef();

	module = engine->GetModule(funcID);
	module->AddContextRef();

	if( (funcID & 0xFFFF) == asFUNC_STRING )
		initialFunction = stringFunction;
	else
		initialFunction = module->GetSpecialFunction(funcID & 0xFFFF);

	currentFunction = initialFunction;
	if( currentFunction == 0 )
		return asERROR;

	byteCode = currentFunction->byteCode.AddressOf();

	doAbort = false;
	doSuspend = false;
	doProcessSuspend = lineCallback;
	externalSuspendRequest = false;
	status = tsPrepared;

	// Determine the minimum stack size needed
	int stackSize = currentFunction->stackNeeded + RESERVE_STACK;

	stackSize = stackSize > engine->initialContextStackSize ? stackSize : engine->initialContextStackSize;

	if( stackSize != stackBlockSize )
	{
		for( asUINT n = 0; n < stackBlocks.GetLength(); n++ )
			if( stackBlocks[n] )
				delete[] stackBlocks[n];
		stackBlocks.SetLength(0);

		stackBlockSize = stackSize;

		asDWORD *stack = new asDWORD[stackBlockSize];
		stackBlocks.PushLast(stack);
	}

	// Reserve space for the arguments and return value
	returnValueSize = currentFunction->GetSpaceNeededForReturnValue();
	argumentsSize = currentFunction->GetSpaceNeededForArguments();

	stackFramePointer = stackBlocks[0] + stackBlockSize - argumentsSize;
	stackPointer = stackFramePointer;
	stackIndex = 0;
	
	// Set arguments to 0
	memset(stackPointer, 0, 4*argumentsSize);

	// Set all object variables to 0
	for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
	{
		int pos = currentFunction->objVariablePos[n];
		stackFramePointer[-pos] = 0;
	}

	return asSUCCESS;
}


asDWORD asCContext::GetReturnDWord()
{
	if( status != tsProgramFinished ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() ) return 0;
	assert(!dt->IsReference());

	return *(asDWORD*)&register1;
}

asQWORD asCContext::GetReturnQWord()
{
	if( status != tsProgramFinished ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() ) return 0;
	assert(!dt->IsReference());

	return register1;
}

float asCContext::GetReturnFloat()
{
	if( status != tsProgramFinished ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() ) return 0;
	assert(!dt->IsReference());

	return *(float*)&register1;
}

double asCContext::GetReturnDouble()
{
	if( status != tsProgramFinished ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() ) return 0;
	assert(!dt->IsReference());

	return *(double*)&register1;
}

void *asCContext::GetReturnObject()
{
	if( status != tsProgramFinished ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	assert(!dt->IsReference());

	if( !dt->IsObject() ) return 0;

	return objectRegister;
}

int asCContext::SetArgDWord(asUINT arg, asDWORD value)
{
	if( status != tsPrepared )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() ) 
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeOnStackDWords() != 1 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	stackFramePointer[offset] = value;

	return 0;
}

int asCContext::SetArgQWord(asUINT arg, asQWORD value)
{
	if( status != tsPrepared )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() ) 
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeOnStackDWords() != 2 ) 
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(asQWORD*)(&stackFramePointer[offset]) = value;

	return 0;
}

int asCContext::SetArgFloat(asUINT arg, float value)
{
	if( status != tsPrepared )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() ) 
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeOnStackDWords() != 1 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(float*)(&stackFramePointer[offset]) = value;

	return 0;
}

int asCContext::SetArgDouble(asUINT arg, double value)
{
	if( status != tsPrepared )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeOnStackDWords() != 2 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(double*)(&stackFramePointer[offset]) = value;

	return 0;
}

int asCContext::SetArgObject(asUINT arg, void *obj)
{
	if( status != tsPrepared )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( !dt->IsObject() ) 
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// If the object should be sent by value we must make a copy of it
	if( !dt->IsReference() ) 
	{
		if( dt->IsObjectHandle() )
		{
			// Increase the reference counter
			asSTypeBehaviour *beh = &dt->GetObjectType()->beh;
			if( beh->addref )
				engine->CallObjectMethod(obj, beh->addref);
		}
		else
		{
			// Allocate memory
			char *mem = (char*)engine->CallAlloc(dt->GetObjectType());

			// Call the object's default constructor
			asSTypeBehaviour *beh = &dt->GetObjectType()->beh;
			if( beh->construct )
				engine->CallObjectMethod(mem, beh->construct);

			// Call the object's assignment operator
			if( beh->copy )
				engine->CallObjectMethod(mem, obj, beh->copy);
			else
			{
				// Default operator is a simple copy
				memcpy(mem, obj, dt->GetSizeInMemoryBytes());
			}

			obj = mem;
		}
	}

	// Determine the position of the argument
	int offset = 0;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	stackFramePointer[offset] = (asDWORD)obj;

	return 0;
}


int asCContext::Abort()
{
	// TODO: Make thread safe

	if( engine == 0 ) return asERROR;

	// TODO: Can't clean the stack here
	if( status == tsSuspended )
	{
		status = tsProgramAborted;
		CleanStack();
	}
	
	CleanReturnObject();

	doSuspend = true;
	doProcessSuspend = true;
	externalSuspendRequest = true;
	doAbort = true;

	return 0;
}

int asCContext::Suspend()
{
	// TODO: Make thread safe

	if( engine == 0 ) return asERROR;

	doSuspend = true;
	doProcessSuspend = true;
	externalSuspendRequest = true;

	return 0;
}

int asCContext::Execute()
{
	// Check engine pointer
	if( engine == 0 ) return asERROR;

	if( status != tsSuspended && status != tsPrepared )
		return asERROR;

	status = tsSuspended;

	asPushActiveContext((asIScriptContext *)this);

	while( !doSuspend && status == tsSuspended )
	{
		status = tsActive;
		while( status == tsActive )
			ExecuteNext();
	}

	doSuspend = false;
	doProcessSuspend = lineCallback;

	asPopActiveContext((asIScriptContext *)this);


#ifdef AS_DEBUG
	// Output instruction statistics
	mkdir("AS_DEBUG");
	FILE *f = fopen("AS_DEBUG/stats.txt", "at");
	fprintf(f, "\n");
	asQWORD total = 0;
	int n;
	for( n = 0; n < 256; n++ )
	{
		if( bcName[n].name && instrCount[n] )
			fprintf(f, "%-10.10s : %d\n", bcName[n].name, instrCount[n]);

		total += instrCount[n];
	}

	fprintf(f, "\ntotal      : %I64d\n", total);

	fprintf(f, "\n");
	for( n = 0; n < 256; n++ )
	{
		if( bcName[n].name )
		{
			for( int m = 0; m < 256; m++ )
			{
				if( instrCount2[n][m] )
					fprintf(f, "%-10.10s, %-10.10s : %d\n", bcName[n].name, bcName[m].name, instrCount2[n][m]);
			}
		}
	}
	fclose(f);
#endif

	if( doAbort )
	{
		doAbort = false;

		// TODO: Cleaning the stack is also an execution thus the context is active
		// We shouldn't decrease the numActiveContexts until after this is complete
		CleanStack();
		status = tsProgramAborted;
		return asEXECUTION_ABORTED;
	}

	if( status == tsSuspended )
		return asEXECUTION_SUSPENDED;

	if( status == tsProgramFinished )
	{
		objectType = initialFunction->returnType.GetObjectType();
		return asEXECUTION_FINISHED;
	}

	if( status == tsUnhandledException )
		return asEXECUTION_EXCEPTION;

	return asERROR;
}

void asCContext::PushCallState()
{
	// TODO: Pointer size
	callStack.SetLength(callStack.GetLength() + CALLSTACK_FRAME_SIZE);

	asDWORD *s = (asDWORD *)callStack.AddressOf() + callStack.GetLength() - CALLSTACK_FRAME_SIZE;

	s[0] = (asDWORD)stackFramePointer;
	s[1] = (asDWORD)currentFunction;
	s[2] = (asDWORD)byteCode;
	s[3] = (asDWORD)stackPointer;
	s[4] = stackIndex;
	s[5] = (asDWORD)module;
}

void asCContext::PopCallState()
{
	// TODO: Pointer size
	asDWORD *s = (asDWORD *)callStack.AddressOf() + callStack.GetLength() - CALLSTACK_FRAME_SIZE;

	stackFramePointer = (asDWORD *)s[0];
	currentFunction   = (asCScriptFunction *)s[1];
	byteCode          = (asDWORD *)s[2];
	stackPointer      = (asDWORD *)s[3];
	stackIndex        = s[4];
	module            = (asCModule *)s[5];

	callStack.SetLength(callStack.GetLength() - CALLSTACK_FRAME_SIZE);
}

int asCContext::GetCallstackSize()
{
	return callStack.GetLength() / CALLSTACK_FRAME_SIZE;
}

int asCContext::GetCallstackFunction(int index)
{
	if( index < 0 || index >= GetCallstackSize() ) return asINVALID_ARG;

	// TODO: Pointer size
	asCScriptFunction *func = (asCScriptFunction*)callStack[index*CALLSTACK_FRAME_SIZE + 1];
	asCModule *module = (asCModule*)callStack[index*CALLSTACK_FRAME_SIZE + 5];

	return module->moduleID | func->id;
}

int asCContext::GetCallstackLineNumber(int index, int *column)
{
	if( index < 0 || index >= GetCallstackSize() ) return asINVALID_ARG;

	// TODO: Pointer size
	asCScriptFunction *func = (asCScriptFunction*)callStack[index*CALLSTACK_FRAME_SIZE + 1];
	asDWORD *bytePos = (asDWORD*)callStack[index*CALLSTACK_FRAME_SIZE + 2];

	asDWORD line = func->GetLineNumber(bytePos - func->byteCode.AddressOf());
	if( column ) *column = (line >> 20);

	return (line & 0xFFFFF);
}

void asCContext::CallScriptFunction(asCModule *mod, asCScriptFunction *func)
{
	// Push the framepointer, functionid and programCounter on the stack
	PushCallState();

	currentFunction = func;
	module = mod;
	byteCode = currentFunction->byteCode.AddressOf();

	// Verify if there is enough room in the stack block. Allocate new block if not
	asDWORD *oldStackPointer = stackPointer;
	while( stackPointer - (func->stackNeeded + RESERVE_STACK) < stackBlocks[stackIndex] )
	{
		// The size of each stack block is determined by the following formula:
		// size = stackBlockSize << index

		// Make sure we don't allocate more space than allowed
		if( engine->maximumContextStackSize )
		{
			// This test will only stop growth once it has already crossed the limit
			if( stackBlockSize * ((1 << (stackIndex+1)) - 1) > engine->maximumContextStackSize )
			{
				isStackMemoryNotAllocated = true;
				
				// Set the stackFramePointer, even though the stackPointer wasn't updated
				stackFramePointer = stackPointer;

				// TODO: Make sure the exception handler doesn't try to free objects that have not been initialized
				SetInternalException(TXT_STACK_OVERFLOW);
				return;
			}
		}

		stackIndex++;
		if( (int)stackBlocks.GetLength() == stackIndex )
		{
			asDWORD *stack = new asDWORD[stackBlockSize << stackIndex];
			stackBlocks.PushLast(stack);
		}

		stackPointer = stackBlocks[stackIndex] + (stackBlockSize<<stackIndex) - func->GetSpaceNeededForArguments();
	}

	if( stackPointer != oldStackPointer )
	{
		// Copy the function arguments to the new stack space
		memcpy(stackPointer, oldStackPointer, 4*func->GetSpaceNeededForArguments());
	}

	// Update framepointer and programCounter
	stackFramePointer = stackPointer;

	// Set all object variables to 0
	// TODO: Pointer size
	for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
	{
		int pos = currentFunction->objVariablePos[n];
		stackFramePointer[-pos] = 0;
	}
}

#define DWORDARG(x)  (asDWORD(*(x+1)))
#define INTARG(x)    (int(*(x+1)))
#define QWORDARG(x)  (*(asQWORD*)(x+1))
#define FLOATARG(x)  (*(float*)(x+1))

#define WORDARG0(x)   (*(((asWORD*)x)+1))
#define WORDARG1(x)   (*(((asWORD*)x)+2))

#define SWORDARG0(x) (*(((short*)x)+1))
#define SWORDARG1(x) (*(((short*)x)+2))
#define SWORDARG2(x) (*(((short*)x)+3))


void asCContext::ExecuteNext()
{
	asDWORD *l_bc = byteCode;
	asDWORD *l_sp = stackPointer;
	asDWORD *l_fp = stackFramePointer;

	for(;;)
	{

#ifdef AS_DEBUG
	++stats.instrCount[(*l_bc)&0xFF];

	++instrCount[(*l_bc)&0xFF];

	++instrCount2[lastBC][(*l_bc)&0xFF];
	lastBC = (*l_bc)&0xFF;

	// Used to verify that the size of the instructions are correct
	asDWORD *old = l_bc;
#endif


	// Remember to keep the cases in order and without 
	// gaps, because that will make the switch faster. 
	// It will be faster since only one lookup will be 
	// made to find the correct jump destination. If not
	// in order, the switch will make two lookups.
	switch( *(asBYTE*)l_bc )
	{
//--------------
// memory access functions
	case BC_POP:
		l_sp += WORDARG0(l_bc);
		l_bc++;
		break;

	case BC_PUSH:
		l_sp -= WORDARG0(l_bc);
		l_bc++;
		break;

	case BC_PshC4:
		--l_sp;
		*l_sp = DWORDARG(l_bc);
		l_bc += 2;
		break;

	case BC_PshV4:
		--l_sp;
		*l_sp = *(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_PSF:
		--l_sp;
		*l_sp = asDWORD(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_SWAP4:
		{
			asDWORD d = *l_sp;
			*l_sp = *(l_sp+1);
			*(l_sp+1) = d;
			l_bc++;
		}
		break;

	case BC_NOT:
		*(l_fp - SWORDARG0(l_bc)) = (*(l_fp - SWORDARG0(l_bc)) == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
		l_bc++;
		break;

	case BC_PshG4:
		--l_sp;
		*l_sp = *(asDWORD*)module->globalVarPointers[WORDARG0(l_bc)];
		l_bc++;
		break;

	case BC_LdGRdR4:
		*(asDWORD**)&register1 = (asDWORD*)module->globalVarPointers[WORDARG1(l_bc)];
		// TODO: Pointer size
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asDWORD*)(asDWORD)register1;
		l_bc += 2;
		break;


//----------------
// path control instructions
	case BC_CALL:
		{
			int i = INTARG(l_bc);
			l_bc += 2;

			assert( i >= 0 );
			assert( (i & FUNC_IMPORTED) == 0 );

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			CallScriptFunction(module, module->GetScriptFunction(i));

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// If status isn't active anymore then we must stop
			if( status != tsActive )
				return;
		}
		break;

	case BC_RET:
		{
			if( callStack.GetLength() == 0 )
			{
				status = tsProgramFinished;
				return;
			}

			asWORD w = WORDARG0(l_bc);

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			// Read the old framepointer, functionid, and programCounter from the stack
			PopCallState();

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// Pop arguments from stack
			l_sp += w;
		}
		break;

	case BC_JMP:
		l_bc += 2 + INTARG(l_bc);
		break;

//----------------
// Conditional jumps
	case BC_JZ:
		if( *(int*)&register1 == 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;

	case BC_JNZ:
		if( *(int*)&register1 != 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;
	case BC_JS:
		if( *(int*)&register1 < 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;
	case BC_JNS:
		if( *(int*)&register1 >= 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;
	case BC_JP:
		if( *(int*)&register1 > 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;
	case BC_JNP:
		if( *(int*)&register1 <= 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;
//--------------------
// test instructions
	case BC_TZ:
		*(int*)&register1 = (*(int*)&register1 == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
		l_bc++;
		break;
	case BC_TNZ:
		*(int*)&register1 = (*(int*)&register1 == 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
		l_bc++;
		break;
	case BC_TS:
		*(int*)&register1 = (*(int*)&register1 < 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
		l_bc++;
		break;
	case BC_TNS:
		*(int*)&register1 = (*(int*)&register1 < 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
		l_bc++;
		break;
	case BC_TP:
		*(int*)&register1 = (*(int*)&register1 > 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
		l_bc++;
		break;
	case BC_TNP:
		*(int*)&register1 = (*(int*)&register1 > 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
		l_bc++;
		break;

//--------------------
// negate value
	case BC_NEGi:
		*(l_fp - SWORDARG0(l_bc)) = asDWORD(-int(*(l_fp - SWORDARG0(l_bc))));
		l_bc++;
		break;
	case BC_NEGf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = -*(float*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_NEGd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = -*(double*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

//-------------------------
// Increment value pointed to by address in register
	case BC_INCi16:
		// TODO: Pointer size
		(*(short*)(asDWORD)register1)++;
		l_bc++;
		break;

	case BC_INCi8:
		// TODO: Pointer size
		(*(char*)(asDWORD)register1)++;
		l_bc++;
		break;

	case BC_DECi16:
		// TODO: Pointer size
		(*(short*)(asDWORD)register1)--;
		l_bc++;
		break;

	case BC_DECi8:
		// TODO: Pointer size
		(*(char*)(asDWORD)register1)--;
		l_bc++;
		break;
	case BC_INCi:
		// TODO: Pointer size
		++(*(int*)(asDWORD)register1);
		l_bc++;
		break;

	case BC_DECi:
		// TODO: Pointer size
		--(*(int*)(asDWORD)register1);
		l_bc++;
		break;

	case BC_INCf:
		// TODO: Pointer size
		++(*(float*)(asDWORD)register1);
		l_bc++;
		break;

	case BC_DECf:
		// TODO: Pointer size
		--(*(float*)(asDWORD)register1);
		l_bc++;
		break;
	case BC_INCd:
		// TODO: Pointer size
		++(*(double*)(asDWORD)register1);
		l_bc++;
		break;

	case BC_DECd:
		// TODO: Pointer size
		--(*(double*)(asDWORD)register1);
		l_bc++;
		break;

//--------------------
// Increment value pointed to by address in register
	case BC_IncVi:
		(*(l_fp - SWORDARG0(l_bc)))++;
		l_bc++;
		break;
	case BC_DecVi:
		(*(l_fp - SWORDARG0(l_bc)))--;
		l_bc++;
		break;

//--------------------
// bits instructions
	case BC_BNOT:
		*(l_fp - SWORDARG0(l_bc)) = ~*(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_BAND:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) & *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BOR:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) | *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BXOR:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) ^ *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BSLL:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) << *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BSRL:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) >> *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BSRA:
		*(l_fp - SWORDARG0(l_bc)) = int(*(l_fp - SWORDARG1(l_bc))) >> *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_COPY:
		{
			void *d = (void*)*l_sp++;
			void *s = (void*)*l_sp;
			if( s == 0 || d == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
			memcpy(d, s, WORDARG0(l_bc)*4);
		}
		l_bc++;
		break;

	case BC_SET8:
		l_sp -= 2;
		*(asQWORD*)l_sp = QWORDARG(l_bc);
		l_bc += 3;
		break;

	case BC_RDS8:
		// TODO: Pointer size
		*(asQWORD*)(l_sp-1) = *(asQWORD*)*l_sp;
		--l_sp;
		l_bc++;
		break;

	case BC_SWAP8:
		{
			asQWORD q = *(asQWORD*)l_sp;
			*(asQWORD*)l_sp = *(asQWORD*)(l_sp+2);
			*(asQWORD*)(l_sp+2) = q;
			l_bc++;
		}
		break;

	//----------------------------
	// Comparisons
	case BC_CMPd:
		{
			double dbl = *(double*)(l_fp - SWORDARG0(l_bc)) - *(double*)(l_fp - SWORDARG1(l_bc));
			if( dbl == 0 )     *(int*)&register1 =  0;
			else if( dbl < 0 ) *(int*)&register1 = -1;
			else               *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPu:
		{
			asDWORD d = *(l_fp - SWORDARG0(l_bc));
			asDWORD d2 = *(l_fp - SWORDARG1(l_bc));
			if( d == d2 )     *(int*)&register1 =  0;
			else if( d < d2 ) *(int*)&register1 = -1;
			else              *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPf:
		{
			float f = *(float*)(l_fp - SWORDARG0(l_bc)) - *(float*)(l_fp - SWORDARG1(l_bc));
			if( f == 0 )     *(int*)&register1 =  0;
			else if( f < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPi:
		{
			int i = *(int*)(l_fp - SWORDARG0(l_bc)) - *(int*)(l_fp - SWORDARG1(l_bc));
			if( i == 0 )     *(int*)&register1 =  0;
			else if( i < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	//----------------------------
	// Comparisons with constant value
	case BC_CMPIi:
		{
			int i = *(int*)(l_fp - SWORDARG0(l_bc)) - INTARG(l_bc);
			if( i == 0 )     *(int*)&register1 =  0;
			else if( i < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPIf:
		{
			float f = *(float*)(l_fp - SWORDARG0(l_bc)) - FLOATARG(l_bc);
			if( f == 0 )     *(int*)&register1 =  0;
			else if( f < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPIu:
		{
			asDWORD d1 = *(l_fp - SWORDARG0(l_bc));
			asDWORD d2 = DWORDARG(l_bc);
			if( d1 == d2 )     *(int*)&register1 =  0;
			else if( d1 < d2 ) *(int*)&register1 = -1;
			else               *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_JMPP:
		l_bc += 1 + (*(int*)(l_fp - SWORDARG0(l_bc)))*2;
		break;

	case BC_PopRPtr:
		// TODO: Pointer size
		*(asDWORD*)&register1 = *l_sp;
		l_sp++;
		l_bc++;
		break;

	case BC_PshRPtr:
		// TODO: Pointer size
		l_sp--;
		*l_sp = *(asDWORD*)&register1;
		l_bc++;
		break;

	case BC_STR:
		{
			// Get the string id from the argument
			asWORD w = WORDARG0(l_bc);
			// Push the string pointer on the stack
			// TODO: Pointer size
			--l_sp;
			const asCString &b = module->GetConstantString(w);
			*l_sp = (asDWORD)b.AddressOf();
			// Push the string length on the stack
			--l_sp;
			*l_sp = b.GetLength();
			l_bc++;
		}
		break;

	case BC_CALLSYS:
		{
			// Get function ID from the argument
			int i = INTARG(l_bc);
			assert( i < 0 );

			// Need to move the values back to the context 
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			l_sp += CallSystemFunction(i, this, 0);

			// Update the program position after the call so that line number is correct
			l_bc += 2;

			// Should the execution be suspended?
			if( doSuspend )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				status = tsSuspended;
				return;
			}
			// An exception might have been raised
			if( status != tsActive )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				return;
			}
		}
		break;

	case BC_CALLBND:
		{
			// Get the function ID from the stack
			int i = INTARG(l_bc);
			l_bc += 2;

			assert( i >= 0 );
			assert( i & FUNC_IMPORTED );

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			int funcID = module->bindInformations[i&0xFFFF].importedFunction;
			if( funcID == -1 )
			{
				SetInternalException(TXT_UNBOUND_FUNCTION);
				return;
			}
			else
			{
				asCModule *callModule = engine->GetModule(funcID);
				asCScriptFunction *func = callModule->GetScriptFunction(funcID);

				CallScriptFunction(callModule, func);
			}

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// If status isn't active anymore then we must stop
			if( status != tsActive )
				return;
		}
		break;

	case BC_SUSPEND:
		if( doProcessSuspend )
		{
			if( lineCallback )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				CallLineCallback();
			}
			l_bc++;
			if( doSuspend )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				status = tsSuspended;
				return;
			}
		}
		else
			l_bc++;
		break;
	case BC_ALLOC:
		{
			// TODO: Pointer size. Don't read pointer from arg, use lookup table instead
			asCObjectType *objType = (asCObjectType*)DWORDARG(l_bc);
			int func = INTARG(l_bc+1);
			asDWORD *mem = (asDWORD*)engine->CallAlloc(objType);
			
			if( func )
			{
				// Need to move the values back to the context 
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				l_sp += CallSystemFunction(func, this, mem);
			}

			// Pop the variable address from the stack
			// TODO: Pointer size
			asDWORD **a = (asDWORD**)*l_sp++;
			if( a ) *a = mem;

			l_bc += 3;

			// Should the execution be suspended?
			if( doSuspend )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				status = tsSuspended;
				return;
			}
			// An exception might have been raised
			if( status != tsActive )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				engine->CallFree(objType, mem);
				*a = 0;

				return;
			}
		}
		break;
	case BC_FREE:
		{
			// TODO: Pointer size
			asDWORD **a = (asDWORD**)*l_sp++;
			if( a && *a )
			{
				// TODO: Pointer size. Don't read pointer from arg, use lookup table instead
				asCObjectType *objType = (asCObjectType*)DWORDARG(l_bc);
				asSTypeBehaviour *beh = &objType->beh;

				// Need to move the values back to the context 
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				if( beh->release )
				{
					engine->CallObjectMethod(*a, beh->release);

					// The release method will free the memory
				}
				else
				{
					if( beh->destruct )
					{
						// Call the destructor
						engine->CallObjectMethod(*a, beh->destruct);
					}

					engine->CallFree(objType, *a);
				}
				*a = 0;
			}
		}
		l_bc += 2;
		break;
	case BC_LOADOBJ:
		{
			// Move the object pointer from the object variable into the object register
			void **a = (void**)asDWORD(l_fp - SWORDARG0(l_bc));
			objectType = 0;
			objectRegister = *a;
			*a = 0;
		}
		l_bc++;
		break;
	case BC_STOREOBJ:
		// Move the object pointer from the object register to the object variable
		// TODO: Pointer size
		*(l_fp - SWORDARG0(l_bc)) = asDWORD(objectRegister);
		objectRegister = 0;
		l_bc++;
		break;
	case BC_GETOBJ:
		{
			asDWORD *a = l_sp + WORDARG0(l_bc);
			asDWORD offset = *a;
			asDWORD *v = l_fp - offset;
			*a = *v;
			*v = 0;
		}
		l_bc++;
		break;
	case BC_REFCPY:
		{
			// TODO: Pointer size. Don't read pointer from arg, use lookup table instead
			asCObjectType *objType = (asCObjectType*)DWORDARG(l_bc);
			asSTypeBehaviour *beh = &objType->beh;
			void **d = (void**)*l_sp++;
			void *s = (void*)*l_sp;

			// Need to move the values back to the context 
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			if( *d != 0 )
				engine->CallObjectMethod(*d, beh->release);
			if( s != 0 )
				engine->CallObjectMethod(s, beh->addref);
			*d = s;
		}
		l_bc += 2;
		break;
	case BC_CHKREF:
		{
			// TODO: Pointer size
			asDWORD a = *l_sp;
			if( a == 0 )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
		}
		l_bc++;
		break;

	case BC_GETOBJREF:
		{
			// TODO: Pointer size
			asDWORD *a = l_sp + WORDARG0(l_bc);
			*(asDWORD**)a = *(asDWORD**)(l_fp - *a);
		}
		l_bc++;
		break;
	case BC_GETREF:
		{
			// TODO: Pointer size
			asDWORD *a = l_sp + WORDARG0(l_bc);
			*(asDWORD**)a = l_fp - *a;
		}
		l_bc++;
		break;
	case BC_SWAP48:
		{
			asDWORD d = *(asDWORD*)l_sp;
			asQWORD q = *(asQWORD*)(l_sp+1);
			*(asQWORD*)l_sp = q;
			*(asDWORD*)(l_sp+2) = d;
			l_bc++;
		}
		break;
	case BC_SWAP84:
		{
			asQWORD q = *(asQWORD*)l_sp;
			asDWORD d = *(asDWORD*)(l_sp+2);
			*(asDWORD*)l_sp = d;
			*(asQWORD*)(l_sp+1) = q;
			l_bc++;
		}
		break;
	case BC_OBJTYPE:
		{
			--l_sp;
			// TODO: Pointer size. Don't read pointer from arg, use lookup table instead
			asCObjectType *objType = (asCObjectType*)DWORDARG(l_bc);
			*l_sp = (asDWORD)objType;
			l_bc += 2;
		}
		break;
	case BC_TYPEID:
		{
			--l_sp;
			asDWORD typeId = DWORDARG(l_bc);
			*l_sp = typeId;
			l_bc += 2;
		}
		break;
	case BC_SetV4:
		*(l_fp - SWORDARG0(l_bc)) = DWORDARG(l_bc);
		l_bc += 2;
		break;
	case BC_SetV8:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = QWORDARG(l_bc);
		l_bc += 3;
		break;
	case BC_ADDSi:
		// TODO: Pointer size. This command is used to offset a pointer on the stack
		*l_sp = asDWORD(int(*l_sp) + INTARG(l_bc));
		l_bc += 2;
		break;

	case BC_CpyVtoV4:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc));
		l_bc += 2;
		break;
	case BC_CpyVtoV8:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc));
		l_bc += 2;
		break;
	case BC_CpyVtoR4:
		*(asDWORD*)&register1 = *(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_CpyVtoR8:
		*(asQWORD*)&register1 = *(asQWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_CpyVtoG4:
		*(asDWORD*)module->globalVarPointers[WORDARG0(l_bc)] = *(asDWORD*)(l_fp - SWORDARG1(l_bc));
		l_bc += 2;
		break;
	case BC_CpyRtoV4:
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asDWORD*)&register1;
		l_bc++;
		break;
	case BC_CpyRtoV8:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = register1;
		l_bc++;
		break;
	case BC_CpyGtoV4:
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asDWORD*)module->globalVarPointers[WORDARG1(l_bc)];
		l_bc += 2;
		break;

	case BC_WRTV1:
		// TODO: Pointer size
		*(asBYTE*)(asDWORD)register1 = *(asBYTE*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_WRTV2:
		// TODO: Pointer size
		*(asWORD*)(asDWORD)register1 = *(asWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_WRTV4:
		// TODO: Pointer size
		*(asDWORD*)(asDWORD)register1 = *(asDWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_WRTV8:
		// TODO: Pointer size
		*(asQWORD*)(asDWORD)register1 = *(asQWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_RDR1:
		// TODO: Pointer size
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asBYTE*)(asDWORD)register1;
		l_bc++;
		break;
	case BC_RDR2:
		// TODO: Pointer size
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asWORD*)(asDWORD)register1;
		l_bc++;
		break;
	case BC_RDR4:
		// TODO: Pointer size
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asDWORD*)(asDWORD)register1;
		l_bc++;
		break;
	case BC_RDR8:
		// TODO: Pointer size
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(asDWORD)register1;
		l_bc++;
		break;

	case BC_LDG:
		*(asDWORD**)&register1 = (asDWORD*)module->globalVarPointers[WORDARG0(l_bc)];
		l_bc++;
		break;
	case BC_LDV:
		// TODO: Pointer size
		register1 = asDWORD(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_PGA:
		// TODO: Pointer size
		--l_sp;
		*l_sp = (asDWORD)module->globalVarPointers[WORDARG0(l_bc)];
		l_bc++;
		break;
	case BC_RDS4:
		*l_sp = *(asDWORD*)(*l_sp);
		l_bc++;
		break;
	case BC_VAR:
		--l_sp;
		*l_sp = SWORDARG0(l_bc);
		l_bc++;
		break;

	//----------------------------
	// Type conversions
	case BC_iTOf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(int*)(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;

	case BC_fTOi:
		*(l_fp - SWORDARG0(l_bc)) = int(*(float*)(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;

	case BC_uTOf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;
	
	case BC_fTOu:
		*(l_fp - SWORDARG0(l_bc)) = asUINT(*(float*)(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;
		
	case BC_sbTOi:
		*(l_fp - SWORDARG0(l_bc)) = *(char*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_swTOi:
		*(l_fp - SWORDARG0(l_bc)) = *(short*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_ubTOi:
		*(l_fp - SWORDARG0(l_bc)) = *(asBYTE*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_uwTOi:
		*(l_fp - SWORDARG0(l_bc)) = *(asWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;
	case BC_dTOi:
		*(l_fp - SWORDARG0(l_bc)) = int(*(double*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_dTOu:
		*(l_fp - SWORDARG0(l_bc)) = asUINT(*(double*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_dTOf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(double*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_iTOd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(int*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_uTOd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(asUINT*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_fTOd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(float*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	//------------------------------
	// Math operations
	case BC_ADDi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) + *(int*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_SUBi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) - *(int*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_MULi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) * *(int*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_DIVi:
		{
			int divider = *(int*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) / divider;
		}
		l_bc += 2;
		break;

	case BC_MODi:
		{
			int divider = *(int*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) % divider;
		}
		l_bc += 2;
		break;

	case BC_ADDf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) + *(float*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_SUBf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) - *(float*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_MULf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) * *(float*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_DIVf:
		{
			float divider = *(float*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) / divider;
		}
		l_bc += 2;
		break;

	case BC_MODf:
		{
			float divider = *(float*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(float*)(l_fp - SWORDARG0(l_bc)) = fmodf(*(float*)(l_fp - SWORDARG1(l_bc)), divider);
		}
		l_bc += 2;
		break;

	case BC_ADDd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) + *(double*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;
	
	case BC_SUBd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) - *(double*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_MULd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) * *(double*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_DIVd:
		{
			double divider = *(double*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			
			*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) / divider;
			l_bc += 2;
		}
		break;

	case BC_MODd:
		{
			double divider = *(double*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			
			*(double*)(l_fp - SWORDARG0(l_bc)) = fmod(*(double*)(l_fp - SWORDARG1(l_bc)), divider);
			l_bc += 2;
		}
		break;

	//------------------------------
	// Math operations with constant value
	case BC_ADDIi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) + INTARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_SUBIi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) - INTARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_MULIi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) * INTARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_ADDIf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) + FLOATARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_SUBIf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) - FLOATARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_MULIf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) * FLOATARG(l_bc+1);
		l_bc += 3;
		break;

	//-----------------------------------
	case BC_SetG4:
		*(asDWORD*)module->globalVarPointers[WORDARG0(l_bc)] = DWORDARG(l_bc);
		l_bc += 2;
		break;

	case BC_ChkRefS:
		{
			// TODO: Pointer size
			asDWORD *a = (asDWORD*)*l_sp;
			if( *a == 0 )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
		}
		l_bc++;
		break;

	case BC_ChkNullV:
		{
			// TODO: Pointer size
			asDWORD *a = *(asDWORD**)(l_fp - SWORDARG0(l_bc));
			if( a == 0 )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
		}
		l_bc++;
		break;

	// Don't let the optimizer optimize for size, 
	// since it requires extra conditions and jumps
	case 139: l_bc = (asDWORD*)139; break;
	case 140: l_bc = (asDWORD*)140; break;
	case 141: l_bc = (asDWORD*)141; break;
	case 142: l_bc = (asDWORD*)142; break;
	case 143: l_bc = (asDWORD*)143; break;
	case 144: l_bc = (asDWORD*)144; break;
	case 145: l_bc = (asDWORD*)145; break;
	case 146: l_bc = (asDWORD*)146; break;
	case 147: l_bc = (asDWORD*)147; break;
	case 148: l_bc = (asDWORD*)148; break;
	case 149: l_bc = (asDWORD*)149; break;
	case 150: l_bc = (asDWORD*)150; break;
	case 151: l_bc = (asDWORD*)151; break;
	case 152: l_bc = (asDWORD*)152; break;
	case 153: l_bc = (asDWORD*)153; break;
	case 154: l_bc = (asDWORD*)154; break;
	case 155: l_bc = (asDWORD*)155; break;
	case 156: l_bc = (asDWORD*)156; break;
	case 157: l_bc = (asDWORD*)157; break;
	case 158: l_bc = (asDWORD*)158; break;
	case 159: l_bc = (asDWORD*)159; break;
	case 160: l_bc = (asDWORD*)160; break;
	case 161: l_bc = (asDWORD*)161; break;
	case 162: l_bc = (asDWORD*)162; break;
	case 163: l_bc = (asDWORD*)163; break;
	case 164: l_bc = (asDWORD*)164; break;
	case 165: l_bc = (asDWORD*)165; break;
	case 166: l_bc = (asDWORD*)166; break;
	case 167: l_bc = (asDWORD*)167; break;
	case 168: l_bc = (asDWORD*)168; break;
	case 169: l_bc = (asDWORD*)169; break;
	case 170: l_bc = (asDWORD*)170; break;
	case 171: l_bc = (asDWORD*)171; break;
	case 172: l_bc = (asDWORD*)172; break;
	case 173: l_bc = (asDWORD*)173; break;
	case 174: l_bc = (asDWORD*)174; break;
	case 175: l_bc = (asDWORD*)175; break;
	case 176: l_bc = (asDWORD*)176; break;
	case 177: l_bc = (asDWORD*)177; break;
	case 178: l_bc = (asDWORD*)178; break;
	case 179: l_bc = (asDWORD*)179; break;
	case 180: l_bc = (asDWORD*)180; break;
	case 181: l_bc = (asDWORD*)181; break;
	case 182: l_bc = (asDWORD*)182; break;
	case 183: l_bc = (asDWORD*)183; break;
	case 184: l_bc = (asDWORD*)184; break;
	case 185: l_bc = (asDWORD*)185; break;
	case 186: l_bc = (asDWORD*)186; break;
	case 187: l_bc = (asDWORD*)187; break;
	case 188: l_bc = (asDWORD*)188; break;
	case 189: l_bc = (asDWORD*)189; break;
	case 190: l_bc = (asDWORD*)190; break;
	case 191: l_bc = (asDWORD*)191; break;
	case 192: l_bc = (asDWORD*)192; break;
	case 193: l_bc = (asDWORD*)193; break;
	case 194: l_bc = (asDWORD*)194; break;
	case 195: l_bc = (asDWORD*)195; break;
	case 196: l_bc = (asDWORD*)196; break;
	case 197: l_bc = (asDWORD*)197; break;
	case 198: l_bc = (asDWORD*)198; break;
	case 199: l_bc = (asDWORD*)199; break;
	case 200: l_bc = (asDWORD*)200; break;
	case 201: l_bc = (asDWORD*)201; break;
	case 202: l_bc = (asDWORD*)202; break;
	case 203: l_bc = (asDWORD*)203; break;
	case 204: l_bc = (asDWORD*)204; break;
	case 205: l_bc = (asDWORD*)205; break;
	case 206: l_bc = (asDWORD*)206; break;
	case 207: l_bc = (asDWORD*)207; break;
	case 208: l_bc = (asDWORD*)208; break;
	case 209: l_bc = (asDWORD*)209; break;
	case 210: l_bc = (asDWORD*)210; break;
	case 211: l_bc = (asDWORD*)211; break;
	case 212: l_bc = (asDWORD*)212; break;
	case 213: l_bc = (asDWORD*)213; break;
	case 214: l_bc = (asDWORD*)214; break;
	case 215: l_bc = (asDWORD*)215; break;
	case 216: l_bc = (asDWORD*)216; break;
	case 217: l_bc = (asDWORD*)217; break;
	case 218: l_bc = (asDWORD*)218; break;
	case 219: l_bc = (asDWORD*)219; break;
	case 220: l_bc = (asDWORD*)220; break;
	case 221: l_bc = (asDWORD*)221; break;
	case 222: l_bc = (asDWORD*)222; break;
	case 223: l_bc = (asDWORD*)223; break;
	case 224: l_bc = (asDWORD*)224; break;
	case 225: l_bc = (asDWORD*)225; break;
	case 226: l_bc = (asDWORD*)226; break;
	case 227: l_bc = (asDWORD*)227; break;
	case 228: l_bc = (asDWORD*)228; break;
	case 229: l_bc = (asDWORD*)229; break;
	case 230: l_bc = (asDWORD*)230; break;
	case 231: l_bc = (asDWORD*)231; break;
	case 232: l_bc = (asDWORD*)232; break;
	case 233: l_bc = (asDWORD*)233; break;
	case 234: l_bc = (asDWORD*)234; break;
	case 235: l_bc = (asDWORD*)235; break;
	case 236: l_bc = (asDWORD*)236; break;
	case 237: l_bc = (asDWORD*)237; break;
	case 238: l_bc = (asDWORD*)238; break;
	case 239: l_bc = (asDWORD*)239; break;
	case 240: l_bc = (asDWORD*)240; break;
	case 241: l_bc = (asDWORD*)241; break;
	case 242: l_bc = (asDWORD*)242; break;
	case 243: l_bc = (asDWORD*)243; break;
	case 244: l_bc = (asDWORD*)244; break;
	case 245: l_bc = (asDWORD*)245; break;
	case 246: l_bc = (asDWORD*)246; break;
	case 247: l_bc = (asDWORD*)247; break;
	case 248: l_bc = (asDWORD*)248; break;
	case 249: l_bc = (asDWORD*)249; break;
	case 250: l_bc = (asDWORD*)250; break;
	case 251: l_bc = (asDWORD*)251; break;
	case 252: l_bc = (asDWORD*)252; break;
	case 253: l_bc = (asDWORD*)253; break;
	case 254: l_bc = (asDWORD*)254; break;
	case 255: l_bc = (asDWORD*)255; break;

#ifdef AS_DEBUG
	default:
		assert(false);
#endif
/*
	default:
		// This Microsoft specific code allows the
		// compiler to optimize the switch case as
		// it will know that the code will never 
		// reach this point
		__assume(0);
*/	}

#ifdef AS_DEBUG
		asDWORD instr = (*old)&0xFF;
		if( instr != BC_JMP && instr != BC_JMPP && (instr < BC_JZ || instr > BC_JNP) &&
			instr != BC_CALL && instr != BC_CALLBND && instr != BC_RET )
		{
			assert( (l_bc - old) == asCByteCode::SizeOfType(bcTypes[instr]) );
		}
#endif
	}

	SetInternalException(TXT_UNRECOGNIZED_BYTE_CODE);
}

int asCContext::SetException(const char *descr)
{
	// Only allow this if we're executing a CALL byte code
	if( !isCallingSystemFunction ) return asERROR;

	SetInternalException(descr);

	return 0;
}

void asCContext::SetInternalException(const char *descr)
{
	if( inExceptionHandler )
	{
		assert(false); // Shouldn't happen
		return; // but if it does, at least this will not crash the application
	}

	status = tsUnhandledException;

	exceptionString = descr;
	exceptionFunction = module->moduleID | currentFunction->id;
	exceptionLine = currentFunction->GetLineNumber(byteCode - currentFunction->byteCode.AddressOf());
	exceptionColumn = exceptionLine >> 20;
	exceptionLine &= 0xFFFFF;

	if( exceptionCallback )
		CallExceptionCallback();

	// Clean up stack
	CleanStack();
}

void asCContext::CleanReturnObject()
{
	if( objectRegister == 0 ) return;

	assert( objectType != 0 );

	if( objectType )
	{
		// Call the destructor on the object
		asSTypeBehaviour *beh = &objectType->beh;
		if( beh->release )
		{
			engine->CallObjectMethod(objectRegister, beh->release);
			objectRegister = 0;

			// The release method is responsible for freeing the memory
		}
		else
		{
			if( beh->destruct )
				engine->CallObjectMethod(objectRegister, beh->destruct);

			// Free the memory
			engine->CallFree(objectType, objectRegister);
			objectRegister = 0;
		}
	}
}

void asCContext::CleanStack()
{
	inExceptionHandler = true;

	// Run the clean up code for each of the functions called
	CleanStackFrame();

	while( callStack.GetLength() > 0 )
	{
		PopCallState();

		CleanStackFrame();
	}
	inExceptionHandler = false;
}

void asCContext::CleanStackFrame()
{
	// Clean object variables
	if( !isStackMemoryNotAllocated )
	{
		for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
		{
			int pos = currentFunction->objVariablePos[n];
			if( stackFramePointer[-pos] )
			{
				// Call the object's destructor
				asSTypeBehaviour *beh = &currentFunction->objVariableTypes[n]->beh;
				if( beh->release )
				{
					engine->CallObjectMethod((void*)stackFramePointer[-pos], beh->release);
					stackFramePointer[-pos] = 0;
				}
				else
				{
					if( beh->destruct )
						engine->CallObjectMethod((void*)stackFramePointer[-pos], beh->destruct);

					// Free the memory
					engine->CallFree(currentFunction->objVariableTypes[n], (void*)stackFramePointer[-pos]);
					stackFramePointer[-pos] = 0;
				}
			}
		}
	}
	else
		isStackMemoryNotAllocated = false;

	// Clean object parameters sent by reference
	int offset = 0;
	for( asUINT n = 0; n < currentFunction->parameterTypes.GetLength(); n++ )
	{
		if( currentFunction->parameterTypes[n].IsObject() && !currentFunction->parameterTypes[n].IsReference() )
		{
			if( stackFramePointer[offset] )
			{
				// Call the object's destructor
				asSTypeBehaviour *beh = currentFunction->parameterTypes[n].GetBehaviour();
				if( beh->release )
				{
					engine->CallObjectMethod((void*)stackFramePointer[offset], beh->release);
					stackFramePointer[offset] = 0;
				}
				else
				{
					if( beh->destruct )
						engine->CallObjectMethod((void*)stackFramePointer[offset], beh->destruct);

					// Free the memory
					engine->CallFree(currentFunction->parameterTypes[n].GetObjectType(), (void*)stackFramePointer[offset]);
					stackFramePointer[offset] = 0;
				}
			}
		}

		offset += currentFunction->parameterTypes[n].GetSizeOnStackDWords();
	}
}

int asCContext::GetExceptionLineNumber(int *column)
{
	if( GetState() != asEXECUTION_EXCEPTION ) return asERROR;

	if( column ) *column = exceptionColumn;

	return exceptionLine;
}

int asCContext::GetExceptionFunction()
{
	if( GetState() != asEXECUTION_EXCEPTION ) return asERROR;

	return exceptionFunction;
}

int asCContext::GetCurrentFunction()
{
	if( status == tsSuspended || status == tsActive )
		return module->moduleID | currentFunction->id;

	return -1;
}

int asCContext::GetCurrentLineNumber(int *column)
{
	if( status == tsSuspended || status == tsActive )
	{
		asDWORD line = currentFunction->GetLineNumber(byteCode - currentFunction->byteCode.AddressOf());
		if( column ) *column = line >> 20;

		return line & 0xFFFFF;
	}

	return -1;
}

const char *asCContext::GetExceptionString(int *length)
{
	if( GetState() != asEXECUTION_EXCEPTION ) return 0;

	if( length ) *length = exceptionString.GetLength();

	return exceptionString.AddressOf();
}

int asCContext::GetState()
{
	if( status == tsSuspended )
		return asEXECUTION_SUSPENDED;

	if( status == tsActive )
		return asEXECUTION_ACTIVE;

	if( status == tsUnhandledException )
		return asEXECUTION_EXCEPTION;

	if( status == tsProgramFinished )
		return asEXECUTION_FINISHED;

	if( status == tsPrepared )
		return asEXECUTION_PREPARED;

	if( status == tsUninitialized )
		return asEXECUTION_UNINITIALIZED;

	return asERROR;
}

int asCContext::SetLineCallback(asUPtr callback, void *obj, int callConv)
{
	lineCallback = true;
	doProcessSuspend = true;
	lineCallbackObj = obj;
	bool isObj = false;
	if( (unsigned)callConv == asCALL_GENERIC )
	{
		lineCallback = false;
		doProcessSuspend = doSuspend;
		return asNOT_SUPPORTED;
	}
	if( (unsigned)callConv >= asCALL_THISCALL )
	{
		isObj = true;
		if( obj == 0 )
		{
			lineCallback = false;
			doProcessSuspend = doSuspend;
			return asINVALID_ARG;
		}
	}
	int r = DetectCallingConvention(isObj, callback, callConv, &lineCallbackFunc);
	if( r < 0 ) lineCallback = false;
	doProcessSuspend = doSuspend || lineCallback;
	return r;
}

void asCContext::CallLineCallback()
{
	if( lineCallbackFunc.callConv < ICC_THISCALL )
		engine->CallGlobalFunction(this, lineCallbackObj, &lineCallbackFunc, 0);
	else
		engine->CallObjectMethod(lineCallbackObj, this, &lineCallbackFunc, 0);
}

int asCContext::SetExceptionCallback(asUPtr callback, void *obj, int callConv)
{
	exceptionCallback = true;
	exceptionCallbackObj = obj;
	bool isObj = false;
	if( (unsigned)callConv == asCALL_GENERIC )
		return asNOT_SUPPORTED;
	if( (unsigned)callConv >= asCALL_THISCALL )
	{
		isObj = true;
		if( obj == 0 )
		{
			exceptionCallback = false;
			return asINVALID_ARG;
		}
	}
	int r = DetectCallingConvention(isObj, callback, callConv, &exceptionCallbackFunc);
	if( r < 0 ) exceptionCallback = false;
	return r;
}

void asCContext::CallExceptionCallback()
{
	if( exceptionCallbackFunc.callConv < ICC_THISCALL )
		engine->CallGlobalFunction(this, exceptionCallbackObj, &exceptionCallbackFunc, 0);
	else
		engine->CallObjectMethod(exceptionCallbackObj, this, &exceptionCallbackFunc, 0);
}

void asCContext::ClearLineCallback()
{
	lineCallback = false;
	doProcessSuspend = doSuspend;
}

void asCContext::ClearExceptionCallback()
{
	exceptionCallback = false;
}

int asCContext::CallGeneric(int id, void *objectPointer)
{
	id = -id - 1;
	asSSystemFunctionInterface *sysFunc = engine->systemFunctionInterfaces[id];
	asCScriptFunction *sysFunction = engine->systemFunctions[id];
	void (*func)(asIScriptGeneric*) = (void (*)(asIScriptGeneric*))sysFunc->func;
	int popSize = sysFunc->paramSize;
	asDWORD *args = stackPointer;

	// Verify the object pointer if it is a class method
	void *currentObject = 0;
	if( sysFunc->callConv == ICC_GENERIC_METHOD )
	{
		if( objectPointer )
		{
			currentObject = objectPointer;

			// Don't increase the reference of this pointer
			// since it will not have been constructed yet
		}
		else
		{
			// The object pointer should be popped from the context stack
			popSize++;

			// Check for null pointer
			currentObject = (void*)*(args);
			if( currentObject == 0 )
			{	
				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return 0;
			}

			// Add the base offset for multiple inheritance
			currentObject = (void*)(int(currentObject) + sysFunc->baseOffset);

			// Keep a reference to the object to protect it 
			// from being released before the method returns
			if( sysFunction->objectType->beh.addref )
				engine->CallObjectMethod(currentObject, sysFunction->objectType->beh.addref);

			// Skip object pointer
			args++;
		}		
	}

	asCGeneric gen(engine, sysFunction, currentObject, args);

	isCallingSystemFunction = true;
	func(&gen);
	isCallingSystemFunction = false;

	register1 = gen.returnVal;
	objectRegister = gen.objectRegister;
	objectType = sysFunction->returnType.GetObjectType();

	// Clean up function parameters
	int offset = 0;
	for( asUINT n = 0; n < sysFunction->parameterTypes.GetLength(); n++ )
	{
		if( sysFunction->parameterTypes[n].IsObject() && !sysFunction->parameterTypes[n].IsReference() )
		{
			void *obj = *(void**)&args[offset];

			// Release the object
			asSTypeBehaviour *beh = &sysFunction->parameterTypes[n].GetObjectType()->beh;
			if( beh->release )
				engine->CallObjectMethod(obj, beh->release);
			else
			{
				// Call the destructor then free the memory
				if( beh->destruct )
					engine->CallObjectMethod(obj, beh->destruct);

				engine->CallFree(sysFunction->parameterTypes[n].GetObjectType(), obj);
			}
		}
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();
	}

	// Release the object pointer
	if( currentObject && sysFunction->objectType->beh.release && !objectPointer )
		engine->CallObjectMethod(currentObject, sysFunction->objectType->beh.release);

	// Return how much should be popped from the stack
	return popSize;
}

int asCContext::GetVarCount(int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return asINVALID_ARG;

	asCScriptFunction *func;
	if( stackLevel == -1 ) 
		func = currentFunction;
	else
		func = (asCScriptFunction*)callStack[stackLevel*CALLSTACK_FRAME_SIZE + 1];

	if( func == 0 )
		return asERROR;

	return func->variables.GetLength();
}

const char *asCContext::GetVarName(int varIndex, int *length, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return 0;

	asCScriptFunction *func;
	if( stackLevel == -1 ) 
		func = currentFunction;
	else
		func = (asCScriptFunction*)callStack[stackLevel*CALLSTACK_FRAME_SIZE + 1];

	if( func == 0 )
		return 0;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return 0;

	if( length )
		*length = func->variables[varIndex]->name.GetLength();

	return func->variables[varIndex]->name.AddressOf();
}

const char *asCContext::GetVarDeclaration(int varIndex, int *length, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return 0;

	asCScriptFunction *func;
	if( stackLevel == -1 ) 
		func = currentFunction;
	else
		func = (asCScriptFunction*)callStack[stackLevel*CALLSTACK_FRAME_SIZE + 1];

	if( func == 0 )
		return 0;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return 0;

	asCString *tempString = &threadManager.GetLocalData()->string;
	*tempString = func->variables[varIndex]->type.Format();
	*tempString += " " + func->variables[varIndex]->name;

	if( length ) *length = tempString->GetLength();

	return tempString->AddressOf();
}

void *asCContext::GetVarPointer(int varIndex, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return 0;

	asCScriptFunction *func;
	asDWORD *sf;
	if( stackLevel == -1 ) 
	{
		func = currentFunction;
		sf = stackFramePointer;
	}
	else
	{
		func = (asCScriptFunction*)callStack[stackLevel*CALLSTACK_FRAME_SIZE + 1];
		sf = (asDWORD*)callStack[stackLevel*CALLSTACK_FRAME_SIZE + 0];
	}

	if( func == 0 )
		return 0;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return 0;
	
	return sf - func->variables[varIndex]->stackOffset;
}

END_AS_NAMESPACE



