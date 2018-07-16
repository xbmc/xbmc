/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "OMXCore.h"
#include "DVDClock.h"
#include "platform/linux/XTimeUtils.h"

#ifdef OMX_SKIP64BIT
static inline OMX_TICKS ToOMXTime(int64_t pts)
{
  OMX_TICKS ticks;
  ticks.nLowPart = pts;
  ticks.nHighPart = pts >> 32;
  return ticks;
}
static inline int64_t FromOMXTime(OMX_TICKS ticks)
{
  int64_t pts = ticks.nLowPart | ((uint64_t)(ticks.nHighPart) << 32);
  return pts;
}
#else
#define FromOMXTime(x) (x)
#define ToOMXTime(x) (x)
#endif

class OMXClock
{
protected:
  bool              m_pause;
  pthread_mutex_t   m_lock;
  int               m_omx_speed;
  OMX_U32           m_WaitMask;
  OMX_TIME_CLOCKSTATE   m_eState;
  OMX_TIME_REFCLOCKTYPE m_eClock;
  CDVDClock         *m_clock;
private:
  COMXCoreComponent m_omx_clock;
  double            m_last_media_time;
  double            m_last_media_time_read;
  double            m_speedAdjust;
public:
  OMXClock();
  ~OMXClock();
  void Lock();
  void UnLock();
  double GetAbsoluteClock(bool interpolated = true) { return m_clock ? m_clock->GetAbsoluteClock(interpolated):0; }
  double GetClock(bool interpolated = true) { return m_clock ? m_clock->GetClock(interpolated):0; }
  double GetClock(double& absolute, bool interpolated = true) { return m_clock ? m_clock->GetClock(absolute, interpolated):0; }
  void Discontinuity(double currentPts = 0LL) { if (m_clock) m_clock->Discontinuity(currentPts); }
  void OMXSetClockPorts(OMX_TIME_CONFIG_CLOCKSTATETYPE *clock, bool has_video, bool has_audio);
  bool OMXSetReferenceClock(bool has_audio, bool lock = true);
  bool OMXInitialize(CDVDClock *clock);
  void OMXDeinitialize();
  bool OMXIsPaused() { return m_pause; };
  bool OMXStop(bool lock = true);
  bool OMXStep(int steps = 1, bool lock = true);
  bool OMXReset(bool has_video, bool has_audio, bool lock = true);
  double OMXMediaTime(bool lock = true);
  double OMXClockAdjustment(bool lock = true);
  bool OMXMediaTime(double pts, bool lock = true);
  bool OMXPause(bool lock = true);
  bool OMXResume(bool lock = true);
  bool OMXSetSpeed(int speed, bool lock = true, bool pause_resume = false);
  void OMXSetSpeedAdjust(double adjust, bool lock = true);
  int  OMXPlaySpeed() { return m_omx_speed; };
  bool OMXFlush(bool lock = true);
  COMXCoreComponent *GetOMXClock();
  bool OMXStateExecute(bool lock = true);
  void OMXStateIdle(bool lock = true);
  bool HDMIClockSync(bool lock = true);
  static int64_t CurrentHostCounter(void);
  static int64_t CurrentHostFrequency(void);

  static double NormalizeFrameduration(double frameduration);
};

