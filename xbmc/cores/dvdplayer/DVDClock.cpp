
#include "stdafx.h"
#include "DVDClock.h"

CDVDClock::CDVDClock()
{
  QueryPerformanceFrequency(&m_systemFrequency);
  m_bReset = true;
  m_lastPts = 0;
}

CDVDClock::~CDVDClock()
{
}
  
__int64 CDVDClock::GetClock()
{
  LARGE_INTEGER current;
  if (m_bReset)
  {
    QueryPerformanceCounter(&m_startClock);
    m_bReset = false;
  }
  QueryPerformanceCounter(&current);
  current.QuadPart -= m_startClock.QuadPart;
  
  //tricky, but very acurate.
  current.QuadPart *= DVD_TIME_BASE;
  current.QuadPart /= m_systemFrequency.QuadPart;
  
  return current.QuadPart;
}

void CDVDClock::Discontinuity(ClockDiscontinuityType type, __int64 currentPts)
{
  m_lastPts = 0;
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
      m_startClock.QuadPart -= ((currentPts * m_systemFrequency.QuadPart) / DVD_TIME_BASE);
      break;
    }
  }
}
