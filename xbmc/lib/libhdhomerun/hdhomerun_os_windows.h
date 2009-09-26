/*
 * hdhomerun_os_windows.h
 *
 * Copyright © 2006-2008 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * As a special exception to the GNU Lesser General Public License,
 * you may link, statically or dynamically, an application with a
 * publicly distributed version of the Library to produce an
 * executable file containing portions of the Library, and
 * distribute that executable file under terms of your choice,
 * without any of the additional requirements listed in clause 4 of
 * the GNU Lesser General Public License.
 * 
 * By "a publicly distributed version of the Library", we mean
 * either the unmodified Library as distributed by Silicondust, or a
 * modified version of the Library that is distributed under the
 * conditions defined in the GNU Lesser General Public License.
 */

#define _WINSOCKAPI_
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#ifdef __MINGW__
extern 
int WSAAPI getaddrinfo(
  const char *nodename,
  const char *servname,
  const struct addrinfo *hints,
  struct addrinfo **res
);

extern
void WSAAPI freeaddrinfo(
  struct addrinfo *ai
);
#else
#include <wspiapi.h>
#endif


#if defined(DLL_IMPORT)
#define LIBTYPE __declspec( dllexport )
#elif  defined(DLL_EXPORT)
#define LIBTYPE __declspec( dllimport )
#else
#define LIBTYPE
#endif

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

#define socklen_t int
#define close closesocket
#define sock_getlasterror WSAGetLastError()
#define sock_getlasterror_socktimeout (WSAGetLastError() == WSAETIMEDOUT)
#ifndef va_copy
#define va_copy(x, y) x = y
#endif
#define atoll _atoi64
#define strdup _strdup
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#define snprintf _snprintf
#define fseeko _fseeki64
#define ftello _ftelli64
#define THREAD_FUNC_PREFIX DWORD WINAPI
#define SIGPIPE SIGABRT

static inline int msleep(unsigned int ms)
{
	Sleep(ms);
	return 0;
}

static inline int sleep(unsigned int sec)
{
	Sleep(sec * 1000);
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
