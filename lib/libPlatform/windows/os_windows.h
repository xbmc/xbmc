#pragma once
/*
 *      Copyright (C) 2011 Team XBMC
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
 
#pragma warning(disable:4005) // Disable "warning C4005: '_WINSOCKAPI_' : macro redefinition"
#include <winsock2.h>
#pragma warning(default:4005)

#include "../pthread_win32/pthread.h"

#if defined _FILE_OFFSET_BITS && _FILE_OFFSET_BITS == 64
# define __USE_FILE_OFFSET64	1
#endif

typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#if defined __USE_FILE_OFFSET64
typedef int64_t off_t;
typedef uint64_t ino_t;
#endif

#define usleep(t) Sleep((t)/1000)
#define snprintf _snprintf

#include <stddef.h>
#include <process.h>
#if defined(_MSC_VER) /* Microsoft C Compiler ONLY */
#pragma warning (push)
#endif
/* prevent inclusion of wingdi.h */
#define NOGDI
#if defined(_MSC_VER) /* Microsoft C Compiler ONLY */
#pragma warning (pop)
#endif
#include <io.h>
#include <stdlib.h>
#include <errno.h>

struct timezone
{
  int	tz_minuteswest;	/* minutes west of Greenwich */
  int	tz_dsttime;	/* type of dst correction */
};

/*!
	\brief	implements similar unix call under windows
	\return		0 on success, -1 on failure (if pcur_time was NULL)
	\param		pcur_time points to a timeval structure, should not be NULL
	\param		tz points to a timezone structure, may be NULL
 */
extern int gettimeofday(struct timeval *pcur_time, struct timezone *tz);
