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
// as_callfunc_xenon.cpp
//
// These functions handle the actual calling of system functions
//
// This version is Xenon specific
// Modified from as_callfunc_ppc.cpp by Laszlo Perneky Februar 2007

#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_XENON

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"

#include <stdio.h>
#include <stdlib.h>

BEGIN_AS_NAMESPACE

#define AS_PPC_MAX_ARGS         32
#define AS_MAX_REG_FLOATS       13
#define AS_MAX_REG_INTS         8
#define AS_PPC_THISCALL_REG     1
#define AS_PPC_RETURNINMEM_REG  1
#define AS_PPC_ENDOFARGS        1

// The array used to send values to the correct places.
// Contains a byte of argTypes to indicate the register type to load, or zero if end of arguments

extern "C" {
	enum argTypes
	{
		ppcENDARG = 0,
		ppcINTARG,
		ppcFLOATARG,
		ppcDOUBLEARG
	};
	static asBYTE ppcArgsType[AS_PPC_MAX_ARGS + AS_PPC_RETURNINMEM_REG + AS_PPC_THISCALL_REG + AS_PPC_ENDOFARGS];
	static asDWORD ppcArgs[AS_PPC_MAX_ARGS + AS_PPC_RETURNINMEM_REG + AS_PPC_THISCALL_REG];
}

// Loads all data into the correct places and calls the function.
// ppcArgsType is an array containing a byte type (enum argTypes) for each argument.
// iStackArgSize is the size in bytes for how much data to put on the stack frame
//--------------------------------------------------------------------
asQWORD __declspec( naked ) ppcFunc(const asDWORD* pArgs, int iStackArgSize, asDWORD dwFunc)
{
	__asm
	{
//////////////////////////////////////////////////////////////////////////
// Prepare args
//////////////////////////////////////////////////////////////////////////
_ppcFunc:
		// setup stack
		// Read link register
		mflr r12
		// Stack the link register
		stw  r12,   -8(r1)
		// Move stack pointer
		stwu r1,  -70h(r1)

		mr r29, r3 //pArgs
		mr r30, r4 //iStackArgSize
		mr r27, r5 //dwFunc

		// Clear some registers
		sub r0, r0, r0
		// Counting of used/assigned GPR's
		mr  r23, r0
		// Counting of used/assigned Float Registers
		mr  r22, r0

		// Fetch argument types array address
		lau r25, ppcArgsType
		lal r25, r25, ppcArgsType

		// Fetch arguments array address
		lau r26, ppcArgs
		lal r26, r26, ppcArgs

		// Begin loading and stacking registers
		subi r25, r25, 1

//////////////////////////////////////////////////////////////////////////
// Fetch the next argument
//////////////////////////////////////////////////////////////////////////
ppcNextArg:
		// Increment rArgTypePtr
		addi r25, r25, 1
		// Get data type
		lbz r24, 0(r25)

		// r24 holds the data type
		cmplwi cr6, r24, 0
		beq    cr6, ppcArgsEnd
		cmplwi cr6, r24, 1
		beq    cr6, ppcArgIsInteger
		cmplwi cr6, r24, 2
		beq    cr6, ppcArgIsFloat
		cmplwi cr6, r24, 3
		beq    cr6, ppcArgIsDouble

//////////////////////////////////////////////////////////////////////////
// Load and stack integer arguments
//////////////////////////////////////////////////////////////////////////
ppcArgIsInteger:
		// Get the arg from the stack
		lwz r11, 0(r26)

		// r23 holds the integer arg count so far
		cmplwi cr6, r23, 0
		beq    cr6, ppcLoadIntReg0
		cmplwi cr6, r23, 1
		beq    cr6, ppcLoadIntReg1
		cmplwi cr6, r23, 2
		beq    cr6, ppcLoadIntReg2
		cmplwi cr6, r23, 3
		beq    cr6, ppcLoadIntReg3
		cmplwi cr6, r23, 4
		beq    cr6, ppcLoadIntReg4
		cmplwi cr6, r23, 5
		beq    cr6, ppcLoadIntReg5
		cmplwi cr6, r23, 6
		beq    cr6, ppcLoadIntReg6
		cmplwi cr6, r23, 7
		beq    cr6, ppcLoadIntReg7

		// no more than 8 parameters
		b      ppcLoadIntRegUpd

		ppcLoadIntReg0:
		mr r3, r11
		b ppcLoadIntRegUpd
		ppcLoadIntReg1:
		mr r4, r11
		b ppcLoadIntRegUpd
		ppcLoadIntReg2:
		mr r5, r11
		b ppcLoadIntRegUpd
		ppcLoadIntReg3:
		mr r6, r11
		b ppcLoadIntRegUpd
		ppcLoadIntReg4:
		mr r7, r11
		b ppcLoadIntRegUpd
		ppcLoadIntReg5:
		mr r8, r11
		b ppcLoadIntRegUpd
		ppcLoadIntReg6:
		mr r9, r11
		b ppcLoadIntRegUpd
		ppcLoadIntReg7:
		mr r10, r11
		b ppcLoadIntRegUpd

		ppcLoadIntRegUpd:
		// Increment used int register count
		addi r23, r23, 1
		// Increment rArgsPtr
		addi r29, r29, 4
		// Increment rStackPtr
		addi r26, r26, 4
		b ppcNextArg

//////////////////////////////////////////////////////////////////////////
// Load and stack float arguments
//////////////////////////////////////////////////////////////////////////
ppcArgIsFloat:
		// Get the arg from the stack
		lfs fr15, 0(r26)

		// r22 holds the float arg count so far
		cmplwi cr6, r23, 0
		beq    cr6, ppcLoadFloatReg0
		cmplwi cr6, r23, 1
		beq    cr6, ppcLoadFloatReg1
		cmplwi cr6, r23, 2
		beq    cr6, ppcLoadFloatReg2
		cmplwi cr6, r23, 3
		beq    cr6, ppcLoadFloatReg3
		cmplwi cr6, r23, 4
		beq    cr6, ppcLoadFloatReg4
		cmplwi cr6, r23, 5
		beq    cr6, ppcLoadFloatReg5
		cmplwi cr6, r23, 6
		beq    cr6, ppcLoadFloatReg6
		cmplwi cr6, r23, 7
		beq    cr6, ppcLoadFloatReg7
		cmplwi cr6, r23, 8
		beq    cr6, ppcLoadFloatReg8
		cmplwi cr6, r23, 9
		beq    cr6, ppcLoadFloatReg9
		cmplwi cr6, r23, 10
		beq    cr6, ppcLoadFloatReg10
		cmplwi cr6, r23, 11
		beq    cr6, ppcLoadFloatReg11
		cmplwi cr6, r23, 12
		beq    cr6, ppcLoadFloatReg12
		cmplwi cr6, r23, 13
		beq    cr6, ppcLoadFloatReg13
		cmplwi cr6, r23, 14
		beq    cr6, ppcLoadFloatReg14

		// no more than 14 parameters
		b      ppcLoadFloatRegUpd

		ppcLoadFloatReg0:
		fmr fr0, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg1:
		fmr fr1, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg2:
		fmr fr2, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg3:
		fmr fr3, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg4:
		fmr fr4, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg5:
		fmr fr5, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg6:
		fmr fr6, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg7:
		fmr fr7, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg8:
		fmr fr8, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg9:
		fmr fr9, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg10:
		fmr fr10, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg11:
		fmr fr11, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg12:
		fmr fr12, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg13:
		fmr fr13, fr15
		b ppcLoadFloatRegUpd
		ppcLoadFloatReg14:
		fmr fr14, fr15
		b ppcLoadFloatRegUpd

		ppcLoadFloatRegUpd:
		// Increment used float register count
		addi r22, r22, 1
		// Increment used int register count - a float reg eats up a GPR
		addi r23, r23, 1
		// Increment rArgsPtr
		addi r29, r29, 4
		// Increment rStackPtr
		addi r26, r26, 4
		b ppcNextArg

//////////////////////////////////////////////////////////////////////////
// Load and stack double float arguments
//////////////////////////////////////////////////////////////////////////
ppcArgIsDouble:
		// Get the arg from the stack
		lfs fr15, 0(r26)

		// r22 holds the float arg count so far
		cmplwi cr6, r23, 0
		beq    cr6, ppcLoadDoubleReg0
		cmplwi cr6, r23, 1
		beq    cr6, ppcLoadDoubleReg1
		cmplwi cr6, r23, 2
		beq    cr6, ppcLoadDoubleReg2
		cmplwi cr6, r23, 3
		beq    cr6, ppcLoadDoubleReg3
		cmplwi cr6, r23, 4
		beq    cr6, ppcLoadDoubleReg4
		cmplwi cr6, r23, 5
		beq    cr6, ppcLoadDoubleReg5
		cmplwi cr6, r23, 6
		beq    cr6, ppcLoadDoubleReg6
		cmplwi cr6, r23, 7
		beq    cr6, ppcLoadDoubleReg7
		cmplwi cr6, r23, 8
		beq    cr6, ppcLoadDoubleReg8
		cmplwi cr6, r23, 9
		beq    cr6, ppcLoadDoubleReg9
		cmplwi cr6, r23, 10
		beq    cr6, ppcLoadDoubleReg10
		cmplwi cr6, r23, 11
		beq    cr6, ppcLoadDoubleReg11
		cmplwi cr6, r23, 12
		beq    cr6, ppcLoadDoubleReg12
		cmplwi cr6, r23, 13
		beq    cr6, ppcLoadDoubleReg13
		cmplwi cr6, r23, 14
		beq    cr6, ppcLoadDoubleReg14

		ppcLoadDoubleReg0:
		fmr fr0, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg1:
		fmr fr1, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg2:
		fmr fr2, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg3:
		fmr fr3, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg4:
		fmr fr4, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg5:
		fmr fr5, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg6:
		fmr fr6, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg7:
		fmr fr7, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg8:
		fmr fr8, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg9:
		fmr fr9, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg10:
		fmr fr10, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg11:
		fmr fr11, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg12:
		fmr fr12, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg13:
		fmr fr13, fr15
		b ppcLoadDoubleRegUpd
		ppcLoadDoubleReg14:
		fmr fr14, fr15
		b ppcLoadDoubleRegUpd

		ppcLoadDoubleRegUpd:
		// Increment used float register count
		addi r22, r22, 1
		// Increment used int register count
		addi r23, r23, 1
		// Increment rArgsPtr
		addi r29, r29, 4
		// Increment rStackPtr
		addi r26, r26, 4
		b ppcNextArg

//////////////////////////////////////////////////////////////////////////
// Finished
//////////////////////////////////////////////////////////////////////////
ppcArgsEnd:
		// Call the function
		mtctr r27
		bctrl

		// Function returned

		// Restore callers stack
		addi r1, r1, 70h
		// Fetch return link to caller
		lwz  r12, -8(r1)
		mtlr r12

		blr
	}
}

// Puts the arguments in the correct place in the stack array.
//-------------------------------------------------------------------
void stackArgs(const asDWORD *pArgs, int& iNumIntArgs, int& iNumFloatArgs, int& iNumDoubleArgs)
{
	int iArgWordPos = iNumIntArgs + iNumFloatArgs + iNumDoubleArgs;

	for(int iArg = 0; iArg < AS_PPC_MAX_ARGS; iArg++)
	{
		if ( ppcArgsType[iArg] == ppcENDARG )
			break;

		if( ppcArgsType[iArg] == ppcFLOATARG )
		{
			// stow float
			((float*)ppcArgs)[iArgWordPos] = ((float*)(pArgs))[iArg];
			iNumFloatArgs++;
			iArgWordPos++; //add one word
		}
		if ( ppcArgsType[iArg] == ppcDOUBLEARG )
		{
			// stow double
			((double*)ppcArgs)[iArgWordPos] = ((double*)(pArgs))[iArg];
			iNumDoubleArgs++;
			iArgWordPos++; //add two words
		}

		if( ppcArgsType[iArg] == ppcINTARG )
		{
			// stow register
			((int*)ppcArgs)[iArgWordPos] = ((int*)(pArgs))[iArg];
			iNumIntArgs++;
			iArgWordPos++;
		}
	}
}

// Prepare the arg list for a CDecl funtion and then call it
//--------------------------------------------------------------------
asQWORD CallCDeclFunction(const asDWORD* pArgs, int iArgSize, asDWORD dwFunc)
{
	int iIntArgs = 0;
	int iFloatArgs = 0;
	int iDoubleArgs = 0;

	// Put the arguments in the correct places in the ppcArgs array
	if ( iArgSize > 0 )
		stackArgs( pArgs, iIntArgs, iFloatArgs, iDoubleArgs );

	return ppcFunc( ppcArgs, iArgSize, dwFunc);
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the first parameter is the object
//--------------------------------------------------------------------
asQWORD CallThisCallFunction(const void* pObj, const asDWORD* pArgs, int iArgSize, asDWORD dwFunc )
{
	int iIntArgs = 0;
	int iFloatArgs = 0;
	int iDoubleArgs = 0;

	// Put the arguments in the correct places in the ppcArgs array /the this ptr is already in pArgs/
	if ( iArgSize > 0 )
		stackArgs( pArgs, iIntArgs, iFloatArgs, iDoubleArgs );

	return ppcFunc( ppcArgs, iArgSize, dwFunc);
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the last parameter is the object
//--------------------------------------------------------------------
asQWORD CallThisCallFunction_objLast(const void* pObj, const asDWORD* pArgs, int iArgSize, asDWORD dwFunc)
{
	int iIntArgs = 0;
	int iFloatArgs = 0;
	int iDoubleArgs = 0;

	// Put the arguments in the correct places in the ppcArgs array /the this ptr is already in pArgs/
	if ( iArgSize > 0 )
		stackArgs( pArgs, iIntArgs, iFloatArgs, iDoubleArgs );

	int iNumArgs = iIntArgs + iFloatArgs + iDoubleArgs;
	if ( iNumArgs < AS_PPC_MAX_ARGS )
	{
		ppcArgs[iNumArgs]     = (asDWORD)pObj;
		ppcArgsType[iNumArgs] = ppcINTARG;
	}

	return ppcFunc( ppcArgs, iArgSize + sizeof(pObj), dwFunc );
}

//--------------------------------------------------------------------
asDWORD GetReturnedFloat()
{
	asDWORD f;

	__asm
	{
		stfs fr0, f
	}

	return f;
}


asQWORD GetReturnedDouble()
//--------------------------------------------------------------------
{
	asQWORD f;

	__asm
	{
		stfd fr0, f
	}

	return f;
}

int CallSystemFunction(int iId, asCContext* pContext, void* pObjectPointer)
//--------------------------------------------------------------------
{
	memset( ppcArgsType, 0, sizeof(ppcArgsType));

	asCScriptEngine*            pEngine  = pContext->engine;
	asCScriptFunction*          pDescr   = pEngine->scriptFunctions[iId];
	asSSystemFunctionInterface* pSysFunc = pDescr->sysFuncIntf;

	int iCallConv = pSysFunc->callConv;
	if ( iCallConv == ICC_GENERIC_FUNC
		|| iCallConv == ICC_GENERIC_METHOD )
		return pContext->CallGeneric( iId, pObjectPointer );

	asQWORD dwRetQW = 0;

	void*    pFunc       = (void*)pSysFunc->func;
	int      iParamSize  = pSysFunc->paramSize;
	asDWORD* pArgs       = pContext->regs.stackPointer;
	void*    pRetPointer = 0;
	void*    pObj        = 0;
	int      iPopSize    = iParamSize;
	asDWORD* pVftable;

	// We generate the parameter list to this, so it fits to teh callingconvention
	asDWORD fixedArgs[ AS_PPC_MAX_ARGS + AS_PPC_RETURNINMEM_REG + AS_PPC_THISCALL_REG ];
	memset(fixedArgs, 0, sizeof(fixedArgs));
	int iArgsPtr = 0;

	pContext->regs.objectType = pDescr->returnType.GetObjectType();

	// If the function returns an object in memory, we allocate the memory and put the ptr to the front (will go to r3)
	if ( pDescr->returnType.IsObject() && !pDescr->returnType.IsReference() && !pDescr->returnType.IsObjectHandle() )
	{
		pRetPointer = pEngine->CallAlloc(pDescr->returnType.GetObjectType());

		if( pSysFunc->hostReturnInMemory )
			iCallConv++;

		fixedArgs  [ iArgsPtr ] = (asDWORD)pRetPointer;
		ppcArgsType[ iArgsPtr ] = ppcINTARG;
		iArgsPtr++;
	}

	// Find out if we have an object
	if ( iCallConv >= ICC_THISCALL )
	{
		if ( pObjectPointer )
		{
			pObj = pObjectPointer;
		}
		else
		{
			// The object pointer should be popped from the context stack
			iPopSize++;

			pObj = (void*)*(pArgs);
			pArgs++;

			// Check for null pointer
			if ( pObj == 0 )
			{
				pContext->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( pRetPointer )
					pEngine->CallFree(pRetPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
			pObj = (void*)(int(pObj) + pSysFunc->baseOffset);
		}
	}

	// If we have an object and it's not objectlast, then we put it az the first arg
	if ( pObj
		&& iCallConv != ICC_CDECL_OBJLAST
		&& iCallConv != ICC_CDECL_OBJLAST_RETURNINMEM )
	{
		fixedArgs  [ iArgsPtr ] = (asDWORD)pObj;
		ppcArgsType[ iArgsPtr ] = ppcINTARG;
		iArgsPtr++;
	}

	asASSERT(pDescr->parameterTypes.GetLength() <= AS_PPC_MAX_ARGS);

	// Parameter calculation magic
	asDWORD paramBuffer[64];
	if ( pSysFunc->takesObjByVal )
	{
		iParamSize = 0;
		int iSpos = 0;
		int iDpos = 1;

		for ( asUINT uParam = 0; uParam < pDescr->parameterTypes.GetLength(); uParam++ )
		{
			// Parameter object by value
			if (  pDescr->parameterTypes[uParam].IsObject()
				&& !pDescr->parameterTypes[uParam].IsObjectHandle()
				&& !pDescr->parameterTypes[uParam].IsReference() )
			{
#ifdef COMPLEX_OBJS_PASSED_BY_REF
				if( pDescr->parameterTypes[uParam].GetObjectType()->flags & COMPLEX_MASK )
				{
					paramBuffer[dpos++] = args[spos++];
					paramSize++;
				}
				else
#endif
				{
					// Copy the object's memory to the buffer
					memcpy( &paramBuffer[iDpos], *(void**)(pArgs + iSpos), pDescr->parameterTypes[uParam].GetSizeInMemoryBytes() );
					// Delete the original memory
					pEngine->CallFree(*(char**)(pArgs + iSpos) );
					pArgs[uParam] = (asDWORD)&paramBuffer[iDpos];
					iSpos++;
					iDpos += pDescr->parameterTypes[uParam].GetSizeInMemoryDWords();
					iParamSize += pDescr->parameterTypes[uParam].GetSizeInMemoryDWords();
				}
			}
			else
			{
				// Copy the value directly
				paramBuffer[iDpos++] = pArgs[iSpos++];
				if( pDescr->parameterTypes[uParam].GetSizeOnStackDWords() > 1 )
					paramBuffer[iDpos++] = pArgs[iSpos++];
				iParamSize += pDescr->parameterTypes[uParam].GetSizeOnStackDWords();
			}
		}
	}

	// Copy the parameter types for the ppcFunc. Also copy the params for the fixed xenon args buffer /containing this and return ptr/
	for( int iParam = 0; iParam < (int)pDescr->parameterTypes.GetLength(); iParam++, iArgsPtr++ )
	{
		ppcArgsType[iArgsPtr] = ppcINTARG;
		if (pDescr->parameterTypes[iParam].IsFloatType())
			ppcArgsType[iArgsPtr] = ppcFLOATARG;
		else if (pDescr->parameterTypes[iParam].IsDoubleType())
			ppcArgsType[iArgsPtr] = ppcDOUBLEARG;

		fixedArgs[iArgsPtr] = pArgs[iParam];

		// If the arg is bool, then endian swap it /it would not neccessary if the AS_BIG_ENDIAN would swap the bool from 0x01000000 to 0x00000010/
		if ( pDescr->parameterTypes[iParam].GetTokenType() == ttBool )
			((asBYTE*)(&fixedArgs[iArgsPtr]))[3] = ((asBYTE*)(&fixedArgs[iArgsPtr]))[0];
	}


	pContext->isCallingSystemFunction = true;
	switch ( iCallConv )
	{
	case ICC_CDECL:
	case ICC_CDECL_RETURNINMEM:
	case ICC_STDCALL:
	case ICC_STDCALL_RETURNINMEM:
		dwRetQW = CallCDeclFunction( fixedArgs, iArgsPtr, (asDWORD)pFunc );
		break;
	case ICC_THISCALL:
	case ICC_THISCALL_RETURNINMEM:
		dwRetQW = CallThisCallFunction( pObj, fixedArgs, iArgsPtr, (asDWORD)pFunc );
		break;
	case ICC_VIRTUAL_THISCALL:
	case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		// Get virtual function table from the object pointer
		pVftable = *(asDWORD**)pObj;
		dwRetQW = CallThisCallFunction( pObj, fixedArgs, iArgsPtr, pVftable[asDWORD(pFunc)>>2] );
		break;
	case ICC_CDECL_OBJLAST:
	case ICC_CDECL_OBJLAST_RETURNINMEM:
		dwRetQW = CallThisCallFunction_objLast( pObj, fixedArgs, iArgsPtr, (asDWORD)pFunc );
		break;
	case ICC_CDECL_OBJFIRST:
	case ICC_CDECL_OBJFIRST_RETURNINMEM:
		dwRetQW = CallThisCallFunction( pObj, fixedArgs, iArgsPtr, (asDWORD)pFunc );
		break;
	default:
		pContext->SetInternalException( TXT_INVALID_CALLING_CONVENTION );
	}
	pContext->isCallingSystemFunction = false;

#ifdef COMPLEX_OBJS_PASSED_BY_REF
	if( pSysFunc->takesObjByVal )
	{
		// Need to free the complex objects passed by value
		pArgs = pContext->regs.stackPointer;
		if ( iCallConv >= ICC_THISCALL
			&& !pObjectPointer )
				pArgs++;

		int iSpos = 0;
		for( int iParam = 0; iParam < (int)descr->parameterTypes.GetLength(); iParam++ )
		{
			if (  pDescr->parameterTypes[iParam].IsObject()
				&& !pDescr->parameterTypes[iParam].IsReference()
				&& (pDescr->parameterTypes[iParam].GetObjectType()->flags & COMPLEX_MASK) )
			{
				void *pObj = (void*)pArgs[iSpos++];
				asSTypeBehaviour *pBeh = &pDescr->parameterTypes[iParam].GetObjectType()->beh;
				if( pBeh->destruct )
					pEngine->CallObjectMethod(pObj, pBeh->destruct);

				pEngine->CallFree(pObj);
			}
			else
				iSpos += pDescr->parameterTypes[iParam].GetSizeInMemoryDWords();
		}
	}
#endif

	// Store the returned value in our stack
	if (  pDescr->returnType.IsObject()
		&& !pDescr->returnType.IsReference() )
	{
		if ( pDescr->returnType.IsObjectHandle() )
		{
			pContext->regs.objectRegister = (void*)(asDWORD)dwRetQW;

			if ( pSysFunc->returnAutoHandle
				&& pContext->regs.objectRegister )
				pEngine->CallObjectMethod( pContext->regs.objectRegister, pDescr->returnType.GetObjectType()->beh.addref );
		}
		else
		{
			if ( !pSysFunc->hostReturnInMemory )
			{
				// Copy the returned value to the pointer sent by the script engine
				if ( pSysFunc->hostReturnSize == 1 )
					*(asDWORD*)pRetPointer = (asDWORD)dwRetQW;
				else
					*(asQWORD*)pRetPointer = dwRetQW;
			}

			// Store the object in the register
			pContext->regs.objectRegister = pRetPointer;
		}
	}
	else
	{
		// If the retval is bool, then endian swap it /it would not neccessary if the AS_BIG_ENDIAN would swap the bool from 0x01000000 to 0x00000010/
		if ( pDescr->returnType.GetTokenType() == ttBool )
		{
			((asBYTE*)(&dwRetQW))[4] = ((asBYTE*)(&dwRetQW))[7];
			((asBYTE*)(&dwRetQW))[7] = 0;
		}

		// Store value in returnVal register
		if ( pSysFunc->hostReturnFloat )
		{
			if ( pSysFunc->hostReturnSize == 1 )
				*(asDWORD*)&pContext->regs.valueRegister = GetReturnedFloat();
			else
				pContext->regs.valueRegister = GetReturnedDouble();
		}
		else if ( pSysFunc->hostReturnSize == 1 )
			*(asDWORD*)&pContext->regs.valueRegister = (asDWORD)dwRetQW;
		else
			pContext->regs.valueRegister = dwRetQW;
	}

	if( pSysFunc->hasAutoHandles )
	{
		pArgs = pContext->regs.stackPointer;
		if ( iCallConv >= ICC_THISCALL
			&& !pObjectPointer )
			pArgs++;

		int iSpos = 0;
		for ( asUINT uParam = 0; uParam < pDescr->parameterTypes.GetLength(); uParam++ )
		{
			if ( pSysFunc->paramAutoHandles[uParam]
				&& pArgs[iSpos] )
			{
				// Call the release method on the type
				pEngine->CallObjectMethod( (void*)pArgs[iSpos], pDescr->parameterTypes[uParam].GetObjectType()->beh.release );
				pArgs[iSpos] = 0;
			}

			if (  pDescr->parameterTypes[uParam].IsObject()
				&& !pDescr->parameterTypes[uParam].IsObjectHandle()
				&& !pDescr->parameterTypes[uParam].IsReference() )
				iSpos++;
			else
				iSpos += pDescr->parameterTypes[uParam].GetSizeOnStackDWords();
		}
	}

	return iPopSize;
}

END_AS_NAMESPACE

#endif // AS_XENON
#endif // AS_MAX_PORTABILITY

//------------------------------------------------------------------

