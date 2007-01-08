
#include "../../stdafx.h"
#include "DVDClock.h"
#include <math.h>


CDVDClock::CDVDClock()
{
  QueryPerformanceFrequency(&m_systemFrequency);
  m_systemUsed = m_systemFrequency;
  m_pauseClock.QuadPart = 0;
  m_bReset = true;
  m_iDisc = 0I64;
}

CDVDClock::~CDVDClock()
{}

__int64 CDVDClock::GetAbsoluteClock()
{
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);
  
  return current.QuadPart * DVD_TIME_BASE / m_systemFrequency.QuadPart;  
}

__int64 CDVDClock::GetClock()
{
  CSharedLock lock(m_critSection);
  LARGE_INTEGER current;
    
  if (m_bReset)
  {
    QueryPerformanceCounter(&m_startClock);
    m_systemUsed = m_systemFrequency;
    m_pauseClock.QuadPart = 0;
    m_iDisc = 0I64;
    m_bReset = false;
  }

  if (m_pauseClock.QuadPart)
    current = m_pauseClock;
  else
    QueryPerformanceCounter(&current);

  current.QuadPart -= m_startClock.QuadPart;
  return current.QuadPart * DVD_TIME_BASE / m_systemUsed.QuadPart + m_iDisc;
}

void CDVDClock::SetSpeed(int iSpeed)
{
  // this will sometimes be a little bit of due to rounding errors, ie clock might jump abit when changing speed
  CExclusiveLock lock(m_critSection);

  LARGE_INTEGER current;
  __int64 newfreq = m_systemFrequency.QuadPart * DVD_PLAYSPEED_NORMAL / iSpeed;
  
  if( m_pauseClock.QuadPart )
    current = m_pauseClock;
  else
    QueryPerformanceCounter(&current);

  m_startClock.QuadPart = current.QuadPart - ( newfreq * (current.QuadPart - m_startClock.QuadPart) ) / m_systemUsed.QuadPart;
  m_systemUsed.QuadPart = newfreq;    
}

void CDVDClock::Discontinuity(ClockDiscontinuityType type, __int64 currentPts, __int64 delay)
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

      QueryPerformanceCounter(&m_startClock);
      m_startClock.QuadPart += delay * m_systemUsed.QuadPart / DVD_TIME_BASE; 
      m_iDisc = currentPts;
      m_bReset = false;
      break;
    }
  }
}

void CDVDClock::Pause()
{
  CExclusiveLock lock(m_critSection);
  QueryPerformanceCounter(&m_pauseClock);
}

void CDVDClock::Resume()
{
  CExclusiveLock lock(m_critSection);
  
  if( m_systemFrequency.QuadPart != m_systemUsed.QuadPart ) SetSpeed(DVD_PLAYSPEED_NORMAL);

  if( m_pauseClock.QuadPart )
  {
    LARGE_INTEGER current;
    QueryPerformanceCounter(&current);

    m_startClock.QuadPart += current.QuadPart - m_pauseClock.QuadPart;
    m_pauseClock.QuadPart = 0;
  }  
}

__int64 CDVDClock::DistanceToDisc()
{
  CSharedLock lock(m_critSection);
  return GetClock() - m_iDisc;
}
