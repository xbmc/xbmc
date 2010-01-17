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
#ifdef _LINUX
#include <time.h>
#ifdef __APPLE__
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#endif
#elif defined(_WIN32)
#include <windows.h>
#endif

int64_t CurrentHostCounter(void)
{
#if defined(__APPLE__)
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
#if defined(__APPLE__)
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
  frameTime = GetTimeMS();
}

unsigned int CTimeUtils::GetFrameTime()
{
  return frameTime;
}

unsigned int CTimeUtils::GetTimeMS()
{
  // best replacement for windows timeGetTime/GetTickCount
  // 1st call sets start_mstime, subsequent are the diff
  // between start_mstime and now_mstime to match SDL_GetTick behavior
  // of previous usage. We might want to change this as CTimeUtils::GetTimeMS is
  // time (ms) since system startup.
#if defined(_WIN32)
  return timeGetTime();
#elif defined(_LINUX)
#if defined(__APPLE__)
  static uint64_t start_time = 0;
  uint64_t now_time;

  now_time = CVGetCurrentHostTime() * 1000 / CVGetHostClockFrequency();
  if (start_time == 0)
    start_time = now_time;

  return(now_time - start_time);

  /*
  static long double cv;
  static uint64_t start_time = 0;
  uint64_t now_time;

  now_time = mach_absolute_time();

  if (start_time == 0)
  {
    mach_timebase_info_data_t tbinfo;

    mach_timebase_info(&tbinfo);
    cv = ((long double) tbinfo.numer) / ((long double) tbinfo.denom);
    start_time = now_time;
  }

  return( (now_time - start_time) * cv / 1000000.0);
  */
#else
  static uint64_t start_mstime = 0;
  uint64_t now_mstime;
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  now_mstime = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
  if (start_mstime == 0)
  {
    start_mstime = now_mstime;
  }

  return(now_mstime - start_mstime);
#endif
#endif
}

