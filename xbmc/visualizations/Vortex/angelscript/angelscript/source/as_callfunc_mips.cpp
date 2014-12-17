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
// as_callfunc_mips.cpp
//
// These functions handle the actual calling of system functions
//
// This version is MIPS specific and was originally written
// by Manu Evans in April, 2006
//


#include "as_config.h"

#ifndef MAX_PORTABILITY
#ifdef AS_MIPS

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"

#include <stdio.h>
#include <stdlib.h>
#include <regdef.h>

BEGIN_AS_NAMESPACE

#define AS_MIPS_MAX_ARGS 32
#define AS_NUM_REG_FLOATS 8
#define AS_NUM_REG_INTS 8

// The array used to send values to the correct places.
// first 0-8 regular values to load into the a0-a3, t0-t3 registers
// then 0-8 float values to load into the f12-f19 registers
// then (AS_MIPS_MAX_ARGS - 16) values to load onto the stack
// the +1 is for when CallThis (object methods) is used
// extra +1 when returning in memory
extern "C" {
static asDWORD mipsArgs[AS_MIPS_MAX_ARGS + 1 + 1];
}

// Loads all data into the correct places and calls the function.
// intArgSize is the size in bytes for how much data to put in int registers
// floatArgSize is the size in bytes for how much data to put in float registers
// stackArgSize is the size in bytes for how much data to put on the callstack
extern "C" asQWORD mipsFunc(int intArgSize, int floatArgSize, int stackArgSize, asDWORD func);

// puts the arguments in the correct place in the mipsArgs-array. See comments above.
// This could be done better.
inline void splitArgs(const asDWORD *args, int argNum, int &numRegIntArgs, int &numRegFloatArgs, int &numRestArgs, int hostFlags)
{
	int i;

	int argBit = 1;
	for (i = 0; i < argNum; i++)
	{
		if (hostFlags & argBit)
		{
			if (numRegFloatArgs < AS_NUM_REG_FLOATS)
			{
				// put in float register
				mipsArgs[AS_NUM_REG_INTS + numRegFloatArgs] = args[i];
				numRegFloatArgs++;
			}
			else
			{
				// put in stack
				mipsArgs[AS_NUM_REG_INTS + AS_NUM_REG_FLOATS + numRestArgs] = args[i];
				numRestArgs++;
			}
		}
		else
		{
			if (numRegIntArgs < AS_NUM_REG_INTS)
			{
				// put in int register
				mipsArgs[numRegIntArgs] = args[i];
				numRegIntArgs++;
			}
			else
			{
				// put in stack
				mipsArgs[AS_NUM_REG_INTS + AS_NUM_REG_FLOATS + numRestArgs] = args[i];
				numRestArgs++;
			}
		}
		argBit <<= 1;
	}
}

asQWORD CallCDeclFunction(const asDWORD *args, int argSize, asDWORD func, int flags)
{
	int argNum = argSize >> 2;

	int intArgs = 0;
	int floatArgs = 0;
	int restArgs = 0;

	// put the arguments in the correct places in the mipsArgs array
	if(argNum > 0)
		splitArgs(args, argNum, intArgs, floatArgs, restArgs, flags);

	return mipsFunc(intArgs << 2, floatArgs << 2, restArgs << 2, func);
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the first parameter is the object
asQWORD CallThisCallFunction(const void *obj, const asDWORD *args, int argSize, asDWORD func, int flags)
{
	int argNum = argSize >> 2;

	int intArgs = 1;
	int floatArgs = 0;
	int restArgs = 0;

	mipsArgs[0] = (asDWORD) obj;

	// put the arguments in the correct places in the mipsArgs array
	if (argNum > 0)
		splitArgs(args, argNum, intArgs, floatArgs, restArgs, flags);

	return mipsFunc(intArgs << 2, floatArgs << 2, restArgs << 2, func);
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the last parameter is the object
asQWORD CallThisCallFunction_objLast(const void *obj, const asDWORD *args, int argSize, asDWORD func, int flags)
{
	int argNum = argSize >> 2;

	int intArgs = 0;
	int floatArgs = 0;
	int restArgs = 0;

	// put the arguments in the correct places in the mipsArgs array
	if(argNum > 0)
		splitArgs(args, argNum, intArgs, floatArgs, restArgs, flags);

	if(intArgs < AS_NUM_REG_INTS)
	{
		mipsArgs[intArgs] = (asDWORD) obj;
		intArgs++;
	}
	else
	{
		mipsArgs[AS_NUM_REG_INTS + AS_NUM_REG_FLOATS + restArgs] = (asDWORD) obj;
		restArgs++;
	}

	return mipsFunc(intArgs << 2, floatArgs << 2, restArgs << 2, func);
}

asDWORD GetReturnedFloat()
{
	asDWORD f;

	asm("swc1 $f0, %0\n" : "=m"(f));

	return f;
}

/*
asDWORD GetReturnedFloat();

asm(
"	.align 4\n"
"	.global GetReturnedFloat\n"
"GetReturnedFloat:\n"
"	.set	noreorder\n"
"	.set	nomacro\n"
"	j	$ra\n"
"	mfc1 $v0, $f0\n"
"	.set	macro\n"
"	.set	reorder\n"
"	.end	Func\n"
*/


// sizeof(double) == 4 with sh-elf-gcc (3.4.0) -m4
// so this isn't really used...
asQWORD GetReturnedDouble()
{
	asQWORD d;

	printf("Broken!!!");
/*
	asm("sw $v0, %0\n" : "=m"(d));
*/
	return d;
}

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine *engine = context->engine;
	asSSystemFunctionInterface *sysFunc = engine->scriptFunctions[id]->sysFuncIntf;
	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	asQWORD retQW = 0;

	asCScriptFunction *descr = engine->scriptFunctions[id];

	void    *func              = (void*)sysFunc->func;
	int      paramSize         = sysFunc->paramSize;
	asDWORD *args              = context->regs.stackPointer;
	void    *retPointer = 0;
	void    *obj = 0;
	asDWORD *vftable;
	int      popSize           = paramSize;

	context->regs.objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retPointer = engine->CallAlloc(descr->returnType.GetObjectType());
		mipsArgs[AS_MIPS_MAX_ARGS+1] = (asDWORD) retPointer;

		if( sysFunc->hostReturnInMemory )
		{
			// The return is made in memory
			callConv++;
		}
	}

	if( callConv >= ICC_THISCALL )
	{
		if( objectPointer )
		{
			obj = objectPointer;
		}
		else
		{
			// The object pointer should be popped from the context stack
			popSize++;

			// Check for null pointer
			obj = (void*)*(args + paramSize);
			if( obj == 0 )
			{
				context->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( retPointer )
					engine->CallFree(retPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
			obj = (void*)(int(obj) + sysFunc->baseOffset);
		}
	}
	asASSERT(descr->parameterTypes.GetLength() <= AS_MIPS_MAX_ARGS);

	// mark all float arguments
	int argBit = 1;
	int hostFlags = 0;
	int intArgs = 0;
	for( size_t a = 0; a < descr->parameterTypes.GetLength(); a++ )
	{
		if (descr->parameterTypes[a].IsFloatType()) 
			hostFlags |= argBit;
		else 
			intArgs++;
		argBit <<= 1;
	}

	asDWORD paramBuffer[64];
	if( sysFunc->takesObjByVal )
	{
		paramSize = 0;
		int spos = 0;
		int dpos = 1;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
			{
#ifdef COMPLEX_OBJS_PASSED_BY_REF
				if( descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK )
				{
					paramBuffer[dpos++] = args[spos++];
					paramSize++;
				}
				else
#endif
				{
					// Copy the object's memory to the buffer
					memcpy(&paramBuffer[dpos], *(void**)(args+spos), descr->parameterTypes[n].GetSizeInMemoryBytes());
					// Delete the original memory
					engine->CallFree(*(char**)(args+spos));
					spos++;
					dpos += descr->parameterTypes[n].GetSizeInMemoryDWords();
					paramSize += descr->parameterTypes[n].GetSizeInMemoryDWords();
				}
			}
			else
			{
				// Copy the value directly
				paramBuffer[dpos++] = args[spos++];
				if( descr->parameterTypes[n].GetSizeOnStackDWords() > 1 )
					paramBuffer[dpos++] = args[spos++];
				paramSize += descr->parameterTypes[n].GetSizeOnStackDWords();
			}
		}
		// Keep a free location at the beginning
		args = &paramBuffer[1];
	}

	context->isCallingSystemFunction = true;
	switch( callConv )
	{
	case ICC_CDECL:
	case ICC_CDECL_RETURNINMEM:
	case ICC_STDCALL:
	case ICC_STDCALL_RETURNINMEM:
		retQW = CallCDeclFunction(args, paramSize<<2, (asDWORD)func, hostFlags);
		break;
	case ICC_THISCALL:
	case ICC_THISCALL_RETURNINMEM:
		retQW = CallThisCallFunction(obj, args, paramSize<<2, (asDWORD)func, hostFlags);
		break;
	case ICC_VIRTUAL_THISCALL:
	case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		// Get virtual function table from the object pointer
		vftable = *(asDWORD**)obj;
		retQW = CallThisCallFunction(obj, args, paramSize<<2, vftable[asDWORD(func)>>2], hostFlags);
		break;
	case ICC_CDECL_OBJLAST:
	case ICC_CDECL_OBJLAST_RETURNINMEM:
		retQW = CallThisCallFunction_objLast(obj, args, paramSize<<2, (asDWORD)func, hostFlags);
		break;
	case ICC_CDECL_OBJFIRST:
	case ICC_CDECL_OBJFIRST_RETURNINMEM:
		retQW = CallThisCallFunction(obj, args, paramSize<<2, (asDWORD)func, hostFlags);
		break;
	default:
		context->SetInternalException(TXT_INVALID_CALLING_CONVENTION);
	}
	context->isCallingSystemFunction = false;

#ifdef COMPLEX_OBJS_PASSED_BY_REF
	if( sysFunc->takesObjByVal )
	{
		// Need to free the complex objects passed by value
		args = context->regs.stackPointer;
		if( callConv >= (int)ICC_THISCALL && !objectPointer )
		    args++;

		int spos = 0;
		for( size_t n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() &&
				!descr->parameterTypes[n].IsReference() &&
				(descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) )
			{
				void *obj = (void*)args[spos++];
				asSTypeBehaviour *beh = &descr->parameterTypes[n].GetObjectType()->beh;
				if( beh->destruct )
					engine->CallObjectMethod(obj, beh->destruct);

				engine->CallFree(obj);
			}
			else
				spos += descr->parameterTypes[n].GetSizeInMemoryDWords();
		}
	}
#endif

	// Store the returned value in our stack
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() )
	{
		if( descr->returnType.IsObjectHandle() )
		{
			context->regs.objectRegister = (void*)(asDWORD)retQW;

			if( sysFunc->returnAutoHandle && context->regs.objectRegister )
				engine->CallObjectMethod(context->regs.objectRegister, descr->returnType.GetObjectType()->beh.addref);
		}
		else
		{
			if( !sysFunc->hostReturnInMemory )
			{
				// Copy the returned value to the pointer sent by the script engine
				if( sysFunc->hostReturnSize == 1 )
					*(asDWORD*)retPointer = (asDWORD)retQW;
				else
					*(asQWORD*)retPointer = retQW;
			}

			// Store the object in the register
			context->regs.objectRegister = retPointer;
		}
	}
	else
	{
		// Store value in valueRegister
		if( sysFunc->hostReturnFloat )
		{
			if( sysFunc->hostReturnSize == 1 )
				*(asDWORD*)&context->regs.valueRegister = GetReturnedFloat();
			else
				context->regs.valueRegister = GetReturnedDouble();
		}
		else if( sysFunc->hostReturnSize == 1 )
			*(asDWORD*)&context->regs.valueRegister = (asDWORD)retQW;
		else
			context->regs.valueRegister = retQW;
	}

	if( sysFunc->hasAutoHandles )
	{
		args = context->regs.stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
			args++;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( sysFunc->paramAutoHandles[n] && args[spos] )
			{
				// Call the release method on the type
				engine->CallObjectMethod((void*)args[spos], descr->parameterTypes[n].GetObjectType()->beh.release);
				args[spos] = 0;
			}

			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
				spos++;
			else
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
		}
	}

	return popSize;
}

asm(
"	.text\n"
//"	.align 2\n"
"	.global mipsFunc\n"
"	.ent	mipsFunc\n"
"mipsFunc:\n"
//"	.frame	$fp,64,$31		# vars= 0, regs= 0/0, args= 0, gp= 0\n"
//"	.mask	0x00000000,0\n"
//"	.fmask	0x00000000,0\n"
"	.set	noreorder\n"
"	.set	nomacro\n"
// align the stack frame to 8 bytes
"	addiu	$12, $6, 7\n"
"	li		$13, -8\n"			// 0xfffffffffffffffc
"	and		$12, $12, $13\n"	// t4 holds the size of the argument block
// and add 8 bytes for the return pointer and s0 backup
"	addiu	$13, $12, 8\n"		// t5 holds the total size of the stack frame (including return pointer)
// save the s0 register (so we can use it to remember where our return pointer is lives)
"	sw		$16, -4($sp)\n"		// store the s0 register (so we can use it to remember how big our stack frame is)
// push the stack
"	subu	$sp, $sp, $13\n"
// find the return address, place in s0
"	addu	$16, $sp, $12\n"
// store the return pointer
"	sw		$31, 0($16)\n"

// backup our function params
"	addiu	$2, $7, 0\n"
"	addiu	$3, $6, 0\n"

// get global mipsArgs[] array pointer
//"	lui		$15, %hi(mipsArgs)\n"
//"	addiu	$15, $15, %lo(mipsArgs)\n"
// we'll use the macro instead because SN Systems doesnt like %hi/%lo
".set macro\n"
" la  $15, mipsArgs\n"
".set nomacro\n"
// load register params
"	lw		$4, 0($15)\n"
"	lw		$5, 4($15)\n"
"	lw		$6, 8($15)\n"
"	lw		$7, 12($15)\n"
"	lw		$8, 16($15)\n"
"	lw		$9, 20($15)\n"
"	lw		$10, 24($15)\n"
"	lw		$11, 28($15)\n"

// load float params
"	lwc1	$f12, 32($15)\n"
"	lwc1	$f13, 36($15)\n"
"	lwc1	$f14, 40($15)\n"
"	lwc1	$f15, 44($15)\n"
"	lwc1	$f16, 48($15)\n"
"	lwc1	$f17, 52($15)\n"
"	lwc1	$f18, 56($15)\n"
"	lwc1	$f19, 60($15)\n"

// skip stack paramaters if there are none
"	beq		$3, $0, andCall\n"

// push stack paramaters
"	addiu	$15, $15, 64\n"
"pushArgs:\n"
"	addiu	$3, -4\n"
// load from $15 + stack bytes ($3)
"	addu	$14, $15, $3\n"
"	lw		$14, 0($14)\n"
// store to $sp + stack bytes ($3)
"	addu	$13, $sp, $3\n"
"	sw		$14, 0($13)\n"
// if there are more, loop...
"	bne		$3, $0, pushArgs\n"
"	nop\n"

// and call the function
"andCall:\n"
"	jal		$2\n"
"	nop\n"

// restore the return pointer
"	lw		$31, 0($16)\n"
// pop the stack pointer (remembering the return pointer was 8 bytes below the top)
"	addiu	$sp, $16, 8\n"
// and return from the function
"	jr		$31\n"
// restore the s0 register (in the branch delay slot)
"	lw		$16, -4($sp)\n"
"	.set	macro\n"
"	.set	reorder\n"
"	.end	mipsFunc\n"
"	.size	mipsFunc, .-mipsFunc\n"
);

END_AS_NAMESPACE

#endif // AS_MIPS
#endif // AS_MAX_PORTABILITY




