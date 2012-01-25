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
#include "threads/SharedSection.h"
#include "threads/CriticalSection.h"

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

#define DVD_TIME_TO_SEC(x)  ((int)((double)(x) / DVD_TIME_BASE))
#define DVD_TIME_TO_MSEC(x) ((int)((double)(x) * 1000 / DVD_TIME_BASE))
#define DVD_SEC_TO_TIME(x)  ((double)(x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((double)(x) * DVD_TIME_BASE / 1000)

#define DVD_PLAYSPEED_PAUSE       0       // frame stepping
#define DVD_PLAYSPEED_NORMAL      1000
#ifdef HAS_DS_PLAYER
  //Time base from directshow is a 100 nanosec unit
  #define DS_TIME_BASE 1E7

  #define DS_TIME_TO_SEC(x)     ((double)(x / DS_TIME_BASE))
  #define DS_TIME_TO_MSEC(x)    ((double)(x * 1000 / DS_TIME_BASE))
  #define SEC_TO_DS_TIME(x)     ((__int64)(x * DS_TIME_BASE))
  #define MSEC_TO_DS_TIME(x)    ((__int64)(x * DS_TIME_BASE / 1000))
  #define SEC_TO_MSEC(x)        ((double)(x * 1E3))
#endif

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

  // Allow a different time base (DirectShow for example use a 100 ns time base)
  static void SetTimeBase(int64_t timeBase) { m_timeBase = timeBase; }
  static int64_t GetTimeBase() { return m_timeBase; }
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
  static int64_t m_timeBase;
  static CCriticalSection m_systemsection;

  double           m_maxspeedadjust;
  bool             m_speedadjust;
  CCriticalSection m_speedsection;
  static bool      m_ismasterclock;
};
