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

#include "TimeUtils.h"
#include "XBDateTime.h"
#include "threads/SystemClock.h"
#include "guilib/GraphicContext.h"

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#if   defined(TARGET_DARWIN)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#elif defined(TARGET_WINDOWS)
#include <windows.h>
#else
#include <time.h>
#endif

#include "TimeSmoother.h"

int64_t CurrentHostCounter(void)
{
#if   defined(TARGET_DARWIN)
  return( (int64_t)CVGetCurrentHostTime() );
#elif defined(TARGET_WINDOWS)
  LARGE_INTEGER PerformanceCount;
  QueryPerformanceCounter(&PerformanceCount);
  return( (int64_t)PerformanceCount.QuadPart );
#else
  struct timespec now;
#ifdef CLOCK_MONOTONIC_RAW
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#else
  clock_gettime(CLOCK_MONOTONIC, &now);
#endif // CLOCK_MONOTONIC_RAW
  return( ((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec );
#endif
}

int64_t CurrentHostFrequency(void)
{
#if defined(TARGET_DARWIN)
  return( (int64_t)CVGetHostClockFrequency() );
#elif defined(TARGET_WINDOWS)
  LARGE_INTEGER Frequency;
  QueryPerformanceFrequency(&Frequency);
  return( (int64_t)Frequency.QuadPart );
#else
  return( (int64_t)1000000000L );
#endif
}

CTimeSmoother CTimeUtils::frameTimer;
unsigned int CTimeUtils::frameTime = 0;

void CTimeUtils::UpdateFrameTime(bool flip, bool vsync)
{
  unsigned int currentTime = XbmcThreads::SystemClockMillis();
  if (vsync)
  {
    unsigned int last = frameTime;
    while (frameTime < currentTime)
    {
      frameTime += (unsigned int)(1000 / g_graphicsContext.GetFPS());
      // observe wrap around
      if (frameTime < last)
        break;
    }
  }
  else
  {
    if (flip)
      frameTimer.AddTimeStamp(currentTime);
    frameTime = frameTimer.GetNextFrameTime(currentTime);
  }
}

unsigned int CTimeUtils::GetFrameTime()
{
  return frameTime;
}

CDateTime CTimeUtils::GetLocalTime(time_t time)
{
  CDateTime result;

  tm *local;
#ifdef HAVE_LOCALTIME_R
  tm res = {};
  local = localtime_r(&time, &res); // Conversion to local time
#else
  local = localtime(&time); // Conversion to local time
#endif
  /*
   * Microsoft implementation of localtime returns NULL if on or before epoch.
   * http://msdn.microsoft.com/en-us/library/bf12f0hc(VS.80).aspx
   */
  if (local)
    result = *local;
  else
    result = time; // Use the original time as close enough.

  return result;
}
