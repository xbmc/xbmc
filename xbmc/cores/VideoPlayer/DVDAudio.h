#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include "threads/CriticalSection.h"
#include "PlatformDefs.h"

#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include <atomic>

extern "C" {
#include "libavcodec/avcodec.h"
}

typedef struct stDVDAudioFrame DVDAudioFrame;

class CSingleLock;
class CDVDClock;

class CDVDAudio : IAEClockCallback
{
public:
  CDVDAudio(CDVDClock *clock);
  ~CDVDAudio();

  void SetVolume(float fVolume);
  void SetDynamicRangeCompression(long drc);
  float GetCurrentAttenuation();
  void Pause();
  void Resume();
  bool Create(const DVDAudioFrame &audioframe, AVCodecID codec, bool needresampler);
  bool IsValidFormat(const DVDAudioFrame &audioframe);
  void Destroy();
  unsigned int AddPackets(const DVDAudioFrame &audioframe);
  double GetPlayingPts();
  double GetCacheTime();
  double GetCacheTotal(); // returns total amount the audio device can buffer
  double GetDelay(); // returns the time it takes to play a packet if we add one at this time
  double GetSyncError();
  void SetSyncErrorCorrection(double correction);
  double GetResampleRatio();
  void SetResampleMode(int mode);
  void Flush();
  void Drain();
  void AbortAddPackets();

  double GetClock();
  double GetClockSpeed();
  IAEStream *m_pAudioStream;

protected:

  double m_playingPts;
  double m_timeOfPts;
  double m_syncError;
  unsigned int m_syncErrorTime;
  double m_resampleRatio;
  CCriticalSection m_critSection;

  unsigned int m_sampeRate;
  int m_iBitsPerSample;
  bool m_bPassthrough;
  CAEChannelInfo m_channelLayout;
  bool m_bPaused;

  std::atomic_bool m_bAbort;
  CDVDClock *m_pClock;
};
