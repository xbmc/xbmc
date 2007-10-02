#pragma once

#include "../../utils/SharedSection.h"

#define DVD_TIME_BASE 1000000
#ifndef _LINUX
#define DVD_NOPTS_VALUE (0x8000000000000000 ## i64)
#else
#define DVD_NOPTS_VALUE (0x8000000000000000LL)
#endif

#define DVD_SEC_TO_TIME(x) ((x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((x) * (DVD_TIME_BASE / 1000))
#define DVD_TIME_TO_MSEC(x) ((x) / (DVD_TIME_BASE / 1000))

#define DVD_PLAYSPEED_RW_2X       -2000
#define DVD_PLAYSPEED_REVERSE     -1000
#define DVD_PLAYSPEED_PAUSE       0       // frame stepping
#define DVD_PLAYSPEED_NORMAL      1000
#define DVD_PLAYSPEED_FF_2X       2000

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

  /* delay should say how long in the future we expect to display this frame */
  void Discontinuity(ClockDiscontinuityType type, __int64 currentPts = 0LL, __int64 delay = 0LL);
  
  /* will return how close we are to a discontinuity */
  __int64 DistanceToDisc();

  void Pause();
  void Resume();
  void SetSpeed(int iSpeed);

  static __int64 GetAbsoluteClock();
  static __int64 GetFrequency() { return (__int64)m_systemFrequency.QuadPart ; }
protected:
  CSharedSection m_critSection;
  LARGE_INTEGER m_systemUsed;  
  LARGE_INTEGER m_startClock;
  LARGE_INTEGER m_pauseClock;
  __int64 m_iDisc;
  bool m_bReset;
  
  static LARGE_INTEGER m_systemFrequency;
};
