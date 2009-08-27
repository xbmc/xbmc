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

#include "stdafx.h"
#include "DVDClock.h"
#include "VideoReferenceClock.h"
#include <math.h>
#include "../../Util.h"

#if defined(_WIN32)
static void TimeGetTimeFrequency(LARGE_INTEGER* freq){ freq->QuadPart = 1000; }
static void TimeGetTimeCounter(LARGE_INTEGER* val)   { val->QuadPart = timeGetTime();}
#define QueryPerformanceFrequency(a) TimeGetTimeFrequency(a)
#define QueryPerformanceCounter(a)   TimeGetTimeCounter(a)
#endif

LARGE_INTEGER CDVDClock::m_systemOffset;
LARGE_INTEGER CDVDClock::m_systemFrequency;
CCriticalSection CDVDClock::m_systemsection;

CDVDClock::CDVDClock()
{
  if(!m_systemFrequency.QuadPart)
    g_VideoReferenceClock.GetFrequency(&m_systemFrequency);

  if(!m_systemOffset.QuadPart)
    g_VideoReferenceClock.GetTime(&m_systemOffset);

  m_systemUsed = m_systemFrequency;
  m_pauseClock.QuadPart = 0;
  m_bReset = true;
  m_iDisc = 0;
  m_maxspeedadjust = 0.0;
  m_speedadjust = false;
}

CDVDClock::~CDVDClock()
{}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
double CDVDClock::GetAbsoluteClock()
{
  CSingleLock lock(m_systemsection);

  if(!m_systemFrequency.QuadPart)
    g_VideoReferenceClock.GetFrequency(&m_systemFrequency);

  if(!m_systemOffset.QuadPart)
    g_VideoReferenceClock.GetTime(&m_systemOffset);

  LARGE_INTEGER current;
  g_VideoReferenceClock.GetTime(&current);
  current.QuadPart -= m_systemOffset.QuadPart;

#if _DEBUG
  static LARGE_INTEGER old;
  if(old.QuadPart > current.QuadPart)
    CLog::Log(LOGWARNING, "QueryPerformanceCounter moving backwords by %"PRId64" ticks with freq of %"PRId64, old.QuadPart - current.QuadPart, m_systemFrequency.QuadPart);
  old = current;
#endif

  return DVD_TIME_BASE * (double)current.QuadPart / m_systemFrequency.QuadPart;
}

double CDVDClock::WaitAbsoluteClock(double target)
{
  CSingleLock lock(m_systemsection);

  __int64 systemtarget, freq, offset;
  if(!m_systemFrequency.QuadPart)
    g_VideoReferenceClock.GetFrequency(&m_systemFrequency);

  if(!m_systemOffset.QuadPart)
    g_VideoReferenceClock.GetTime(&m_systemOffset);

  freq   = m_systemFrequency.QuadPart;
  offset = m_systemOffset.QuadPart;

  lock.Leave();

  systemtarget = (__int64)(target / DVD_TIME_BASE * (double)freq);
  systemtarget += offset;
  systemtarget = g_VideoReferenceClock.Wait(systemtarget);
  systemtarget -= offset;
  return (double)systemtarget / freq * DVD_TIME_BASE;
}

double CDVDClock::GetClock()
{
  CSharedLock lock(m_critSection);
  LARGE_INTEGER current;

  if (m_bReset)
  {
    g_VideoReferenceClock.GetTime(&m_startClock);
    m_systemUsed = m_systemFrequency;
    m_pauseClock.QuadPart = 0;
    m_iDisc = 0;
    m_bReset = false;
  }

  if (m_pauseClock.QuadPart)
    current = m_pauseClock;
  else
    g_VideoReferenceClock.GetTime(&current);

  current.QuadPart -= m_startClock.QuadPart;
  return DVD_TIME_BASE * (double)current.QuadPart / m_systemUsed.QuadPart + m_iDisc;
}

void CDVDClock::SetSpeed(int iSpeed)
{
  // this will sometimes be a little bit of due to rounding errors, ie clock might jump abit when changing speed
  CExclusiveLock lock(m_critSection);

  if(iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    if(!m_pauseClock.QuadPart)
      g_VideoReferenceClock.GetTime(&m_pauseClock);
    return;
  }

  LARGE_INTEGER current;
  __int64 newfreq = m_systemFrequency.QuadPart * DVD_PLAYSPEED_NORMAL / iSpeed;

  g_VideoReferenceClock.GetTime(&current);
  if( m_pauseClock.QuadPart )
  {
    m_startClock.QuadPart += current.QuadPart - m_pauseClock.QuadPart;
    m_pauseClock.QuadPart = 0;
  }

  m_startClock.QuadPart = current.QuadPart - ( newfreq * (current.QuadPart - m_startClock.QuadPart) ) / m_systemUsed.QuadPart;
  m_systemUsed.QuadPart = newfreq;
}

void CDVDClock::Discontinuity(ClockDiscontinuityType type, double currentPts, double delay)
{
  CExclusiveLock lock(m_critSection);
  switch (type)
  {
  case CLOCK_DISC_FULL:
    {
      m_bReset = true;
      break;
    }
  case CLOCK_DISC_NORMAL:
    {
      g_VideoReferenceClock.GetTime(&m_startClock);
      m_startClock.QuadPart += (__int64)(delay * m_systemUsed.QuadPart / DVD_TIME_BASE);
      m_iDisc = currentPts;
      m_bReset = false;
      break;
    }
  }
}

void CDVDClock::Pause()
{
  CExclusiveLock lock(m_critSection);
  if(!m_pauseClock.QuadPart)
    g_VideoReferenceClock.GetTime(&m_pauseClock);
}

void CDVDClock::Resume()
{
  CExclusiveLock lock(m_critSection);
  if( m_pauseClock.QuadPart )
  {
    LARGE_INTEGER current;
    g_VideoReferenceClock.GetTime(&current);

    m_startClock.QuadPart += current.QuadPart - m_pauseClock.QuadPart;
    m_pauseClock.QuadPart = 0;
  }
}

double CDVDClock::DistanceToDisc()
{
  // GetClock will lock. if we lock the shared lock here there's potentialy a chance that another thread will try exclusive lock on the section and we'll deadlock
  return GetClock() - m_iDisc;
}

bool CDVDClock::SetMaxSpeedAdjust(double speed)
{
  CSingleLock lock(m_speedsection);

  m_maxspeedadjust = speed;
  return m_speedadjust;
}

//returns the refreshrate if the videoreferenceclock is running, -1 otherwise
int CDVDClock::UpdateFramerate(double fps)
{
  //sent with fps of 0 means we are not playing video
  if(fps == 0.0)
  {
    CSingleLock lock(m_speedsection);
    m_speedadjust = false;
    return -1;
  }

  //check if the videoreferenceclock is running, will return -1 if not
  int rate = g_VideoReferenceClock.GetRefreshRate();

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
