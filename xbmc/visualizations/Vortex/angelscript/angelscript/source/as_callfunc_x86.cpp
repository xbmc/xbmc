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
// as_callfunc_x86.cpp
//
// These functions handle the actual calling of system functions
//



#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_X86

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"

BEGIN_AS_NAMESPACE

typedef asQWORD (*t_CallCDeclQW)(const asDWORD *, int, size_t);
typedef asQWORD (*t_CallCDeclQWObj)(void *obj, const asDWORD *, int, size_t);
typedef asDWORD (*t_CallCDeclRetByRef)(const asDWORD *, int, size_t, void *);
typedef asDWORD (*t_CallCDeclObjRetByRef)(void *obj, const asDWORD *, int, size_t, void *);
typedef asQWORD (*t_CallSTDCallQW)(const asDWORD *, int, size_t);
typedef asQWORD (*t_CallThisCallQW)(const void *, const asDWORD *, int, size_t);
typedef asDWORD (*t_CallThisCallRetByRef)(const void *, const asDWORD *, int, size_t, void *);

// Prototypes
void CallCDeclFunction(const asDWORD *args, int paramSize, size_t func);
void CallCDeclFunctionObjLast(const void *obj, const asDWORD *args, int paramSize, size_t func);
void CallCDeclFunctionObjFirst(const void *obj, const asDWORD *args, int paramSize, size_t func);
void CallCDeclFunctionRetByRef_impl(const asDWORD *args, int paramSize, size_t func, void *retPtr);
void CallCDeclFunctionRetByRefObjLast_impl(const void *obj, const asDWORD *args, int paramSize, size_t func, void *retPtr);
void CallCDeclFunctionRetByRefObjFirst_impl(const void *obj, const asDWORD *args, int paramSize, size_t func, void *retPtr);
void CallSTDCallFunction(const asDWORD *args, int paramSize, size_t func);
void CallThisCallFunction(const void *obj, const asDWORD *args, int paramSize, size_t func);
void CallThisCallFunctionRetByRef_impl(const void *, const asDWORD *, int, size_t, void *retPtr);

// Initialize function pointers
const t_CallCDeclQW CallCDeclFunctionQWord = (t_CallCDeclQW)CallCDeclFunction;
const t_CallCDeclQWObj CallCDeclFunctionQWordObjLast = (t_CallCDeclQWObj)CallCDeclFunctionObjLast;
const t_CallCDeclQWObj CallCDeclFunctionQWordObjFirst = (t_CallCDeclQWObj)CallCDeclFunctionObjFirst;
const t_CallCDeclRetByRef CallCDeclFunctionRetByRef = (t_CallCDeclRetByRef)CallCDeclFunctionRetByRef_impl;
const t_CallCDeclObjRetByRef CallCDeclFunctionRetByRefObjLast = (t_CallCDeclObjRetByRef)CallCDeclFunctionRetByRefObjLast_impl;
const t_CallCDeclObjRetByRef CallCDeclFunctionRetByRefObjFirst = (t_CallCDeclObjRetByRef)CallCDeclFunctionRetByRefObjFirst_impl;
const t_CallSTDCallQW CallSTDCallFunctionQWord = (t_CallSTDCallQW)CallSTDCallFunction;
const t_CallThisCallQW CallThisCallFunctionQWord = (t_CallThisCallQW)CallThisCallFunction;
const t_CallThisCallRetByRef CallThisCallFunctionRetByRef = (t_CallThisCallRetByRef)CallThisCallFunctionRetByRef_impl;

asDWORD GetReturnedFloat();
asQWORD GetReturnedDouble();

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine *engine = context->engine;
	asCScriptFunction *descr = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = descr->sysFuncIntf;
	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	asQWORD  retQW             = 0;
	void    *func              = (void*)sysFunc->func;
	int      paramSize         = sysFunc->paramSize;
	asDWORD *args              = context->regs.stackPointer;
	void    *retPointer        = 0;
	void    *obj               = 0;
	asDWORD *vftable;
	int      popSize           = paramSize;

	context->regs.objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retPointer = engine->CallAlloc(descr->returnType.GetObjectType());

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
			obj = (void*)*(size_t*)(args);
			if( obj == 0 )
			{
				context->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( retPointer )
					engine->CallFree(retPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
			obj = (void*)(size_t(obj) + sysFunc->baseOffset);

			// Skip the object pointer
			args++;
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
		retQW = CallCDeclFunctionQWord(args, paramSize<<2, (size_t)func);
		break;

	case ICC_CDECL_RETURNINMEM:
		retQW = CallCDeclFunctionRetByRef(args, paramSize<<2, (size_t)func, retPointer);
		break;

	case ICC_STDCALL:
		retQW = CallSTDCallFunctionQWord(args, paramSize<<2, (size_t)func);
		break;

	case ICC_STDCALL_RETURNINMEM:
		// Push the return pointer on the stack
		paramSize++;
		args--;
		*(size_t*)args = (size_t)retPointer;

		retQW = CallSTDCallFunctionQWord(args, paramSize<<2, (size_t)func);
		break;

	case ICC_THISCALL:
		retQW = CallThisCallFunctionQWord(obj, args, paramSize<<2, (size_t)func);
		break;

	case ICC_THISCALL_RETURNINMEM:
		retQW = CallThisCallFunctionRetByRef(obj, args, paramSize<<2, (size_t)func, retPointer);
		break;

	case ICC_VIRTUAL_THISCALL:
		// Get virtual function table from the object pointer
		vftable = *(asDWORD**)obj;

		retQW = CallThisCallFunctionQWord(obj, args, paramSize<<2, vftable[size_t(func)>>2]);
		break;

	case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		// Get virtual function table from the object pointer
		vftable = *(asDWORD**)obj;

		retQW = CallThisCallFunctionRetByRef(obj, args, paramSize<<2, vftable[size_t(func)>>2], retPointer);
		break;

	case ICC_CDECL_OBJLAST:
		retQW = CallCDeclFunctionQWordObjLast(obj, args, paramSize<<2, (size_t)func);
		break;

	case ICC_CDECL_OBJLAST_RETURNINMEM:
		// Call the system object method as a cdecl with the obj ref as the last parameter
		retQW = CallCDeclFunctionRetByRefObjLast(obj, args, paramSize<<2, (size_t)func, retPointer);
		break;

	case ICC_CDECL_OBJFIRST:
		// Call the system object method as a cdecl with the obj ref as the first parameter
		retQW = CallCDeclFunctionQWordObjFirst(obj, args, paramSize<<2, (size_t)func);
		break;

	case ICC_CDECL_OBJFIRST_RETURNINMEM:
		// Call the system object method as a cdecl with the obj ref as the first parameter
		retQW = CallCDeclFunctionRetByRefObjFirst(obj, args, paramSize<<2, (size_t)func, retPointer);
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
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
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
			context->regs.objectRegister = (void*)(size_t)retQW;

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
		// Store value in value register
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
				engine->CallObjectMethod((void*)*(size_t*)&args[spos], descr->parameterTypes[n].GetObjectType()->beh.release);
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

// On GCC we need to prevent the compiler from inlining these assembler routines when
// optimizing for speed (-O3), as the loop labels get duplicated which cause compile errors.

#ifdef __GNUC__
    #define NOINLINE __attribute ((__noinline__))
#else
    #define NOINLINE
#endif


void NOINLINE CallCDeclFunction(const asDWORD *args, int paramSize, size_t func)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

		// Call function
		call [func]

		// Pop arguments from stack
		add  esp, paramSize

		// Restore registers
		pop  ecx

		// return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  12(%ebp), %eax \n" // paramSize
		"addl  $4, %eax       \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"movl  12(%ebp), %ecx \n" // paramSize
		"movl  8(%ebp), %eax  \n" // args
		"addl  %ecx, %eax     \n" // push arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy        \n"
		"copyloop:            \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop       \n"
		"endcopy:             \n"
		"call  *16(%ebp)      \n"
		"addl  12(%ebp), %esp \n" // pop arguments

		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

void NOINLINE CallCDeclFunctionObjLast(const void *obj, const asDWORD *args, int paramSize, size_t func)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Push the object pointer as the last argument to the function
		push obj

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

		// Call function
		call [func]

		// Pop arguments from stack
		add  esp, paramSize
		add  esp, 4

		// Restore registers
		pop  ecx

		// return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(obj);
    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  16(%ebp), %eax \n" // paramSize
		"addl  $8, %eax       \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"pushl 8(%ebp)        \n"
		"movl  16(%ebp), %ecx \n" // paramSize
		"movl  12(%ebp), %eax \n" // args
		"addl  %ecx, %eax     \n" // push arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy8       \n"
		"copyloop8:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop8      \n"
		"endcopy8:            \n"
		"call  *20(%ebp)      \n"
		"addl  16(%ebp), %esp \n" // pop arguments
		"addl  $4, %esp       \n"

		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

void NOINLINE CallCDeclFunctionObjFirst(const void *obj, const asDWORD *args, int paramSize, size_t func)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

		// push object as first parameter
        push obj

		// Call function
		call [func]

		// Pop arguments from stack
		add  esp, paramSize
        add  esp, 4

		// Restore registers
		pop  ecx

		// return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(obj);
    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  16(%ebp), %eax \n" // paramSize
		"addl  $8, %eax       \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"movl  16(%ebp), %ecx \n" // paramSize
		"movl  12(%ebp), %eax \n" // args
		"addl  %ecx, %eax     \n" // push arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy6       \n"
		"copyloop6:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop6      \n"
		"endcopy6:            \n"
		"pushl 8(%ebp)        \n" // push obj
		"call  *20(%ebp)      \n"
		"addl  16(%ebp), %esp \n" // pop arguments
        "addl  $4, %esp       \n"

		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

void NOINLINE CallCDeclFunctionRetByRefObjFirst_impl(const void *obj, const asDWORD *args, int paramSize, size_t func, void *retPtr)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

		// Push the object pointer
        push obj

		// Push the return pointer
		push retPtr;

		// Call function
		call [func]

		// Pop arguments from stack
		add  esp, paramSize

#ifndef CALLEE_POPS_HIDDEN_RETURN_POINTER
		// Pop the return pointer
		add  esp, 8
#else
        add  esp, 4
#endif
		// Restore registers
		pop  ecx

		// return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(obj);
    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);
    UNUSED_VAR(retPtr);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  16(%ebp), %eax \n" // paramSize
		"addl  $12, %eax      \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"movl  16(%ebp), %ecx \n" // paramSize
		"movl  12(%ebp), %eax \n" // args
		"addl  %ecx, %eax     \n" // push arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy5       \n"
		"copyloop5:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop5      \n"
		"endcopy5:            \n"
        "pushl 8(%ebp)        \n" // push object first
		"pushl 24(%ebp)       \n" // retPtr
		"call  *20(%ebp)      \n" // func
		"addl  16(%ebp), %esp \n" // pop arguments
#ifndef CALLEE_POPS_HIDDEN_RETURN_POINTER
		"addl  $8, %esp       \n" // Pop the return pointer and object pointer
#else
		"addl  $4, %esp       \n" // Pop the object pointer
#endif
		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

void NOINLINE CallCDeclFunctionRetByRef_impl(const asDWORD *args, int paramSize, size_t func, void *retPtr)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

		// Push the return pointer
		push retPtr;

		// Call function
		call [func]

		// Pop arguments from stack
		add  esp, paramSize

#ifndef CALLEE_POPS_HIDDEN_RETURN_POINTER
		// Pop the return pointer
		add  esp, 4
#endif
		// Restore registers
		pop  ecx

		// return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);
    UNUSED_VAR(retPtr);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  12(%ebp), %eax \n" // paramSize
		"addl  $8, %eax       \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"movl  12(%ebp), %ecx \n" // paramSize
		"movl  8(%ebp), %eax  \n" // args
		"addl  %ecx, %eax     \n" // push arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy7       \n"
		"copyloop7:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop7      \n"
		"endcopy7:            \n"
		"pushl 20(%ebp)       \n" // retPtr
		"call  *16(%ebp)      \n" // func
		"addl  12(%ebp), %esp \n" // pop arguments
#ifndef CALLEE_POPS_HIDDEN_RETURN_POINTER
		"addl  $4, %esp       \n" // Pop the return pointer
#endif
		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

void NOINLINE CallCDeclFunctionRetByRefObjLast_impl(const void *obj, const asDWORD *args, int paramSize, size_t func, void *retPtr)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		push obj

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

		// Push the return pointer
		push retPtr;

		// Call function
		call [func]

		// Pop arguments from stack
		add  esp, paramSize
		add  esp, 4

#ifndef CALLEE_POPS_HIDDEN_RETURN_POINTER
		// Pop the return pointer
		add  esp, 4
#endif
		// Restore registers
		pop  ecx

		// return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(obj);
    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);
    UNUSED_VAR(retPtr);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  16(%ebp), %eax \n" // paramSize
		"addl  $12, %eax      \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"pushl 8(%ebp)        \n"
		"movl  16(%ebp), %ecx \n" // paramSize
		"movl  12(%ebp), %eax \n" // args
		"addl  %ecx, %eax     \n" // push arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy4       \n"
		"copyloop4:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop4      \n"
		"endcopy4:            \n"
		"pushl 24(%ebp)       \n" // retPtr
		"call  *20(%ebp)      \n" // func
		"addl  16(%ebp), %esp \n" // pop arguments
#ifndef CALLEE_POPS_HIDDEN_RETURN_POINTER
		"addl  $8, %esp       \n" // Pop the return pointer
#else
		"addl  $4, %esp       \n" // Pop the return pointer
#endif
		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

void NOINLINE CallSTDCallFunction(const asDWORD *args, int paramSize, size_t func)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

		// Call function
		call [func]

		// The callee already removed parameters from the stack

		// Restore registers
		pop  ecx

		// return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  12(%ebp), %eax \n" // paramSize
		"addl  $4, %eax       \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"movl  12(%ebp), %ecx \n" // paramSize
		"movl  8(%ebp), %eax  \n" // args
		"addl  %ecx, %eax     \n" // push arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy2       \n"
		"copyloop2:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop2      \n"
		"endcopy2:            \n"
		"call  *16(%ebp)      \n" // callee pops the arguments

		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}


void NOINLINE CallThisCallFunction(const void *obj, const asDWORD *args, int paramSize, size_t func)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

#ifdef THISCALL_PASS_OBJECT_POINTER_ON_THE_STACK
		// Push the object pointer on the stack
		push obj
#else
		// Move object pointer to ECX
		mov  ecx, obj
#endif

		// Call function
		call [func]

#ifndef THISCALL_CALLEE_POPS_ARGUMENTS
		// Pop arguments
		add  esp, paramSize
#ifdef THISCALL_PASS_OBJECT_POINTER_ON_THE_STACK
		// Pop object pointer
		add esp, 4
#endif
#endif

		// Restore registers
		pop  ecx

		// Return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(obj);
    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  16(%ebp), %eax \n" // paramSize
		"addl  $8, %eax       \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"movl  16(%ebp), %ecx \n" // paramSize
		"movl  12(%ebp), %eax \n" // args
		"addl  %ecx, %eax     \n" // push all arguments on the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy1       \n"
		"copyloop1:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop1      \n"
		"endcopy1:            \n"
		"movl  8(%ebp), %ecx  \n" // move obj into ECX
		"pushl 8(%ebp)        \n" // push obj on the stack
		"call  *20(%ebp)      \n"
		"addl  16(%ebp), %esp \n" // pop arguments
		"addl  $4, %esp       \n" // pop obj

		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

void NOINLINE CallThisCallFunctionRetByRef_impl(const void *obj, const asDWORD *args, int paramSize, size_t func, void *retPtr)
{
#if defined ASM_INTEL

	// Copy the data to the real stack. If we fail to do
	// this we may run into trouble in case of exceptions.
	__asm
	{
		// We must save registers that are used
	    push ecx

		// Clear the FPU stack, in case the called function doesn't do it by itself
		fninit

		// Copy arguments from script
		// stack to application stack
        mov  ecx, paramSize
		mov  eax, args
		add  eax, ecx
		cmp  ecx, 0
		je   endcopy
copyloop:
		sub  eax, 4
		push dword ptr [eax]
		sub  ecx, 4
		jne  copyloop
endcopy:

#ifdef THISCALL_PASS_OBJECT_POINTER_ON_THE_STACK
		// Push the object pointer on the stack
		push obj
#else
		// Move object pointer to ECX
		mov  ecx, obj
#endif

		// Push the return pointer
		push retPtr

		// Call function
		call [func]

#ifndef THISCALL_CALLEE_POPS_ARGUMENTS
		// Pop arguments
		add  esp, paramSize
#ifdef THISCALL_PASS_OBJECT_POINTER_ON_THE_STACK
		// Pop object pointer
		add esp, 4
#endif
#endif

		// Restore registers
		pop  ecx

		// Return value in EAX or EAX:EDX
	}

#elif defined ASM_AT_N_T

    UNUSED_VAR(obj);
    UNUSED_VAR(args);
    UNUSED_VAR(paramSize);
    UNUSED_VAR(func);
    UNUSED_VAR(retPtr);

	asm("pushl %ecx           \n"
	    "fninit               \n"

		// Need to align the stack pointer so that it is aligned to 16 bytes when making the function call.
		// It is assumed that when entering this function, the stack pointer is already aligned, so we need
		// to calculate how much we will put on the stack during this call.
		"movl  16(%ebp), %eax \n" // paramSize
		"addl  $12, %eax      \n" // counting esp that we will push on the stack
		"movl  %esp, %ecx     \n"
		"subl  %eax, %ecx     \n"
		"andl  $15, %ecx      \n"
		"movl  %esp, %eax     \n"
		"subl  %ecx, %esp     \n"
		"pushl %eax           \n" // Store the original stack pointer

		"movl  16(%ebp), %ecx \n" // paramSize
		"movl  12(%ebp), %eax \n" // args
		"addl  %ecx, %eax     \n" // push all arguments to the stack
		"cmp   $0, %ecx       \n"
		"je    endcopy3       \n"
		"copyloop3:           \n"
		"subl  $4, %eax       \n"
		"pushl (%eax)         \n"
		"subl  $4, %ecx       \n"
		"jne   copyloop3      \n"
		"endcopy3:            \n"
		"movl  8(%ebp), %ecx  \n" // move obj into ECX
		"pushl 8(%ebp)        \n" // push obj on the stack
		"pushl 24(%ebp)       \n" // push retPtr on the stack
		"call  *20(%ebp)      \n"
		"addl  16(%ebp), %esp \n" // pop arguments
		"addl  $4, %esp       \n" // pop the object pointer
		                          // the return pointer was popped by the callee
		// Pop the alignment bytes
		"popl  %esp           \n"

		"popl  %ecx           \n");

#endif
}

asDWORD GetReturnedFloat()
{
    asDWORD f;

#if defined ASM_INTEL

	// Get the float value from ST0
    __asm fstp dword ptr [f]

#elif defined ASM_AT_N_T

	asm("fstps %0 \n" : "=m" (f));

#endif

	return f;
}

asQWORD GetReturnedDouble()
{
    asQWORD d;

#if defined ASM_INTEL

	// Get the double value from ST0
    __asm fstp qword ptr [d]

#elif defined ASM_AT_N_T

	asm("fstpl %0 \n" : "=m" (d));

#endif

	return d;
}

END_AS_NAMESPACE

#endif // AS_X86
#endif // AS_MAX_PORTABILITY




