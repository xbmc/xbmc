#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "streams.h"
#include "system.h"
#include "utils/SharedSection.h"
#include "utils/CriticalSection.h"

//Time base from directshow is a 100 nanosec unit
#define DS_TIME_BASE 1E7
#define DS_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

#define DS_TIME_TO_SEC(x)     ((double)(x * 1E-7))
#define DS_TIME_TO_MSEC(x)    ((double)(x * 1E-4))
#define SEC_TO_DS_TIME(x)     ((__int64)(x * DS_TIME_BASE))
//MSEC_TO_DS_TIME is the one used to convert from directshow to the one rendermanager is using
#define MSEC_TO_DS_TIME(x)    ((__int64)(x * 1E4))
#define SEC_TO_MSEC(x)        ((double)(x * 1E3))

#define DS_PLAYSPEED_RW_2X       -2000
#define DS_PLAYSPEED_REVERSE     -1000
#define DS_PLAYSPEED_PAUSE       0       // frame stepping
#define DS_PLAYSPEED_NORMAL      1000
#define DS_PLAYSPEED_FF_2X       2000

class CDSClock
{
public:
  CDSClock();
  ~CDSClock();

  double GetClock();

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

extern CDSClock g_DsClock;