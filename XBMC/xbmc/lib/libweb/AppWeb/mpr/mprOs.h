///
///	@file 	mprOs.h
/// @brief 	Include O/S headers and smooth out per-O/S differences
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//////////////////////////////// Documentation /////////////////////////////////
///
///	This header is part of the Mbedthis Portable Runtime and aims to include
///	all necessary O/S headers and to unify the constants and declarations 
///	required by Mbedthis products. It can be included by C or C++ programs.
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _h_MPR_OS_HDRS
#define _h_MPR_OS_HDRS 1

#include	"buildConfig.h"

////////////////////////////////// CPU Families ////////////////////////////////
//
//	Porters, add your CPU families here and update configure code. 
//
#define MPR_CPU_UNKNOWN		0
#define MPR_CPU_IX86		1
#define MPR_CPU_PPC 		2
#define MPR_CPU_SPARC 		3
#define MPR_CPU_XSCALE 		4
#define MPR_CPU_ARM 		5
#define MPR_CPU_MIPS 		6
#define MPR_CPU_68K 		7
#define MPR_CPU_SIMNT 		8			//	VxWorks NT simulator
#define MPR_CPU_SIMSPARC 	9			//	VxWorks sparc simulator
#define MPR_CPU_IX64		10
#define MPR_CPU_UNIVERSAL	11			/* MAC OS X universal binaries */
#define MPR_CPU_SH4			12

////////////////////////////////// O/S Includes ////////////////////////////////

#if CYGWIN || LINUX || SOLARIS
	#include	<sys/types.h>
	#include	<time.h>
	#include	<arpa/inet.h>
	#include	<ctype.h>
	#include	<dirent.h>
	#include	<dlfcn.h>
	#include	<fcntl.h>
	#include	<grp.h> 
	#include	<errno.h>
	#include	<libgen.h>
	#include	<limits.h>
	#include	<netdb.h>
	#include	<net/if.h>
	#include	<netinet/in.h>
	#include	<netinet/tcp.h>
	#include	<netinet/ip.h>
	#include	<pthread.h> 
	#include	<pwd.h> 
#if !CYGWIN
	#include	<resolv.h>
#endif
	#include	<signal.h>
	#include	<stdarg.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<string.h>
	#include	<syslog.h>
	#include	<sys/ioctl.h>
	#include	<sys/stat.h>
	#include	<sys/param.h>
	#include	<sys/resource.h>
	#include	<sys/sem.h>
	#include	<sys/shm.h>
	#include	<sys/socket.h>
	#include	<sys/select.h>
	#include	<sys/time.h>
	#include	<sys/times.h>
	#include	<sys/utsname.h>
	#include	<sys/wait.h>
	#include	<unistd.h>

#if CYGWIN || LINUX
	#include	<stdint.h>
#endif

#if SOLARIS
	#include	<netinet/in_systm.h>
#endif

#if BLD_FEATURE_FLOATING_POINT
	#define __USE_ISOC99 1
	#include	<math.h>
#if !CYGWIN
	#include	<values.h>
#endif
#endif

#endif // CYGWIN || LINUX || SOLARIS

#if VXWORKS
	#include	<vxWorks.h>
	#include	<envLib.h>
	#include	<sys/types.h>
	#include	<time.h>
	#include	<arpa/inet.h>
	#include	<ctype.h>
	#include	<fcntl.h>
	#include	<errno.h>
	#include	<limits.h>
	#include	<loadLib.h>
	#include	<dirent.h>
	#include	<math.h>
	#include	<netdb.h>
	#include	<net/if.h>
	#include	<netinet/tcp.h>
	#include	<netinet/in.h>
	#include	<netinet/ip.h>
	#include	<selectLib.h>
	#include	<signal.h>
	#include	<stdarg.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<string.h>
	#include	<sysSymTbl.h>
	#include	<sys/fcntlcom.h>
	#include	<sys/ioctl.h>
	#include	<sys/stat.h>
	#include	<sys/socket.h>
	#include	<sys/times.h>
	#include	<sys/wait.h>
	#include	<unistd.h>
	#include	<unldLib.h>

	#if BLD_FEATURE_FLOATING_POINT
	#include	<float.h>
	#define __USE_ISOC99 1
	#include	<math.h>
	#endif

	#include	<sockLib.h>
	#include	<inetLib.h>
	#include	<ioLib.h>
	#include	<pipeDrv.h>
	#include	<hostLib.h>
	#include	<netdb.h>
	#include	<tickLib.h>
	#include	<taskHookLib.h>

#endif // VXWORKS

#if MACOSX
	#include	<time.h>
	#include	<arpa/inet.h>
	#include	<ctype.h>
	#include	<dirent.h>
	#include	<dlfcn.h>
	#include	<fcntl.h>
	#include	<grp.h> 
	#include	<errno.h>
	#include	<libgen.h>
	#include	<limits.h>
	#include	<mach-o/dyld.h>
	#include	<netdb.h>
	#include	<net/if.h>
	#include	<netinet/in_systm.h>
	#include	<netinet/in.h>
	#include	<netinet/tcp.h>
	#include	<netinet/ip.h>
	#include	<pthread.h> 
	#include	<pwd.h> 
	#include	<resolv.h>
	#include	<signal.h>
	#include	<stdarg.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<stdint.h>
	#include	<string.h>
	#include	<syslog.h>
	#include	<sys/ioctl.h>
	#include	<sys/types.h>
	#include	<sys/stat.h>
	#include	<sys/param.h>
	#include 	<sys/resource.h>
	#include	<sys/sem.h>
	#include	<sys/shm.h>
	#include	<sys/socket.h>
	#include	<sys/select.h>
	#include	<sys/time.h>
	#include	<sys/times.h>
	#include	<sys/types.h>
	#include	<sys/utsname.h>
	#include	<sys/wait.h>
	#include	<unistd.h>
	#include	<libkern/OSAtomic.h>

#if BLD_FEATURE_FLOATING_POINT
	#include	<float.h>
	#define __USE_ISOC99 1
	#include	<math.h>
#endif

#endif // MACOSX

#if FREEBSD
	#include	<time.h>
	#include	<arpa/inet.h>
	#include	<ctype.h>
	#include	<dirent.h>
	#include	<dlfcn.h>
	#include	<fcntl.h>
	#include	<grp.h> 
	#include	<errno.h>
	#include	<libgen.h>
	#include	<limits.h>
	#include	<netdb.h>
	#include	<sys/socket.h>
	#include	<net/if.h>
	#include	<netinet/in_systm.h>
	#include	<netinet/in.h>
	#include	<netinet/tcp.h>
	#include	<netinet/ip.h>
	#include	<pthread.h> 
	#include	<pwd.h> 
	#include	<resolv.h>
	#include	<signal.h>
	#include	<stdarg.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<stdint.h>
	#include	<string.h>
	#include	<syslog.h>
	#include	<sys/ioctl.h>
	#include	<sys/types.h>
	#include	<sys/stat.h>
	#include	<sys/param.h>
	#include 	<sys/resource.h>
	#include	<sys/sem.h>
	#include	<sys/shm.h>
	#include	<sys/select.h>
	#include	<sys/time.h>
	#include	<sys/times.h>
	#include	<sys/types.h>
	#include	<sys/utsname.h>
	#include	<sys/wait.h>
	#include	<unistd.h>

#if BLD_FEATURE_FLOATING_POINT
	#include	<float.h>
	#define __USE_ISOC99 1
	#include	<math.h>
#endif

	#define CLD_EXITED 1
	#define CLD_KILLED 2
#endif // FREEBSD

#if WIN
	#include	<ctype.h>
	#include	<conio.h>
	#include	<direct.h>
	#include	<errno.h>
	#include	<fcntl.h>
	#include	<io.h>
	#include	<limits.h>
	#include	<malloc.h>
	#include	<process.h>
	#include	<sys/stat.h>
	#include	<sys/types.h>
	#include	<stddef.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<string.h>
	#include	<stdarg.h>
	#include	<time.h>
	#define WIN32_LEAN_AND_MEAN
	#include	<winsock2.h>
	#include	<windows.h>
	#include	<winbase.h>
	#if BLD_FEATURE_FLOATING_POINT
	#include	<float.h>
	#endif
	#include	<shlobj.h>
	#include	<shellapi.h>
	#include	<wincrypt.h>
#endif // WIN 

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// General Defines ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define	MAXINT			INT_MAX
#define BITS(type)		(BITSPERBYTE * (int) sizeof(type))

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

//
//	Set FD_SETSIZE to the maximum number of files (sockets) that you want to
//	support. It is used in select.cpp.
//
//	#ifdef FD_SETSIZE
//		#undef FD_SETSIZE
//	#endif
//	#define FD_SETSIZE		128
//

typedef char	*MprStr;					// Used for dynamic strings

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Linux Defines ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if CYGWIN || LINUX
#if CYGWIN
	typedef unsigned long ulong;
#endif
	typedef unsigned char uchar;

	__extension__ typedef long long int int64;
	__extension__ typedef unsigned long long int uint64;
	#define INT64(x) (x##LL)

	typedef socklen_t	 	MprSocklen;
	#define	SocketLenPtr	MprSocklen*
	#define closesocket(x)	close(x)
	#define MPR_BINARY		""
	#define MPR_TEXT		""
	#define	SOCKET_ERROR	-1
	#define MPR_DLL_EXT		".so"

#if CYGWIN
	#define __WALL			0
#else
	#define O_BINARY		0
	#define O_TEXT			0
#endif

#if BLD_FEATURE_FLOATING_POINT
	#define MAX_FLOAT		MAXFLOAT
#endif

	#if BLD_FEATURE_MALLOC
		//
		//	PORTERS: You will need add assembler code for your architecture here
		//	only if you want to use the fast malloc (BLD_FEATURE_MALLOC)
		//
		#if UNUSED
			#define MPR_GET_RETURN(ip)	__builtin_return_address(0)
		#else
			#if BLD_HOST_CPU_ARCH == MPR_CPU_IX86
				#define MPR_GET_RETURN(ip)	\
					asm("movl 4(%%ebp),%%eax ; movl %%eax,%0" : \
						"=g" (ip) : \
						: "eax")
			#else
				#define MPR_GET_RETURN(ip)
			#endif
		#endif // UNUSED
	#endif // BLD_FEATURE_MALLOC

#ifndef PTHREAD_MUTEX_RECURSIVE_NP
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

#endif 	// CYGWIN || LINUX 

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// VxWorks Defines ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if VXWORKS

	typedef unsigned char uchar;
	typedef unsigned int uint;
	typedef unsigned long ulong;

//6.3	#define HAVE_SOCKLEN_T
	typedef uint 			MprSocklen;
	#define	SocketLenPtr	int*

	typedef long long int int64;
	typedef unsigned long long int uint64;
	#define INT64(x) (x##LL)

	#define closesocket(x)	close(x)
	#define getpid() 		taskIdSelf()
	#define MPR_BINARY		""
	#define MPR_TEXT		""
	#define O_BINARY		0
	#define O_TEXT			0
	#define	SOCKET_ERROR	-1
	#define MPR_DLL_EXT		".so"

#if BLD_FEATURE_FLOATING_POINT
	#define MAX_FLOAT 		FLT_MAX
#endif

	#undef R_OK
	#define R_OK	4
	#undef W_OK
	#define W_OK	2
	#undef X_OK
	#define X_OK	1
	#undef F_OK
	#define F_OK	0

	#define MSG_NOSIGNAL 0
	
	#ifndef SHUT_WR
	#define SHUT_WR			1
	#endif
	#ifndef SHUT_RD
	#define SHUT_RD			0
	#endif

	extern int sysClkRateGet();

	#if BLD_FEATURE_MALLOC
		//
		//	PORTERS: You will need add assembler code for your architecture here
		//	only if you want to use the fast malloc (BLD_FEATURE_MALLOC)
		//
		#if UNUSED
			#define MPR_GET_RETURN(ip)	__builtin_return_address(0)
		#else
			#if BLD_HOST_CPU_ARCH == MPR_CPU_IX86
				#define MPR_GET_RETURN(ip)	\
					asm("movl 4(%%ebp),%%eax ; movl %%eax,%0" : \
						"=g" (ip) : \
						: "eax")
			#else
				#define MPR_GET_RETURN(ip)
			#endif
		#endif // UNUSED
	#endif // BLD_FEATURE_MALLOC

#if _WRS_VXWORKS_MAJOR < 6
	extern STATUS access(const char *path, int mode);
#else

	#if BLD_HOST_CPU_ARCH == MPR_CPU_PPC
	#define __va_copy(dest, src) *(dest) = *(src)
	#endif
#endif

	#define IOCTL_CAST int
#else
	#define IOCTL_CAST void*
#endif 	// VXWORKS 

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MACOSX Defines ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if MACOSX
	typedef unsigned long ulong;
	typedef unsigned char uchar;

	__extension__ typedef long long int int64;
	__extension__ typedef unsigned long long int uint64;
	#define INT64(x) (x##LL)

	typedef socklen_t	 	MprSocklen;
	#define	SocketLenPtr	MprSocklen*
	#define closesocket(x)	close(x)
	#define MPR_BINARY		""
	#define MPR_TEXT		""
	#define O_BINARY		0
	#define O_TEXT			0
	#define	SOCKET_ERROR	-1
	#define MPR_DLL_EXT		".dylib"
	#define MSG_NOSIGNAL	0
	#define __WALL          0x40000000
	#define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
	#define MAX_FLOAT		MAXFLOAT
#endif

	#if MPR_FEATURE_MALLOC
	//
	//	PORTERS: You will need add assembler code for your architecture here
	//	only if you want to use the fast malloc (MPR_FEATURE_MALLOC)
	//
	#define MPR_GET_RETURN(ip)	__builtin_return_address
	#endif

#endif // MACOSX

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// FREEBSD Defines ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if FREEBSD
	typedef unsigned long ulong;
	typedef unsigned char uchar;

	__extension__ typedef long long int int64;
	__extension__ typedef unsigned long long int uint64;
	#define INT64(x) (x##LL)

	typedef socklen_t	 	MprSocklen;
	#define	SocketLenPtr	MprSocklen*
	#define closesocket(x)	close(x)
	#define MPR_BINARY		""
	#define MPR_TEXT		""
	#define O_BINARY		0
	#define O_TEXT			0
	#define	SOCKET_ERROR	-1
	#define MPR_DLL_EXT		".dylib"
	#define __WALL          0x40000000
	#define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
	#define MAX_FLOAT		MAXFLOAT
#endif

	#if MPR_FEATURE_MALLOC
	//
	//	PORTERS: You will need add assembler code for your architecture here
	//	only if you want to use the fast malloc (MPR_FEATURE_MALLOC)
	//
	#define MPR_GET_RETURN(ip)	__builtin_return_address
	#endif
#endif // FREEBSD

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Windows Defines ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if WIN
	typedef unsigned char uchar;
	typedef unsigned int uint;
	typedef unsigned long ulong;
	typedef unsigned short ushort;

	typedef __int64 int64;
	typedef unsigned __int64 uint64;
	#define INT64(x) (x##i64)

	typedef int 	uid_t;
	typedef void 	*handle;
	typedef char 	*caddr_t;
	typedef long 	pid_t;
	typedef int	 	gid_t;
	typedef ushort 	mode_t;
	typedef void 	*siginfo_t;

	#define HAVE_SOCKLEN_T
	typedef int 			MprSocklen;
	typedef int 			socklen_t;
	#define	SocketLenPtr	MprSocklen*

	/*
 	 *	On windows map X_OK to R_OK
	 */
	#undef R_OK
	#define R_OK	4
	#undef W_OK
	#define W_OK	2
	#undef X_OK
	#define X_OK	R_OK
	#undef F_OK
	#define F_OK	0
	
	#ifndef EADDRINUSE
	#define EADDRINUSE		46
	#endif
	#ifndef EWOULDBLOCK
	#define EWOULDBLOCK		EAGAIN
	#endif
	#ifndef ENETDOWN
	#define ENETDOWN		43
	#endif
	#ifndef ECONNRESET
	#define ECONNRESET		44
	#endif
	#ifndef ECONNREFUSED
	#define ECONNREFUSED	45
	#endif

	#define MSG_NOSIGNAL	0
	#define MPR_BINARY		"b"
	#define MPR_TEXT		"t"

	#define SHUT_WR			SD_SEND
	#define SHUT_RD			SD_RECEIVE

#if BLD_FEATURE_FLOATING_POINT
	#define MAX_FLOAT		DBL_MAX
#endif

#ifndef FILE_FLAG_FIRST_PIPE_INSTANCE
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000
#endif

	#define access 	_access
	#define close 	_close
	#define fileno 	_fileno
	#define fstat 	_fstat
	#define getpid 	_getpid
	#define open 	_open
	#define putenv 	_putenv
	#define read 	_read
	#define stat 	_stat
	#define umask 	_umask
	#define unlink 	_unlink
	#define write 	_write
	#define strdup 	_strdup
	#define lseek 	_lseek
	#define getcwd 	_getcwd
	#ifndef chdir
	#define chdir 	_chdir
	#endif

	#define mkdir(a,b) 	_mkdir(a)
	#define rmdir(a) 	_rmdir(a)

	#if BLD_FEATURE_MALLOC
	//
	//	PORTERS: You will need add assembler code for your architecture here
	//	only if you want to use the fast malloc (BLD_FEATURE_MALLOC)
	//
	#if MPR_CPU_IX86
	#define MPR_GET_RETURN(ip) \
		__asm {	mov	eax, 4[ebp] } \
		__asm {	mov ip, eax	}
	#else
		#define MPR_GET_RETURN(ip)
	#endif
	#endif

	#define MPR_DLL_EXT		".dll"

	extern void		srand48(long);
	extern long		lrand48(void);
	extern long 	ulimit(int, ...);
	extern long 	nap(long);
	extern uint 	sleep(unsigned int secs);
	extern uid_t 	getuid(void);
	extern uid_t 	geteuid(void);

#endif // WIN 

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Solaris Defines ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if SOLARIS
	typedef unsigned char uchar;

	typedef long long int int64;
	typedef unsigned long long int uint64;
	#define INT64(x) (x##LL)

	typedef socklen_t	 	MprSocklen;
	#define	SocketLenPtr	MprSocklen*
	#define closesocket(x)	close(x)
	#define MPR_BINARY		""
	#define MPR_TEXT		""
	#define O_BINARY		0
	#define O_TEXT			0
	#define	SOCKET_ERROR	-1
	#define MPR_DLL_EXT		".so"
	#define MSG_NOSIGNAL	0
	#define INADDR_NONE		((in_addr_t) 0xffffffff)
	#define __WALL	0
	#define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
	#define MAX_FLOAT		MAXFLOAT
#endif

#endif // SOLARIS 

////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif // _h_MPR_OS_HDRS 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
