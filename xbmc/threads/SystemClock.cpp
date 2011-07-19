/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include <stdint.h>

#ifdef __APPLE__
#if defined(__ppc__) || defined(__arm__)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#else
#include <time.h>
#include "posix-realtime-stub.h"
#endif
#elif defined(_LINUX)
#include <time.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace XbmcThreads
{
  unsigned int SystemClockMillis()
  {
    uint64_t now_time;
    static uint64_t start_time = 0;
    static bool start_time_set = false;
#ifdef _LINUX
#if defined(__APPLE__) && (defined(__ppc__) || defined(__arm__))
    now_time = CVGetCurrentHostTime() *  1000 / CVGetHostClockFrequency();
#else
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#endif
#else
    now_time = (uint64_t)timeGetTime();
#endif
    if (!start_time_set)
    {
      start_time = now_time;
      start_time_set = true;
    }
    return (now_time - start_time);
  }
}
