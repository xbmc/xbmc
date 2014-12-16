/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>

#if   defined(TARGET_DARWIN)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#elif defined(TARGET_WINDOWS)
#include <windows.h>
#else
#include <time.h>
#endif
#include "SystemClock.h"

namespace XbmcThreads
{
  unsigned int SystemClockMillis()
  {
    uint64_t now_time;
    static uint64_t start_time = 0;
    static bool start_time_set = false;
#if defined(TARGET_DARWIN)
    now_time = CVGetCurrentHostTime() *  1000 / CVGetHostClockFrequency();
#elif defined(TARGET_WINDOWS)
    now_time = (uint64_t)timeGetTime();
#else
    struct timespec ts = {};
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif // CLOCK_MONOTONIC_RAW
    now_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#endif
    if (!start_time_set)
    {
      start_time = now_time;
      start_time_set = true;
    }
    return (unsigned int)(now_time - start_time);
  }
  const unsigned int EndTime::InfiniteValue = std::numeric_limits<unsigned int>::max();
}
