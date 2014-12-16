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
// as_callfunc_ppc.cpp
//
// These functions handle the actual calling of system functions
//
// This version is PPC specific
//

#include <stdio.h>

#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_PPC

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"

#include <stdlib.h>

BEGIN_AS_NAMESPACE

// This part was originally written by Pecan Heber, June 2006, for
// use on MacOS X with 32bit PPC processor. He based the code on the
// code in as_callfunc_sh4.cpp

#define AS_PPC_MAX_ARGS 32

// The array used to send values to the correct places.
// Contains a byte of argTypes to indicate the register tYpe to load
// or zero if end of arguments
// The +1 is for when CallThis (object methods) is used
// Extra +1 when returning in memory
// Extra +1 in ppcArgsType to ensure zero end-of-args marker

// TODO: multithread: We need to remove these global variables for thread-safety

enum argTypes { ppcENDARG, ppcINTARG, ppcFLOATARG, ppcDOUBLEARG };
static asDWORD ppcArgs[2*AS_PPC_MAX_ARGS + 1 + 1];

// Using extern "C" because we use this symbol name in the assembly code
extern "C"
{
	static asBYTE ppcArgsType[2*AS_PPC_MAX_ARGS + 1 + 1 + 1];
}

// NOTE: these values are for PowerPC 32 bit. 
#define PPC_LINKAGE_SIZE  (24)                                   // how big the PPC linkage area is in a stack frame
#define PPC_NUM_REGSTORE  (9)                                    // how many registers of the PPC we need to store/restore for ppcFunc()
#define PPC_REGSTORE_SIZE (4*PPC_NUM_REGSTORE)                   // how many bytes are required for register store/restore
#define EXTRA_STACK_SIZE  (PPC_LINKAGE_SIZE + PPC_REGSTORE_SIZE) // memory required, not including parameters, for the stack frame
#define PPC_STACK_SIZE(numParams)  (-( ( ((((numParams)<8)?8:(numParams))<<2) + EXTRA_STACK_SIZE + 15 ) & ~15 ))  // calculates the total stack size needed for ppcFunc64, must pad to 16bytes

// Loads all data into the correct places and calls the function.
// ppcArgsType is an array containing a byte type (enum argTypes) for each argument.
// stackArgSize is the size in bytes for how much data to put on the stack frame
extern "C" asQWORD ppcFunc(const asDWORD* argsPtr, int StackArgSize, asDWORD func);

asm(" .text\n"
	" .align 2\n"		  // align the code to 1 << 2 = 4 bytes
	" .globl _ppcFunc\n"
	"_ppcFunc:\n"

	// We're receiving the following parameters
	
	// r3 : argsPtr
	// r4 : StackArgSize
	// r5 : func

	// The following registers are used through out the function
	
	// r31 : the address of the label address, as reference for all other labels
	// r30 : temporary variable
	// r29 : arg list pointer
	// r28 : number of FPR registers used by the parameters
	// r27 : the function pointer that will be called
	// r26 : the location of the parameters for the call
	// r25 : arg type list pointer
	// r24 : temporary variable
	// r23 : number of GPR registers used by the parameters
	// r1  : this is stack pointer
	// r0  : temporary variable
	// f0  : temporary variable

	// We need to store some of the registers for restoral before returning to caller
	
	// lr - always stored in 8(r1) - this is the return address
	// cr - not required to be stored, but if it is, its place is in 4(r1) - this is the condition register 
	// r1 - always stored in 0(r1) - this is the stack pointer
	// r11 
	// r13 to r31 
	// f14 to f31

	// Store register values and setup our stack frame
	" mflr  r0            \n"   // move the return address into r0
	" stw   r0, 8(r1)     \n"   // Store the return address on the stack
	" stmw  r23, -36(r1)  \n"   // Store registers r23 to r31 on the stack
	" stwux r1, r1, r4    \n"   // Increase the stack with the needed space and store the original value in the destination	
	
	// Obtain an address that we'll use as our position of reference when obtaining addresses of other labels
	" bl    address         \n"
	"address:               \n"
	" mflr  r31             \n"

    // initial registers for the function
	" mr    r29, r3       \n"   // (r29) args list
	" mr    r27, r5       \n"   // load the function pointer to call.  func actually holds the pointer to our function
	" addi  r26, r1, 24   \n"   // setup the pointer to the parameter area to the function we're going to call
	" sub   r0, r0, r0    \n"   // zero out r0
	" mr    r23, r0       \n"   // zero out r23, which holds the number of used GPR registers
	" mr    r28, r0       \n"   // zero our r22, which holds the number of used float registers
	
	// load the global ppcArgsType which holds the types of arguments for each argument
	" addis r25, r31, ha16(_ppcArgsType - address) \n" // load the upper 16 bits of the address to r25
	" la    r25, lo16(_ppcArgsType - address)(r25) \n" // load the lower 16 bits of the address to r25
	" subi  r25, r25, 1    \n"   // since we increment r25 on its use, we'll pre-decrement it

    // loop through the arguments
	"ppcNextArg:           \n"
	" addi r25, r25, 1     \n"   // increment r25, our arg type pointer
	// switch based on the current argument type (0:end, 1:int, 2:float 3:double)
	" lbz  r24, 0(r25)     \n"   // load the current argument type (it's a byte)
	" mulli r24, r24, 4    \n"   // our jump table has 4 bytes per case (1 instruction)
	" addis r30, r31, ha16(ppcTypeSwitch - address) \n"   // load the address of the jump table for the switch
	" la    r30, lo16(ppcTypeSwitch - address)(r30) \n"
	
    " add    r0, r30, r24  \n"   // offset by our argument type
	" mtctr  r0            \n"   // load the jump address into CTR
	" bctr                 \n"   // jump into the jump table/switch
	" nop                  \n"
	
	// the jump table/switch based on the current argument type
	"ppcTypeSwitch:        \n"
	" b ppcArgsEnd         \n"
	" b ppcArgIsInteger    \n"
	" b ppcArgIsFloat      \n"
	" b ppcArgIsDouble     \n"
	
	// when we get here we have finished processing all the arguments
	// everything is ready to go to call the function
	"ppcArgsEnd:           \n"
	" mtctr r27            \n"   // the function pointer is stored in r27, load that into CTR
	" bctrl                \n"   // call the function.  We have to do it this way so that the LR gets the proper
	" nop                  \n"   // return value (the next instruction below).  So we have to branch from CTR instead of LR.
	
	// Restore registers and caller's stack frame, then return to caller
	" lwz   r1, 0(r1)      \n"   // restore the caller's stack pointer
	" lwz   r0, 8(r1)      \n"   // load in the caller's LR
	" mtlr  r0             \n"   // restore the caller's LR
	" lmw   r23, -36(r1)   \n"   // restore registers r23 to r31 from the stack
    " blr                  \n"   // return back to the caller
	" nop                  \n"
	
	// Integer argument (GPR register)
	"ppcArgIsInteger:             \n"
	" addis r30, r31, ha16(ppcLoadIntReg - address) \n" // load the address to the jump table for integer registers
	" la    r30, lo16(ppcLoadIntReg - address)(r30) \n"
	" mulli r0, r23, 8            \n"    // each item in the jump table is 2 instructions (8 bytes)
	" add   r0, r0, r30           \n"    // calculate ppcLoadIntReg[numUsedGPRRegs]
	" lwz   r30, 0(r29)           \n"    // load the next argument from the argument list into r30
	" cmpwi r23, 8                \n"    // we can only load GPR3 through GPR10 (8 registers)
	" bgt   ppcLoadIntRegUpd      \n"    // if we're beyond 8 GPR registers, we're in the stack, go there
	" mtctr r0                    \n"    // load the address of our ppcLoadIntReg jump table (we're below 8 GPR registers)
	" bctr                        \n"    // load the argument into a GPR register
	" nop                         \n"
	// jump table for GPR registers, for the first 8 GPR arguments
	"ppcLoadIntReg:               \n"
	" mr    r3, r30               \n"    // arg0 (to r3)
	" b     ppcLoadIntRegUpd      \n"
	" mr    r4, r30               \n"    // arg1 (to r4)
	" b     ppcLoadIntRegUpd      \n"
	" mr    r5, r30               \n"    // arg2 (to r5)
	" b     ppcLoadIntRegUpd      \n"
	" mr    r6, r30               \n"    // arg3 (to r6)
	" b     ppcLoadIntRegUpd      \n"
	" mr    r7, r30               \n"    // arg4 (to r7)
	" b     ppcLoadIntRegUpd      \n"
	" mr    r8, r30               \n"    // arg5 (to r8)
	" b     ppcLoadIntRegUpd      \n"
	" mr    r9, r30               \n"    // arg6 (to r9)
	" b     ppcLoadIntRegUpd      \n"
	" mr    r10, r30              \n"    // arg7 (to r10)
	" b     ppcLoadIntRegUpd      \n"
	// all GPR arguments still go on the stack
	"ppcLoadIntRegUpd:            \n"
	" stw   r30, 0(r26)           \n"    // store the argument into the next slot on the stack's argument list
	" addi  r23, r23, 1           \n"    // count a used GPR register
	" addi  r29, r29, 4           \n"    // move to the next argument on the list
	" addi  r26, r26, 4           \n"    // adjust our argument stack pointer for the next
	" b     ppcNextArg            \n"    // next argument

	// single Float argument
	"ppcArgIsFloat:\n"
	" addis r30, r31, ha16(ppcLoadFloatReg - address) \n" // get the base address of the float register jump table
	" la    r30, lo16(ppcLoadFloatReg - address)(r30) \n"
	" mulli r0, r28, 8           \n"     // each jump table entry is 8 bytes
	" add   r0, r0, r30          \n"     // calculate the offset to ppcLoadFloatReg[numUsedFloatReg]
	" lfs   f0, 0(r29)           \n"     // load the next argument as a float into f0
	" cmpwi r28, 13              \n"     // can't load more than 13 float/double registers
	" bgt   ppcLoadFloatRegUpd   \n"     // if we're beyond 13 registers, just fall to inserting into the stack
	" mtctr r0                   \n"     // jump into the float jump table
	" bctr                       \n"
	" nop                        \n"
	// jump table for float registers, for the first 13 float arguments
	"ppcLoadFloatReg:            \n"
	" fmr   f1, f0               \n"     // arg0 (f1)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f2, f0               \n"     // arg1 (f2)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f3, f0               \n"     // arg2 (f3)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f4, f0               \n"     // arg3 (f4)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f5, f0               \n"     // arg4 (f5)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f6, f0               \n"     // arg5 (f6)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f7, f0               \n"     // arg6 (f7)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f8, f0               \n"     // arg7 (f8)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f9, f0               \n"     // arg8 (f9)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f10, f0              \n"     // arg9 (f10)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f11, f0              \n"     // arg10 (f11)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f12, f0              \n"     // arg11 (f12)
	" b     ppcLoadFloatRegUpd   \n"
	" fmr   f13, f0              \n"     // arg12 (f13)
	" b     ppcLoadFloatRegUpd   \n"
	" nop                        \n"
	// all float arguments still go on the stack
	"ppcLoadFloatRegUpd:         \n"
	" stfs  f0, 0(r26)           \n"     // store, as a single float, f0 (current argument) on to the stack argument list
	" addi  r23, r23, 1          \n"     // a float register eats up a GPR register
	" addi  r28, r28, 1          \n"     // ...and, of course, a float register
	" addi  r29, r29, 4          \n"     // move to the next argument in the list
	" addi  r26, r26, 4          \n"     // move to the next stack slot
	" b     ppcNextArg           \n"     // on to the next argument
	" nop                        \n"
	
	// double Float argument
	"ppcArgIsDouble:             \n"
	" addis r30, r31, ha16(ppcLoadDoubleReg - address) \n" // load the base address of the jump table for double registers
	" la    r30, lo16(ppcLoadDoubleReg - address)(r30) \n"
	" mulli r0, r28, 8           \n"     // each slot of the jump table is 8 bytes
	" add   r0, r0, r30          \n"     // calculate ppcLoadDoubleReg[numUsedFloatReg]
	" lfd   f0, 0(r29)           \n"     // load the next argument, as a double float, into f0
	" cmpwi r28, 13              \n"     // the first 13 floats must go into float registers also
	" bgt   ppcLoadDoubleRegUpd  \n"     // if we're beyond 13, then just put on to the stack
	" mtctr r0                   \n"     // we're under 13, first load our register
	" bctr                       \n"     // jump into the jump table
	" nop                        \n"
	// jump table for float registers, for the first 13 float arguments
	"ppcLoadDoubleReg:           \n"
	" fmr   f1, f0               \n"     // arg0 (f1)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f2, f0               \n"     // arg1 (f2)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f3, f0               \n"     // arg2 (f3)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f4, f0               \n"     // arg3 (f4)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f5, f0               \n"     // arg4 (f5)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f6, f0               \n"     // arg5 (f6)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f7, f0               \n"     // arg6 (f7)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f8, f0               \n"     // arg7 (f8)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f9, f0               \n"     // arg8 (f9)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f10, f0              \n"     // arg9 (f10)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f11, f0              \n"     // arg10 (f11)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f12, f0              \n"     // arg11 (f12)
	" b     ppcLoadDoubleRegUpd  \n"
	" fmr   f13, f0              \n"     // arg12 (f13)
	" b     ppcLoadDoubleRegUpd  \n"
	" nop                        \n"
	// all float arguments still go on the stack
	"ppcLoadDoubleRegUpd: \n"
	" stfd  f0, 0(r26)     \n"    // store f0, as a double, into the argument list on the stack
	" addi  r23, r23, 2   \n"     // a double float eats up two GPRs
	" addi  r28, r28, 1   \n"     // ...and, of course, a float
	" addi  r29, r29, 8   \n"     // increment to our next argument we need to process (8 bytes for the 64bit float)
	" addi  r26, r26, 8   \n"     // increment to the next slot on the argument list on the stack (8 bytes)
	" b     ppcNextArg    \n"     // on to the next argument
	" nop                 \n"
);

asDWORD GetReturnedFloat()
{
	asDWORD f;
	asm(" stfs f1, %0\n" : "=m"(f));
	return f;
}

asQWORD GetReturnedDouble()
{
	asQWORD f;
	asm(" stfd f1, %0\n" : "=m"(f));
	return f;
}

// puts the arguments in the correct place in the stack array. See comments above.
void stackArgs(const asDWORD *args, const asBYTE *argsType, int& numIntArgs, int& numFloatArgs, int& numDoubleArgs)
{
	int i;
	int argWordPos = numIntArgs + numFloatArgs + (numDoubleArgs*2);
	int typeOffset = numIntArgs + numFloatArgs + numDoubleArgs;

	int typeIndex;
	for( i = 0, typeIndex = 0; ; i++, typeIndex++ )
	{
		// store the type
		ppcArgsType[typeOffset++] = argsType[typeIndex];
		if( argsType[typeIndex] == ppcENDARG )
			break;

		switch( argsType[typeIndex] )
		{
		case ppcFLOATARG:
			// stow float
			ppcArgs[argWordPos] = args[i]; // it's just a bit copy
			numFloatArgs++;
			argWordPos++; //add one word
			break;

		case ppcDOUBLEARG:
			// stow double
			memcpy( &ppcArgs[argWordPos], &args[i], sizeof(double) ); // we have to do this because of alignment
			numDoubleArgs++;
			argWordPos+=2; //add two words
			i++;//doubles take up 2 argument slots
			break;

		case ppcINTARG:
			// stow register
			ppcArgs[argWordPos] = args[i];
			numIntArgs++;
			argWordPos++;
			break;
		}
	}

	// close off the argument list (if we have max args we won't close it off until here)
	ppcArgsType[typeOffset] = ppcENDARG;
}

static asQWORD CallCDeclFunction(const asDWORD* pArgs, const asBYTE *pArgsType, int argSize, asDWORD func, void *retInMemory)
{
	int baseArgCount = 0;
	if( retInMemory )
	{
		// the first argument is the 'return in memory' pointer
		ppcArgs[0]     = (asDWORD)retInMemory;
		ppcArgsType[0] = ppcINTARG;
		ppcArgsType[1] = ppcENDARG;
		baseArgCount   = 1;
	}

	// put the arguments in the correct places in the ppcArgs array
	int numTotalArgs = baseArgCount;
	if( argSize > 0 )
	{
		int intArgs = baseArgCount, floatArgs = 0, doubleArgs = 0;
		stackArgs( pArgs, pArgsType, intArgs, floatArgs, doubleArgs );
		numTotalArgs = intArgs + floatArgs + 2*doubleArgs; // doubles occupy two slots
	}
	else
	{
		// no arguments, cap the type list
		ppcArgsType[baseArgCount] = ppcENDARG;
	}

	// call the function with the arguments
	return ppcFunc( ppcArgs, PPC_STACK_SIZE(numTotalArgs), func );
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the first parameter is the object (unless we are returning in memory)
static asQWORD CallThisCallFunction(const void *obj, const asDWORD* pArgs, const asBYTE *pArgsType, int argSize, asDWORD func, void *retInMemory )
{
	int baseArgCount = 0;
	if( retInMemory )
	{
		// the first argument is the 'return in memory' pointer
		ppcArgs[0]     = (asDWORD)retInMemory;
		ppcArgsType[0] = ppcINTARG;
		ppcArgsType[1] = ppcENDARG;
		baseArgCount = 1;
	}

	// the first argument is the 'this' of the object
	ppcArgs[baseArgCount]       = (asDWORD)obj;
	ppcArgsType[baseArgCount++] = ppcINTARG;
	ppcArgsType[baseArgCount]   = ppcENDARG;

	// put the arguments in the correct places in the ppcArgs array
	int numTotalArgs = baseArgCount;
	if( argSize > 0 )
	{
		int intArgs = baseArgCount, floatArgs = 0, doubleArgs = 0;
		stackArgs( pArgs, pArgsType, intArgs, floatArgs, doubleArgs );
		numTotalArgs = intArgs + floatArgs + 2*doubleArgs; // doubles occupy two slots
	}

	// call the function with the arguments
	return ppcFunc( ppcArgs, PPC_STACK_SIZE(numTotalArgs), func);
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the last parameter is the object 
// NOTE: on PPC the order for the args is reversed
static asQWORD CallThisCallFunction_objLast(const void *obj, const asDWORD* pArgs, const asBYTE *pArgsType, int argSize, asDWORD func, void *retInMemory)
{
	UNUSED_VAR(argSize);
	int baseArgCount = 0;
	if( retInMemory )
	{
		// the first argument is the 'return in memory' pointer
		ppcArgs[0]     = (asDWORD)retInMemory;
		ppcArgsType[0] = ppcINTARG;
		ppcArgsType[1] = ppcENDARG;
		baseArgCount = 1;
	}

	// stack any of the arguments
	int intArgs = baseArgCount, floatArgs = 0, doubleArgs = 0;
	stackArgs( pArgs, pArgsType, intArgs, floatArgs, doubleArgs );
	int numTotalArgs = intArgs + floatArgs + doubleArgs;

	// can we fit the object in at the end?
	if( numTotalArgs < AS_PPC_MAX_ARGS )
	{
		// put the object pointer at the end
		int argPos = intArgs + floatArgs + (doubleArgs * 2);
		ppcArgs[argPos]             = (asDWORD)obj;
		ppcArgsType[numTotalArgs++] = ppcINTARG;
		ppcArgsType[numTotalArgs]   = ppcENDARG;
	}

	// call the function with the arguments
	return ppcFunc( ppcArgs, PPC_STACK_SIZE(numTotalArgs), func );
}

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	// use a working array of types, we'll configure the final one in stackArgs
	asBYTE argsType[2*AS_PPC_MAX_ARGS + 1 + 1 + 1];
	memset( argsType, 0, sizeof(argsType));

	asCScriptEngine *engine = context->engine;
	asCScriptFunction *descr = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = descr->sysFuncIntf;

	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
	{
		// we're only handling native calls, handle generic calls in here
		return context->CallGeneric( id, objectPointer);
	}

	asQWORD  retQW           = 0;
	void    *func            = (void*)sysFunc->func;
	int      paramSize       = sysFunc->paramSize;
	int      popSize         = paramSize;
	asDWORD *args            = context->regs.stackPointer;	
	void    *obj             = NULL;
	asDWORD *vftable         = NULL;
	void    *retObjPointer   = NULL; // for system functions that return AngelScript objects
	void    *retInMemPointer = NULL; // for host functions that need to return data in memory instead of by register
	int      a, s;

	// convert the parameters that are < 4 bytes from little endian to big endian
	int argDwordOffset = 0;
	
	// if this is a THISCALL function and no object pointer was given, then the
	// first argument on the stack is the object pointer -- we MUST skip it for doing
	// the endian flipping.
	if( ( callConv >= ICC_THISCALL ) && (objectPointer == NULL) )
	{
		++argDwordOffset;
	}
	
	for( a = 0; a < (int)descr->parameterTypes.GetLength(); a++ )
	{
		int numBytes = descr->parameterTypes[a].GetSizeInMemoryBytes();
		if( numBytes >= 4 || descr->parameterTypes[a].IsReference() || descr->parameterTypes[a].IsObjectHandle() )
		{
			argDwordOffset += descr->parameterTypes[a].GetSizeOnStackDWords();
			continue;
		}

		// flip
		asASSERT( numBytes == 1 || numBytes == 2 );
		switch( numBytes )
		{
		case 1:
			{
				volatile asBYTE *bPtr = (asBYTE*)ARG_DW(args[argDwordOffset]);
				asBYTE t = bPtr[0];
				bPtr[0] = bPtr[3];
				bPtr[3] = t;
				t = bPtr[1];
				bPtr[1] = bPtr[2];
				bPtr[2] = t;
			}
			break;
		case 2:
			{
				volatile asWORD *wPtr = (asWORD*)ARG_DW(args[argDwordOffset]);
				asWORD t = wPtr[0];
				wPtr[0] = wPtr[1];
				wPtr[1] = t;
			}
			break;
		}
		argDwordOffset++;
	}

	// Objects returned to AngelScript must be via an object pointer.  This goes for
	// ALL objects, including those of simple, complex, primitive or float.  Whether
	// the host system (PPC in this case) returns the 'object' as a pointer depends on the type of object.
	context->regs.objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retObjPointer = engine->CallAlloc( descr->returnType.GetObjectType() );
		
		if( sysFunc->hostReturnInMemory )
		{
			// The return is made in memory on the host system
			callConv++;
			retInMemPointer = retObjPointer;
		}
	}

	// make sure that host functions that will be returning in memory have a memory pointer
	asASSERT( sysFunc->hostReturnInMemory==false || retInMemPointer!=NULL );

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
			obj = (void*)*(args);
			if( obj == NULL )
			{
				context->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( retObjPointer )
				{
					engine->CallFree(retObjPointer);
				}
				return 0;
			}

			// Add the base offset for multiple inheritance
			obj = (void*)(int(obj) + sysFunc->baseOffset);

			// Skip the object pointer
			args++;
		}
	}
	asASSERT( descr->parameterTypes.GetLength() <= AS_PPC_MAX_ARGS );

	// mark all float/double/int arguments
	for( s = 0, a = 0; s < (int)descr->parameterTypes.GetLength(); s++, a++ )
	{
		if( descr->parameterTypes[s].IsFloatType() && !descr->parameterTypes[s].IsReference() )
		{
			argsType[a] = ppcFLOATARG;
		}
		else if( descr->parameterTypes[s].IsDoubleType() && !descr->parameterTypes[s].IsReference() )
		{
			argsType[a] = ppcDOUBLEARG;
		}
		else
		{
			argsType[a] = ppcINTARG;
			if( descr->parameterTypes[s].GetSizeOnStackDWords() == 2 )
			{
				// Add an extra integer argument for the extra size
				a++;
				argsType[a] = ppcINTARG;
			}
		}
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
					// NOTE: we may have to do endian flipping here

					// Copy the object's memory to the buffer
					memcpy( &paramBuffer[dpos], *(void**)(args+spos), descr->parameterTypes[n].GetSizeInMemoryBytes() );

					// Delete the original memory
					engine->CallFree(*(char**)(args+spos) );
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
				{
					paramBuffer[dpos++] = args[spos++];
				}
				paramSize += descr->parameterTypes[n].GetSizeOnStackDWords();
			}
		}

		// Keep a free location at the beginning
		args = &paramBuffer[1];
	}
	
	// one last verification to make sure things are how we expect
	asASSERT( (retInMemPointer!=NULL && sysFunc->hostReturnInMemory) || (retInMemPointer==NULL && !sysFunc->hostReturnInMemory) );
	context->isCallingSystemFunction = true;
	switch( callConv )
	{
	case ICC_CDECL:
	case ICC_CDECL_RETURNINMEM:
	case ICC_STDCALL:
	case ICC_STDCALL_RETURNINMEM:
		retQW = CallCDeclFunction( args, argsType, paramSize, (asDWORD)func, retInMemPointer );
		break;
	case ICC_THISCALL:
	case ICC_THISCALL_RETURNINMEM:
		retQW = CallThisCallFunction(obj, args, argsType, paramSize, (asDWORD)func, retInMemPointer );
		break;
	case ICC_VIRTUAL_THISCALL:
	case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		// Get virtual function table from the object pointer
		vftable = *(asDWORD**)obj;
		retQW = CallThisCallFunction( obj, args, argsType, paramSize, vftable[asDWORD(func)>>2], retInMemPointer );
		break;
	case ICC_CDECL_OBJLAST:
	case ICC_CDECL_OBJLAST_RETURNINMEM:
		retQW = CallThisCallFunction_objLast( obj, args, argsType, paramSize, (asDWORD)func, retInMemPointer );
		break;
	case ICC_CDECL_OBJFIRST:
	case ICC_CDECL_OBJFIRST_RETURNINMEM:
		retQW = CallThisCallFunction( obj, args, argsType, paramSize, (asDWORD)func, retInMemPointer );
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
		if( callConv >= ICC_THISCALL && !objectPointer )
		    args++;

		int spos = 0;
		for( int n = 0; n < (int)descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() &&
				!descr->parameterTypes[n].IsReference() &&
				(descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) )
			{
				void *obj = (void*)args[spos++];
				asSTypeBehaviour *beh = &descr->parameterTypes[n].GetObjectType()->beh;
				if( beh->destruct )
				{
					engine->CallObjectMethod(obj, beh->destruct);
				}

				engine->CallFree(obj);
			}
			else
			{
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
			}
		}
	}
#endif

	// Store the returned value in our stack
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() )
	{
		if( descr->returnType.IsObjectHandle() )
		{
			// Since we're treating the system function as if it is returning a QWORD we are
			// actually receiving the value in the high DWORD of retQW.
			retQW >>= 32;
		
			// returning an object handle
			context->regs.objectRegister = (void*)(asDWORD)retQW;

			if( sysFunc->returnAutoHandle && context->regs.objectRegister )
			{
				engine->CallObjectMethod(context->regs.objectRegister, descr->returnType.GetObjectType()->beh.addref);
			}
		}
		else
		{
			// returning an object
			if( !sysFunc->hostReturnInMemory )
			{
				// In this case, AngelScript wants an object pointer back, but the host system
				// didn't use 'return in memory', so its results were passed back by the return register.
				// We have have to take the results of the return register and store them IN the pointer for the object.
				// The data for the object could fit into a register; we need to copy that data to the object pointer's
				// memory.
				asASSERT( retInMemPointer == NULL );
				asASSERT( retObjPointer != NULL );

				// Copy the returned value to the pointer sent by the script engine
				if( sysFunc->hostReturnSize == 1 )
				{
					// Since we're treating the system function as if it is returning a QWORD we are
					// actually receiving the value in the high DWORD of retQW.
					retQW >>= 32;
				
					*(asDWORD*)retObjPointer = (asDWORD)retQW;						
				}
				else
				{
					*(asQWORD*)retObjPointer = retQW;
				}
			}
			else
			{
				// In this case, AngelScript wants an object pointer back, and the host system
				// used 'return in memory'.  So its results were already passed back in memory, and
				// stored in the object pointer.
				asASSERT( retInMemPointer != NULL );
				asASSERT( retObjPointer != NULL );
			}

			// store the return results into the object register
			context->regs.objectRegister = retObjPointer;
		}
	}
	else
	{
		// Store value in valueRegister
		if( sysFunc->hostReturnFloat )
		{
			// floating pointer primitives
			if( sysFunc->hostReturnSize == 1 )
			{
				// single float
				*(asDWORD*)&context->regs.valueRegister = GetReturnedFloat();
			}
			else
			{
				// double float
				context->regs.valueRegister = GetReturnedDouble();
			}
		}
		else if( sysFunc->hostReturnSize == 1 )
		{
			// <= 32 bit primitives
			
			// Since we're treating the system function as if it is returning a QWORD we are
			// actually receiving the value in the high DWORD of retQW.
			retQW >>= 32;

			// due to endian issues we need to handle return values, that are
			// less than a DWORD (32 bits) in size, special
			int numBytes = descr->returnType.GetSizeInMemoryBytes();
			if( descr->returnType.IsReference() ) numBytes = 4;
			switch( numBytes )
			{
			case 1:
				{
					// 8 bits
					asBYTE *val = (asBYTE*)ARG_DW(context->regs.valueRegister);
					val[0] = (asBYTE)retQW;
					val[1] = 0;
					val[2] = 0;
					val[3] = 0;
					val[4] = 0;
					val[5] = 0;
					val[6] = 0;
					val[7] = 0;
				}
				break;
			case 2:
				{
					// 16 bits
					asWORD *val = (asWORD*)ARG_DW(context->regs.valueRegister);
					val[0] = (asWORD)retQW;
					val[1] = 0;
					val[2] = 0;
					val[3] = 0;
				}
				break;
			default:
				{
					// 32 bits
					asDWORD *val = (asDWORD*)ARG_DW(context->regs.valueRegister);
					val[0] = (asDWORD)retQW;
					val[1] = 0;
				}
				break;
			}
		}
		else
		{
			// 64 bit primitive
			context->regs.valueRegister = retQW;
		}
	}

	if( sysFunc->hasAutoHandles )
	{
		args = context->regs.stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
		{
			args++;
		}

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
			{
				spos++;
			}
			else
			{
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
			}
		}
	}

	return popSize;
}

END_AS_NAMESPACE

#endif // AS_PPC
#endif // AS_MAX_PORTABILITY

