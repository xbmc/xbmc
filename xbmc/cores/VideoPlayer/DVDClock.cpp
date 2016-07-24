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

#include "DVDClock.h"
#include "video/VideoReferenceClock.h"
#include <math.h>
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <inttypes.h>

CDVDClock::CDVDClock()
{
  CSingleLock lock(m_systemsection);

  m_pauseClock = 0;
  m_bReset = true;
  m_paused = false;
  m_iDisc = 0;
  m_maxspeedadjust = 0.0;
  m_systemAdjust = 0;
  m_speedAdjust = 0;
  m_startClock = 0;
  m_vSyncAdjust = 0;
  m_frameTime = DVD_TIME_BASE / 60.0;

  m_videoRefClock.reset(new CVideoReferenceClock());
  m_lastSystemTime = m_videoRefClock->GetTime();
  m_systemOffset = m_videoRefClock->GetTime();
  m_systemFrequency = CurrentHostFrequency();
  m_systemUsed = m_systemFrequency;
}

CDVDClock::~CDVDClock()
{
}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
double CDVDClock::GetAbsoluteClock(bool interpolated /*= true*/)
{
  CSingleLock lock(m_systemsection);

  int64_t current;
  current = m_videoRefClock->GetTime(interpolated);

  return SystemToAbsolute(current);
}

double CDVDClock::GetClock(bool interpolated /*= true*/)
{
  CSingleLock lock(m_critSection);

  int64_t current = m_videoRefClock->GetTime(interpolated);
  m_systemAdjust += m_speedAdjust * (current - m_lastSystemTime);
  m_lastSystemTime = current;

  return SystemToPlaying(current);
}

double CDVDClock::GetClock(double& absolute, bool interpolated /*= true*/)
{
  int64_t current = m_videoRefClock->GetTime(interpolated);

  CSingleLock lock(m_systemsection);
  absolute = SystemToAbsolute(current);

  m_systemAdjust += m_speedAdjust * (current - m_lastSystemTime);
  m_lastSystemTime = current;

  return SystemToPlaying(current);
}

void CDVDClock::SetVsyncAdjust(double adjustment)
{
  CSingleLock lock(m_critSection);
  m_vSyncAdjust = adjustment;
}

double CDVDClock::GetVsyncAdjust()
{
  CSingleLock lock(m_critSection);
  return m_vSyncAdjust;
}

void CDVDClock::Pause(bool pause)
{
  CSingleLock lock(m_critSection);

  if (pause && !m_paused)
  {
    if (!m_pauseClock)
      m_speedAfterPause = m_systemFrequency * DVD_PLAYSPEED_NORMAL / m_systemUsed;
    else
      m_speedAfterPause = DVD_PLAYSPEED_PAUSE;

    SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_paused = true;
  }
  else if (!pause && m_paused)
  {
    m_paused = false;
    SetSpeed(m_speedAfterPause);
  }
}

void CDVDClock::SetSpeed(int iSpeed)
{
  // this will sometimes be a little bit of due to rounding errors, ie clock might jump abit when changing speed
  CSingleLock lock(m_critSection);

  if (m_paused)
  {
    m_speedAfterPause = iSpeed;
    return;
  }

  if (iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    if (!m_pauseClock)
      m_pauseClock = m_videoRefClock->GetTime();
    return;
  }

  int64_t current;
  int64_t newfreq = m_systemFrequency * DVD_PLAYSPEED_NORMAL / iSpeed;

  current = m_videoRefClock->GetTime();
  if (m_pauseClock)
  {
    m_startClock += current - m_pauseClock;
    m_pauseClock = 0;
  }

  m_startClock = current - (int64_t)((double)(current - m_startClock) * newfreq / m_systemUsed);
  m_systemUsed = newfreq;
}

void CDVDClock::SetSpeedAdjust(double adjust)
{
  CSingleLock lock(m_critSection);
  m_speedAdjust = adjust;
}

double CDVDClock::GetSpeedAdjust()
{
  CSingleLock lock(m_critSection);
  return m_speedAdjust;
}

double CDVDClock::ErrorAdjust(double error, const char* log)
{
  CSingleLock lock(m_critSection);

  double clock, absolute, adjustment;
  clock = GetClock(absolute);

  // skip minor updates while speed adjust is active
  // -> adjusting buffer levels
  if (m_speedAdjust != 0 && error < DVD_MSEC_TO_TIME(100))
  {
    return 0;
  }

  adjustment = error;

  if (m_vSyncAdjust != 0)
  {
    if (error > 0.5 * m_frameTime)
      adjustment = m_frameTime;
    else if (error < -0.5 * m_frameTime)
      adjustment = -m_frameTime;
    else
      adjustment = 0;
  }

  if (adjustment == 0)
    return 0;

  Discontinuity(clock+adjustment, absolute);

  CLog::Log(LOGDEBUG, "CDVDClock::ErrorAdjust - %s - error:%f, adjusted:%f",
                      log, error, adjustment);
  return adjustment;
}

void CDVDClock::Discontinuity(double clock, double absolute)
{
  CSingleLock lock(m_critSection);
  m_startClock = AbsoluteToSystem(absolute);
  if(m_pauseClock)
    m_pauseClock = m_startClock;
  m_iDisc = clock;
  m_bReset = false;
  m_systemAdjust = 0;
  m_speedAdjust = 0;
}

void CDVDClock::SetMaxSpeedAdjust(double speed)
{
  CSingleLock lock(m_speedsection);

  m_maxspeedadjust = speed;
}

//returns the refreshrate if the videoreferenceclock is running, -1 otherwise
int CDVDClock::UpdateFramerate(double fps, double* interval /*= NULL*/)
{
  //sent with fps of 0 means we are not playing video
  if(fps == 0.0)
    return -1;

  m_frameTime = 1/fps * DVD_TIME_BASE;

  //check if the videoreferenceclock is running, will return -1 if not
  double rate = m_videoRefClock->GetRefreshRate(interval);

  if (rate <= 0)
    return -1;

  CSingleLock lock(m_speedsection);

  double weight = MathUtils::round_int(rate) / (double)MathUtils::round_int(fps);

  //set the speed of the videoreferenceclock based on fps, refreshrate and maximum speed adjust set by user
  if (m_maxspeedadjust > 0.05)
  {
    if (weight / MathUtils::round_int(weight) < 1.0 + m_maxspeedadjust / 100.0
    &&  weight / MathUtils::round_int(weight) > 1.0 - m_maxspeedadjust / 100.0)
      weight = MathUtils::round_int(weight);
  }
  double speed = rate / (fps * weight);
  lock.Leave();

  m_videoRefClock->SetSpeed(speed);

  return rate;
}

bool CDVDClock::GetClockInfo(int& MissedVblanks, double& ClockSpeed, double& RefreshRate) const
{
  return m_videoRefClock->GetClockInfo(MissedVblanks, ClockSpeed, RefreshRate);
}

double CDVDClock::SystemToAbsolute(int64_t system)
{
  return DVD_TIME_BASE * (double)(system - m_systemOffset) / m_systemFrequency;
}

int64_t CDVDClock::AbsoluteToSystem(double absolute)
{
  return (int64_t)(absolute / DVD_TIME_BASE * m_systemFrequency) + m_systemOffset;
}

double CDVDClock::SystemToPlaying(int64_t system)
{
  int64_t current;

  if (m_bReset)
  {
    m_startClock = system;
    m_systemUsed = m_systemFrequency;
    if(m_pauseClock)
      m_pauseClock = m_startClock;
    m_iDisc = 0;
    m_systemAdjust = 0;
    m_speedAdjust = 0;
    m_vSyncAdjust = 0;
    m_bReset = false;
  }
  
  if (m_pauseClock)
    current = m_pauseClock;
  else
    current = system;

  return DVD_TIME_BASE * (double)(current - m_startClock + m_systemAdjust) / m_systemUsed + m_iDisc;
}

double CDVDClock::GetClockSpeed()
{
  CSingleLock lock(m_critSection);

  double speed = (double)m_systemFrequency / m_systemUsed;
  return m_videoRefClock->GetSpeed() * speed + m_speedAdjust;
}
