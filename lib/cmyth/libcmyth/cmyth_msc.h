/*
 *  Copyright (C) 2012, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file cmyth_msc.h
 * Contain most of the Microsoft related differences in a single file.
 */

#ifndef __CMYTH_MSC_H
#define __CMYTH_MSC_H

#if !defined(_MSC_VER)
#error This file may only be included on windows builds!
#endif /* !_MSC_VER */

#include <malloc.h>
#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma warning(disable:4267)
#pragma warning(disable:4996)

#define pthread_mutex_lock(a)
#define pthread_mutex_unlock(a)
#define PTHREAD_MUTEX_INITIALIZER NULL;
typedef void *pthread_mutex_t;

#undef ECANCELED
#undef ETIMEDOUT

#define ECANCELED -1
#define ETIMEDOUT -1
#define SHUT_RDWR SD_BOTH

typedef SOCKET cmyth_socket_t;
typedef int socklen_t;

#define snprintf _snprintf
#define sleep(a) Sleep(a*1000)
#define usleep(a) Sleep(a/1000)

static inline struct tm *localtime_r(const time_t * clock, struct tm *result)
{
	struct tm *data;
	if (!clock || !result)
		return NULL;
	data = localtime(clock);
	if (!data)
		return NULL;
	memcpy(result, data, sizeof(*result));
	return result;
}

static inline __int64 atoll(const char *s)
{
	__int64 value;
	if (sscanf(s, "%I64d", &value))
		return value;
	else
		return 0;
}

#endif /* __CMYTH_MSC_H */
