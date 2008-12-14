/*
 *	@file 	miniMpr.h
 *	@brief 	Mini Mbedthis Portable Runtime (MPR) Environment.
 *	@copy	default
 *	
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Mbedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.mbedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Mbedthis 
 *	Software at http://www.mbedthis.com 
 *	
 *	@end
 */
#ifndef _h_MINI_MPR
#define _h_MINI_MPR 1

/********************************** Includes **********************************/
/*
 *	Find out about our configuration
 */
	#include 	"buildConfig.h"

#if BLD_APPWEB
	/*
	 *	If building within Appweb, use the full MPR
	 */
	#include 	"mpr.h"
#else

	#include	<ctype.h>
	#include	<fcntl.h>
	#include	<stdarg.h>
	#include	<stdlib.h>
	#include	<stdio.h>
	#include	<string.h>
	#include	<sys/stat.h>

#if !WIN
	#include	<unistd.h>
#endif

#if CE
	#include	<io.h>
	#include	"CE/wincompat.h"
#endif

#if LYNX
	#include	<unistd.h>
#endif

#if QNX4
	#include	<dirent.h>
#endif

/********************************** Defines ***********************************/

#ifdef __cplusplus
extern "C" {
#endif

#if BLD_FEATURE_SQUEEZE
///
///	Reasonable length of a file path name to use in most cases where you know
///	the expected file name and it is certain to be less than this limit.
///
#define MPR_MAX_FNAME			128
#define MPR_MAX_STRING			512
#define MPR_DEFAULT_HASH_SIZE	23			// Default size of hash table index 
#define MPR_MAX_HEAP_SIZE 		(32 * 1024)
#else
#define MPR_MAX_FNAME			256
#define MPR_MAX_STRING			4096
#define MPR_DEFAULT_HASH_SIZE	43			// Default size of hash table index 
#define MPR_MAX_HEAP_SIZE 		(64 * 1024)
#endif

/*
 *	Useful for debugging
 */
#define MPR_L			__FILE__, __LINE__

#if BLD_FEATURE_ASSERT
#define mprAssert(C)  \
	if (C) ; else mprBreakpoint(__FILE__, __LINE__, #C)
#else
	#define mprAssert(C)	if (1) ; else
#endif

///
///	Standard MPR return and error codes 
///
#define MPR_ERR_BASE					(-200) 				///< Error code
#define MPR_ERR_GENERAL					(MPR_ERR_BASE - 1)	///< Error code
#define MPR_ERR_ABORTED					(MPR_ERR_BASE - 2)	///< Error code
#define MPR_ERR_ALREADY_EXISTS			(MPR_ERR_BASE - 3)	///< Error code
#define MPR_ERR_BAD_ARGS				(MPR_ERR_BASE - 4)	///< Error code
#define MPR_ERR_BAD_FORMAT				(MPR_ERR_BASE - 5)	///< Error code
#define MPR_ERR_BAD_HANDLE				(MPR_ERR_BASE - 6)	///< Error code
#define MPR_ERR_BAD_STATE				(MPR_ERR_BASE - 7)	///< Error code
#define MPR_ERR_BAD_SYNTAX				(MPR_ERR_BASE - 8)	///< Error code
#define MPR_ERR_BAD_TYPE				(MPR_ERR_BASE - 9)	///< Error code
#define MPR_ERR_BAD_VALUE				(MPR_ERR_BASE - 10)	///< Error code
#define MPR_ERR_BUSY					(MPR_ERR_BASE - 11)	///< Error code
#define MPR_ERR_CANT_ACCESS				(MPR_ERR_BASE - 12)	///< Error code
#define MPR_ERR_CANT_COMPLETE			(MPR_ERR_BASE - 13)	///< Error code
#define MPR_ERR_CANT_CREATE				(MPR_ERR_BASE - 14)	///< Error code
#define MPR_ERR_CANT_INITIALIZE			(MPR_ERR_BASE - 15)	///< Error code
#define MPR_ERR_CANT_OPEN				(MPR_ERR_BASE - 16)	///< Error code
#define MPR_ERR_CANT_READ				(MPR_ERR_BASE - 17)	///< Error code
#define MPR_ERR_CANT_WRITE				(MPR_ERR_BASE - 18)	///< Error code
#define MPR_ERR_DELETED					(MPR_ERR_BASE - 19)	///< Error code
#define MPR_ERR_NETWORK					(MPR_ERR_BASE - 20)	///< Error code
#define MPR_ERR_NOT_FOUND				(MPR_ERR_BASE - 21)	///< Error code
#define MPR_ERR_NOT_INITIALIZED			(MPR_ERR_BASE - 22)	///< Error code
#define MPR_ERR_NOT_READY				(MPR_ERR_BASE - 23)	///< Error code
#define MPR_ERR_READ_ONLY				(MPR_ERR_BASE - 24)	///< Error code
#define MPR_ERR_TIMEOUT					(MPR_ERR_BASE - 25)	///< Error code
#define MPR_ERR_TOO_MANY				(MPR_ERR_BASE - 26)	///< Error code
#define MPR_ERR_WONT_FIT				(MPR_ERR_BASE - 27)	///< Error code
#define MPR_ERR_WOULD_BLOCK				(MPR_ERR_BASE - 28)	///< Error code
#define MPR_ERR_CANT_ALLOCATE			(MPR_ERR_BASE - 29)	///< Error code
#define MPR_ERR_MAX						(MPR_ERR_BASE - 30)	///< Error code

//
//	Standard error severity and trace levels. These are ored with the error 
//	severities below. The MPR_LOG_MASK is used to extract the trace level 
//	from a flags word. We expect most apps to run with level 2 trace.
//
#define	MPR_FATAL		0				///< Fatal error. Cant continue.
#define	MPR_ERROR		1				///< Hard error
#define MPR_WARN		2				///< Soft warning
#define	MPR_CONFIG		2				///< Essential configuration settings 
#define MPR_INFO		3				///< Informational only 
#define MPR_DEBUG		4				///< Debug information 
#define MPR_VERBOSE		9				///< Highest level of trace 
#define MPR_LOG_MASK	0xf				///< Level mask 

//
//	Error flags. Specify where the error should be sent to. Note that the 
//	product.xml setting "headless" will modify how errors are reported.
//	Assert errors are trapped when in DEV mode. Otherwise ignored.
//
#define	MPR_TRAP		0x10			///< Assert error -- trap in debugger 
#define	MPR_LOG			0x20			///< Log the error in the O/S event log
#define	MPR_USER		0x40			///< Display to the user 
#define	MPR_ALERT		0x80			///< Send a management alert 
#define	MPR_TRACE		0x100			///< Trace

//
//	Error format flags
//
#define MPR_RAW			0x200			// Raw trace output

//
//	Error line number information
//
#define MPR_L		__FILE__, __LINE__

typedef char*			MprStr;

#ifndef __cplusplus
typedef unsigned char 	uchar;
typedef int 			bool;
#endif

/*
 *	Porters: put other operating system type defines here
 */
#if WIN
	typedef unsigned int 		uint;
	typedef __int64 			int64;
	typedef unsigned __int64 	uint64;
#else
#if !CYGWIN
	#define O_BINARY 0
#endif
	__extension__ typedef long long int int64;
	__extension__ typedef unsigned long long int uint64;
#endif

/*
 *	Flexible array data type
 */
typedef struct {
	int		max;						/* Size of the handles array */
	int		used;						/* Count of used entries in handles */
	void	**handles;
} MprArray;

#if BLD_FEATURE_SQUEEZE
#define MPR_ARRAY_INCR		8
#else
#define MPR_ARRAY_INCR		16
#endif

/********************************* Prototypes *********************************/
/*
 *	If running in the GoAhead WebServer, map some MPR routines to WebServer
 *	equivalents.
 */

#define mprMalloc malloc
#define mprSprintf snprintf
#define mtVsprintf vsnprintf
extern void		*mprRealloc(void *ptr, uint size);
extern void 	mprFree(void *ptr);
extern char		*mprStrdup(const char *str);
extern int 		mprAllocVsprintf(char **msgbuf, int maxSize, char *fmt, 
					va_list args);
extern int 		mprAllocSprintf(char **msgbuf, int maxSize, char *fmt, ...);
extern char 	*mprItoa(int num, char *buf, int width);
extern void		mprLog(int level, char *fmt, ...);
extern void		mprBreakpoint(char *file, int line, char *msg);

extern MprArray	*mprCreateArray();
extern void 	mprDestroyArray(MprArray *array);
extern int 		mprAddToArray(MprArray *array, void *item);
extern int 		mprRemoveFromArray(MprArray *array, int index);
extern char 	*mprStrTok(char *str, const char *delim, char **tok);

extern int		mprGetDirName(char *buf, int bufsize, char *path);
extern int		mprReallocStrcat(char **dest, int max, int existingLen,
						const char *delim, const char *src, ...);
extern int		mprStrcpy(char *dest, int destMax, const char *src);
extern int 		mprMemcpy(char *dest, int destMax, const char *src, int nbytes);

#ifdef __cplusplus
}
#endif
#endif /* !BLD_APPWEB */
#endif /* _h_MINI_MPR */

/*****************************************************************************/

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
