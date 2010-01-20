#pragma once

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

#include "system.h"
#include "utils/SharedSection.h"
#include "utils/CriticalSection.h"

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

  /* tells clock at what framerate video is, to  *
   * allow it to adjust speed for a better match */
  int UpdateFramerate(double fps);

  bool   SetMaxSpeedAdjust(double speed);

  static double GetAbsoluteClock();
  static double GetFrequency() { return (double)m_systemFrequency ; }
  static double WaitAbsoluteClock(double target);
protected:
  CSharedSection m_critSection;
  int64_t m_systemUsed;
  int64_t m_startClock;
  int64_t m_pauseClock;
  double m_iDisc;
  bool m_bReset;

  static int64_t m_systemFrequency;
  static int64_t m_systemOffset;
  static CCriticalSection m_systemsection;

  double           m_maxspeedadjust;
  bool             m_speedadjust;
  CCriticalSection m_speedsection;
};
