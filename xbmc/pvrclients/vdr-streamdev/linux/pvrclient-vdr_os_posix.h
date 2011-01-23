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

#ifndef PVRCLIENT_VDR_OS_POSIX_H
#define PVRCLIENT_VDR_OS_POSIX_H

#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/resource.h>
#ifndef __APPLE__
#include <sys/prctl.h> 
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <poll.h>

typedef int bool_t;
typedef int SOCKET;

#define __close(a) close(a)
#define __select select
#define __recv recv
#define __shutdown shutdown
#define __socket socket
#define __bind bind
#define __getsockname getsockname
#define __connect connect
#define __getpeername getpeername
#define __send send
#define __getsockopt getsockopt
#define __listen listen
#define __accept accept
#define __setsockopt setsockopt
#define __fcntl fcntl
#define __gethostbyname gethostbyname
#define __read read
#define __write write
#define __poll poll
#define SOCKET_ERROR   (-1)
#define INVALID_SOCKET (-1)
#define SD_BOTH SHUT_RDWR

#define LIBTYPE
#define sock_getlasterror errno
#define sock_getlasterror_socktimeout (errno == EAGAIN)
#define console_vprintf vprintf
#define console_printf printf
#define THREAD_FUNC_PREFIX void *

#ifndef __STL_CONFIG_H
template<class T> inline T min(T a, T b) { return a <= b ? a : b; }
template<class T> inline T max(T a, T b) { return a >= b ? a : b; }
template<class T> inline int sgn(T a) { return a < 0 ? -1 : a > 0 ? 1 : 0; }
template<class T> inline void swap(T &a, T &b) { T t = a; a = b; b = t; }
#endif

#define Sleep(t) usleep(t*1000)

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
