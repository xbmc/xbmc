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
// as_config.h
//
// this file is used for configuring the compilation of the library
//

#ifndef AS_CONFIG_H
#define AS_CONFIG_H



//
// Features
//-----------------------------------------

// AS_NO_THREADS
// Turns off support for multithreading. By turning off
// this when it's not needed a bit of performance is gained.

// AS_WINDOWS_THREADS
// If the library should be compiled using windows threads.

// AS_POSIX_THREADS
// If the library should be compiled using posix threads.

// AS_NO_ATOMIC
// If the compiler/platform doesn't support atomic instructions
// then this should be defined to use critical sections instead.

// AS_DEBUG
// This flag can be defined to make the library write some extra output when
// compiling and executing scripts.

// AS_DEPRECATED
// If this flag is defined then some backwards compatibility is maintained.
// There is no guarantee for how well deprecated functionality will work though
// so it is best to exchange it for the new functionality as soon as possible.

// AS_NO_CLASS_METHODS
// Disables the possibility to add class methods. Can increase the
// portability of the library.

// AS_MAX_PORTABILITY
// Disables all platform specific code. Only the asCALL_GENERIC calling
// convention will be available in with this flag set.

// AS_DOUBLEBYTE_CHARSET
// When this flag is defined, the parser will treat all characters in strings
// that are greater than 127 as lead characters and automatically include the
// next character in the script without checking its value. This should be 
// compatible with common encoding schemes, e.g. Big5. Shift-JIS is not compatible 
// though as it encodes some single byte characters above 127. 
//
// If support for international text is desired, it is recommended that UTF-8
// is used as this is supported natively by the compiler without the use for this
// preprocessor flag.




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

// asVSNPRINTF(a,b,c,d)
// Some compilers use different names for this function. You must 
// define this macro to map to the proper function.

// ASM_AT_N_T or ASM_INTEL
// You should choose what inline assembly syntax to use when compiling.

// VALUE_OF_BOOLEAN_TRUE
// This flag allows to customize the exact value of boolean true.

// AS_SIZEOF_BOOL
// On some target platforms the sizeof(bool) is 4, but on most it is 1.

// STDCALL
// This is used to declare a function to use the stdcall calling convention.

// AS_USE_NAMESPACE
// Adds the AngelScript namespace on the declarations.

// AS_NO_MEMORY_H
// Some compilers don't come with the memory.h header file.



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

// AS_MIPS
// Use assembler code for the MIPS CPU family

// AS_PPC
// Use assembler code for the 32bit PowerPC CPU family

// AS_PPC_64
// Use assembler code for the 64bit PowerPC CPU family

// AS_XENON
// Use assembler code for the Xenon (XBOX360) CPU family

// AS_ARM
// Use assembler code for the ARM CPU family

// AS_64BIT_PTR
// Define this to make the engine store all pointers in 64bit words. 

// AS_BIG_ENDIAN
// Define this for CPUs that use big endian memory layout, e.g. PPC



//
// Target systems
//--------------------------------
// This group shows a few of the flags used to identify different target systems.
// Sometimes there are differences on different target systems, while both CPU and
// compiler is the same for both, when this is so these flags are used to produce the
// right code.

// AS_WIN     - Microsoft Windows
// AS_LINUX   - Linux
// AS_MAC     - Apple Macintosh
// AS_BSD     - FreeBSD
// AS_XBOX    - Microsoft XBox
// AS_XBOX360 - Microsoft XBox 360
// AS_PSP     - Sony Playstation Portable
// AS_PS2     - Sony Playstation 2
// AS_PS3     - Sony Playstation 3
// AS_DC      - Sega Dreamcast
// AS_GC      - Nintendo GameCube
// AS_WII     - Nintendo Wii
// AS_IPHONE  - Apple IPhone
// AS_ANDROID - Android




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

// THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE
// STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE
// CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE
// Specifies the minimum size in dwords a class/struct needs to be to be passed in memory


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

// HAS_128_BIT_PRIMITIVES
// 64bit processors often support 128bit primitives. These may require special
// treatment when passed in function arguments or returned by functions.

// SPLIT_OBJS_BY_MEMBER_TYPES
// On some platforms objects with primitive members are split over different
// register types when passed by value to functions. 





//
// Detect compiler
//------------------------------------------------

#define VALUE_OF_BOOLEAN_TRUE  1
#define STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 0
#define CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 0
#define THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 0

// Microsoft Visual C++
#if defined(_MSC_VER) && !defined(__MWERKS__)
	#define MULTI_BASE_OFFSET(x) (*((asDWORD*)(&x)+1))
	#define HAVE_VIRTUAL_BASE_OFFSET
	#define VIRTUAL_BASE_OFFSET(x) (*((asDWORD*)(&x)+3))
	#define THISCALL_RETURN_SIMPLE_IN_MEMORY
	#define THISCALL_PASS_OBJECT_POINTER_IN_ECX
	#if _MSC_VER < 1500 // MSVC++ 9 (aka MSVC++ .NET 2008)
		#define asVSNPRINTF(a, b, c, d) _vsnprintf(a, b, c, d)
	#else
		#define asVSNPRINTF(a, b, c, d) vsnprintf_s(a, b, _TRUNCATE, c, d)
	#endif
	#define THISCALL_CALLEE_POPS_ARGUMENTS
	#define STDCALL __stdcall
	#define AS_SIZEOF_BOOL 1
	#define AS_WINDOWS_THREADS

	#define ASM_INTEL  // Intel style for inline assembly on microsoft compilers

	#if defined(WIN32)
		#define AS_WIN
	#endif

	#if _XBOX_VER >= 200
		// 360 uses a Xenon processor (which is a modified 64bit PPC)
		#define AS_XBOX360
		#define AS_XENON
		#define AS_BIG_ENDIAN
	#else
		// Support native calling conventions on x86, but not 64bit yet
		#if defined(_XBOX) || (defined(_M_IX86) && !defined(__LP64__))
			#define AS_X86
		#endif
	#endif

	#if _MSC_VER <= 1200 // MSVC++ 6
		#define I64(x) x##l
	#else
		#define I64(x) x##ll
	#endif

    #ifdef _ARM_
        #define AS_ALIGN
        #define AS_ARM
        #define CDECL_RETURN_SIMPLE_IN_MEMORY
        #define STDCALL_RETURN_SIMPLE_IN_MEMORY
        #define COMPLEX_OBJS_PASSED_BY_REF
        #define COMPLEX_MASK asOBJ_APP_CLASS_ASSIGNMENT
    #else
        #define COMPLEX_MASK (asOBJ_APP_CLASS_CONSTRUCTOR | asOBJ_APP_CLASS_DESTRUCTOR | asOBJ_APP_CLASS_ASSIGNMENT)
    #endif

	#define UNREACHABLE_RETURN
#endif

// Metrowerks CodeWarrior (experimental, let me know if something isn't working)
#if defined(__MWERKS__) && !defined(EPPC) // JWC -- If Wii DO NOT use this even when using Metrowerks Compiler. Even though they are called Freescale...
	#define MULTI_BASE_OFFSET(x) (*((asDWORD*)(&x)+1))
	#define HAVE_VIRTUAL_BASE_OFFSET
	#define VIRTUAL_BASE_OFFSET(x) (*((asDWORD*)(&x)+3))
	#define THISCALL_RETURN_SIMPLE_IN_MEMORY
	#define THISCALL_PASS_OBJECT_POINTER_IN_ECX
	#define asVSNPRINTF(a, b, c, d) _vsnprintf(a, b, c, d)
	#define THISCALL_CALLEE_POPS_ARGUMENTS
	#define COMPLEX_MASK (asOBJ_APP_CLASS_CONSTRUCTOR | asOBJ_APP_CLASS_DESTRUCTOR | asOBJ_APP_CLASS_ASSIGNMENT)
	#define AS_SIZEOF_BOOL 1
	#define AS_WINDOWS_THREADS
	#define STDCALL __stdcall

	// Support native calling conventions on x86, but not 64bit yet
	#if defined(_M_IX86) && !defined(__LP64__)
		#define AS_X86
		#define ASM_INTEL  // Intel style for inline assembly
	#endif

	#if _MSC_VER <= 1200 // MSVC++ 6
		#define I64(x) x##l
	#else
		#define I64(x) x##ll
	#endif

	#define UNREACHABLE_RETURN
#endif

// SN Systems ProDG (also experimental, let me know if something isn't working)
#if defined(__SNC__) || defined(SNSYS)
	#define GNU_STYLE_VIRTUAL_METHOD
	#define MULTI_BASE_OFFSET(x) (*((asDWORD*)(&x)+1))
	#define CALLEE_POPS_HIDDEN_RETURN_POINTER
	#define COMPLEX_OBJS_PASSED_BY_REF
	#define ASM_AT_N_T  // AT&T style inline assembly
	#define COMPLEX_MASK (asOBJ_APP_CLASS_DESTRUCTOR)
	#define AS_SIZEOF_BOOL 1
	#define asVSNPRINTF(a, b, c, d) vsnprintf(a, b, c, d)

	// SN doesnt seem to like STDCALL.
	// Maybe it can work with some fiddling, but I can't imagine linking to
	// any STDCALL functions with a console anyway...
	#define STDCALL

	// Linux specific
	#ifdef __linux__
		#define THISCALL_RETURN_SIMPLE_IN_MEMORY
		#define CDECL_RETURN_SIMPLE_IN_MEMORY
		#define STDCALL_RETURN_SIMPLE_IN_MEMORY
	#endif

	// Support native calling conventions on x86, but not 64bit yet
	#if defined(i386) && !defined(__LP64__)
		#define AS_X86
	#endif

	#define I64(x) x##ll

	#define UNREACHABLE_RETURN
#endif

// GNU C (and MinGW on Windows)
#if (defined(__GNUC__) && !defined(__SNC__)) || defined(EPPC) // JWC -- use this instead for Wii
	#define GNU_STYLE_VIRTUAL_METHOD
#if !defined( __amd64__ )
	#define MULTI_BASE_OFFSET(x) (*((asDWORD*)(&x)+1))
#else
	#define MULTI_BASE_OFFSET(x) (*((asQWORD*)(&x)+1))
#endif
	#define asVSNPRINTF(a, b, c, d) vsnprintf(a, b, c, d)
	#define CALLEE_POPS_HIDDEN_RETURN_POINTER
	#define COMPLEX_OBJS_PASSED_BY_REF
	#define COMPLEX_MASK (asOBJ_APP_CLASS_DESTRUCTOR)
	#define AS_NO_MEMORY_H
	#define AS_SIZEOF_BOOL 1
	#define STDCALL __attribute__((stdcall))
	#define ASM_AT_N_T

	// MacOSX and IPhone
	#ifdef __APPLE__

		// Is this a Mac or an IPhone?
		#ifdef TARGET_OS_IPHONE
			#define AS_IPHONE
		#else
			#define AS_MAC
		#endif

		// The sizeof bool is different depending on the target CPU
		#undef AS_SIZEOF_BOOL
		#if defined(__ppc__)
			#define AS_SIZEOF_BOOL 4
			// STDCALL is not available on PPC
			#undef STDCALL
			#define STDCALL
		#else
			#define AS_SIZEOF_BOOL 1
		#endif

		#if defined(i386) && !defined(__LP64__)
			// Support native calling conventions on Mac OS X + Intel 32bit CPU
			#define AS_X86
		#elif (defined(__ppc__) || defined(__PPC__)) && !defined(__LP64__)
			// Support native calling conventions on Mac OS X + PPC 32bit CPU
			#define AS_PPC
			#define THISCALL_RETURN_SIMPLE_IN_MEMORY
			#define CDECL_RETURN_SIMPLE_IN_MEMORY
			#define STDCALL_RETURN_SIMPLE_IN_MEMORY
		#elif (defined(__ppc__) || defined(__PPC__)) && defined(__LP64__)
			#define AS_PPC_64
		#elif (defined(_ARM_) || defined(__arm__))
			// The IPhone use an ARM processor
			#define AS_ARM
			#define AS_IPHONE
			#define AS_ALIGN
			#define CDECL_RETURN_SIMPLE_IN_MEMORY
			#define STDCALL_RETURN_SIMPLE_IN_MEMORY
			#define THISCALL_RETURN_SIMPLE_IN_MEMORY

			#undef THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE
			#undef CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE
			#undef STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE

			#define THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 2
			#define CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 2
			#define STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 2
			#define COMPLEX_OBJS_PASSED_BY_REF
			#undef COMPLEX_MASK
			#define COMPLEX_MASK asOBJ_APP_CLASS_DESTRUCTOR
		#else
			// Unknown CPU type
			#define AS_MAX_PORTABILITY
		#endif
		#define AS_POSIX_THREADS
 
	// Windows
	#elif defined(WIN32)
		// On Windows the simple classes are returned in the EAX:EDX registers
		//#define THISCALL_RETURN_SIMPLE_IN_MEMORY
		//#define CDECL_RETURN_SIMPLE_IN_MEMORY
		//#define STDCALL_RETURN_SIMPLE_IN_MEMORY

		#if defined(i386) && !defined(__LP64__)
			// Support native calling conventions on Intel 32bit CPU
			#define AS_X86
		#else
			// No support for native calling conventions yet
			#define AS_MAX_PORTABILITY
			// STDCALL is not available on 64bit Linux
			#undef STDCALL
			#define STDCALL
		#endif
        #define AS_WIN
        #define AS_WINDOWS_THREADS

	// Linux
	#elif defined(__linux__)
		#if defined(i386) && !defined(__LP64__)
			#define THISCALL_RETURN_SIMPLE_IN_MEMORY
			#define CDECL_RETURN_SIMPLE_IN_MEMORY
			#define STDCALL_RETURN_SIMPLE_IN_MEMORY

			// Support native calling conventions on Intel 32bit CPU
			#define AS_X86
		#else
			#define AS_X64_GCC
			#define HAS_128_BIT_PRIMITIVES
			#define SPLIT_OBJS_BY_MEMBER_TYPES
			// STDCALL is not available on 64bit Linux
			#undef STDCALL
			#define STDCALL
		#endif
       	#define AS_LINUX
       	#define AS_POSIX_THREADS

		#if !( ( (__GNUC__ == 4) && (__GNUC_MINOR__ >= 1) || __GNUC__ > 4) )
			// Only with GCC 4.1 was the atomic instructions available
			#define AS_NO_ATOMIC
		#endif

	// Free BSD
	#elif __FreeBSD__
		#define AS_BSD
		#if defined(i386) && !defined(__LP64__)
			#define AS_X86
		#else
			#define AS_MAX_PORTABILITY
		#endif
		#define AS_POSIX_THREADS
		#if !( ( (__GNUC__ == 4) && (__GNUC_MINOR__ >= 1) || __GNUC__ > 4) )
			// Only with GCC 4.1 was the atomic instructions available
			#define AS_NO_ATOMIC
		#endif

	// PSP and PS2
	#elif defined(__PSP__) || defined(__psp__) || defined(_EE_) || defined(_PSP) || defined(_PS2)
		// Support native calling conventions on MIPS architecture
		#if (defined(_MIPS_ARCH) || defined(_mips) || defined(__MIPSEL__)) && !defined(__LP64__)
			#define AS_MIPS
		#else
			#define AS_MAX_PORTABILITY
		#endif

	// PS3
	#elif (defined(__PPC__) || defined(__ppc__)) && defined(__PPU__)
		// Support native calling conventions on PS3
		#define AS_PS3
		#define AS_PPC_64
		#define SPLIT_OBJS_BY_MEMBER_TYPES
		#define THISCALL_RETURN_SIMPLE_IN_MEMORY
		#define CDECL_RETURN_SIMPLE_IN_MEMORY
		#define STDCALL_RETURN_SIMPLE_IN_MEMORY
		// PS3 doesn't have STDCALL
		#undef STDCALL
		#define STDCALL

	// Dreamcast
	#elif __SH4_SINGLE_ONLY__
		// Support native calling conventions on Dreamcast
		#define AS_DC
		#define AS_SH4

	// Wii JWC - Close to PS3 just no PPC_64 and AS_PS3
	#elif defined(EPPC)
		#define AS_WII
		#define THISCALL_RETURN_SIMPLE_IN_MEMORY
		#define CDECL_RETURN_SIMPLE_IN_MEMORY
		#define STDCALL_RETURN_SIMPLE_IN_MEMORY
		#undef STDCALL
		#define STDCALL

    // Android
	#elif defined(ANDROID)
		#define AS_ANDROID
		#define AS_NO_ATOMIC

		#define CDECL_RETURN_SIMPLE_IN_MEMORY
		#define STDCALL_RETURN_SIMPLE_IN_MEMORY
		#define THISCALL_RETURN_SIMPLE_IN_MEMORY

		#undef THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE
		#undef CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE
		#undef STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE

		#define THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 2
		#define CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 2
		#define STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE 2

		#if (defined(_ARM_) || defined(__arm__))
		    #define AS_ARM
			#define AS_ALIGN
		#endif
	#endif

	#define I64(x) x##ll

	#define UNREACHABLE_RETURN
#endif


//
// Detect target hardware
//------------------------------------------------

// X86, Intel, AMD, etc, i.e. most PCs
#if defined(__i386__) || defined(_M_IX86)
	// Nothing special here
#endif

// MIPS architecture (generally PS2 and PSP consoles, potentially supports N64 as well)
#if defined(_MIPS_ARCH) || defined(_mips) || defined(__MIPSEL__) || defined(__PSP__) || defined(__psp__) || defined(_EE_) || defined(_PSP) || defined(_PS2)
	#define AS_ALIGN				// align datastructures
	#define AS_USE_DOUBLE_AS_FLOAT	// use 32bit floats instead of doubles
#endif

// PowerPC, e.g. Mac, GameCube, PS3, XBox 360, Wii
#if defined(__PPC__) || defined(__ppc__) || defined(_PPC_) || defined(EPPC)
	#define AS_BIG_ENDIAN

	// Gamecube
	#if defined(_GC)
		#define AS_ALIGN
		#define AS_USE_DOUBLE_AS_FLOAT
	#endif
	// XBox 360
	#if (_XBOX_VER >= 200 )
		#define AS_ALIGN
	#endif
	// PS3
	#if defined(__PPU__)
		#define AS_ALIGN
	#endif
	// Wii
	#if defined(EPPC)
		#define AS_ALIGN
	#endif
#endif

// Dreamcast console
#ifdef __SH4_SINGLE_ONLY__
	#define AS_ALIGN				// align datastructures
	#define AS_USE_DOUBLE_AS_FLOAT	// use 32bit floats instead of doubles
#endif

// Is the target a 64bit system?
#if defined(__LP64__) || defined(__amd64__) || defined(_M_X64)
	#ifndef AS_64BIT_PTR
		#define AS_64BIT_PTR
	#endif
#endif

// If there are no current support for native calling
// conventions, then compile with AS_MAX_PORTABILITY
#if (!defined(AS_X86) && !defined(AS_SH4) && !defined(AS_MIPS) && !defined(AS_PPC) && !defined(AS_PPC_64) && !defined(AS_XENON) && !defined(AS_X64_GCC) && !defined(AS_ARM))
	#ifndef AS_MAX_PORTABILITY
		#define AS_MAX_PORTABILITY
	#endif
#endif

// If the form of threads to use hasn't been chosen
// then the library will be compiled without support
// for multithreading
#if !defined(AS_POSIX_THREADS) && !defined(AS_WINDOWS_THREADS)
	#define AS_NO_THREADS
#endif


// The assert macro
#include <assert.h>
#define asASSERT(x) assert(x)



//
// Internal defines (do not change these)
//----------------------------------------------------------------

#ifdef AS_ALIGN
	#define	ALIGN(b) (((b)+3)&(~3))
#else
	#define	ALIGN(b) (b)
#endif

#define	ARG_W(b)    ((asWORD*)&b)
#define	ARG_DW(b)   ((asDWORD*)&b)
#define	ARG_QW(b)   ((asQWORD*)&b)
#define	BCARG_W(b)  ((asWORD*)&(b)[1])
#define	BCARG_DW(b) ((asDWORD*)&(b)[1])
#define	BCARG_QW(b) ((asQWORD*)&(b)[1])

#ifdef AS_64BIT_PTR
	#define AS_PTR_SIZE  2
	#define asPTRWORD    asQWORD
	#define asBC_RDSPTR  asBC_RDS8
#else
	#define AS_PTR_SIZE  1
	#define asPTRWORD    asDWORD
	#define asBC_RDSPTR  asBC_RDS4
#endif
#define ARG_PTR(b)   ((asPTRWORD*)&b)
#define BCARG_PTR(b) ((asPTRWORD*)&(b)[1])

// This macro is used to avoid warnings about unused variables.
// Usually where the variables are only used in debug mode.
#define UNUSED_VAR(x) (x)=(x)

#include "../include/angelscript.h"
#include "as_memory.h"

#ifdef AS_USE_NAMESPACE
using namespace AngelScript;
#endif

#endif
