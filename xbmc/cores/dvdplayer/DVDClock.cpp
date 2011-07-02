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

#include "DVDClock.h"
#include "video/VideoReferenceClock.h"
#include <math.h>
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

int64_t CDVDClock::m_systemOffset;
int64_t CDVDClock::m_systemFrequency;
CCriticalSection CDVDClock::m_systemsection;

bool CDVDClock::m_ismasterclock;

CDVDClock::CDVDClock()
{
  if(!m_systemFrequency)
    m_systemFrequency = g_VideoReferenceClock.GetFrequency();

  if(!m_systemOffset)
    m_systemOffset = g_VideoReferenceClock.GetTime();

  m_systemUsed = m_systemFrequency;
  m_pauseClock = 0;
  m_bReset = true;
  m_iDisc = 0;
  m_maxspeedadjust = 0.0;
  m_speedadjust = false;

  m_ismasterclock = true;
}

CDVDClock::~CDVDClock()
{}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
double CDVDClock::GetAbsoluteClock(bool interpolated /*= true*/)
{
  CSingleLock lock(m_systemsection);

  if(!m_systemFrequency)
    m_systemFrequency = g_VideoReferenceClock.GetFrequency();

  if(!m_systemOffset)
    m_systemOffset = g_VideoReferenceClock.GetTime(interpolated);

  int64_t current;
  current = g_VideoReferenceClock.GetTime(interpolated);
  current -= m_systemOffset;

#if _DEBUG
  if (interpolated) //only compare interpolated time, clock might go backwards otherwise
  {
    static int64_t old;
    if(old > current)
      CLog::Log(LOGWARNING, "CurrentHostCounter() moving backwords by %"PRId64" ticks with freq of %"PRId64, old - current, m_systemFrequency);
    old = current;
  }
#endif

  return DVD_TIME_BASE * (double)current / m_systemFrequency;
}

double CDVDClock::WaitAbsoluteClock(double target)
{
  CSingleLock lock(m_systemsection);

  int64_t systemtarget, freq, offset;
  if(!m_systemFrequency)
    m_systemFrequency = g_VideoReferenceClock.GetFrequency();

  if(!m_systemOffset)
    m_systemOffset = g_VideoReferenceClock.GetTime();

  freq   = m_systemFrequency;
  offset = m_systemOffset;

  lock.Leave();

  systemtarget = (int64_t)(target / DVD_TIME_BASE * (double)freq);
  systemtarget += offset;
  systemtarget = g_VideoReferenceClock.Wait(systemtarget);
  systemtarget -= offset;
  return (double)systemtarget / freq * DVD_TIME_BASE;
}

double CDVDClock::GetClock(bool interpolated /*= true*/)
{
  CSharedLock lock(m_critSection);
  int64_t current;

  if (m_bReset)
  {
    m_startClock = g_VideoReferenceClock.GetTime(interpolated);
    m_systemUsed = m_systemFrequency;
    m_pauseClock = 0;
    m_iDisc = 0;
    m_bReset = false;
  }

  if (m_pauseClock)
    current = m_pauseClock;
  else
    current = g_VideoReferenceClock.GetTime(interpolated);

  current -= m_startClock;
  return DVD_TIME_BASE * (double)current / m_systemUsed + m_iDisc;
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

void CDVDClock::Discontinuity(double currentPts)
{
  CExclusiveLock lock(m_critSection);
  m_startClock = g_VideoReferenceClock.GetTime();
  if(m_pauseClock)
    m_pauseClock = m_startClock;
  m_iDisc = currentPts;
  m_bReset = false;
}

void CDVDClock::Pause()
{
  CExclusiveLock lock(m_critSection);
  if(!m_pauseClock)
    m_pauseClock = g_VideoReferenceClock.GetTime();
}

void CDVDClock::Resume()
{
  CExclusiveLock lock(m_critSection);
  if( m_pauseClock )
  {
    int64_t current;
    current = g_VideoReferenceClock.GetTime();

    m_startClock += current - m_pauseClock;
    m_pauseClock = 0;
  }
}

bool CDVDClock::SetMaxSpeedAdjust(double speed)
{
  CSingleLock lock(m_speedsection);

  m_maxspeedadjust = speed;
  return m_speedadjust;
}

//returns the refreshrate if the videoreferenceclock is running, -1 otherwise
int CDVDClock::UpdateFramerate(double fps, double* interval /*= NULL*/)
{
  //sent with fps of 0 means we are not playing video
  if(fps == 0.0)
  {
    CSingleLock lock(m_speedsection);
    m_speedadjust = false;
    return -1;
  }

  //check if the videoreferenceclock is running, will return -1 if not
  int rate = g_VideoReferenceClock.GetRefreshRate(interval);

  if (rate <= 0)
    return -1;

  CSingleLock lock(m_speedsection);

  m_speedadjust = true;

  double weight = (double)rate / (double)MathUtils::round_int(fps);

  //set the speed of the videoreferenceclock based on fps, refreshrate and maximum speed adjust set by user
  if (m_maxspeedadjust > 0.05)
  {
    if (weight / MathUtils::round_int(weight) < 1.0 + m_maxspeedadjust / 100.0
    &&  weight / MathUtils::round_int(weight) > 1.0 - m_maxspeedadjust / 100.0)
      weight = MathUtils::round_int(weight);
  }
  double speed = (double)rate / (fps * weight);
  lock.Leave();

  g_VideoReferenceClock.SetSpeed(speed);

  return rate;
}
