#pragma once
/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "../os.h"

#if defined(__APPLE__)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#elif defined(__WINDOWS__)
#include <time.h>
#else
#include <sys/time.h>
#endif

namespace PLATFORM
{
  #if defined(__WINDOWS__)
  struct timezone
  {
    int	tz_minuteswest;
    int	tz_dsttime;
  };

  #define usleep(t) Sleep((DWORD)(t)/1000)

  inline int gettimeofday(struct timeval *pcur_time, struct timezone *tz)
  {
    if (pcur_time == NULL)
    {
      SetLastError(EFAULT);
      return -1;
    }
    struct _timeb current;

    _ftime(&current);

    pcur_time->tv_sec = (long) current.time;
    pcur_time->tv_usec = current.millitm * 1000L;
    if (tz)
    {
      tz->tz_minuteswest = current.timezone;	/* minutes west of Greenwich  */
      tz->tz_dsttime = current.dstflag;	      /* type of dst correction  */
    }
    return 0;
  }
  #endif

  inline int64_t GetTimeMs()
  {
  #if defined(__APPLE__)
    return (int64_t) (CVGetCurrentHostTime() * 1000 / CVGetHostClockFrequency());
  #elif defined(__WINDOWS__)
    time_t rawtime;
    time(&rawtime);

    LARGE_INTEGER tickPerSecond;
    LARGE_INTEGER tick;
    if (QueryPerformanceFrequency(&tickPerSecond))
    {
      QueryPerformanceCounter(&tick);
      return (int64_t) (tick.QuadPart / 1000.);
    }
    return -1;
  #else
    timeval time;
    gettimeofday(&time, NULL);
    return (int64_t) (time.tv_sec * 1000 + time.tv_usec / 1000);
  #endif
  }

  template <class T>
  inline T GetTimeSec()
  {
    return (T)GetTimeMs() / (T)1000.0;
  }
};
