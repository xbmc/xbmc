/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TimeUtils.h"
#include "XBDateTime.h"
#include "threads/SystemClock.h"
#include "windowing/GraphicContext.h"

#if   defined(TARGET_DARWIN)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#elif defined(TARGET_WINDOWS)
#include <windows.h>
#else
#include <time.h>
#endif

int64_t CurrentHostCounter(void)
{
#if defined(TARGET_DARWIN)
  return( (int64_t)CVGetCurrentHostTime() );
#elif defined(TARGET_WINDOWS)
  LARGE_INTEGER PerformanceCount;
  QueryPerformanceCounter(&PerformanceCount);
  return( (int64_t)PerformanceCount.QuadPart );
#else
  struct timespec now;
#if defined(CLOCK_MONOTONIC_RAW) && !defined(TARGET_ANDROID)
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#else
  clock_gettime(CLOCK_MONOTONIC, &now);
#endif // CLOCK_MONOTONIC_RAW && !TARGET_ANDROID
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

unsigned int CTimeUtils::frameTime = 0;

void CTimeUtils::UpdateFrameTime(bool flip)
{
  unsigned int currentTime = XbmcThreads::SystemClockMillis();
  unsigned int last = frameTime;
  while (frameTime < currentTime)
  {
    frameTime += (unsigned int)(1000 / CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS());
    // observe wrap around
    if (frameTime < last)
      break;
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

std::string CTimeUtils::WithoutSeconds(const std::string& hhmmss)
{
  return hhmmss.substr(0, 5);
}
