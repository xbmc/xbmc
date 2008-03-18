#pragma once

#include "utils/SharedSection.h"
#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

#define DVD_TIME_TO_SEC(x)  ((int)((double)(x) / DVD_TIME_BASE))
#define DVD_TIME_TO_MSEC(x) ((int)((double)(x) * 1000 / DVD_TIME_BASE))
#define DVD_SEC_TO_TIME(x)  ((double)(x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((double)(x) * DVD_TIME_BASE / 1000)

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

  double GetClock();

  /* delay should say how long in the future we expect to display this frame */
  void Discontinuity(ClockDiscontinuityType type, double currentPts = 0LL, double delay = 0LL);
  
  /* will return how close we are to a discontinuity */
  double DistanceToDisc();

  void Pause();
  void Resume();
  void SetSpeed(int iSpeed);

  static double GetAbsoluteClock();
  static double GetFrequency() { return (double)m_systemFrequency.QuadPart ; }
protected:
  CSharedSection m_critSection;
  LARGE_INTEGER m_systemUsed;  
  LARGE_INTEGER m_startClock;
  LARGE_INTEGER m_pauseClock;
  double m_iDisc;
  bool m_bReset;
  
  static LARGE_INTEGER m_systemFrequency;
};
