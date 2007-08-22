/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May		wmay@cisco.com
 */

#ifndef __SYSTEMS_H__
#define __SYSTEMS_H__

#ifdef WIN32
#define HAVE_IN_PORT_T
#define HAVE_SOCKLEN_T
#include <win32_ver.h>
#define NEED_SDL_VIDEO_IN_MAIN_THREAD
#else
#undef PACKAGE
#undef VERSION
#include <config.h>
#endif




#ifdef WIN32

#define _WIN32_WINNT 0x0400
#define _WINSOCKAPI_
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int64 u_int64_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int16 u_int16_t;
typedef unsigned __int8 u_int8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
typedef unsigned short in_port_t;
typedef int socklen_t;
typedef int ssize_t;
#define snprintf _snprintf
#define strncasecmp _strnicmp
#define strcasecmp _stricmp

#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#define write _write
#define lseek _lseek
#define close _close
#define open _open
#define access _access
#define vsnprintf _vsnprintf
#define F_OK 0
#define OPEN_RDWR (_O_RDWR | _O_BINARY)
#define OPEN_CREAT (_O_CREAT | _O_BINARY)
#define OPEN_RDONLY (_O_RDONLY | _O_BINARY)
#define srandom srand
#define random rand

#define IOSBINARY ios::binary

#ifdef __cplusplus
extern "C" {
#endif
int gettimeofday(struct timeval *t, void *);
#ifdef __cplusplus
}
#endif

#define PATH_MAX MAX_PATH
#define MAX_UINT64 -1
#define LLD "%I64d"
#define LLU "%I64u"
#define LLX "%I64x"
#define LLX16 "%016I64x"
#define M_LLU 1000i64
#define C_LLU 100i64
#define I_LLU 1i64

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

#if     !__STDC__ && _INTEGRAL_MAX_BITS >= 64
#define VAR_TO_FPOS(fpos, var) (fpos) = (var)
#define FPOS_TO_VAR(fpos, typed, var) (var) = (typed)(_FPOSOFF(fpos))
#else
#define VAR_TO_FPOS(fpos, var) (fpos).lopart = ((var) & UINT_MAX); (fpos).hipart = ((var) >> 32)
#define FPOS_TO_VAR(fpos, typed, var) (var) = (typed)((uint64_t)((fpos).hipart ) << 32 | (fpos).lopart)
#endif

#define __STRING(expr) #expr

#define FOPEN_READ_BINARY "rb"
#define FOPEN_WRITE_BINARY "wb"

#define UINT64_TO_DOUBLE(a) ((double)((int64_t)(a)))
#else /* UNIX */
/*****************************************************************************
 *   UNIX LIKE DEFINES BELOW THIS POINT
 *****************************************************************************/
#ifdef sun
#include <sys/feature_tests.h>
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#else
#if _FILE_OFFSET_BITS < 64
#error File offset bits is already set to non-64 value
#endif
#endif

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#error "Don't have stdint.h or inttypes.h - no way to get uint8_t"
#endif
#endif

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/stat.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#include <sys/param.h>

#define OPEN_RDWR O_RDWR
#define OPEN_CREAT O_CREAT 
#define OPEN_RDONLY O_RDONLY

#define closesocket close
#define IOSBINARY ios::bin
#define MAX_UINT64 -1LLU
#define LLD "%lld"
#define LLU "%llu"
#define LLX "%llx"
#define LLX16 "%016llx"
#define M_LLU 1000LLU
#define C_LLU 100LLU
#define I_LLU 1LLU
#ifdef HAVE_FPOS_T_POS
#define FPOS_TO_VAR(fpos, typed, var) (var) = (typed)((fpos).__pos)
#define VAR_TO_FPOS(fpos, var) (fpos).__pos = (var)
#else
#define FPOS_TO_VAR(fpos, typed, var) (var) = (typed)(fpos)
#define VAR_TO_FPOS(fpos, var) (fpos) = (var)
#endif

#define FOPEN_READ_BINARY "r"
#define FOPEN_WRITE_BINARY "w"
#define UINT64_TO_DOUBLE(a) ((double)(a))
#endif /* define unix */

/*****************************************************************************
 *             Generic type includes used in the whole package               *
 *****************************************************************************/
#include <stdarg.h>
typedef void (*error_msg_func_t)(int loglevel,
				 const char *lib,
				 const char *fmt,
				 va_list ap);
typedef void (*lib_message_func_t)(int loglevel,
				   const char *lib,
				   const char *fmt,
				   ...);
#ifndef HAVE_IN_PORT_T
typedef uint16_t in_port_t;
#endif

#ifndef HAVE_SOCKLEN_T
typedef unsigned int socklen_t;
#endif

#ifdef sun
#include <limits.h>
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#define __STRING(expr) #expr
#endif

#ifndef HAVE_STRSEP
#ifdef __cplusplus
extern "C" {
#endif
char *strsep(char **strp, const char *delim); 
#ifdef __cplusplus
}
#endif
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

#define MALLOC_STRUCTURE(a) ((a *)malloc(sizeof(a)))

#define CHECK_AND_FREE(a) if ((a) != NULL) { free((void *)(a)); (a) = NULL;}

#define NUM_ELEMENTS_IN_ARRAY(name) ((sizeof((name))) / (sizeof(*(name))))

#ifndef HAVE_GLIB_H
typedef char gchar;
typedef unsigned char guchar;

typedef int gint;
typedef unsigned int guint;

typedef long glong;
typedef unsigned long gulong;

typedef double gdouble;

typedef int gboolean;

typedef int16_t gint16;
typedef uint16_t guint16;

typedef int32_t gint32;
typedef uint32_t guint32;

typedef int64_t gint64;
typedef uint64_t guint64;

typedef uint8_t  guint8;
typedef int8_t gint8;

#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif /* __SYSTEMS_H__ */




