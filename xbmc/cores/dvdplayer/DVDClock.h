#pragma once

#include "../../utils/sharedsection.h"
#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE (0x8000000000000000 ## i64)

#define DVD_SEC_TO_TIME(x) ((x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((x) * (DVD_TIME_BASE / 1000))



enum ClockDiscontinuityType
{
  CLOCK_DISC_FULL,  // pts is starting form 0 again
  CLOCK_DISC_NORMAL // after a pause
};

class CDVDClock
{
public:
  CDVDClock();
  ~CDVDClock();

  __int64 GetClock();
  __int64 GetAbsoluteClock();
  __int64 GetFrequency() { return (__int64)m_systemFrequency.QuadPart ; }

  void Discontinuity(ClockDiscontinuityType type, __int64 currentPts = 0LL);
  
  bool HadDiscontinuity(__int64 delay);
  void AdjustSpeedToMatch(__int64 currPts );
  
  void Pause();
  void Resume();
  void SetSpeed(int iSpeed);
protected:
  CSharedSection m_critSection;
  LARGE_INTEGER m_systemUsed;
  LARGE_INTEGER m_systemFrequency;
  LARGE_INTEGER m_startClock;
  __int64 m_iDisc;
  __int64 m_iPaused;
  bool m_bReset;

  __int64 m_lastPts;
};
