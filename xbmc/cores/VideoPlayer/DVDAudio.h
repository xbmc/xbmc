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
class IAEStream;

extern "C" {
#include "libavcodec/avcodec.h"
}

typedef struct stDVDAudioFrame DVDAudioFrame;

class CSingleLock;

class CDVDAudio
{
public:
  CDVDAudio(volatile bool& bStop);
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
  double GetDelay(); // returns the time it takes to play a packet if we add one at this time
  double GetPlayingPts();
  void   SetPlayingPts(double pts);
  double GetCacheTime();  // returns total amount of data cached in audio output at this time
  double GetCacheTotal(); // returns total amount the audio device can buffer
  void Flush();
  void Finish();
  void Drain();

  void SetSpeed(int iSpeed);
  void SetResampleRatio(double ratio);

  IAEStream *m_pAudioStream;
protected:
  double m_playingPts;
  double m_timeOfPts;
  CCriticalSection m_critSection;

  int m_iBitrate;
  int m_iBitsPerSample;
  double m_SecondsPerByte;
  bool m_bPassthrough;
  CAEChannelInfo m_channelLayout;
  bool m_bPaused;

  volatile bool& m_bStop;
  //counter that will go from 0 to m_iSpeed-1 and reset, data will only be output when speedstep is 0
  //int m_iSpeedStep;
};
