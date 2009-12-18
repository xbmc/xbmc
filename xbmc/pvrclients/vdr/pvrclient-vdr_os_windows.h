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

#include <process.h>
#include <io.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#if defined(DLL_IMPORT)
#define LIBTYPE __declspec( dllexport )
#elif  defined(DLL_EXPORT)
#define LIBTYPE __declspec( dllimport )
#else
#define LIBTYPE
#endif

#define strncasecmp strnicmp

typedef int bool_t;
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef HANDLE pthread_t;
typedef HANDLE pthread_mutex_t;
typedef LONG_PTR ssize_t;

#define socklen_t int
#define close closesocket
#define sock_getlasterror WSAGetLastError()
#define sock_getlasterror_socktimeout (WSAGetLastError() == WSAETIMEDOUT)
#ifndef va_copy
#define va_copy(x, y) x = y
#endif
#define atoll _atoi64
#define strdup _strdup
#define strcasecmp _stricmp
#define snprintf _snprintf
#define fseeko _fseeki64
#define ftello _ftelli64
#define THREAD_FUNC_PREFIX DWORD WINAPI
#define SIGPIPE SIGABRT
#define read _read
#define write _write
#define poll WSAPoll

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
 
struct timespec
{
  long tv_sec;
  long tv_nsec;
};

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

__inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tmpres /= 10;  /*convert into microseconds*/
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
}

static inline int usleep(unsigned int us)
{
	Sleep((us)/1000);
	return 0;
}

static inline int sleep(unsigned int sec)
{
	Sleep((sec)*1000);
	return 0;
}

static inline uint64_t getcurrenttime(void)
{
	struct timeb tb;
	ftime(&tb);
	return ((uint64_t)tb.time * 1000) + tb.millitm;
}

static inline int setsocktimeout(int s, int level, int optname, uint64_t timeout)
{
	int t = (int)timeout;
	return setsockopt(s, level, optname, (char *)&t, sizeof(t));
}

static inline int pthread_create(pthread_t *tid, void *attr, LPTHREAD_START_ROUTINE start, void *arg)
{
	*tid = CreateThread(NULL, 0, start, arg, 0, NULL);
	if (!*tid) {
		return (int)GetLastError();
	}
	return 0;
}

static inline int pthread_join(pthread_t tid, void **value_ptr)
{
	while (1) {
		DWORD ExitCode = 0;
		if (!GetExitCodeThread(tid, &ExitCode)) {
			return (int)GetLastError();
		}
		if (ExitCode != STILL_ACTIVE) {
			return 0;
		}
	}
}

static inline void pthread_mutex_init(pthread_mutex_t *mutex, void *attr)
{
	*mutex = CreateMutex(NULL, FALSE, NULL);
}

static inline void pthread_mutex_lock(pthread_mutex_t *mutex)
{
	WaitForSingleObject(*mutex, INFINITE);
}

static inline void pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	ReleaseMutex(*mutex);
}

/*
 * The console output format should be set to UTF-8, however in XP and Vista this breaks batch file processing.
 * Attempting to restore on exit fails to restore if the program is terminated by the user.
 * Solution - set the output format each printf.
 */
static inline void console_vprintf(const char *fmt, va_list ap)
{
	UINT cp = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
	vprintf(fmt, ap);
	SetConsoleOutputCP(cp);
}

static inline void console_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	console_vprintf(fmt, ap);
	va_end(ap);
}

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
