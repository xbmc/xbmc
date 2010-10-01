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
// as_config.h
//
// this file is used for configuring the compilation of the library
//

#ifndef AS_CONFIG_H
#define AS_CONFIG_H



//
// Features
//-----------------------------------------

// USE_THREADS
// Adds basic support for multithreading. 
// This is currently only supported on Win32 platforms.

// BUILD_WITHOUT_LINE_CUES
// This flag makes the script compiler remove some extra bytecodes that is used
// to allow the VM to call the line callback after each statement. The performance 
// is improved slightly but the scripts are only guaranteed to allow one suspension
// per loop iteration, not one per statement.

// AS_DEBUG
// This flag can be defined to make the library write some extra output when
// compiling and executing scripts.

// AS_DEPRECATED
// If this flag is defined then some backwards compatibility is maintained. 
// There is no guarantee for how well deprecated functionality will work though
// so it is best to exchange it for the new functionality as soon as possible.

// AS_C_INTERFACE
// Make the C interface available.

// AS_NO_CLASS_METHODS
// Disables the possibility to add class methods. Can increase the  
// portability of the library.

// AS_MAX_PORTABILITY
// Disables all platform specific code. Only the asCALL_GENERIC calling 
// convention will be available in with this flag set.

// AS_ALLOW_UNSAFE_REFERENCES
// When this flag is defined it is not required to define the in, out, or 
// inout keywords for parameter references. The compiler will generate code 
// that passes the true reference to functions. It is however possible to 
// write scripts that could crash the application due to invalid references.




//
// Library usage
//------------------------------------------

// ANGELSCRIPT_EXPORT
// This flag should be defined when compiling the library as a lib or dll.

// ANGELSCRIPT_DLL_LIBRARY_IMPORT
// This flag should be defined when using AngelScript as a dll with automatic 
// library import.

// ANGELSCRIPT_DLL_MANUAL_IMPORT
// This flag should be defined when using AngelScript as a dll with manual
// loading of the library.




// 
// Compiler differences
//-----------------------------------------

// vsnprintf() 
// Some compilers use different names for this function. If your compiler
// doesn't use the name vsnprintf() then you need to write a macro to translate 
// the function into its real name.

// ASM_AT_N_T or ASM_INTEL
// You should choose what inline assembly syntax to use when compiling.

// __int64
// If your compiler doesn't support the __int64 type you'll need to define
// a substitute for it that is 64 bits large.

// VALUE_OF_BOOLEAN_TRUE
// This flag allows to customize the exact value of boolean true.

// STDCALL
// This is used to declare a function to use the stdcall calling convention.

// AS_USE_NAMESPACE
// Adds the AngelScript namespace on the declarations.



//
// How to identify different compilers
//-----------------------------------------

// MS Visual C++
//  _MSC_VER   is defined
//  __MWERKS__ is not defined

// Metrowerks
//  _MSC_VER   is defined
//  __MWERKS__ is defined

// GNU C based compilers
//  __GNUC__   is defined



//
// CPU differences
//---------------------------------------

// AS_ALIGN
// Some CPUs require that data words are aligned in some way. This macro
// should be defined if the words should be aligned to boundaries of the same 
// size as the word, i.e. 
//  1 byte  on 1 byte boundaries 
//  2 bytes on 2 byte boundaries 
//  4 bytes on 4 byte boundaries
//  8 bytes on 4 byte boundaries (no it's not a typo)

// AS_USE_DOUBLE_AS_FLOAT
// If there is no 64 bit floating point type, then this constant can be defined
// to treat double like normal floats.

// AS_X86
// Use assembler code for the x86 CPU family

// AS_SH4
// Use assembler code for the SH4 CPU family



//
// Calling conventions
//-----------------------------------------

// GNU_STYLE_VIRTUAL_METHOD
// This constant should be defined if method pointers store index for virtual 
// functions in the same location as the function pointer. In such cases the method 
// is identified as virtual if the least significant bit is set.

// MULTI_BASE_OFFSET(x)
// This macro is used to retrieve the offset added to the object pointer in order to
// implicitly cast the object to the base object. x is the method pointer received by
// the register function.

// HAVE_VIRTUAL_BASE_OFFSET
// Define this constant if the compiler stores the virtual base offset in the method
// pointers. If it is not stored in the pointers then AngelScript have no way of
// identifying a method as coming from a class with virtual inheritance.

// VIRTUAL_BASE_OFFSET(x)
// This macro is used to retrieve the offset added to the object pointer in order to
// find the virtual base object. x is the method pointer received by the register 
// function;

// COMPLEX_MASK
// This constant shows what attributes determines if an object is returned in memory 
// or in the registers as normal structures

// THISCALL_RETURN_SIMPLE_IN_MEMORY
// CDECL_RETURN_SIMPLE_IN_MEMORY
// STDCALL_RETURN_SIMPLE_IN_MEMORY
// When these constants are defined then the corresponding calling convention always 
// return classes/structs in memory regardless of size or complexity.

// CALLEE_POPS_HIDDEN_RETURN_POINTER
// This constant should be defined if the callee pops the hidden return pointer, 
// used when returning an object in memory.

// THISCALL_PASS_OBJECT_POINTER_ON_THE_STACK
// With this constant defined AngelScript will pass the object pointer on the stack

// THISCALL_CALLEE_POPS_ARGUMENTS
// If the callee pops arguments for class methods then define this constant

// COMPLEX_OBJS_PASSED_BY_REF
// Some compilers always pass certain objects by reference. GNUC for example does 
// this if the the class has a defined destructor.



// 
// Configurations 
//------------------------------------------------

#define VALUE_OF_BOOLEAN_TRUE  1

// Microsoft Visual C++
#if defined(_MSC_VER) && !defined(__MWERKS__)
  #define BUILD_WITHOUT_LINE_CUES
  #define AS_ALLOW_UNSAFE_REFERENCES
  #define AS_USE_DOUBLE_AS_FLOAT
	#define MULTI_BASE_OFFSET(x) (*((asDWORD*)(&x)+1))
	#define HAVE_VIRTUAL_BASE_OFFSET
	#define VIRTUAL_BASE_OFFSET(x) (*((asDWORD*)(&x)+3))
	#define THISCALL_RETURN_SIMPLE_IN_MEMORY
	#define THISCALL_PASS_OBJECT_POINTER_IN_ECX
	#define ASM_INTEL
	#define vsnprintf(a, b, c, d) _vsnprintf(a, b, c, d)
	#define THISCALL_CALLEE_POPS_ARGUMENTS
	#define COMPLEX_MASK (asOBJ_CLASS_CONSTRUCTOR | asOBJ_CLASS_DESTRUCTOR | asOBJ_CLASS_ASSIGNMENT)
	#define STDCALL __stdcall
	#ifdef _M_IX86
		#define AS_X86
	#endif
#endif

// Metrowerks CodeWarrior (experimental, let me know if something isn't working)
#if defined(__MWERKS__)
	#define MULTI_BASE_OFFSET(x) (*((asDWORD*)(&x)+1))
	#define HAVE_VIRTUAL_BASE_OFFSET
	#define VIRTUAL_BASE_OFFSET(x) (*((asDWORD*)(&x)+3))
	#define THISCALL_RETURN_SIMPLE_IN_MEMORY
	#define THISCALL_PASS_OBJECT_POINTER_IN_ECX
	#define ASM_INTEL
	#define vsnprintf(a, b, c, d) _vsnprintf(a, b, c, d)
	#define THISCALL_CALLEE_POPS_ARGUMENTS
	#define COMPLEX_MASK (asOBJ_CLASS_CONSTRUCTOR | asOBJ_CLASS_DESTRUCTOR | asOBJ_CLASS_ASSIGNMENT)
	#define STDCALL __stdcall
	#ifdef _M_IX86
		#define AS_X86
	#endif
	#define __int64 long long
#endif

// GNU C
#ifdef __GNUC__
	#define GNU_STYLE_VIRTUAL_METHOD
	#define MULTI_BASE_OFFSET(x) (*((asDWORD*)(&x)+1))
	#define CALLEE_POPS_HIDDEN_RETURN_POINTER
	#define COMPLEX_OBJS_PASSED_BY_REF
	#define __int64 long long
	#define ASM_AT_N_T
	#define COMPLEX_MASK (asOBJ_CLASS_DESTRUCTOR)
	#define STDCALL __attribute__((stdcall))
	#ifdef __linux__
		#define THISCALL_RETURN_SIMPLE_IN_MEMORY
		#define CDECL_RETURN_SIMPLE_IN_MEMORY
		#define STDCALL_RETURN_SIMPLE_IN_MEMORY
	#endif
	#ifdef i386
		#define AS_X86
	#endif
#endif

// Dreamcast console
#ifdef __SH4_SINGLE_ONLY__
	#define AS_ALIGN				// align datastructures
	#define AS_USE_DOUBLE_AS_FLOAT	// use 32bit floats instead of doubles
	#define AS_SH4
#endif



// 
// Alignment macros
//----------------------------------------------------------------

#ifdef AS_ALIGN
	#define	ALIGN(b) (((b)+3)&(~3))
#else
	#define	ALIGN(b) (b)
#endif

#define	ARG_W(b) ((asWORD*)&b)
#define	ARG_DW(b) ((asDWORD*)&b)
#define	ARG_QW(b) ((asQWORD*)&b)	
#define	BCARG_W(b) ((asWORD*)&(b)[1])
#define	BCARG_DW(b) ((asDWORD*)&(b)[1])
#define	BCARG_QW(b) ((asQWORD*)&(b)[1])



#include "../include/angelscript.h"

#endif


