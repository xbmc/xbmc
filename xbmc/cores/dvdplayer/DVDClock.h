#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "threads/SharedSection.h"
#include "threads/CriticalSection.h"

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and int64_t

#define DVD_TIME_TO_SEC(x)  ((int)((double)(x) / DVD_TIME_BASE))
#define DVD_TIME_TO_MSEC(x) ((int)((double)(x) * 1000 / DVD_TIME_BASE))
#define DVD_SEC_TO_TIME(x)  ((double)(x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((double)(x) * DVD_TIME_BASE / 1000)

#define DVD_PLAYSPEED_PAUSE       0       // frame stepping
#define DVD_PLAYSPEED_NORMAL      1000

class CDVDClock
{
public:
  CDVDClock();
  ~CDVDClock();

  double GetClock(bool interpolated = true);
  double GetClock(double& absolute, bool interpolated = true);

  void Discontinuity(double currentPts = 0LL);

  void Reset() { m_bReset = true; }
  void Pause();
  void Resume();
  void SetSpeed(int iSpeed);

  /* tells clock at what framerate video is, to  *
   * allow it to adjust speed for a better match */
  int UpdateFramerate(double fps, double* interval = NULL);

  bool   SetMaxSpeedAdjust(double speed);

  static double GetAbsoluteClock(bool interpolated = true);
  static double GetFrequency() { return (double)m_systemFrequency ; }
  static double WaitAbsoluteClock(double target);

  //when m_ismasterclock is true, CDVDPlayerAudio synchronizes the clock to the audio stream
  //when it's false, CDVDPlayerAudio synchronizes the audio stream to the clock
  //the rendermanager needs to know about that because it can synchronize the videoreferenceclock to the video timestamps
  static void SetMasterClock(bool ismasterclock) { m_ismasterclock = ismasterclock; }
  static bool IsMasterClock()                    { return m_ismasterclock;          }

protected:
  static void   CheckSystemClock();
  static double SystemToAbsolute(int64_t system);
  double        SystemToPlaying(int64_t system);

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
  static bool      m_ismasterclock;
};
