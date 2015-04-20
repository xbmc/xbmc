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

int64_t CDVDClock::m_systemOffset;
int64_t CDVDClock::m_systemFrequency;
CCriticalSection CDVDClock::m_systemsection;
CDVDClock *CDVDClock::m_playerclock = NULL;

CDVDClock::CDVDClock()
  :  m_master(MASTER_CLOCK_NONE)
{
  CSingleLock lock(m_systemsection);
  CheckSystemClock();

  m_systemUsed = m_systemFrequency;
  m_pauseClock = 0;
  m_bReset = true;
  m_iDisc = 0;
  m_maxspeedadjust = 0.0;
  m_lastSystemTime = g_VideoReferenceClock.GetTime();
  m_systemAdjust = 0;
  m_speedAdjust = 0;

  m_startClock = 0;

  m_playerclock = this;
}

CDVDClock::~CDVDClock()
{
  CSingleLock lock(m_systemsection);
  m_playerclock = NULL;
}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
double CDVDClock::GetAbsoluteClock(bool interpolated /*= true*/)
{
  CSingleLock lock(m_systemsection);
  CheckSystemClock();

  int64_t current;
  current = g_VideoReferenceClock.GetTime(interpolated);

#if _DEBUG
  if (interpolated) //only compare interpolated time, clock might go backwards otherwise
  {
    static int64_t old;
    if(old > current)
      CLog::Log(LOGWARNING, "CurrentHostCounter() moving backwords by %" PRId64" ticks with freq of %" PRId64, old - current, m_systemFrequency);
    old = current;
  }
#endif

  return SystemToAbsolute(current);
}

double CDVDClock::WaitAbsoluteClock(double target)
{
  CSingleLock lock(m_systemsection);
  CheckSystemClock();

  int64_t systemtarget, freq, offset;
  freq   = m_systemFrequency;
  offset = m_systemOffset;

  lock.Leave();

  systemtarget = (int64_t)(target / DVD_TIME_BASE * (double)freq);
  systemtarget += offset;
  systemtarget = g_VideoReferenceClock.Wait(systemtarget);
  systemtarget -= offset;
  return (double)systemtarget / freq * DVD_TIME_BASE;
}

CDVDClock* CDVDClock::GetMasterClock()
{
  CSingleLock lock(m_systemsection);
  return m_playerclock;
}

double CDVDClock::GetClock(bool interpolated /*= true*/)
{
  CSharedLock lock(m_critSection);

  int64_t current = g_VideoReferenceClock.GetTime(interpolated);
  m_systemAdjust += m_speedAdjust * (current - m_lastSystemTime);
  m_lastSystemTime = current;

  return SystemToPlaying(current);
}

double CDVDClock::GetClock(double& absolute, bool interpolated /*= true*/)
{
  int64_t current = g_VideoReferenceClock.GetTime(interpolated);
  {
    CSingleLock lock(m_systemsection);
    CheckSystemClock();
    absolute = SystemToAbsolute(current);
  }

  return GetClock(interpolated);
}

void CDVDClock::SetSpeed(int iSpeed)
{
  // this will sometimes be a little bit of due to rounding errors, ie clock might jump abit when changing speed
  CExclusiveLock lock(m_critSection);

  if(iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    if(!m_pauseClock)
      m_pauseClock = g_VideoReferenceClock.GetTime();
    return;
  }

  int64_t current;
  int64_t newfreq = m_systemFrequency * DVD_PLAYSPEED_NORMAL / iSpeed;

  current = g_VideoReferenceClock.GetTime();
  if( m_pauseClock )
  {
    m_startClock += current - m_pauseClock;
    m_pauseClock = 0;
  }

  m_startClock = current - (int64_t)((double)(current - m_startClock) * newfreq / m_systemUsed);
  m_systemUsed = newfreq;
}

void CDVDClock::SetSpeedAdjust(double adjust)
{
  CExclusiveLock lock(m_critSection);
  m_speedAdjust = adjust;
}

double CDVDClock::GetSpeedAdjust()
{
  CExclusiveLock lock(m_critSection);
  return m_speedAdjust;
}

bool CDVDClock::Update(double clock, double absolute, double limit, const char* log)
{
  CExclusiveLock lock(m_critSection);
  double was_absolute = SystemToAbsolute(m_startClock);
  double was_clock    = m_iDisc + absolute - was_absolute;
  lock.Leave();
  if(std::abs(clock - was_clock) > limit)
  {
    Discontinuity(clock, absolute);

    CLog::Log(LOGDEBUG, "CDVDClock::Discontinuity - %s - was:%f, should be:%f, error:%f"
                      , log
                      , was_clock
                      , clock
                      , clock - was_clock);
    return true;
  }
  else
    return false;
}

void CDVDClock::Discontinuity(double clock, double absolute)
{
  CExclusiveLock lock(m_critSection);
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

  //check if the videoreferenceclock is running, will return -1 if not
  double rate = g_VideoReferenceClock.GetRefreshRate(interval);

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

  g_VideoReferenceClock.SetSpeed(speed);

  return rate;
}

void CDVDClock::CheckSystemClock()
{
  if(!m_systemFrequency)
    m_systemFrequency = g_VideoReferenceClock.GetFrequency();

  if(!m_systemOffset)
    m_systemOffset = g_VideoReferenceClock.GetTime();
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
    m_bReset = false;
  }
  
  if (m_pauseClock)
    current = m_pauseClock;
  else
    current = system;

  return DVD_TIME_BASE * (double)(current - m_startClock + m_systemAdjust) / m_systemUsed + m_iDisc;
}

EMasterClock CDVDClock::GetMaster()
{
  CSharedLock lock(m_critSection);
  return m_master;
}

void CDVDClock::SetMaster(EMasterClock master)
{
  CExclusiveLock lock(m_critSection);

  if(m_master != master)
  {
    /* if we switched from video ref clock, we need to normalize speed */
    if(m_master == MASTER_CLOCK_AUDIO_VIDEOREF)
      g_VideoReferenceClock.SetSpeed(1.0);
  }

  m_master = master;
}

double CDVDClock::GetClockSpeed()
{
  return g_VideoReferenceClock.GetSpeed();
}
