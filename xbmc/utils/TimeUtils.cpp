/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "TimeUtils.h"
#include "XBDateTime.h"
#include "threads/SystemClock.h"

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

int64_t CurrentHostCounter(void)
{
#if defined(__APPLE__) && (defined(__ppc__) || defined(__arm__))
  return( (int64_t)CVGetCurrentHostTime() );
#elif defined(_LINUX)
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return( ((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec );
#else
  LARGE_INTEGER PerformanceCount;
  QueryPerformanceCounter(&PerformanceCount);
  return( (int64_t)PerformanceCount.QuadPart );
#endif
}

int64_t CurrentHostFrequency(void)
{
#if defined(__APPLE__) && (defined(__ppc__) || defined(__arm__))
  // needed for 10.5.8 on ppc
  return( (int64_t)CVGetHostClockFrequency() );
#elif defined(_LINUX)
  return( (int64_t)1000000000L );
#else
  LARGE_INTEGER Frequency;
  QueryPerformanceFrequency(&Frequency);
  return( (int64_t)Frequency.QuadPart );
#endif
}

unsigned int CTimeUtils::frameTime = 0;

void CTimeUtils::UpdateFrameTime()
{
  frameTime = XbmcThreads::SystemClockMillis();
}

unsigned int CTimeUtils::GetFrameTime()
{
  return frameTime;
}

CDateTime CTimeUtils::GetLocalTime(time_t time)
{
  CDateTime result;

  tm *local = localtime(&time); // Conversion to local time
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
