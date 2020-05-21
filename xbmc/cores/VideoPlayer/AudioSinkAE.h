/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "PlatformDefs.h"

#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
}

typedef struct stDVDAudioFrame DVDAudioFrame;

class CSingleLock;
class CDVDClock;

class CAudioSinkAE : IAEClockCallback
{
public:
  explicit CAudioSinkAE(CDVDClock *clock);
  ~CAudioSinkAE() override;

  void SetVolume(float fVolume);
  void SetDynamicRangeCompression(long drc);
  void Pause();
  void Resume();
  bool Create(const DVDAudioFrame &audioframe, AVCodecID codec, bool needresampler);
  bool IsValidFormat(const DVDAudioFrame &audioframe);
  void Destroy(bool finish);
  unsigned int AddPackets(const DVDAudioFrame &audioframe);
  double GetPlayingPts();
  double GetCacheTime();
  double GetCacheTotal(); // returns total time a stream can buffer
  double GetMaxDelay(); // returns total time of audio in AE for the stream
  double GetDelay(); // returns the time it takes to play a packet if we add one at this time
  double GetSyncError();
  void SetSyncErrorCorrection(double correction);

  /*!
   * \brief Returns the resample ratio, or 0.0 if unknown/invalid
   */
  double GetResampleRatio();

  void SetResampleMode(int mode);
  void Flush();
  void Drain();
  void AbortAddPackets();

  double GetClock() override;
  double GetClockSpeed() override;

  CAEStreamInfo::DataType GetPassthroughStreamType(AVCodecID codecId, int samplerate, int profile);

protected:

  IAEStream *m_pAudioStream;
  double m_playingPts;
  double m_timeOfPts;
  double m_syncError;
  unsigned int m_syncErrorTime;
  double m_resampleRatio = 0.0; // invalid
  CCriticalSection m_critSection;

  AEDataFormat m_dataFormat;
  unsigned int m_sampleRate;
  int m_iBitsPerSample;
  bool m_bPassthrough;
  CAEChannelInfo m_channelLayout;
  CAEStreamInfo::DataType m_dataType;
  bool m_bPaused;

  std::atomic_bool m_bAbort;
  CDVDClock *m_pClock;
};
