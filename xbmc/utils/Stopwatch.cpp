/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
