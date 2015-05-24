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

#include "Stopwatch.h"
#if defined(TARGET_POSIX)
#include "threads/SystemClock.h" 
#if !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD)
#include <sys/sysinfo.h>
#endif
#endif
#include "utils/TimeUtils.h"

CStopWatch::CStopWatch(bool useFrameTime /*=false*/)
{
  m_timerPeriod      = 0.0f;
  m_startTick        = 0;
  m_stopTick         = 0;
  m_isRunning        = false;
  m_useFrameTime     = useFrameTime;

#ifdef TARGET_POSIX
  m_timerPeriod = 1.0f / 1000.0f; // we want seconds
#else
  if (m_useFrameTime)
    m_timerPeriod = 1.0f / 1000.0f; //frametime is in milliseconds
  else
    m_timerPeriod = 1.0f / (float)CurrentHostFrequency();
#endif
}

CStopWatch::~CStopWatch()
{
}

int64_t CStopWatch::GetTicks() const
{
  if (m_useFrameTime)
    return CTimeUtils::GetFrameTime();
#ifndef TARGET_POSIX
  return CurrentHostCounter();
#else
  return XbmcThreads::SystemClockMillis();
#endif
}
