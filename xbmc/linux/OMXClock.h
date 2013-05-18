/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef _AVCLOCK_H_
#define _AVCLOCK_H_

#if defined(HAVE_OMXLIB)

#include "OMXCore.h"
#include "DVDClock.h"
#include "linux/XTimeUtils.h"

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

enum {
  AV_SYNC_AUDIO_MASTER,
  AV_SYNC_VIDEO_MASTER,
  AV_SYNC_EXTERNAL_MASTER,
};

class OMXClock
{
protected:
  bool              m_pause;
  bool              m_has_video;
  bool              m_has_audio;
  int               m_play_speed;
  pthread_mutex_t   m_lock;
  double            m_fps;
  int               m_omx_speed;
  bool              m_video_start;
  bool              m_audio_start;
  bool              m_audio_buffer;
  CDVDClock         *m_clock;
private:
  COMXCoreComponent m_omx_clock;
public:
  OMXClock();
  ~OMXClock();
  void Lock();
  void UnLock();
  double GetAbsoluteClock(bool interpolated = true) { return m_clock ? m_clock->GetAbsoluteClock(interpolated):0; }
  double GetClock(bool interpolated = true) { return m_clock ? m_clock->GetClock(interpolated):0; }
  double GetClock(double& absolute, bool interpolated = true) { return m_clock ? m_clock->GetClock(absolute, interpolated):0; }
  void Discontinuity(double currentPts = 0LL) { if (m_clock) m_clock->Discontinuity(currentPts); }
  void OMXSetClockPorts(OMX_TIME_CONFIG_CLOCKSTATETYPE *clock);
  bool OMXSetReferenceClock(bool lock = true);
  bool OMXInitialize(CDVDClock *clock, bool has_video, bool has_audio);
  void OMXDeinitialize();
  bool OMXIsPaused() { return m_pause; };
  bool OMXStop(bool lock = true);
  bool OMXStart(bool lock = true);
  bool OMXStep(int steps = 1, bool lock = true);
  bool OMXReset(bool lock = true);
  double OMXMediaTime(bool fixPreroll = true, bool lock = true);
  double OMXClockAdjustment(bool lock = true);
  bool OMXMediaTime(double pts, bool fixPreroll = true, bool lock = true);
  bool OMXPause(bool lock = true);
  bool OMXResume(bool lock = true);
  bool OMXSetSpeed(int speed, bool lock = true, bool pause_resume = false);
  int  OMXPlaySpeed() { return m_omx_speed; };
  COMXCoreComponent *GetOMXClock();
  bool OMXStateExecute(bool lock = true);
  void OMXStateIdle(bool lock = true);
  bool HDMIClockSync(bool lock = true);
  static int64_t CurrentHostCounter(void);
  static int64_t CurrentHostFrequency(void);
  bool HasVideo() { return m_has_video; };
  bool HasAudio() { return m_has_audio; };
  void HasVideo(bool has_video) { m_has_video = has_video; };
  void HasAudio(bool has_audio) { m_has_audio = has_audio; };
  bool VideoStart() { return m_video_start; };
  bool AudioStart() { return m_audio_start; };
  void VideoStart(bool video_start);
  void AudioStart(bool audio_start);
  void OMXAudioBufferStart();
  void OMXAudioBufferStop();
  bool OMXAudioBuffer() { return m_audio_buffer; };

  int     GetRefreshRate(double* interval = NULL);
  void    SetRefreshRate(double fps) { m_fps = fps; };

  static double NormalizeFrameduration(double frameduration);
};

#endif

#endif
