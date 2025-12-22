/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AudioSinkAE.h"
#include "DVDClock.h"
#include "DVDCodecs/Audio/FloatingAverage.h"
#include "DVDMessageQueue.h"
#include "DVDStreamInfo.h"
#include "IVideoPlayer.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"
#include "utils/BitstreamStats.h"

#include <chrono>
#include <list>
#include <mutex>
#include <utility>

class CVideoPlayer;
class CDVDAudioCodec;
class CDVDAudioCodec;

class CVideoPlayerAudio : public CThread, public IDVDStreamPlayerAudio
{
public:
  CVideoPlayerAudio(
    CDVDClock* pClock,
    CDVDMessageQueue& parent,
    CRenderManager& renderManager,
    CProcessInfo &processInfo,
    double messageQueueTimeSize);
  ~CVideoPlayerAudio() override;

  bool OpenStream(CDVDStreamInfo hints) override;
  void CloseStream(bool bWaitForBuffers) override;

  void SetSpeed(int speed) override;
  void Flush(bool sync) override;

  // waits until all available data has been rendered
  bool AcceptsData() const override;
  bool HasData() const override { return m_messageQueue.GetDataSize() > 0; }
  int  GetLevel() const override { return m_messageQueue.GetLevel(); }
  bool IsInited() const override { return m_messageQueue.IsInited(); }
  void SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority = 0) override
  {
    m_messageQueue.Put(pMsg, priority);
  }
  void FlushMessages() override { m_messageQueue.Flush(); }

  void SetDynamicRangeCompression(long drc) override { m_audioSink.SetDynamicRangeCompression(drc); }
  float GetDynamicRangeAmplification() const override { return 0.0f; }

  std::string GetPlayerInfo() override;
  int GetAudioChannels() override;

  double GetCurrentPts() override
  {
    std::unique_lock<CCriticalSection> lock(m_info_section);
    return m_info.pts;
  }

  bool IsStalled() const override { return m_stalled;  }
  bool IsPassthrough() const override;

protected:

  void OnStartup() override;
  void OnExit() override;
  void Process() override;

  bool ProcessDecoderOutput(DVDAudioFrame &audioframe);
  void UpdatePlayerInfo();
  void OpenStream(CDVDStreamInfo& hints, std::unique_ptr<CDVDAudioCodec> codec);
  //! Switch codec if needed. Called when the sample rate gotten from the
  //! codec changes, in which case we may want to switch passthrough on/off.
  bool SwitchCodecIfNeeded();
  void SetSyncType(bool passthrough);

  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;

  // Access to adjust the tweak the latency because of audio
  CRenderManager& m_renderManager;

  // holds stream information for current playing stream
  CDVDStreamInfo m_streaminfo;

  double m_audioClock;

  CAudioSinkAE m_audioSink; // audio output device
  CDVDClock* m_pClock; // dvd master clock
  std::unique_ptr<CDVDAudioCodec> m_pAudioCodec; // audio codec
  BitstreamStats m_audioStats;

  int m_speed;
  bool m_stalled;
  bool m_paused;
  IDVDStreamPlayer::ESyncState m_syncState;
  XbmcThreads::EndTime<> m_syncTimer;

  int m_synctype;
  int m_prevsynctype;

  bool   m_prevskipped;
  double m_maxspeedadjust;

  struct SInfo
  {
    std::string      info;
    double           pts = DVD_NOPTS_VALUE;
    bool             passthrough = false;
  };

  mutable CCriticalSection m_info_section;
  SInfo            m_info;

  bool m_displayReset = false;
  unsigned int m_disconAdjustTimeMs = 20; // maximum sync-off before adjusting
  int m_disconAdjustCounter = 0;

  //============================================================================
  // LAV Jitter Tracking for PCM/Decoded Audio
  // These members are only used when m_lavStylePcmSyncEnabled = true
  //============================================================================
  bool m_lavStylePcmSyncEnabled{false};  // Toggle LAV PCM sync on/off
  
  static constexpr size_t PCM_JITTER_WINDOW_SIZE = 64;
  static constexpr double PCM_JITTER_THRESHOLD = 10000.0;  // 10ms in DVD_TIME_BASE units
  AudioSync::CFloatingAverage<double, PCM_JITTER_WINDOW_SIZE> m_pcmJitterTracker;
  double m_pcmOutputClock{0.0};       // Separate running output timestamp (like LAV's m_rtStart)
  bool m_pcmResyncTimestamp{true};    // Flag to resync on next valid PTS (like LAV's m_bResyncTimestamp)
};