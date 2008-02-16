/*****************************************************************
|
|   Neptune - Configuration
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_CONFIG_H_
#define _NPT_CONFIG_H_

/*----------------------------------------------------------------------
|   defaults
+---------------------------------------------------------------------*/
#define NPT_CONFIG_HAVE_ASSERT_H
#define NPT_CONFIG_HAVE_STD_C
#define NPT_CONFIG_HAVE_STDLIB_H
#define NPT_CONFIG_HAVE_STDIO_H
#define NPT_CONFIG_HAVE_STDARG_H
#define NPT_CONFIG_HAVE_STRING_H

/*----------------------------------------------------------------------
|   standard C runtime
+---------------------------------------------------------------------*/
#if defined(NPT_CONFIG_HAVE_STD_C)
#define NPT_CONFIG_HAVE_MALLOC
#define NPT_CONFIG_HAVE_CALLOC
#define NPT_CONFIG_HAVE_REALLOC
#define NPT_CONFIG_HAVE_FREE
#define NPT_CONFIG_HAVE_MEMCPY
#define NPT_CONFIG_HAVE_MEMSET
#define NPT_CONFIG_HAVE_MEMCMP
#define NPT_CONFIG_HAVE_GETENV
#endif /* NPT_CONFIG_HAS_STD_C */

#if defined(NPT_CONFIG_HAVE_STRING_H)
#define NPT_CONFIG_HAVE_STRCMP
#define NPT_CONFIG_HAVE_STRNCMP
#define NPT_CONFIG_HAVE_STRDUP
#define NPT_CONFIG_HAVE_STRLEN
#define NPT_CONFIG_HAVE_STRCPY
#define NPT_CONFIG_HAVE_STRNCPY
#endif /* NPT_CONFIG_HAVE_STRING_H */

#if defined(NPT_CONFIG_HAVE_STDIO_H)
#define NPT_CONFIG_HAVE_SPRINTF
#define NPT_CONFIG_HAVE_SNPRINTF
#define NPT_CONFIG_HAVE_VSPRINTF
#define NPT_CONFIG_HAVE_VSNPRINTF
#endif /* NPT_CONFIG_HAVE_STDIO_H */

/*----------------------------------------------------------------------
|   standard C++ runtime
+---------------------------------------------------------------------*/
#define NPT_CONFIG_HAVE_NEW_H

/*----------------------------------------------------------------------
|   sockets
+---------------------------------------------------------------------*/
#define NPT_CONFIG_HAVE_SOCKADDR_SA_LEN

/*----------------------------------------------------------------------
|   platform specifics
+---------------------------------------------------------------------*/
/* Windows 32 */
#if defined(_WIN32) || defined(_XBOX)
#if !defined(STRICT)
#define STRICT
#endif
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

/* XBox */
#if defined(_XBOX)
#define NPT_CONFIG_THREAD_STACK_SIZE 0x10000
#endif

/* QNX */
#if defined(__QNX__)
#endif

/* cygwin */
#if defined(__CYGWIN__)
#undef NPT_CONFIG_HAVE_SOCKADDR_SA_LEN
#endif

/* linux */
#if defined(__linux__)
#undef NPT_CONFIG_HAVE_SOCKADDR_SA_LEN
#endif

/* symbian */
#if defined(__SYMBIAN32__)
/* If defined, specify the stack size of each NPT_Thread. */
#define NPT_CONFIG_THREAD_STACK_SIZE   0x14000
#endif

/*----------------------------------------------------------------------
|   compiler specifics
+---------------------------------------------------------------------*/
/* GCC */
#if defined(__GNUC__)
#define NPT_COMPILER_UNUSED(p) (void)p
#else
#define NPT_COMPILER_UNUSED(p) 
#endif

/* TriMedia C/C++ Compiler */
#if defined(__TCS__)
#undef NPT_CONFIG_HAVE_ASSERT_H
#undef NPT_CONFIG_HAVE_SNPRINTF
#undef NPT_CONFIG_HAVE_VSNPRINTF
#endif

/* palmos compiler */
#if defined(__PALMOS__)
#if __PALMOS__ <= 0x05000000
#undef NPT_CONFIG_HAVE_ASSERT_H
#undef NPT_CONFIG_HAVE_SNPRINTF
#undef NPT_CONFIG_HAVE_VSNPRINTF
#endif
#endif

/* Microsoft C/C++ Compiler */
#if defined(_MSC_VER)
#if defined(_WIN64)
typedef __int64 NPT_PointerLong;
#else
#if _MSC_VER >= 1400
typedef __w64 long NPT_PointerLong;
#else
typedef long NPT_PointerLong;
#endif
#endif
#define NPT_POINTER_TO_LONG(_p) ((NPT_PointerLong) (_p) )
#if _MSC_VER >= 1400 && !defined(_WIN32_WCE)
#define NPT_CONFIG_HAVE_FOPEN_S
#define NPT_CONFIG_HAVE_FSOPEN
#define NPT_CONFIG_HAVE_SHARE_H
#define NPT_vsnprintf(s,c,f,a)  _vsnprintf_s(s,c,_TRUNCATE,f,a)
#define NPT_snprintf(s,c,f,...) _snprintf_s(s,c,_TRUNCATE,f,__VA_ARGS__)
#define NPT_strncpy(d,s,c)       strncpy_s(d,c,s,_TRUNCATE)
#undef NPT_CONFIG_HAVE_GETENV
#define NPT_CONFIG_HAVE_DUPENV_S
#define dupenv_s _dupenv_s
#else
#define NPT_vsnprintf  _vsnprintf
#define NPT_snprintf   _snprintf
#endif
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif
#endif

/* Windows CE */
#if defined(_WIN32_WCE)
#if defined(NPT_CONFIG_HAVE_FOPEN_S)
#undef NPT_CONFIG_HAVE_FOPEN_S
#endif
#endif

/* Symbian */
#if defined(__SYMBIAN32__)
#undef NPT_CONFIG_HAVE_NEW_H
#include "e32std.h"
#define explicit
#endif

/*----------------------------------------------------------------------
|   defaults
+---------------------------------------------------------------------*/
#ifndef NPT_POINTER_TO_LONG
#define NPT_POINTER_TO_LONG(_p) ((long)(_p))
#endif

#if !defined(NPT_snprintf)
#define NPT_snprintf snprintf
#endif
#if !defined(NPT_strncpy)
#define NPT_strncpy strncpy
#endif
#if !defined(NPT_vsnprintf)
#define NPT_vsnprintf vsnprintf
#endif

#if !defined(NPT_CONFIG_THREAD_STACK_SIZE)
#define NPT_CONFIG_THREAD_STACK_SIZE 0
#endif

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#if defined(DMALLOC)
#include <dmalloc.h>
#endif

#endif // _NPT_CONFIG_H_ 
