#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef PVRCLIENT_VDR_OS_WIN_H
#define PVRCLIENT_VDR_OS_WIN_H

#if defined _FILE_OFFSET_BITS && _FILE_OFFSET_BITS == 64
# define __USE_FILE_OFFSET64	1
#endif

//typedef int ssize_t;
typedef int mode_t;
typedef int bool_t;
typedef __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#if defined __USE_FILE_OFFSET64
typedef int64_t off_t;
typedef uint64_t ino_t;
#else
typedef long off_t;
#endif

#define NAME_MAX         255   /* # chars in a file name */
#define MAXPATHLEN       255
#define INT64_MAX _I64_MAX
#define INT64_MIN _I64_MIN

#ifndef S_ISLNK
# define S_ISLNK(x) 0
#endif

#ifndef S_ISREG
#define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

/* Some tricks for MS Compilers */
#define THREADLOCAL __declspec(thread)

#ifndef DEFFILEMODE
#define DEFFILEMODE 0
#endif

#define alloca _alloca
#define chdir _chdir
#define dup _dup
#define dup2 _dup2
#define fdopen _fdopen
#define fileno _fileno
#define getcwd _getcwd
#define getpid _getpid
#define ioctl ioctlsocket
#define mkdir(p) _mkdir(p)
#define mktemp _mktemp
#define open _open
#define pclose _pclose
#define popen _popen
#define putenv _putenv
#define setmode _setmode
#define sleep(t) Sleep((t)*1000)
#define usleep(t) Sleep((t)/1000)
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strdup _strdup
#define strlwr _strlwr
#define strncasecmp _strnicmp
#define tempnam _tempnam
#define umask _umask
#define unlink _unlink
#define close _close

#define O_RDONLY        _O_RDONLY
#define O_WRONLY        _O_WRONLY
#define O_RDWR          _O_RDWR
#define O_APPEND        _O_APPEND

#define O_CREAT         _O_CREAT
#define O_TRUNC         _O_TRUNC
#define O_EXCL          _O_EXCL

#define O_TEXT          _O_TEXT
#define O_BINARY        _O_BINARY
#define O_RAW           _O_BINARY
#define O_TEMPORARY     _O_TEMPORARY
#define O_NOINHERIT     _O_NOINHERIT
#define O_SEQUENTIAL    _O_SEQUENTIAL
#define O_RANDOM        _O_RANDOM
#define O_NDELAY	0

#define S_IRWXO 007
//#define	S_ISDIR(m) (((m) & _S_IFDIR) == _S_IFDIR)
//#define	S_ISREG(m) (((m) & _S_IFREG) == _S_IFREG)

#ifndef SIGHUP
#define	SIGHUP	1	/* hangup */
#endif
#ifndef SIGBUS
#define	SIGBUS  7	/* bus error */
#endif
#ifndef SIGKILL
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#endif
#ifndef	SIGSEGV
#define	SIGSEGV 11      /* segment violation */
#endif
#ifndef SIGPIPE
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#endif
#ifndef SIGCHLD
#define	SIGCHLD	20	/* to parent on child stop or exit */
#endif
#ifndef SIGUSR1
#define SIGUSR1 30	/* user defined signal 1 */
#endif
#ifndef SIGUSR2
#define SIGUSR2 31	/* user defined signal 2 */
#endif

typedef unsigned short in_port_t;
typedef unsigned short int ushort;
typedef unsigned int in_addr_t;
typedef int socklen_t;
typedef int uid_t;
typedef int gid_t;

#if defined __USE_FILE_OFFSET64
#define stat _stati64
#define lseek _lseeki64
#define fstat _fstati64
#define tell _telli64
#else
#define stat _stat
#define lseek _lseek
#define fstat _fstat
#define tell _tell
#endif

#define atoll _atoi64
#ifndef va_copy
#define va_copy(x, y) x = y
#endif

#include <stddef.h>
#include <process.h>
#if defined(_MSC_VER) /* Microsoft C Compiler ONLY */
#pragma warning (push)
/* Hack to suppress compiler warnings on FD_SET() & FD_CLR() */
#pragma warning (disable:4142)
/* Suppress compiler warnings about double definition of _WINSOCKAPI_ */
#pragma warning (disable:4005)
#endif
/* prevent inclusion of wingdi.h */
#define NOGDI
#include <ws2spi.h>
#include <ws2ipdef.h>
#if defined(_MSC_VER) /* Microsoft C Compiler ONLY */
#pragma warning (pop)
#endif
#include <io.h>
#include <stdlib.h>
#include <errno.h>
#include "pthread_win32/pthread.h"

typedef char * caddr_t;

#undef FD_CLOSE
#undef FD_OPEN
#undef FD_READ
#undef FD_WRITE

#if (_MSC_VER < 1600)
// Not yet defined in errno.h under VS2008
#define EISCONN WSAEISCONN
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EALREADY WSAEALREADY
#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif
#define ECONNABORTED WSAECONNABORTED
#define ECONNREFUSED WSAECONNREFUSED
#define ECONNRESET WSAECONNRESET
#define ERESTART WSATRY_AGAIN
#define ENOTCONN WSAENOTCONN
#define ENOBUFS WSAENOBUFS
#define EOVERFLOW 2006
#endif

#undef h_errno
#define h_errno errno /* we'll set it ourselves */

struct timezone
{
  int	tz_minuteswest;	/* minutes west of Greenwich */
  int	tz_dsttime;	/* type of dst correction */
};

extern int gettimeofday(struct timeval *, struct timezone *);

/* Unix socket emulation macros */
#define __close closesocket

#undef FD_CLR
#define FD_CLR(fd, set) do { \
    u_int __i; \
    SOCKET __sock = _get_osfhandle(fd); \
    for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count ; __i++) { \
        if (((fd_set FAR *)(set))->fd_array[__i] == __sock) { \
            while (__i < ((fd_set FAR *)(set))->fd_count-1) { \
                ((fd_set FAR *)(set))->fd_array[__i] = \
                    ((fd_set FAR *)(set))->fd_array[__i+1]; \
                __i++; \
            } \
            ((fd_set FAR *)(set))->fd_count--; \
            break; \
        } \
    } \
} while(0)

#undef FD_SET
#define FD_SET(fd, set) do { \
    u_int __i; \
    SOCKET __sock = _get_osfhandle(fd); \
    for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
        if (((fd_set FAR *)(set))->fd_array[__i] == (__sock)) { \
            break; \
        } \
    } \
    if (__i == ((fd_set FAR *)(set))->fd_count) { \
        if (((fd_set FAR *)(set))->fd_count < FD_SETSIZE) { \
            ((fd_set FAR *)(set))->fd_array[__i] = (__sock); \
            ((fd_set FAR *)(set))->fd_count++; \
        } \
    } \
} while(0)

#undef FD_ISSET
#define FD_ISSET(fd, set) __WSAFDIsSet((SOCKET)(_get_osfhandle(fd)), (fd_set FAR *)(set))

extern THREADLOCAL int ws32_result;
#define __poll(f,n,t) \
	(SOCKET_ERROR == WSAPoll(f,n,t) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __socket(f,t,p) \
	(INVALID_SOCKET == ((SOCKET)(ws32_result = (int)socket(f,t,p))) ? \
	((WSAEMFILE == (errno = WSAGetLastError()) ? errno = EMFILE : -1), -1) : \
	(SOCKET)_open_osfhandle(ws32_result,0))
#define __accept(s,a,l) \
	(INVALID_SOCKET == ((SOCKET)(ws32_result = (int)accept(_get_osfhandle(s),a,l))) ? \
	((WSAEMFILE == (errno = WSAGetLastError()) ? errno = EMFILE : -1), -1) : \
	(SOCKET)_open_osfhandle(ws32_result,0))
#define __bind(s,n,l) \
	(SOCKET_ERROR == bind(_get_osfhandle(s),n,l) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __connect(s,n,l) \
	(SOCKET_ERROR == connect(_get_osfhandle(s),n,l) ? \
	(WSAEMFILE == (errno = WSAGetLastError()) ? errno = EMFILE : -1, -1) : 0)
#define __listen(s,b) \
	(SOCKET_ERROR == listen(_get_osfhandle(s),b) ? \
	(WSAEMFILE == (errno = WSAGetLastError()) ? errno = EMFILE : -1, -1) : 0)
#define __shutdown(s,h) \
	(SOCKET_ERROR == shutdown(_get_osfhandle(s),h) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __select(n,r,w,e,t) \
	(SOCKET_ERROR == (ws32_result = select(n,r,w,e,t)) ? \
	(errno = WSAGetLastError()), -1 : ws32_result)
#define __recv(s,b,l,f) \
	(SOCKET_ERROR == (ws32_result = recv(_get_osfhandle(s),b,l,f)) ? \
  (errno = WSAGetLastError()), -1 : ws32_result)
#define __recvfrom(s,b,l,f,fr,frl) \
	(SOCKET_ERROR == (ws32_result = recvfrom(_get_osfhandle(s),b,l,f,fr,frl)) ? \
	(errno = WSAGetLastError()), -1 : ws32_result)
#define __send(s,b,l,f) \
	(SOCKET_ERROR == (ws32_result = send(_get_osfhandle(s),b,l,f)) ? \
	(errno = WSAGetLastError()), -1 : ws32_result)
#define __sendto(s,b,l,f,t,tl) \
	(SOCKET_ERROR == (ws32_result = sendto(_get_osfhandle(s),b,l,f,t,tl)) ? \
	(errno = WSAGetLastError()), -1 : ws32_result)
#define __getsockname(s,n,l) \
	(SOCKET_ERROR == getsockname(_get_osfhandle(s),n,l) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __getpeername(s,n,l) \
	(SOCKET_ERROR == getpeername(_get_osfhandle(s),n,l) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __getsockopt(s,l,o,v,n) \
	(Sleep(1), SOCKET_ERROR == getsockopt(_get_osfhandle(s),l,o,(char*)v,n) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __setsockopt(s,l,o,v,n) \
	(SOCKET_ERROR == setsockopt(_get_osfhandle(s),l,o,v,n) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __ioctlsocket(s,c,a) \
	(SOCKET_ERROR == ioctlsocket(_get_osfhandle(s),c,a) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __gethostname(n,l) \
	(SOCKET_ERROR == gethostname(n,l) ? \
	(errno = WSAGetLastError()), -1 : 0)
#define __gethostbyname(n) \
	(NULL == ((HOSTENT FAR*)(ws32_result = (int)gethostbyname(n))) ? \
	(errno = WSAGetLastError()), NULL : (HOSTENT FAR*)ws32_result)
#define __getservbyname(n,p) \
	(NULL == ((SERVENT FAR*)(ws32_result = (int)getservbyname(n,p))) ? \
	(errno = WSAGetLastError()), NULL : (SERVENT FAR*)ws32_result)
#define __gethostbyaddr(a,l,t) \
	(NULL == ((HOSTENT FAR*)(ws32_result = (int)gethostbyaddr(a,l,t))) ? \
	(errno = WSAGetLastError()), NULL : (HOSTENT FAR*)ws32_result)
extern THREADLOCAL int _so_err;
extern THREADLOCAL int _so_err_siz;
#define __read(fd,buf,siz) \
	(_so_err_siz = sizeof(_so_err), \
	__getsockopt((fd),SOL_SOCKET,SO_ERROR,&_so_err,&_so_err_siz) \
	== 0 ? __recv((fd),(char *)(buf),(siz),0) : _read((fd),(char *)(buf),(siz)))
#define __write(fd,buf,siz) \
	(_so_err_siz = sizeof(_so_err), \
	__getsockopt((fd),SOL_SOCKET,SO_ERROR,&_so_err,&_so_err_siz) \
	== 0 ? __send((fd),(const char *)(buf),(siz),0) : _write((fd),(const char *)(buf),(siz)))


#if !defined(__MINGW32__)
#define strtok_r( _s, _sep, _lasts ) \
	( *(_lasts) = strtok( (_s), (_sep) ) )
#endif /* !__MINGW32__ */

#define asctime_r( _tm, _buf ) \
	( strcpy( (_buf), asctime( (_tm) ) ), \
	  (_buf) )

#define ctime_r( _clock, _buf ) \
	( strcpy( (_buf), ctime( (_clock) ) ),  \
	  (_buf) )

#define gmtime_r( _clock, _result ) \
	( *(_result) = *gmtime( (_clock) ), \
	  (_result) )

#define localtime_r( _clock, _result ) \
	( *(_result) = *localtime( (_clock) ), \
	  (_result) )

#define rand_r( _seed ) \
	( _seed == _seed? rand() : rand() )

#endif
