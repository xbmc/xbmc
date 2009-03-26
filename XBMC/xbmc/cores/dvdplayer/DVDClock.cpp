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

#if defined(_WIN32)
static void TimeGetTimeFrequency(LARGE_INTEGER* freq){ freq->QuadPart = 1000; }
static void TimeGetTimeCounter(LARGE_INTEGER* val)   { val->QuadPart = timeGetTime();}
#define QueryPerformanceFrequency(a) TimeGetTimeFrequency(a)
#define QueryPerformanceCounter(a)   TimeGetTimeCounter(a)
#endif

LARGE_INTEGER CDVDClock::m_systemOffset;
LARGE_INTEGER CDVDClock::m_systemFrequency;
CDVDClock::CDVDClock()
{
  if(!m_systemFrequency.QuadPart)
    //QueryPerformanceFrequency(&m_systemFrequency);
    g_VideoReferenceClock.GetFrequency(&m_systemFrequency);

  if(!m_systemOffset.QuadPart)
    //QueryPerformanceCounter(&m_systemOffset);
    g_VideoReferenceClock.GetTime(&m_systemOffset);

  m_systemUsed = m_systemFrequency;
  m_pauseClock.QuadPart = 0;
  m_bReset = true;
  m_iDisc = 0;
}

CDVDClock::~CDVDClock()
{}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
double CDVDClock::GetAbsoluteClock()
{
  static CCriticalSection section;
  CSingleLock lock(section);

  if(!m_systemFrequency.QuadPart)
    //QueryPerformanceFrequency(&m_systemFrequency);
    g_VideoReferenceClock.GetFrequency(&m_systemFrequency);

  if(!m_systemOffset.QuadPart)
    //QueryPerformanceCounter(&m_systemOffset);
    g_VideoReferenceClock.GetTime(&m_systemOffset);

  LARGE_INTEGER current;
  //QueryPerformanceCounter(&current);
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

double CDVDClock::GetClock()
{
  CSharedLock lock(m_critSection);
  LARGE_INTEGER current;
    
  if (m_bReset)
  {
    //QueryPerformanceCounter(&m_startClock);
    g_VideoReferenceClock.GetTime(&m_startClock);
    m_systemUsed = m_systemFrequency;
    m_pauseClock.QuadPart = 0;
    m_iDisc = 0;
    m_bReset = false;
  }

  if (m_pauseClock.QuadPart)
    current = m_pauseClock;
  else
    //QueryPerformanceCounter(&current);
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
      //QueryPerformanceCounter(&m_pauseClock);
      g_VideoReferenceClock.GetTime(&m_pauseClock);
    return;
  }
  
  LARGE_INTEGER current;
  __int64 newfreq = m_systemFrequency.QuadPart * DVD_PLAYSPEED_NORMAL / iSpeed;
  
  //QueryPerformanceCounter(&current);
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
      //QueryPerformanceCounter(&m_startClock);
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
    //QueryPerformanceCounter(&m_pauseClock);
    g_VideoReferenceClock.GetTime(&m_pauseClock);
}

void CDVDClock::Resume()
{
  CExclusiveLock lock(m_critSection);
  if( m_pauseClock.QuadPart )
  {
    LARGE_INTEGER current;
    //QueryPerformanceCounter(&current);
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
