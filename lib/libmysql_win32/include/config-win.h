/* Copyright 2000-2008 MySQL AB, 2008 Sun Microsystems, Inc.

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

/* Defines for Win32 to make it compatible for MySQL */

#define BIG_TABLES

#if defined(_MSC_VER) && _MSC_VER >= 1400
/* Avoid endless warnings about sprintf() etc. being unsafe. */
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <sys/locking.h>
#include <winsock2.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>

#define HAVE_SMEM 1

#if defined(_WIN64) || defined(WIN64) 
#define SYSTEM_TYPE	"Win64" 
#elif defined(_WIN32) || defined(WIN32) 
#define SYSTEM_TYPE	"Win32" 
#else
#define SYSTEM_TYPE	"Windows"
#endif

#if defined(_M_IA64) 
#define MACHINE_TYPE	"ia64" 
#elif defined(_M_IX86) 
#define MACHINE_TYPE	"ia32" 
#elif defined(_M_ALPHA) 
#define MACHINE_TYPE	"axp" 
#else
#define MACHINE_TYPE	"unknown"	/* Define to machine type name */
#endif 
 
#if !(defined(_WIN64) || defined(WIN64)) 
#ifndef _WIN32
#define _WIN32				/* Compatible with old source */
#endif
#ifndef __WIN32__
#define __WIN32__
#endif
#endif /* _WIN64 */
#ifndef __WIN__
#define __WIN__			      /* To make it easier in VC++ */
#endif

#ifndef MAX_INDEXES
#define MAX_INDEXES 64
#endif

/* File and lock constants */
#define O_SHARE		0x1000		/* Open file in sharing mode */
#ifdef __BORLANDC__
#define F_RDLCK		LK_NBLCK	/* read lock */
#define F_WRLCK		LK_NBRLCK	/* write lock */
#define F_UNLCK		LK_UNLCK	/* remove lock(s) */
#else
#define F_RDLCK		_LK_NBLCK	/* read lock */
#define F_WRLCK		_LK_NBRLCK	/* write lock */
#define F_UNLCK		_LK_UNLCK	/* remove lock(s) */
#endif

#define F_EXCLUSIVE	1		/* We have only exclusive locking */
#define F_TO_EOF	(INT_MAX32/2)	/* size for lock of all file */
#define F_OK		0		/* parameter to access() */
#define W_OK		2

#define S_IROTH		S_IREAD		/* for my_lib */

#ifdef __BORLANDC__
#define FILE_BINARY	O_BINARY	/* my_fopen in binary mode */
#define O_TEMPORARY	0
#define O_SHORT_LIVED	0
#define SH_DENYNO	_SH_DENYNO
#else
#define O_BINARY	_O_BINARY	/* compability with older style names */
#define FILE_BINARY	_O_BINARY	/* my_fopen in binary mode */
#define O_TEMPORARY	_O_TEMPORARY
#define O_SHORT_LIVED	_O_SHORT_LIVED
#define SH_DENYNO	_SH_DENYNO
#endif
#define NO_OPEN_3			/* For my_create() */

#define SIGQUIT		SIGTERM		/* No SIGQUIT */

#undef _REENTRANT			/* Crashes something for win32 */
#undef SAFE_MUTEX			/* Can't be used on windows */

#if defined(_MSC_VER) && _MSC_VER >= 1310
#define LL(A)           A##ll
#define ULL(A)          A##ull
#else
#define LL(A)           ((__int64) A)
#define ULL(A)          ((unsigned __int64) A)
#endif

#define LONGLONG_MIN	LL(0x8000000000000000)
#define LONGLONG_MAX	LL(0x7FFFFFFFFFFFFFFF)
#define ULONGLONG_MAX	ULL(0xFFFFFFFFFFFFFFFF)

/* Type information */

#if !defined(HAVE_UINT)
#undef HAVE_UINT
#define HAVE_UINT
typedef unsigned short	ushort;
typedef unsigned int	uint;
#endif /* !defined(HAVE_UINT) */

typedef unsigned __int64 ulonglong;	/* Microsofts 64 bit types */
typedef __int64 longlong;
#ifndef HAVE_SIGSET_T
typedef int sigset_t;
#endif
#define longlong_defined
/*
  off_t should not be __int64 because of conflicts in header files;
  Use my_off_t or os_off_t instead
*/
#ifndef HAVE_OFF_T
typedef long off_t;
#endif
typedef __int64 os_off_t;
#ifdef _WIN64
typedef UINT_PTR rf_SetTimer;
#else
#ifndef HAVE_SIZE_T
typedef unsigned int size_t;
#endif
typedef uint rf_SetTimer;
#endif

#define Socket_defined
#define my_socket SOCKET
#define SIGPIPE SIGINT
#define RETQSORTTYPE void
#define QSORT_TYPE_IS_VOID
#define RETSIGTYPE void
#define SOCKET_SIZE_TYPE int
#define my_socket_defined
#define byte_defined
#define HUGE_PTR
#define STDCALL __stdcall	    /* Used by libmysql.dll */
#define isnan(X) _isnan(X)
#define finite(X) _finite(X)

#ifndef MYSQL_CLIENT_NO_THREADS
#define THREAD
#endif
#define VOID_SIGHANDLER
#define SIZEOF_CHAR		1
#define SIZEOF_INT		4
#define SIZEOF_LONG		4
#define SIZEOF_LONG_LONG	8
#define SIZEOF_OFF_T		8
#ifdef _WIN64
#define SIZEOF_CHARP		8
#else
#define SIZEOF_CHARP		4
#endif
#define HAVE_BROKEN_NETINET_INCLUDES
#ifdef __NT__
#define HAVE_NAMED_PIPE			/* We can only create pipes on NT */
#endif

/* ERROR is defined in wingdi.h */
#undef ERROR

/* We need to close files to break connections on shutdown */
#ifndef SIGNAL_WITH_VIO_CLOSE
#define SIGNAL_WITH_VIO_CLOSE
#endif

/* All windows servers should support .sym files */
#undef USE_SYMDIR
#define USE_SYMDIR

/* If LOAD DATA LOCAL INFILE should be enabled by default */
#define ENABLED_LOCAL_INFILE 1

/* If query profiling should be enabled by default */
#define ENABLED_PROFILING 1

/* Convert some simple functions to Posix */

#define my_sigset(A,B) signal((A),(B))
#define finite(A) _finite(A)
#define sleep(A)  Sleep((A)*1000)
#define popen(A,B) _popen((A),(B))
#define pclose(A) _pclose(A)

#ifndef __BORLANDC__
#define access(A,B) _access(A,B)
#endif

#if !defined(__cplusplus)
#define inline __inline
#endif /* __cplusplus */

#ifdef _WIN64
#define ulonglong2double(A) ((double) (ulonglong) (A))
#define my_off_t2double(A)  ((double) (my_off_t) (A))

#else
inline double ulonglong2double(ulonglong value)
{
  longlong nr=(longlong) value;
  if (nr >= 0)
    return (double) nr;
  return (18446744073709551616.0 + (double) nr);
}
#define my_off_t2double(A) ulonglong2double(A)
#endif /* _WIN64 */

inline ulonglong double2ulonglong(double d)
{
  double t= d - (double) 0x8000000000000000ULL;

  if (t >= 0)
    return  ((ulonglong) t) + 0x8000000000000000ULL;
  return (ulonglong) d;
}

#if SIZEOF_OFF_T > 4
#define lseek(A,B,C) _lseeki64((A),(longlong) (B),(C))
#define tell(A) _telli64(A)
#endif

#define STACK_DIRECTION -1

/* Difference between GetSystemTimeAsFileTime() and now() */
#define OFFSET_TO_EPOCH ULL(116444736000000000)

#define HAVE_PERROR
#define HAVE_VFPRINT
#define HAVE_RENAME		/* Have rename() as function */
#define HAVE_BINARY_STREAMS	/* Have "b" flag in streams */
#define HAVE_LONG_JMP		/* Have long jump function */
#define HAVE_LOCKING		/* have locking() call */
#define HAVE_ERRNO_AS_DEFINE	/* errno is a define */
#define HAVE_STDLIB		/* everything is include in this file */
#define HAVE_MEMCPY
#define HAVE_MEMMOVE
#define HAVE_GETCWD
#define HAVE_TELL
#define HAVE_TZNAME
#define HAVE_PUTENV
#define HAVE_SELECT
#define HAVE_SETLOCALE
#define HAVE_SOCKET		/* Giangi */
#define HAVE_FLOAT_H
#define HAVE_LIMITS_H
#define HAVE_STDDEF_H
#define NO_FCNTL_NONBLOCK	/* No FCNTL */
#define HAVE_ALLOCA
#define HAVE_STRPBRK
#define HAVE_STRSTR
#define HAVE_COMPRESS
#define HAVE_CREATESEMAPHORE
#define HAVE_ISNAN
#define HAVE_FINITE
#define HAVE_QUERY_CACHE
#define SPRINTF_RETURNS_INT
#define HAVE_SETFILEPOINTER
#define HAVE_VIO_READ_BUFF
#if defined(_MSC_VER) && _MSC_VER >= 1400
/* strnlen() appeared in Studio 2005 */
#define HAVE_STRNLEN
#endif
#define HAVE_WINSOCK2

#define strcasecmp stricmp
#define strncasecmp strnicmp

#ifndef __NT__
#undef FILE_SHARE_DELETE
#define FILE_SHARE_DELETE 0     /* Not implemented on Win 98/ME */
#endif

#ifdef NOT_USED
#define HAVE_SNPRINTF		/* Gave link error */
#define _snprintf snprintf
#endif

#ifdef _MSC_VER
#define HAVE_LDIV		/* The optimizer breaks in zortech for ldiv */
#define HAVE_ANSI_INCLUDE
#define HAVE_SYS_UTIME_H
#define HAVE_STRTOUL
#endif
#define my_reinterpret_cast(A) reinterpret_cast <A>
#define my_const_cast(A) const_cast<A>


/* MYSQL OPTIONS */

#ifdef _CUSTOMCONFIG_
#include <custom_conf.h>
#else
#ifndef CMAKE_CONFIGD
#define DEFAULT_MYSQL_HOME	"c:\\mysql"
#define MYSQL_DATADIR          "c:\\mysql\\data"
#define PACKAGE			"mysql"
#define DEFAULT_BASEDIR		"C:\\"
#define SHAREDIR		"share"
#define DEFAULT_CHARSET_HOME	"C:/mysql/"
#endif
#endif
#ifndef DEFAULT_HOME_ENV
#define DEFAULT_HOME_ENV MYSQL_HOME
#endif
#ifndef DEFAULT_GROUP_SUFFIX_ENV
#define DEFAULT_GROUP_SUFFIX_ENV MYSQL_GROUP_SUFFIX
#endif

/* File name handling */

#define FN_LIBCHAR	'\\'
#define FN_ROOTDIR	"\\"
#define FN_DEVCHAR	':'
#define FN_NETWORK_DRIVES	/* Uses \\ to indicate network drives */
#define FN_NO_CASE_SENCE	/* Files are not case-sensitive */
#define OS_FILE_LIMIT	2048

#define DO_NOT_REMOVE_THREAD_WRAPPERS
#define thread_safe_increment(V,L) InterlockedIncrement((long*) &(V))
#define thread_safe_decrement(V,L) InterlockedDecrement((long*) &(V))
/* The following is only used for statistics, so it should be good enough */
#ifdef __NT__  /* This should also work on Win98 but .. */
#define thread_safe_add(V,C,L) InterlockedExchangeAdd((long*) &(V),(C))
#define thread_safe_sub(V,C,L) InterlockedExchangeAdd((long*) &(V),-(long) (C))
#endif

#define shared_memory_buffer_length 16000
#define default_shared_memory_base_name "MYSQL"

#define HAVE_SPATIAL 1
#define HAVE_RTREE_KEYS 1

#define HAVE_OPENSSL 1
#define HAVE_YASSL 1

#define COMMUNITY_SERVER 1
#define ENABLED_PROFILING 1

/*
  Our Windows binaries include all character sets which MySQL supports.
  Any changes to the available character sets must also go into
  config/ac-macros/character_sets.m4
*/

#define MYSQL_DEFAULT_CHARSET_NAME "latin1"
#define MYSQL_DEFAULT_COLLATION_NAME "latin1_swedish_ci"

#define USE_MB 1
#define USE_MB_IDENT 1
#define USE_STRCOLL 1

#define HAVE_CHARSET_armscii8
#define HAVE_CHARSET_ascii
#define HAVE_CHARSET_big5 1
#define HAVE_CHARSET_cp1250 1
#define HAVE_CHARSET_cp1251
#define HAVE_CHARSET_cp1256
#define HAVE_CHARSET_cp1257
#define HAVE_CHARSET_cp850
#define HAVE_CHARSET_cp852
#define HAVE_CHARSET_cp866
#define HAVE_CHARSET_cp932 1
#define HAVE_CHARSET_dec8
#define HAVE_CHARSET_eucjpms 1
#define HAVE_CHARSET_euckr 1
#define HAVE_CHARSET_gb2312 1
#define HAVE_CHARSET_gbk 1
#define HAVE_CHARSET_geostd8
#define HAVE_CHARSET_greek
#define HAVE_CHARSET_hebrew
#define HAVE_CHARSET_hp8
#define HAVE_CHARSET_keybcs2
#define HAVE_CHARSET_koi8r
#define HAVE_CHARSET_koi8u
#define HAVE_CHARSET_latin1 1
#define HAVE_CHARSET_latin2 1
#define HAVE_CHARSET_latin5
#define HAVE_CHARSET_latin7
#define HAVE_CHARSET_macce
#define HAVE_CHARSET_macroman
#define HAVE_CHARSET_sjis 1
#define HAVE_CHARSET_swe7
#define HAVE_CHARSET_tis620 1
#define HAVE_CHARSET_ucs2 1
#define HAVE_CHARSET_ujis 1
#define HAVE_CHARSET_utf8 1

#define HAVE_UCA_COLLATIONS 1
#define HAVE_BOOL 1
