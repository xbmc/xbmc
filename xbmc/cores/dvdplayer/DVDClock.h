
#pragma once

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE (0x8000000000000000 ## i64)

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
  void Discontinuity(ClockDiscontinuityType type, __int64 currentPts = 0LL);
  void AdjustSpeedToMatch(__int64 currPts );
protected:

  LARGE_INTEGER m_systemUsed;
  LARGE_INTEGER m_systemFrequency;
  LARGE_INTEGER m_startClock;
  __int64 diffsum;
  bool m_bReset;

  __int64 m_lastPts;
};
