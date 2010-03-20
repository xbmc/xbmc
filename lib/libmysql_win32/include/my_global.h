/* Copyright (C) 2000-2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* This is the include file that should be included 'first' in every C file. */

#ifndef _global_h
#define _global_h

/*
  InnoDB depends on some MySQL internals which other plugins should not
  need.  This is because of InnoDB's foreign key support, "safe" binlog
  truncation, and other similar legacy features.

  We define accessors for these internals unconditionally, but do not
  expose them in mysql/plugin.h.  They are declared in ha_innodb.h for
  InnoDB's use.
*/
#define INNODB_COMPATIBILITY_HOOKS

#ifdef __CYGWIN__
/* We use a Unix API, so pretend it's not Windows */
#undef WIN
#undef WIN32
#undef _WIN
#undef _WIN32
#undef _WIN64
#undef __WIN__
#undef __WIN32__
#define HAVE_ERRNO_AS_DEFINE
#endif /* __CYGWIN__ */

#if defined(__QNXNTO__) && !defined(FD_SETSIZE)
#define FD_SETSIZE 1024         /* Max number of file descriptor bits in
                                   fd_set, used when calling 'select'
                                   Must be defined before including
                                   "sys/select.h" and "sys/time.h"
                                 */
#endif


/* to make command line shorter we'll define USE_PRAGMA_INTERFACE here */
#ifdef USE_PRAGMA_IMPLEMENTATION
#define USE_PRAGMA_INTERFACE
#endif

#if defined(i386) && !defined(__i386__)
#define __i386__
#endif

/* Macros to make switching between C and C++ mode easier */
#ifdef __cplusplus
#define C_MODE_START    extern "C" {
#define C_MODE_END	}
#else
#define C_MODE_START
#define C_MODE_END
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(WIN32)
#include <config-win.h>
#elif defined(__NETWARE__)
#include <my_config.h>
#include <config-netware.h>
#if defined(__cplusplus) && defined(inline)
#undef inline				/* fix configure problem */
#endif
#else
#include <my_config.h>
#if defined(__cplusplus) && defined(inline)
#undef inline				/* fix configure problem */
#endif
#endif /* _WIN32... */

/* Make it easier to add conditionl code for windows */
#ifdef __WIN__
#define IF_WIN(A,B) (A)
#else
#define IF_WIN(A,B) (B)
#endif

#ifndef EMBEDDED_LIBRARY
#ifdef WITH_NDB_BINLOG
#define HAVE_NDB_BINLOG 1
#endif
#endif /* !EMBEDDED_LIBRARY */

#ifndef EMBEDDED_LIBRARY
#define HAVE_REPLICATION
#define HAVE_EXTERNAL_CLIENT
#endif

/* Some defines to avoid ifdefs in the code */
#ifndef NETWARE_YIELD
#define NETWARE_YIELD
#define NETWARE_SET_SCREEN_MODE(A)
#endif

/* Workaround for _LARGE_FILES and _LARGE_FILE_API incompatibility on AIX */
#if defined(_AIX) && defined(_LARGE_FILE_API)
#undef _LARGE_FILE_API
#endif

/*
  The macros below are used to allow build of Universal/fat binaries of
  MySQL and MySQL applications under darwin. 
*/
#if defined(__APPLE__) && defined(__MACH__)
#  undef SIZEOF_CHARP 
#  undef SIZEOF_SHORT 
#  undef SIZEOF_INT 
#  undef SIZEOF_LONG 
#  undef SIZEOF_LONG_LONG 
#  undef SIZEOF_OFF_T 
#  undef WORDS_BIGENDIAN
#  define SIZEOF_SHORT 2
#  define SIZEOF_INT 4
#  define SIZEOF_LONG_LONG 8
#  define SIZEOF_OFF_T 8
#  if defined(__i386__) || defined(__ppc__)
#    define SIZEOF_CHARP 4
#    define SIZEOF_LONG 4
#  elif defined(__x86_64__) || defined(__ppc64__)
#    define SIZEOF_CHARP 8
#    define SIZEOF_LONG 8
#  else
#    error Building FAT binary for an unknown architecture.
#  endif
#  if defined(__ppc__) || defined(__ppc64__)
#    define WORDS_BIGENDIAN
#  endif
#endif /* defined(__APPLE__) && defined(__MACH__) */


/*
  The macros below are borrowed from include/linux/compiler.h in the
  Linux kernel. Use them to indicate the likelyhood of the truthfulness
  of a condition. This serves two purposes - newer versions of gcc will be
  able to optimize for branch predication, which could yield siginficant
  performance gains in frequently executed sections of the code, and the
  other reason to use them is for documentation
*/

#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x)	__builtin_expect((x),1)
#define unlikely(x)	__builtin_expect((x),0)


/*
  The macros below are useful in optimising places where it has been
  discovered that cache misses stall the process and where a prefetch
  of the cache line can improve matters. This is available in GCC 3.1.1
  and later versions.
  PREFETCH_READ says that addr is going to be used for reading and that
  it is to be kept in caches if possible for a while
  PREFETCH_WRITE also says that the item to be cached is likely to be
  updated.
  The *LOCALITY scripts are also available for experimentation purposes
  mostly and should only be used if they are verified to improve matters.
  For more input see GCC manual (available in GCC 3.1.1 and later)
*/

#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR > 10)
#define PREFETCH_READ(addr) __builtin_prefetch(addr, 0, 3)
#define PREFETCH_WRITE(addr) \
  __builtin_prefetch(addr, 1, 3)
#define PREFETCH_READ_LOCALITY(addr, locality) \
  __builtin_prefetch(addr, 0, locality)
#define PREFETCH_WRITE_LOCALITY(addr, locality) \
  __builtin_prefetch(addr, 1, locality)
#else
#define PREFETCH_READ(addr)
#define PREFETCH_READ_LOCALITY(addr, locality)
#define PREFETCH_WRITE(addr)
#define PREFETCH_WRITE_LOCALITY(addr, locality)
#endif

/*
  The following macro is used to ensure that code often used in most
  SQL statements and definitely for parts of the SQL processing are
  kept in a code segment by itself. This has the advantage that the
  risk of common code being overlapping in caches of the CPU is less.
  This can be a cause of big performance problems.
  Routines should be put in this category with care and when they are
  put there one should also strive to make as much of the error handling
  as possible (or uncommon code of the routine) to execute in a
  separate method to avoid moving to much code to this code segment.

  It is very easy to use, simply add HOT_METHOD at the end of the
  function declaration.
  For more input see GCC manual (available in GCC 2.95 and later)
*/

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR > 94)
#define HOT_METHOD \
  __attribute__ ((section ("hot_code_section")))
#else
#define HOT_METHOD
#endif

/*
  The following macro is used to ensure that popular global variables
  are located next to each other to avoid that they contend for the
  same cache lines.

  It is very easy to use, simply add HOT_DATA at the end of the declaration
  of the variable, the variable must be initialised because of the way
  that linker works so a declaration using HOT_DATA should look like:
  uint global_hot_data HOT_DATA = 0;
  For more input see GCC manual (available in GCC 2.95 and later)
*/

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR > 94)
#define HOT_DATA \
  __attribute__ ((section ("hot_data_section")))
#else
#define HOT_DATA
#endif

/*
  now let's figure out if inline functions are supported
  autoconf defines 'inline' to be empty, if not
*/
#define inline_test_1(X)        X ## 1
#define inline_test_2(X)        inline_test_1(X)
#if inline_test_2(inline) != 1
#define HAVE_INLINE
#endif
#undef inline_test_2
#undef inline_test_1
/* helper macro for "instantiating" inline functions */
#define STATIC_INLINE static inline

/*
  The following macros are used to control inlining a bit more than
  usual. These macros are used to ensure that inlining always or
  never occurs (independent of compilation mode).
  For more input see GCC manual (available in GCC 3.1.1 and later)
*/

#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR > 10)
#define ALWAYS_INLINE __attribute__ ((always_inline))
#define NEVER_INLINE __attribute__ ((noinline))
#else
#define ALWAYS_INLINE
#define NEVER_INLINE
#endif


/* Fix problem with S_ISLNK() on Linux */
#if defined(TARGET_OS_LINUX) || defined(__GLIBC__)
#undef  _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/*
  Temporary solution to solve bug#7156. Include "sys/types.h" before
  the thread headers, else the function madvise() will not be defined
*/
#if defined(HAVE_SYS_TYPES_H) && ( defined(sun) || defined(__sun) )
#include <sys/types.h>
#endif

/* The client defines this to avoid all thread code */
#if defined(MYSQL_CLIENT_NO_THREADS) || defined(UNDEF_THREADS_HACK)
#undef THREAD
#undef HAVE_LINUXTHREADS
#undef HAVE_NPTL
#endif

#ifdef HAVE_THREADS_WITHOUT_SOCKETS
/* MIT pthreads does not work with unix sockets */
#undef HAVE_SYS_UN_H
#endif

#define __EXTENSIONS__ 1	/* We want some extension */
#ifndef __STDC_EXT__
#define __STDC_EXT__ 1          /* To get large file support on hpux */
#endif

/*
  Solaris 9 include file <sys/feature_tests.h> refers to X/Open document

    System Interfaces and Headers, Issue 5

  saying we should define _XOPEN_SOURCE=500 to get POSIX.1c prototypes,
  but apparently other systems (namely FreeBSD) don't agree.

  On a newer Solaris 10, the above file recognizes also _XOPEN_SOURCE=600.
  Furthermore, it tests that if a program requires older standard
  (_XOPEN_SOURCE<600 or _POSIX_C_SOURCE<200112L) it cannot be
  run on a new compiler (that defines _STDC_C99) and issues an #error.
  It's also an #error if a program requires new standard (_XOPEN_SOURCE=600
  or _POSIX_C_SOURCE=200112L) and a compiler does not define _STDC_C99.

  To add more to this mess, Sun Studio C compiler defines _STDC_C99 while
  C++ compiler does not!

  So, in a desperate attempt to get correct prototypes for both
  C and C++ code, we define either _XOPEN_SOURCE=600 or _XOPEN_SOURCE=500
  depending on the compiler's announced C standard support.

  Cleaner solutions are welcome.
*/
#ifdef __sun
#if __STDC_VERSION__ - 0 >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#endif

#if defined(THREAD) && !defined(__WIN__)
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS /* We want posix threads */
#endif

#if !defined(SCO)
#define _REENTRANT	1	/* Some thread libraries require this */
#endif
#if !defined(_THREAD_SAFE) && !defined(_AIX)
#define _THREAD_SAFE            /* Required for OSF1 */
#endif
#if defined(HPUX10) || defined(HPUX11)
C_MODE_START			/* HPUX needs this, signal.h bug */
#include <pthread.h>
C_MODE_END
#else
#include <pthread.h>		/* AIX must have this included first */
#endif
#if !defined(SCO) && !defined(_REENTRANT)
#define _REENTRANT	1	/* Threads requires reentrant code */
#endif
#endif /* THREAD */

/* Go around some bugs in different OS and compilers */
#ifdef _AIX			/* By soren@t.dk */
#define _H_STRINGS
#define _SYS_STREAM_H
/* #define _AIX32_CURSES */	/* XXX: this breaks AIX 4.3.3 (others?). */
#define ulonglong2double(A) my_ulonglong2double(A)
#define my_off_t2double(A)  my_ulonglong2double(A)
C_MODE_START
double my_ulonglong2double(unsigned long long A);
C_MODE_END
#endif /* _AIX */

#ifdef HAVE_BROKEN_SNPRINTF	/* HPUX 10.20 don't have this defined */
#undef HAVE_SNPRINTF
#endif
#ifdef HAVE_BROKEN_PREAD
/*
  pread()/pwrite() are not 64 bit safe on HP-UX 11.0 without
  installing the kernel patch PHKL_20349 or greater
*/
#undef HAVE_PREAD
#undef HAVE_PWRITE
#endif
#if defined(HAVE_BROKEN_INLINE) && !defined(__cplusplus)
#undef inline
#define inline
#endif

#ifdef UNDEF_HAVE_GETHOSTBYNAME_R		/* For OSF4.x */
#undef HAVE_GETHOSTBYNAME_R
#endif
#ifdef UNDEF_HAVE_INITGROUPS			/* For AIX 4.3 */
#undef HAVE_INITGROUPS
#endif

/* gcc/egcs issues */

#if defined(__GNUC) && defined(__EXCEPTIONS)
#error "Please add -fno-exceptions to CXXFLAGS and reconfigure/recompile"
#endif


/* Fix a bug in gcc 2.8.0 on IRIX 6.2 */
#if SIZEOF_LONG == 4 && defined(__LONG_MAX__) && (__GNUC__ == 2 && __GNUC_MINOR__ == 8)
#undef __LONG_MAX__             /* Is a longlong value in gcc 2.8.0 ??? */
#define __LONG_MAX__ 2147483647
#endif

/* egcs 1.1.2 has a problem with memcpy on Alpha */
#if defined(__GNUC__) && defined(__alpha__) && ! (__GNUC__ > 2 || (__GNUC__ == 2 &&  __GNUC_MINOR__ >= 95))
#define BAD_MEMCPY
#endif

#if defined(_lint) && !defined(lint)
#define lint
#endif
#if SIZEOF_LONG_LONG > 4 && !defined(_LONG_LONG)
#define _LONG_LONG 1		/* For AIX string library */
#endif

#ifndef stdin
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_FENV_H
#include <fenv.h> /* For fesetround() */
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>				/* Avoid warnings on SCO */
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif /* TIME_WITH_SYS_TIME */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(__cplusplus) && defined(NO_CPLUSPLUS_ALLOCA)
#undef HAVE_ALLOCA
#undef HAVE_ALLOCA_H
#endif
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <errno.h>				/* Recommended by debian */
/* We need the following to go around a problem with openssl on solaris */
#if defined(HAVE_CRYPT_H)
#include <crypt.h>
#endif

/*
  A lot of our programs uses asserts, so better to always include it
  This also fixes a problem when people uses DBUG_ASSERT without including
  assert.h
*/
#include <assert.h>

/* an assert that works at compile-time. only for constant expression */
#ifndef __GNUC__
#define compile_time_assert(X)  do { } while(0)
#else
#define compile_time_assert(X)                                  \
  do                                                            \
  {                                                             \
    char compile_time_assert[(X) ? 1 : -1]                      \
                             __attribute__ ((unused));          \
  } while(0)
#endif

/* Go around some bugs in different OS and compilers */
#if defined (HPUX11) && defined(_LARGEFILE_SOURCE)
#define _LARGEFILE64_SOURCE
#endif
#if defined(_HPUX_SOURCE) && defined(HAVE_SYS_STREAM_H)
#include <sys/stream.h>		/* HPUX 10.20 defines ulong here. UGLY !!! */
#define HAVE_ULONG
#endif
#if defined(HPUX10) && defined(_LARGEFILE64_SOURCE) && defined(THREAD)
/* Fix bug in setrlimit */
#undef setrlimit
#define setrlimit cma_setrlimit64
#endif
/* Declare madvise where it is not declared for C++, like Solaris */
#if HAVE_MADVISE && !HAVE_DECL_MADVISE && defined(__cplusplus)
extern "C" int madvise(void *addr, size_t len, int behav);
#endif

#ifdef __QNXNTO__
/* This has to be after include limits.h */
#define HAVE_ERRNO_AS_DEFINE
#define HAVE_FCNTL_LOCK
#undef  HAVE_FINITE
#undef  LONGLONG_MIN            /* These get wrongly defined in QNX 6.2 */
#undef  LONGLONG_MAX            /* standard system library 'limits.h' */
#ifdef __cplusplus
#ifndef HAVE_RINT
#define HAVE_RINT
#endif                          /* rint() and isnan() functions are not */
#define rint(a) std::rint(a)    /* visible in C++ scope due to an error */
#define isnan(a) std::isnan(a)  /* in the usr/include/math.h on QNX     */
#endif
#endif

/* We can not live without the following defines */

#define USE_MYFUNC 1		/* Must use syscall indirection */
#define MASTER 1		/* Compile without unireg */
#define ENGLISH 1		/* Messages in English */
#define POSIX_MISTAKE 1		/* regexp: Fix stupid spec error */
#define USE_REGEX 1		/* We want the use the regex library */
/* Do not define for ultra sparcs */
#define USE_BMOVE512 1		/* Use this unless system bmove is faster */

#define QUOTE_ARG(x)		#x	/* Quote argument (before cpp) */
#define STRINGIFY_ARG(x) QUOTE_ARG(x)	/* Quote argument, after cpp */

/* Paranoid settings. Define I_AM_PARANOID if you are paranoid */
#ifdef I_AM_PARANOID
#define DONT_ALLOW_USER_CHANGE 1
#define DONT_USE_MYSQL_PWD 1
#endif

/* Does the system remember a signal handler after a signal ? */
#ifndef HAVE_BSD_SIGNALS
#define DONT_REMEMBER_SIGNAL
#endif

/* Define void to stop lint from generating "null effekt" comments */
#ifndef DONT_DEFINE_VOID
#ifdef _lint
int	__void__;
#define VOID(X)		(__void__ = (int) (X))
#else
#undef VOID
#define VOID(X)		(X)
#endif
#endif /* DONT_DEFINE_VOID */

#if defined(_lint) || defined(FORCE_INIT_OF_VARS)
#define LINT_INIT(var)	var=0			/* No uninitialize-warning */
#else
#define LINT_INIT(var)
#endif

/* 
   Suppress uninitialized variable warning without generating code.

   The _cplusplus is a temporary workaround for C++ code pending a fix
   for a g++ bug (http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34772). 
*/
#if defined(_lint) || defined(FORCE_INIT_OF_VARS) || defined(__cplusplus) || \
  !defined(__GNUC__)
#define UNINIT_VAR(x) x= 0
#else
#define UNINIT_VAR(x) x= x
#endif

/* Define some useful general macros */
#if !defined(max)
#define max(a, b)	((a) > (b) ? (a) : (b))
#define min(a, b)	((a) < (b) ? (a) : (b))
#endif

#if !defined(HAVE_UINT)
#undef HAVE_UINT
#define HAVE_UINT
typedef unsigned int uint;
typedef unsigned short ushort;
#endif

#define CMP_NUM(a,b)    (((a) < (b)) ? -1 : ((a) == (b)) ? 0 : 1)
#define sgn(a)		(((a) < 0) ? -1 : ((a) > 0) ? 1 : 0)
#define swap_variables(t, a, b) { t dummy; dummy= a; a= b; b= dummy; }
#define test(a)		((a) ? 1 : 0)
#define set_if_bigger(a,b)  do { if ((a) < (b)) (a)=(b); } while(0)
#define set_if_smaller(a,b) do { if ((a) > (b)) (a)=(b); } while(0)
#define test_all_bits(a,b) (((a) & (b)) == (b))
#define set_bits(type, bit_count) (sizeof(type)*8 <= (bit_count) ? ~(type) 0 : ((((type) 1) << (bit_count)) - (type) 1))
#define array_elements(A) ((uint) (sizeof(A)/sizeof(A[0])))

/* Define some general constants */
#ifndef TRUE
#define TRUE		(1)	/* Logical true */
#define FALSE		(0)	/* Logical false */
#endif

#if defined(__GNUC__)
#define function_volatile	volatile
#define my_reinterpret_cast(A) reinterpret_cast<A>
#define my_const_cast(A) const_cast<A>
# ifndef GCC_VERSION
#  define GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
# endif
#elif !defined(my_reinterpret_cast)
#define my_reinterpret_cast(A) (A)
#define my_const_cast(A) (A)
#endif

#include <my_attribute.h>

/*
  Wen using the embedded library, users might run into link problems,
  duplicate declaration of __cxa_pure_virtual, solved by declaring it a
  weak symbol.
*/
#if defined(USE_MYSYS_NEW) && ! defined(DONT_DECLARE_CXA_PURE_VIRTUAL)
C_MODE_START
int __cxa_pure_virtual () __attribute__ ((weak));
C_MODE_END
#endif

/* From old s-system.h */

/*
  Support macros for non ansi & other old compilers. Since such
  things are no longer supported we do nothing. We keep then since
  some of our code may still be needed to upgrade old customers.
*/
#define _VARARGS(X) X
#define _STATIC_VARARGS(X) X

/* The DBUG_ON flag always takes precedence over default DBUG_OFF */
#if defined(DBUG_ON) && defined(DBUG_OFF)
#undef DBUG_OFF
#endif

/* We might be forced to turn debug off, if not turned off already */
#if (defined(FORCE_DBUG_OFF) || defined(_lint)) && !defined(DBUG_OFF)
#  define DBUG_OFF
#  ifdef DBUG_ON
#    undef DBUG_ON
#  endif
#endif

#include <my_dbug.h>

#define MIN_ARRAY_SIZE	0	/* Zero or One. Gcc allows zero*/
#define ASCII_BITS_USED 8	/* Bit char used */
#define NEAR_F			/* No near function handling */

/* Some types that is different between systems */

typedef int	File;		/* File descriptor */
#ifndef Socket_defined
typedef int	my_socket;	/* File descriptor for sockets */
#define INVALID_SOCKET -1
#endif
/* Type for fuctions that handles signals */
#define sig_handler RETSIGTYPE
C_MODE_START
typedef void	(*sig_return)();/* Returns type from signal */
C_MODE_END
#if defined(__GNUC__) && !defined(_lint)
typedef char	pchar;		/* Mixed prototypes can take char */
typedef char	puchar;		/* Mixed prototypes can take char */
typedef char	pbool;		/* Mixed prototypes can take char */
typedef short	pshort;		/* Mixed prototypes can take short int */
typedef float	pfloat;		/* Mixed prototypes can take float */
#else
typedef int	pchar;		/* Mixed prototypes can't take char */
typedef uint	puchar;		/* Mixed prototypes can't take char */
typedef int	pbool;		/* Mixed prototypes can't take char */
typedef int	pshort;		/* Mixed prototypes can't take short int */
typedef double	pfloat;		/* Mixed prototypes can't take float */
#endif
C_MODE_START
typedef int	(*qsort_cmp)(const void *,const void *);
typedef int	(*qsort_cmp2)(void*, const void *,const void *);
C_MODE_END
#define qsort_t RETQSORTTYPE	/* Broken GCC cant handle typedef !!!! */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
typedef SOCKET_SIZE_TYPE size_socket;

#ifndef SOCKOPT_OPTLEN_TYPE
#define SOCKOPT_OPTLEN_TYPE size_socket
#endif

/* file create flags */

#ifndef O_SHARE			/* Probably not windows */
#define O_SHARE		0	/* Flag to my_open for shared files */
#ifndef O_BINARY
#define O_BINARY	0	/* Flag to my_open for binary files */
#endif
#ifndef FILE_BINARY
#define FILE_BINARY	O_BINARY /* Flag to my_fopen for binary streams */
#endif
#ifdef HAVE_FCNTL
#define HAVE_FCNTL_LOCK
#define F_TO_EOF	0L	/* Param to lockf() to lock rest of file */
#endif
#endif /* O_SHARE */

#ifndef O_TEMPORARY
#define O_TEMPORARY	0
#endif
#ifndef O_SHORT_LIVED
#define O_SHORT_LIVED	0
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW      0
#endif

/* additional file share flags for win32 */
#ifdef __WIN__
#define _SH_DENYRWD     0x110    /* deny read/write mode & delete */
#define _SH_DENYWRD     0x120    /* deny write mode & delete      */
#define _SH_DENYRDD     0x130    /* deny read mode & delete       */
#define _SH_DENYDEL     0x140    /* deny delete only              */
#endif /* __WIN__ */


/* #define USE_RECORD_LOCK	*/

	/* Unsigned types supported by the compiler */
#define UNSINT8			/* unsigned int8 (char) */
#define UNSINT16		/* unsigned int16 */
#define UNSINT32		/* unsigned int32 */

	/* General constants */
#define FN_LEN		256	/* Max file name len */
#define FN_HEADLEN	253	/* Max length of filepart of file name */
#define FN_EXTLEN	20	/* Max length of extension (part of FN_LEN) */
#define FN_REFLEN	512	/* Max length of full path-name */
#define FN_EXTCHAR	'.'
#define FN_HOMELIB	'~'	/* ~/ is used as abbrev for home dir */
#define FN_CURLIB	'.'	/* ./ is used as abbrev for current dir */
#define FN_PARENTDIR	".."	/* Parent directory; Must be a string */

#ifndef FN_LIBCHAR
#define FN_LIBCHAR	'/'
#define FN_ROOTDIR	"/"
#endif
#define MY_NFILE	64	/* This is only used to save filenames */
#ifndef OS_FILE_LIMIT
#define OS_FILE_LIMIT	65535
#endif

/* #define EXT_IN_LIBNAME     */
/* #define FN_NO_CASE_SENCE   */
/* #define FN_UPPER_CASE TRUE */

/*
  Io buffer size; Must be a power of 2 and a multiple of 512. May be
  smaller what the disk page size. This influences the speed of the
  isam btree library. eg to big to slow.
*/
#define IO_SIZE			4096
/*
  How much overhead does malloc have. The code often allocates
  something like 1024-MALLOC_OVERHEAD bytes
*/
#ifdef SAFEMALLOC
#define MALLOC_OVERHEAD (8+24+4)
#else
#define MALLOC_OVERHEAD 8
#endif
	/* get memory in huncs */
#define ONCE_ALLOC_INIT		(uint) (4096-MALLOC_OVERHEAD)
	/* Typical record cash */
#define RECORD_CACHE_SIZE	(uint) (64*1024-MALLOC_OVERHEAD)
	/* Typical key cash */
#define KEY_CACHE_SIZE		(uint) (8*1024*1024-MALLOC_OVERHEAD)
	/* Default size of a key cache block  */
#define KEY_CACHE_BLOCK_SIZE	(uint) 1024


	/* Some things that this system doesn't have */

#define NO_HASH			/* Not needed anymore */
#ifdef __WIN__
#define NO_DIR_LIBRARY		/* Not standar dir-library */
#define USE_MY_STAT_STRUCT	/* For my_lib */
#endif

/* Some defines of functions for portability */

#undef remove		/* Crashes MySQL on SCO 5.0.0 */
#ifndef __WIN__
#define closesocket(A)	close(A)
#ifndef ulonglong2double
#define ulonglong2double(A) ((double) (ulonglong) (A))
#define my_off_t2double(A)  ((double) (my_off_t) (A))
#endif
#ifndef double2ulonglong
#define double2ulonglong(A) ((ulonglong) (double) (A))
#endif
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
#define ulong_to_double(X) ((double) (ulong) (X))
#define SET_STACK_SIZE(X)	/* Not needed on real machines */

#ifndef STACK_DIRECTION
#error "please add -DSTACK_DIRECTION=1 or -1 to your CPPFLAGS"
#endif

#if !defined(HAVE_STRTOK_R)
#define strtok_r(A,B,C) strtok((A),(B))
#endif

/* This is from the old m-machine.h file */

#if SIZEOF_LONG_LONG > 4
#define HAVE_LONG_LONG 1
#endif

/*
  Some pre-ANSI-C99 systems like AIX 5.1 and Linux/GCC 2.95 define
  ULONGLONG_MAX, LONGLONG_MIN, LONGLONG_MAX; we use them if they're defined.
  Also on Windows we define these constants by hand in config-win.h.
*/

#if defined(HAVE_LONG_LONG) && !defined(LONGLONG_MIN)
#define LONGLONG_MIN	((long long) 0x8000000000000000LL)
#define LONGLONG_MAX	((long long) 0x7FFFFFFFFFFFFFFFLL)
#endif

#if defined(HAVE_LONG_LONG) && !defined(ULONGLONG_MAX)
/* First check for ANSI C99 definition: */
#ifdef ULLONG_MAX
#define ULONGLONG_MAX  ULLONG_MAX
#else
#define ULONGLONG_MAX ((unsigned long long)(~0ULL))
#endif
#endif /* defined (HAVE_LONG_LONG) && !defined(ULONGLONG_MAX)*/

#define INT_MIN32       (~0x7FFFFFFFL)
#define INT_MAX32       0x7FFFFFFFL
#define UINT_MAX32      0xFFFFFFFFL
#define INT_MIN24       (~0x007FFFFF)
#define INT_MAX24       0x007FFFFF
#define UINT_MAX24      0x00FFFFFF
#define INT_MIN16       (~0x7FFF)
#define INT_MAX16       0x7FFF
#define UINT_MAX16      0xFFFF
#define INT_MIN8        (~0x7F)
#define INT_MAX8        0x7F
#define UINT_MAX8       0xFF

/* From limits.h instead */
#ifndef DBL_MIN
#define DBL_MIN		4.94065645841246544e-324
#define FLT_MIN		((float)1.40129846432481707e-45)
#endif
#ifndef DBL_MAX
#define DBL_MAX		1.79769313486231470e+308
#define FLT_MAX		((float)3.40282346638528860e+38)
#endif
#ifndef SIZE_T_MAX
#define SIZE_T_MAX      (~((size_t) 0))
#endif

#ifndef isfinite
#ifdef HAVE_FINITE
#define isfinite(x) finite(x)
#else
#define finite(x) (1.0 / fabs(x) > 0.0)
#endif /* HAVE_FINITE */
#endif /* isfinite */

#ifndef HAVE_ISNAN
#define isnan(x) ((x) != (x))
#endif

#ifdef HAVE_ISINF
/* Check if C compiler is affected by GCC bug #39228 */
#if !defined(__cplusplus) && defined(HAVE_BROKEN_ISINF)
/* Force store/reload of the argument to/from a 64-bit double */
static inline double my_isinf(double x)
{
  volatile double t= x;
  return isinf(t);
}
#else
/* System-provided isinf() is available and safe to use */
#define my_isinf(X) isinf(X)
#endif
#else /* !HAVE_ISINF */
#define my_isinf(X) (!finite(X) && !isnan(X))
#endif

/* Define missing math constants. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.7182818284590452354
#endif
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

/*
  Max size that must be added to a so that we know Size to make
  adressable obj.
*/
#if SIZEOF_CHARP == 4
typedef long		my_ptrdiff_t;
#else
typedef long long	my_ptrdiff_t;
#endif

#define MY_ALIGN(A,L)	(((A) + (L) - 1) & ~((L) - 1))
#define ALIGN_SIZE(A)	MY_ALIGN((A),sizeof(double))
/* Size to make adressable obj. */
#define ALIGN_PTR(A, t) ((t*) MY_ALIGN((A),sizeof(t)))
			 /* Offset of field f in structure t */
#define OFFSET(t, f)	((size_t)(char *)&((t *)0)->f)
#define ADD_TO_PTR(ptr,size,type) (type) ((uchar*) (ptr)+size)
#define PTR_BYTE_DIFF(A,B) (my_ptrdiff_t) ((uchar*) (A) - (uchar*) (B))

#define MY_DIV_UP(A, B) (((A) + (B) - 1) / (B))
#define MY_ALIGNED_BYTE_ARRAY(N, S, T) T N[MY_DIV_UP(S, sizeof(T))]

/*
  Custom version of standard offsetof() macro which can be used to get
  offsets of members in class for non-POD types (according to the current
  version of C++ standard offsetof() macro can't be used in such cases and
  attempt to do so causes warnings to be emitted, OTOH in many cases it is
  still OK to assume that all instances of the class has the same offsets
  for the same members).

  This is temporary solution which should be removed once File_parser class
  and related routines are refactored.
*/

#define my_offsetof(TYPE, MEMBER) \
        ((size_t)((char *)&(((TYPE *)0x10)->MEMBER) - (char*)0x10))

#define NullS		(char *) 0
/* Nowdays we do not support MessyDos */
#ifndef NEAR
#define NEAR				/* Who needs segments ? */
#define FAR				/* On a good machine */
#ifndef HUGE_PTR
#define HUGE_PTR
#endif
#endif
#if defined(__IBMC__) || defined(__IBMCPP__)
/* This was  _System _Export but caused a lot of warnings on _AIX43 */
#define STDCALL
#elif !defined( STDCALL)
#define STDCALL
#endif

/* Typdefs for easyier portability */

#ifndef HAVE_UCHAR
typedef unsigned char	uchar;	/* Short for unsigned char */
#endif

#ifndef HAVE_INT8
typedef signed char int8;       /* Signed integer >= 8  bits */
#endif
#ifndef HAVE_UINT8
typedef unsigned char uint8;    /* Unsigned integer >= 8  bits */
#endif
#ifndef HAVE_INT16
typedef short int16;
#endif
#ifndef HAVE_UINT16
typedef unsigned short uint16;
#endif
#if SIZEOF_INT == 4
#ifndef HAVE_INT32
typedef int int32;
#endif
#ifndef HAVE_UINT32
typedef unsigned int uint32;
#endif
#elif SIZEOF_LONG == 4
#ifndef HAVE_INT32
typedef long int32;
#endif
#ifndef HAVE_UINT32
typedef unsigned long uint32;
#endif
#else
#error Neither int or long is of 4 bytes width
#endif

#if !defined(HAVE_ULONG) && !defined(__USE_MISC)
typedef unsigned long	ulong;		  /* Short for unsigned long */
#endif
#ifndef longlong_defined
/* 
  Using [unsigned] long long is preferable as [u]longlong because we use 
  [unsigned] long long unconditionally in many places, 
  for example in constants with [U]LL suffix.
*/
#if defined(HAVE_LONG_LONG) && SIZEOF_LONG_LONG == 8
typedef unsigned long long int ulonglong; /* ulong or unsigned long long */
typedef long long int	longlong;
#else
typedef unsigned long	ulonglong;	  /* ulong or unsigned long long */
typedef long		longlong;
#endif
#endif
#ifndef HAVE_INT64
typedef longlong int64;
#endif
#ifndef HAVE_UINT64
typedef ulonglong uint64;
#endif

#if defined(NO_CLIENT_LONG_LONG)
typedef unsigned long my_ulonglong;
#elif defined (__WIN__)
typedef unsigned __int64 my_ulonglong;
#else
typedef unsigned long long my_ulonglong;
#endif

#if SIZEOF_CHARP == SIZEOF_INT
typedef int intptr;
#elif SIZEOF_CHARP == SIZEOF_LONG
typedef long intptr;
#elif SIZEOF_CHARP == SIZEOF_LONG_LONG
typedef long long intptr;
#else
#error sizeof(void *) is neither sizeof(int) nor sizeof(long) nor sizeof(long long)
#endif

#define MY_ERRPTR ((void*)(intptr)1)

#ifdef USE_RAID
/*
  The following is done with a if to not get problems with pre-processors
  with late define evaluation
*/
#if SIZEOF_OFF_T == 4
#define SYSTEM_SIZEOF_OFF_T 4
#else
#define SYSTEM_SIZEOF_OFF_T 8
#endif
#undef  SIZEOF_OFF_T
#define SIZEOF_OFF_T	    8
#else
#define SYSTEM_SIZEOF_OFF_T SIZEOF_OFF_T
#endif /* USE_RAID */

#if SIZEOF_OFF_T > 4
typedef ulonglong my_off_t;
#else
typedef unsigned long my_off_t;
#endif
#define MY_FILEPOS_ERROR	(~(my_off_t) 0)
#if !defined(__WIN__)
typedef off_t os_off_t;
#endif

#if defined(__WIN__)
#define socket_errno	WSAGetLastError()
#define SOCKET_EINTR	WSAEINTR
#define SOCKET_EAGAIN	WSAEINPROGRESS
#define SOCKET_ETIMEDOUT WSAETIMEDOUT
#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#define SOCKET_EADDRINUSE WSAEADDRINUSE
#define SOCKET_ENFILE	ENFILE
#define SOCKET_EMFILE	EMFILE
#else /* Unix */
#define socket_errno	errno
#define closesocket(A)	close(A)
#define SOCKET_EINTR	EINTR
#define SOCKET_EAGAIN	EAGAIN
#define SOCKET_ETIMEDOUT SOCKET_EINTR
#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#define SOCKET_EADDRINUSE EADDRINUSE
#define SOCKET_ENFILE	ENFILE
#define SOCKET_EMFILE	EMFILE
#endif

typedef uint8		int7;	/* Most effective integer 0 <= x <= 127 */
typedef short		int15;	/* Most effective integer 0 <= x <= 32767 */
typedef int		myf;	/* Type of MyFlags in my_funcs */
typedef char		my_bool; /* Small bool */
#if !defined(bool) && (!defined(HAVE_BOOL) || !defined(__cplusplus))
typedef char		bool;	/* Ordinary boolean values 0 1 */
#endif
	/* Macros for converting *constants* to the right type */
#define INT8(v)		(int8) (v)
#define INT16(v)	(int16) (v)
#define INT32(v)	(int32) (v)
#define MYF(v)		(myf) (v)

#ifndef LL
#ifdef HAVE_LONG_LONG
#define LL(A) A ## LL
#else
#define LL(A) A ## L
#endif
#endif

#ifndef ULL
#ifdef HAVE_LONG_LONG
#define ULL(A) A ## ULL
#else
#define ULL(A) A ## UL
#endif
#endif

/*
  Defines to make it possible to prioritize register assignments. No
  longer that important with modern compilers.
*/
#ifndef USING_X
#define reg1 register
#define reg2 register
#define reg3 register
#define reg4 register
#define reg5 register
#define reg6 register
#define reg7 register
#define reg8 register
#define reg9 register
#define reg10 register
#define reg11 register
#define reg12 register
#define reg13 register
#define reg14 register
#define reg15 register
#define reg16 register
#endif

/*
  Sometimes we want to make sure that the variable is not put into
  a register in debugging mode so we can see its value in the core
*/

#ifndef DBUG_OFF
#define dbug_volatile volatile
#else
#define dbug_volatile
#endif

/* Some helper macros */
#define YESNO(X) ((X) ? "yes" : "no")

/* Defines for time function */
#define SCALE_SEC	100
#define SCALE_USEC	10000
#define MY_HOW_OFTEN_TO_ALARM	2	/* How often we want info on screen */
#define MY_HOW_OFTEN_TO_WRITE	1000	/* How often we want info on screen */



/*
  Define-funktions for reading and storing in machine independent format
  (low byte first)
*/

/* Optimized store functions for Intel x86 */
#if defined(__i386__) || defined(_WIN32)
#define sint2korr(A)	(*((int16 *) (A)))
#define sint3korr(A)	((int32) ((((uchar) (A)[2]) & 128) ? \
				  (((uint32) 255L << 24) | \
				   (((uint32) (uchar) (A)[2]) << 16) |\
				   (((uint32) (uchar) (A)[1]) << 8) | \
				   ((uint32) (uchar) (A)[0])) : \
				  (((uint32) (uchar) (A)[2]) << 16) |\
				  (((uint32) (uchar) (A)[1]) << 8) | \
				  ((uint32) (uchar) (A)[0])))
#define sint4korr(A)	(*((long *) (A)))
#define uint2korr(A)	(*((uint16 *) (A)))
#if defined(HAVE_purify) && !defined(_WIN32)
#define uint3korr(A)	(uint32) (((uint32) ((uchar) (A)[0])) +\
				  (((uint32) ((uchar) (A)[1])) << 8) +\
				  (((uint32) ((uchar) (A)[2])) << 16))
#else
/*
   ATTENTION !
   
    Please, note, uint3korr reads 4 bytes (not 3) !
    It means, that you have to provide enough allocated space !
*/
#define uint3korr(A)	(long) (*((unsigned int *) (A)) & 0xFFFFFF)
#endif /* HAVE_purify && !_WIN32 */
#define uint4korr(A)	(*((uint32 *) (A)))
#define uint5korr(A)	((ulonglong)(((uint32) ((uchar) (A)[0])) +\
				    (((uint32) ((uchar) (A)[1])) << 8) +\
				    (((uint32) ((uchar) (A)[2])) << 16) +\
				    (((uint32) ((uchar) (A)[3])) << 24)) +\
				    (((ulonglong) ((uchar) (A)[4])) << 32))
#define uint6korr(A)	((ulonglong)(((uint32)    ((uchar) (A)[0]))          + \
                                     (((uint32)    ((uchar) (A)[1])) << 8)   + \
                                     (((uint32)    ((uchar) (A)[2])) << 16)  + \
                                     (((uint32)    ((uchar) (A)[3])) << 24)) + \
                         (((ulonglong) ((uchar) (A)[4])) << 32) +       \
                         (((ulonglong) ((uchar) (A)[5])) << 40))
#define uint8korr(A)	(*((ulonglong *) (A)))
#define sint8korr(A)	(*((longlong *) (A)))
#define int2store(T,A)	*((uint16*) (T))= (uint16) (A)
#define int3store(T,A)  do { *(T)=  (uchar) ((A));\
                            *(T+1)=(uchar) (((uint) (A) >> 8));\
                            *(T+2)=(uchar) (((A) >> 16)); } while (0)
#define int4store(T,A)	*((long *) (T))= (long) (A)
#define int5store(T,A)  do { *(T)= (uchar)((A));\
                             *((T)+1)=(uchar) (((A) >> 8));\
                             *((T)+2)=(uchar) (((A) >> 16));\
                             *((T)+3)=(uchar) (((A) >> 24)); \
                             *((T)+4)=(uchar) (((A) >> 32)); } while(0)
#define int6store(T,A)  do { *(T)=    (uchar)((A));          \
                             *((T)+1)=(uchar) (((A) >> 8));  \
                             *((T)+2)=(uchar) (((A) >> 16)); \
                             *((T)+3)=(uchar) (((A) >> 24)); \
                             *((T)+4)=(uchar) (((A) >> 32)); \
                             *((T)+5)=(uchar) (((A) >> 40)); } while(0)
#define int8store(T,A)	*((ulonglong *) (T))= (ulonglong) (A)

typedef union {
  double v;
  long m[2];
} doubleget_union;
#define doubleget(V,M)	\
do { doubleget_union _tmp; \
     _tmp.m[0] = *((long*)(M)); \
     _tmp.m[1] = *(((long*) (M))+1); \
     (V) = _tmp.v; } while(0)
#define doublestore(T,V) do { *((long *) T) = ((doubleget_union *)&V)->m[0]; \
			     *(((long *) T)+1) = ((doubleget_union *)&V)->m[1]; \
                         } while (0)
#define float4get(V,M)   do { *((float *) &(V)) = *((float*) (M)); } while(0)
#define float8get(V,M)   doubleget((V),(M))
#define float4store(V,M) memcpy((uchar*) V,(uchar*) (&M),sizeof(float))
#define floatstore(T,V)  memcpy((uchar*)(T), (uchar*)(&V),sizeof(float))
#define floatget(V,M)    memcpy((uchar*) &V,(uchar*) (M),sizeof(float))
#define float8store(V,M) doublestore((V),(M))
#else

/*
  We're here if it's not a IA-32 architecture (Win32 and UNIX IA-32 defines
  were done before)
*/
#define sint2korr(A)	(int16) (((int16) ((uchar) (A)[0])) +\
				 ((int16) ((int16) (A)[1]) << 8))
#define sint3korr(A)	((int32) ((((uchar) (A)[2]) & 128) ? \
				  (((uint32) 255L << 24) | \
				   (((uint32) (uchar) (A)[2]) << 16) |\
				   (((uint32) (uchar) (A)[1]) << 8) | \
				   ((uint32) (uchar) (A)[0])) : \
				  (((uint32) (uchar) (A)[2]) << 16) |\
				  (((uint32) (uchar) (A)[1]) << 8) | \
				  ((uint32) (uchar) (A)[0])))
#define sint4korr(A)	(int32) (((int32) ((uchar) (A)[0])) +\
				(((int32) ((uchar) (A)[1]) << 8)) +\
				(((int32) ((uchar) (A)[2]) << 16)) +\
				(((int32) ((int16) (A)[3]) << 24)))
#define sint8korr(A)	(longlong) uint8korr(A)
#define uint2korr(A)	(uint16) (((uint16) ((uchar) (A)[0])) +\
				  ((uint16) ((uchar) (A)[1]) << 8))
#define uint3korr(A)	(uint32) (((uint32) ((uchar) (A)[0])) +\
				  (((uint32) ((uchar) (A)[1])) << 8) +\
				  (((uint32) ((uchar) (A)[2])) << 16))
#define uint4korr(A)	(uint32) (((uint32) ((uchar) (A)[0])) +\
				  (((uint32) ((uchar) (A)[1])) << 8) +\
				  (((uint32) ((uchar) (A)[2])) << 16) +\
				  (((uint32) ((uchar) (A)[3])) << 24))
#define uint5korr(A)	((ulonglong)(((uint32) ((uchar) (A)[0])) +\
				    (((uint32) ((uchar) (A)[1])) << 8) +\
				    (((uint32) ((uchar) (A)[2])) << 16) +\
				    (((uint32) ((uchar) (A)[3])) << 24)) +\
				    (((ulonglong) ((uchar) (A)[4])) << 32))
#define uint6korr(A)	((ulonglong)(((uint32)    ((uchar) (A)[0]))          + \
                                     (((uint32)    ((uchar) (A)[1])) << 8)   + \
                                     (((uint32)    ((uchar) (A)[2])) << 16)  + \
                                     (((uint32)    ((uchar) (A)[3])) << 24)) + \
                         (((ulonglong) ((uchar) (A)[4])) << 32) +       \
                         (((ulonglong) ((uchar) (A)[5])) << 40))
#define uint8korr(A)	((ulonglong)(((uint32) ((uchar) (A)[0])) +\
				    (((uint32) ((uchar) (A)[1])) << 8) +\
				    (((uint32) ((uchar) (A)[2])) << 16) +\
				    (((uint32) ((uchar) (A)[3])) << 24)) +\
			(((ulonglong) (((uint32) ((uchar) (A)[4])) +\
				    (((uint32) ((uchar) (A)[5])) << 8) +\
				    (((uint32) ((uchar) (A)[6])) << 16) +\
				    (((uint32) ((uchar) (A)[7])) << 24))) <<\
				    32))
#define int2store(T,A)       do { uint def_temp= (uint) (A) ;\
                                  *((uchar*) (T))=  (uchar)(def_temp); \
                                   *((uchar*) (T)+1)=(uchar)((def_temp >> 8)); \
                             } while(0)
#define int3store(T,A)       do { /*lint -save -e734 */\
                                  *((uchar*)(T))=(uchar) ((A));\
                                  *((uchar*) (T)+1)=(uchar) (((A) >> 8));\
                                  *((uchar*)(T)+2)=(uchar) (((A) >> 16)); \
                                  /*lint -restore */} while(0)
#define int4store(T,A)       do { *((char *)(T))=(char) ((A));\
                                  *(((char *)(T))+1)=(char) (((A) >> 8));\
                                  *(((char *)(T))+2)=(char) (((A) >> 16));\
                                  *(((char *)(T))+3)=(char) (((A) >> 24)); } while(0)
#define int5store(T,A)       do { *((char *)(T))=     (char)((A));  \
                                  *(((char *)(T))+1)= (char)(((A) >> 8)); \
                                  *(((char *)(T))+2)= (char)(((A) >> 16)); \
                                  *(((char *)(T))+3)= (char)(((A) >> 24)); \
                                  *(((char *)(T))+4)= (char)(((A) >> 32)); \
		                } while(0)
#define int6store(T,A)       do { *((char *)(T))=     (char)((A)); \
                                  *(((char *)(T))+1)= (char)(((A) >> 8)); \
                                  *(((char *)(T))+2)= (char)(((A) >> 16)); \
                                  *(((char *)(T))+3)= (char)(((A) >> 24)); \
                                  *(((char *)(T))+4)= (char)(((A) >> 32)); \
                                  *(((char *)(T))+5)= (char)(((A) >> 40)); \
                                } while(0)
#define int8store(T,A)       do { uint def_temp= (uint) (A), def_temp2= (uint) ((A) >> 32); \
                                  int4store((T),def_temp); \
                                  int4store((T+4),def_temp2); } while(0)
#ifdef WORDS_BIGENDIAN
#define float4store(T,A) do { *(T)= ((uchar *) &A)[3];\
                              *((T)+1)=(char) ((uchar *) &A)[2];\
                              *((T)+2)=(char) ((uchar *) &A)[1];\
                              *((T)+3)=(char) ((uchar *) &A)[0]; } while(0)

#define float4get(V,M)   do { float def_temp;\
                              ((uchar*) &def_temp)[0]=(M)[3];\
                              ((uchar*) &def_temp)[1]=(M)[2];\
                              ((uchar*) &def_temp)[2]=(M)[1];\
                              ((uchar*) &def_temp)[3]=(M)[0];\
                              (V)=def_temp; } while(0)
#define float8store(T,V) do { *(T)= ((uchar *) &V)[7];\
                              *((T)+1)=(char) ((uchar *) &V)[6];\
                              *((T)+2)=(char) ((uchar *) &V)[5];\
                              *((T)+3)=(char) ((uchar *) &V)[4];\
                              *((T)+4)=(char) ((uchar *) &V)[3];\
                              *((T)+5)=(char) ((uchar *) &V)[2];\
                              *((T)+6)=(char) ((uchar *) &V)[1];\
                              *((T)+7)=(char) ((uchar *) &V)[0]; } while(0)

#define float8get(V,M)   do { double def_temp;\
                              ((uchar*) &def_temp)[0]=(M)[7];\
                              ((uchar*) &def_temp)[1]=(M)[6];\
                              ((uchar*) &def_temp)[2]=(M)[5];\
                              ((uchar*) &def_temp)[3]=(M)[4];\
                              ((uchar*) &def_temp)[4]=(M)[3];\
                              ((uchar*) &def_temp)[5]=(M)[2];\
                              ((uchar*) &def_temp)[6]=(M)[1];\
                              ((uchar*) &def_temp)[7]=(M)[0];\
                              (V) = def_temp; } while(0)
#else
#define float4get(V,M)   memcpy_fixed((uchar*) &V,(uchar*) (M),sizeof(float))
#define float4store(V,M) memcpy_fixed((uchar*) V,(uchar*) (&M),sizeof(float))

#if defined(__FLOAT_WORD_ORDER) && (__FLOAT_WORD_ORDER == __BIG_ENDIAN)
#define doublestore(T,V) do { *(((char*)T)+0)=(char) ((uchar *) &V)[4];\
                              *(((char*)T)+1)=(char) ((uchar *) &V)[5];\
                              *(((char*)T)+2)=(char) ((uchar *) &V)[6];\
                              *(((char*)T)+3)=(char) ((uchar *) &V)[7];\
                              *(((char*)T)+4)=(char) ((uchar *) &V)[0];\
                              *(((char*)T)+5)=(char) ((uchar *) &V)[1];\
                              *(((char*)T)+6)=(char) ((uchar *) &V)[2];\
                              *(((char*)T)+7)=(char) ((uchar *) &V)[3]; }\
                         while(0)
#define doubleget(V,M)   do { double def_temp;\
                              ((uchar*) &def_temp)[0]=(M)[4];\
                              ((uchar*) &def_temp)[1]=(M)[5];\
                              ((uchar*) &def_temp)[2]=(M)[6];\
                              ((uchar*) &def_temp)[3]=(M)[7];\
                              ((uchar*) &def_temp)[4]=(M)[0];\
                              ((uchar*) &def_temp)[5]=(M)[1];\
                              ((uchar*) &def_temp)[6]=(M)[2];\
                              ((uchar*) &def_temp)[7]=(M)[3];\
                              (V) = def_temp; } while(0)
#endif /* __FLOAT_WORD_ORDER */

#define float8get(V,M)   doubleget((V),(M))
#define float8store(V,M) doublestore((V),(M))
#endif /* WORDS_BIGENDIAN */

#endif /* __i386__ OR _WIN32 */

/*
  Macro for reading 32-bit integer from network byte order (big-endian)
  from unaligned memory location.
*/
#define int4net(A)        (int32) (((uint32) ((uchar) (A)[3]))        |\
				  (((uint32) ((uchar) (A)[2])) << 8)  |\
				  (((uint32) ((uchar) (A)[1])) << 16) |\
				  (((uint32) ((uchar) (A)[0])) << 24))
/*
  Define-funktions for reading and storing in machine format from/to
  short/long to/from some place in memory V should be a (not
  register) variable, M is a pointer to byte
*/

#ifdef WORDS_BIGENDIAN

#define ushortget(V,M)  do { V = (uint16) (((uint16) ((uchar) (M)[1]))+\
                                 ((uint16) ((uint16) (M)[0]) << 8)); } while(0)
#define shortget(V,M)   do { V = (short) (((short) ((uchar) (M)[1]))+\
                                 ((short) ((short) (M)[0]) << 8)); } while(0)
#define longget(V,M)    do { int32 def_temp;\
                             ((uchar*) &def_temp)[0]=(M)[0];\
                             ((uchar*) &def_temp)[1]=(M)[1];\
                             ((uchar*) &def_temp)[2]=(M)[2];\
                             ((uchar*) &def_temp)[3]=(M)[3];\
                             (V)=def_temp; } while(0)
#define ulongget(V,M)   do { uint32 def_temp;\
                            ((uchar*) &def_temp)[0]=(M)[0];\
                            ((uchar*) &def_temp)[1]=(M)[1];\
                            ((uchar*) &def_temp)[2]=(M)[2];\
                            ((uchar*) &def_temp)[3]=(M)[3];\
                            (V)=def_temp; } while(0)
#define shortstore(T,A) do { uint def_temp=(uint) (A) ;\
                             *(((char*)T)+1)=(char)(def_temp); \
                             *(((char*)T)+0)=(char)(def_temp >> 8); } while(0)
#define longstore(T,A)  do { *(((char*)T)+3)=((A));\
                             *(((char*)T)+2)=(((A) >> 8));\
                             *(((char*)T)+1)=(((A) >> 16));\
                             *(((char*)T)+0)=(((A) >> 24)); } while(0)

#define floatget(V,M)    memcpy_fixed((uchar*) &V,(uchar*) (M),sizeof(float))
#define floatstore(T,V)  memcpy_fixed((uchar*) (T),(uchar*)(&V),sizeof(float))
#define doubleget(V,M)	 memcpy_fixed((uchar*) &V,(uchar*) (M),sizeof(double))
#define doublestore(T,V) memcpy_fixed((uchar*) (T),(uchar*) &V,sizeof(double))
#define longlongget(V,M) memcpy_fixed((uchar*) &V,(uchar*) (M),sizeof(ulonglong))
#define longlongstore(T,V) memcpy_fixed((uchar*) (T),(uchar*) &V,sizeof(ulonglong))

#else

#define ushortget(V,M)	do { V = uint2korr(M); } while(0)
#define shortget(V,M)	do { V = sint2korr(M); } while(0)
#define longget(V,M)	do { V = sint4korr(M); } while(0)
#define ulongget(V,M)   do { V = uint4korr(M); } while(0)
#define shortstore(T,V) int2store(T,V)
#define longstore(T,V)	int4store(T,V)
#ifndef floatstore
#define floatstore(T,V)  memcpy_fixed((uchar*) (T),(uchar*) (&V),sizeof(float))
#define floatget(V,M)    memcpy_fixed((uchar*) &V, (uchar*) (M), sizeof(float))
#endif
#ifndef doubleget
#define doubleget(V,M)	 memcpy_fixed((uchar*) &V,(uchar*) (M),sizeof(double))
#define doublestore(T,V) memcpy_fixed((uchar*) (T),(uchar*) &V,sizeof(double))
#endif /* doubleget */
#define longlongget(V,M) memcpy_fixed((uchar*) &V,(uchar*) (M),sizeof(ulonglong))
#define longlongstore(T,V) memcpy_fixed((uchar*) (T),(uchar*) &V,sizeof(ulonglong))

#endif /* WORDS_BIGENDIAN */

/* sprintf does not always return the number of bytes :- */
#ifdef SPRINTF_RETURNS_INT
#define my_sprintf(buff,args) sprintf args
#else
#ifdef SPRINTF_RETURNS_PTR
#define my_sprintf(buff,args) ((int)(sprintf args - buff))
#else
#define my_sprintf(buff,args) ((ulong) sprintf args, (ulong) strlen(buff))
#endif
#endif

#ifndef THREAD
#define thread_safe_increment(V,L) (V)++
#define thread_safe_decrement(V,L) (V)--
#define thread_safe_add(V,C,L)     (V)+=(C)
#define thread_safe_sub(V,C,L)     (V)-=(C)
#define statistic_increment(V,L)   (V)++
#define statistic_decrement(V,L)   (V)--
#define statistic_add(V,C,L)       (V)+=(C)
#define statistic_sub(V,C,L)       (V)-=(C)
#endif

#ifdef HAVE_CHARSET_utf8
#define MYSQL_UNIVERSAL_CLIENT_CHARSET "utf8"
#else
#define MYSQL_UNIVERSAL_CLIENT_CHARSET MYSQL_DEFAULT_CHARSET_NAME
#endif

#if defined(EMBEDDED_LIBRARY) && !defined(HAVE_EMBEDDED_PRIVILEGE_CONTROL)
#define NO_EMBEDDED_ACCESS_CHECKS
#endif

#ifdef HAVE_DLOPEN
#if defined(__WIN__)
#define dlsym(lib, name) GetProcAddress((HMODULE)lib, name)
#define dlopen(libname, unused) LoadLibraryEx(libname, NULL, 0)
#define dlclose(lib) FreeLibrary((HMODULE)lib)
#elif defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif
#endif

/* FreeBSD 2.2.2 does not define RTLD_NOW) */
#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif

#ifndef HAVE_DLERROR
#define dlerror() ""
#endif


#ifndef __NETWARE__
/*
 *  Include standard definitions of operator new and delete.
 */
#ifdef __cplusplus
#include <new>
#endif
#else
/*
 *  Define placement versions of operator new and operator delete since
 *  we don't have <new> when building for Netware.
 */
#ifdef __cplusplus
inline void *operator new(size_t, void *ptr) { return ptr; }
inline void *operator new[](size_t, void *ptr) { return ptr; }
inline void  operator delete(void*, void*) { /* Do nothing */ }
inline void  operator delete[](void*, void*) { /* Do nothing */ }
#endif
#endif

/* Length of decimal number represented by INT32. */
#define MY_INT32_NUM_DECIMAL_DIGITS 11

/* Length of decimal number represented by INT64. */
#define MY_INT64_NUM_DECIMAL_DIGITS 21

/* Define some useful general macros (should be done after all headers). */
#if !defined(max)
#define max(a, b)	((a) > (b) ? (a) : (b))
#define min(a, b)	((a) < (b) ? (a) : (b))
#endif  
/*
  Only Linux is known to need an explicit sync of the directory to make sure a
  file creation/deletion/renaming in(from,to) this directory durable.
*/
#ifdef TARGET_OS_LINUX
#define NEED_EXPLICIT_SYNC_DIR 1
#endif

#if !defined(__cplusplus) && !defined(bool)
#define bool In_C_you_should_use_my_bool_instead()
#endif

#ifndef HAVE_RINT
/**
   All integers up to this number can be represented exactly as double precision
   values (DBL_MANT_DIG == 53 for IEEE 754 hardware).
*/
#define MAX_EXACT_INTEGER ((1LL << DBL_MANT_DIG) - 1)

/**
   rint(3) implementation for platforms that do not have it.
   Always rounds to the nearest integer with ties being rounded to the nearest
   even integer to mimic glibc's rint() behavior in the "round-to-nearest"
   FPU mode. Hardware-specific optimizations are possible (frndint on x86).
   Unlike this implementation, hardware will also honor the FPU rounding mode.
*/

static inline double rint(double x)
{
  double f, i;
  f = modf(x, &i);
  /*
    All doubles with absolute values > MAX_EXACT_INTEGER are even anyway,
    no need to check it.
  */
  if (x > 0.0)
    i += (double) ((f > 0.5) || (f == 0.5 &&
                                 i <= (double) MAX_EXACT_INTEGER &&
                                 (longlong) i % 2));
  else
    i -= (double) ((f < -0.5) || (f == -0.5 &&
                                  i >= (double) -MAX_EXACT_INTEGER &&
                                  (longlong) i % 2));
  return i;
}
#endif /* HAVE_RINT */

/* 
  MYSQL_PLUGIN_IMPORT macro is used to export mysqld data
  (i.e variables) for usage in storage engine loadable plugins.
  Outside of Windows, it is dummy.
*/
#ifndef MYSQL_PLUGIN_IMPORT
#if (defined(_WIN32) && defined(MYSQL_DYNAMIC_PLUGIN))
#define MYSQL_PLUGIN_IMPORT __declspec(dllimport)
#else
#define MYSQL_PLUGIN_IMPORT
#endif
#endif

#endif /* my_global_h */
