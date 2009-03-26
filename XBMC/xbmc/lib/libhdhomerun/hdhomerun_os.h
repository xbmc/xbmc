/*
 * hdhomerun_os.h
 *
 * Copyright © 2006 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if defined(WIN32)
#define __WINDOWS__
#endif
#if defined(_XBOX)
#include <xtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>
#elif defined(__WINDOWS__)
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>
#else
#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif

#if defined(__WINDOWS__) || defined(_XBOX)

typedef int bool_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#define socklen_t int
#define close closesocket
#define sock_getlasterror WSAGetLastError()
#define sock_getlasterror_socktimeout (WSAGetLastError() == WSAETIMEDOUT)
#define atoll _atoi64
#define strdup _strdup
#define strcasecmp _stricmp
#define snprintf _snprintf
#define fseeko _fseeki64
#define ftello _ftelli64
#define usleep(us) Sleep((us)/1000)
#define sleep(sec) Sleep((sec)*1000)

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

#else

typedef int bool_t;

#define sock_getlasterror errno
#define sock_getlasterror_socktimeout (errno == EAGAIN)

static inline uint64_t getcurrenttime(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((uint64_t)t.tv_sec * 1000) + (t.tv_usec / 1000);
}

static inline int setsocktimeout(int s, int level, int optname, uint64_t timeout)
{
	struct timeval t;
	t.tv_sec = timeout / 1000;
	t.tv_usec = (timeout % 1000) * 1000;
	return setsockopt(s, level, optname, (char *)&t, sizeof(t));
}

#endif

