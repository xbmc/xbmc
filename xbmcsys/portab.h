/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Portable macros, types and inlined assembly -
 *
 *  Copyright(C) 2002      Michael Militzer <isibaar@xvid.org>
 *               2002-2003 Peter Ross <pross@xvid.org>
 *               2002-2003 Edouard Gomez <ed.gomez@free.fr>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 ****************************************************************************/

#ifndef _PORTAB_H_
#define _PORTAB_H_

/*****************************************************************************
 *  Common things
 ****************************************************************************/

/* Buffer size for msvc implementation because it outputs to DebugOutput */
#if defined(_DEBUG)
extern unsigned int xvid_debug;
#define DPRINTF_BUF_SZ  1024
#endif

/*****************************************************************************
 *  Types used in XviD sources
 ****************************************************************************/

/*----------------------------------------------------------------------------
  | For MSVC
 *---------------------------------------------------------------------------*/

#if defined(_MSC_VER) || defined (__WATCOMC__)
#    define int8_t   char
#    define uint8_t  unsigned char
#    define int16_t  short
#    define uint16_t unsigned short
#    define int32_t  int
#    define uint32_t unsigned int
#    define int64_t  __int64
#    define uint64_t unsigned __int64

/*----------------------------------------------------------------------------
  | For all other compilers, use the standard header file
  | (compiler should be ISO C99 compatible, perhaps ISO C89 is enough)
 *---------------------------------------------------------------------------*/

#else

#    include <inttypes.h>

#endif

/*****************************************************************************
 *  Some things that are only architecture dependant
 ****************************************************************************/

#if defined(ARCH_IS_32BIT)
#    define CACHE_LINE 64
#    define ptr_t uint32_t
#    define intptr_t int32_t
#    if defined(_MSC_VER) && _MSC_VER >= 1300 && !defined(__INTEL_COMPILER)
#        include <stdarg.h>
#    else
#        define uintptr_t uint32_t
#    endif
#elif defined(ARCH_IS_64BIT)
#    define CACHE_LINE  64
#    define ptr_t uint64_t
#    define intptr_t int64_t
#    if defined (_MSC_VER) && _MSC_VER >= 1300 && !defined(__INTEL_COMPILER)
#        include <stdarg.h>
#    else
#        define uintptr_t uint64_t
#    endif
#else
#    error You are trying to compile XviD without defining address bus size.
#endif

/*****************************************************************************
 *  Things that must be sorted by compiler and then by architecture
 ****************************************************************************/

/*****************************************************************************
 *  MSVC compiler specific macros, functions
 ****************************************************************************/

#if defined(_MSC_VER)

/*----------------------------------------------------------------------------
  | Common msvc stuff
 *---------------------------------------------------------------------------*/

#    include <windows.h>
#    include <stdio.h>

/* Non ANSI mapping */
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf

/*
 * This function must be declared/defined all the time because MSVC does
 * not support C99 variable arguments macros.
 *
 * Btw, if the MS compiler does its job well, it should remove the nop
 * DPRINTF function when not compiling in _DEBUG mode
 */
#   ifdef _DEBUG
static __inline void DPRINTF(int level, char *fmt, ...)
{
	if (xvid_debug & level) {
		va_list args;
		char buf[DPRINTF_BUF_SZ];
		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		va_end(args);
		OutputDebugString(buf);
		fprintf(stderr, "%s", buf);
	}
}
#    else
static __inline void DPRINTF(int level, char *fmt, ...) {}
#    endif

#    if _MSC_VER <= 1200
#        define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
type * name = (type *) (((int32_t) name##_storage+(alignment - 1)) & ~((int32_t)(alignment)-1))
#    else
#        define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	__declspec(align(alignment)) type name[(sizex)*(sizey)]
#    endif


/*----------------------------------------------------------------------------
  | msvc x86 specific macros/functions
 *---------------------------------------------------------------------------*/
#    if defined(ARCH_IS_IA32)
#        define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax

static __inline int64_t read_counter(void)
{
	int64_t ts;
	uint32_t ts1, ts2;
	__asm {
		rdtsc
			mov ts1, eax
			mov ts2, edx
	}
	ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
	return ts;
}

/*----------------------------------------------------------------------------
  | msvc GENERIC (plain C only) - Probably alpha or some embedded device
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_IS_GENERIC)
#        define BSWAP(a) \
	((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
	 (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))

#        include <time.h>
static __inline int64_t read_counter(void)
{
	return (int64_t)clock();
}

/*----------------------------------------------------------------------------
  | msvc Not given architecture - This is probably an user who tries to build
  | XviD the wrong way.
 *---------------------------------------------------------------------------*/
#    else
#        error You are trying to compile XviD without defining the architecture type.
#    endif




/*****************************************************************************
 *  GNU CC compiler stuff
 ****************************************************************************/

#elif defined(__GNUC__) || defined(__ICC) /* Compiler test */

/*----------------------------------------------------------------------------
  | Common gcc stuff
 *---------------------------------------------------------------------------*/

/*
 * As gcc is (mostly) C99 compliant, we define DPRINTF only if it's realy needed
 * and it's a macro calling fprintf directly
 */
#    ifdef _DEBUG

/* Needed for all debuf fprintf calls */
#       include <stdio.h>
#       include <stdarg.h>

static __inline void DPRINTF(int level, char *format, ...)
{
	va_list args;
	va_start(args, format);
	if(xvid_debug & level) {
		vfprintf(stderr, format, args);
	}
	va_end(args);
}

#    else /* _DEBUG */
static __inline void DPRINTF(int level, char *format, ...) {}
#    endif /* _DEBUG */


#    define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
type * name = (type *) (((ptr_t) name##_storage+(alignment - 1)) & ~((ptr_t)(alignment)-1))

/*----------------------------------------------------------------------------
  | gcc IA32 specific macros/functions
 *---------------------------------------------------------------------------*/
#    if defined(ARCH_IS_IA32)
#        define BSWAP(a) __asm__ ( "bswapl %0\n" : "=r" (a) : "0" (a) );

static __inline int64_t read_counter(void)
{
	int64_t ts;
	uint32_t ts1, ts2;
	__asm__ __volatile__("rdtsc\n\t":"=a"(ts1), "=d"(ts2));
	ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
	return ts;
}

/*----------------------------------------------------------------------------
  | gcc PPC and PPC Altivec specific macros/functions
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_IS_PPC)
#        define BSWAP(a) __asm__ __volatile__ \
	( "lwbrx %0,0,%1; eieio" : "=r" (a) : "r" (&(a)), "m" (a));

static __inline unsigned long get_tbl(void)
{
	unsigned long tbl;
	asm volatile ("mftb %0":"=r" (tbl));
	return tbl;
}

static __inline unsigned long get_tbu(void)
{
	unsigned long tbl;
	asm volatile ("mftbu %0":"=r" (tbl));
	return tbl;
}

static __inline int64_t read_counter(void)
{
	unsigned long tb, tu;
	do {
		tu = get_tbu();
		tb = get_tbl();
	}while (tb != get_tbl());
	return (((int64_t) tu) << 32) | (int64_t) tb;
}

/*----------------------------------------------------------------------------
  | gcc IA64 specific macros/functions
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_IS_IA64)
#        define BSWAP(a)  __asm__ __volatile__ \
	("mux1 %1 = %0, @rev" ";;" \
	 "shr.u %1 = %1, 32" : "=r" (a) : "r" (a));

static __inline int64_t read_counter(void)
{
	unsigned long result;
	__asm__ __volatile__("mov %0=ar.itc" : "=r"(result) :: "memory");
	return result;
}

/*----------------------------------------------------------------------------
  | gcc GENERIC (plain C only) specific macros/functions
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_IS_GENERIC)
#        define BSWAP(a) \
	((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
	 (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))

#        include <time.h>
static __inline int64_t read_counter(void)
{
	return (int64_t)clock();
}

/*----------------------------------------------------------------------------
  | gcc Not given architecture - This is probably an user who tries to build
  | XviD the wrong way.
 *---------------------------------------------------------------------------*/
#    else
#        error You are trying to compile XviD without defining the architecture type.
#    endif




/*****************************************************************************
 *  Open WATCOM C/C++ compiler
 ****************************************************************************/

#elif defined(__WATCOMC__)

#    include <stdio.h>
#    include <stdarg.h>

#    ifdef _DEBUG
static __inline void DPRINTF(int level, char *fmt, ...)
{
	if (xvid_debug & level) {
		va_list args;
		char buf[DPRINTF_BUF_SZ];
		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		va_end(args);
		fprintf(stderr, "%s", buf);
	}
}
#    else /* _DEBUG */
static __inline void DPRINTF(int level, char *format, ...) {}
#    endif /* _DEBUG */

#        define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
type * name = (type *) (((int32_t) name##_storage+(alignment - 1)) & ~((int32_t)(alignment)-1))

/*----------------------------------------------------------------------------
  | watcom ia32 specific macros/functions
 *---------------------------------------------------------------------------*/
#    if defined(ARCH_IS_IA32)

#        define BSWAP(a)  __asm mov eax,a __asm bswap eax __asm mov a, eax

static __inline int64_t read_counter(void)
{
	uint64_t ts;
	uint32_t ts1, ts2;
	__asm {
		rdtsc
			mov ts1, eax
			mov ts2, edx
	}
	ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
	return ts;
}

/*----------------------------------------------------------------------------
  | watcom GENERIC (plain C only) specific macros/functions.
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_IS_GENERIC)

#        define BSWAP(x) \
	x = ((((x) & 0xff000000) >> 24) | \
			(((x) & 0x00ff0000) >>  8) | \
			(((x) & 0x0000ff00) <<  8) | \
			(((x) & 0x000000ff) << 24))

static int64_t read_counter() { return 0; }

/*----------------------------------------------------------------------------
  | watcom Not given architecture - This is probably an user who tries to build
  | XviD the wrong way.
 *---------------------------------------------------------------------------*/
#    else
#        error You are trying to compile XviD without defining the architecture type.
#    endif


/*****************************************************************************
 *  Unknown compiler
 ****************************************************************************/
#else /* Compiler test */

/*
 * Ok we know nothing about the compiler, so we fallback to ANSI C
 * features, so every compiler should be happy and compile the code.
 *
 * This is (mostly) equivalent to ARCH_IS_GENERIC.
 */

#    ifdef _DEBUG

/* Needed for all debuf fprintf calls */
#       include <stdio.h>
#       include <stdarg.h>

static __inline void DPRINTF(int level, char *format, ...)
{
	va_list args;
	va_start(args, format);
	if(xvid_debug & level) {
		vfprintf(stderr, format, args);
	}
	va_end(args);
}

#    else /* _DEBUG */
static __inline void DPRINTF(int level, char *format, ...) {}
#    endif /* _DEBUG */

#    define BSWAP(a) \
	((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
	 (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))

#    include <time.h>
static __inline int64_t read_counter(void)
{
	return (int64_t)clock();
}

#    define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	type name[(sizex)*(sizey)]

#endif /* Compiler test */


#endif /* PORTAB_H */
