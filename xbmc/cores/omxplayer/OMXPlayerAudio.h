/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <deque>
#include <sys/types.h>

#include "OMXClock.h"
#include "DVDStreamInfo.h"
#include "OMXAudio.h"
#include "OMXAudioCodecOMX.h"
#include "threads/Thread.h"
#include "IVideoPlayer.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDMessageQueue.h"
#include "utils/BitstreamStats.h"
#include "platform/linux/DllBCM.h"

class OMXPlayerAudio : public CThread, public IDVDStreamPlayerAudio
{
protected:
  CDVDMessageQueue      m_messageQueue;
  CDVDMessageQueue      &m_messageParent;

  CDVDStreamInfo            m_hints_current;
  CDVDStreamInfo            m_hints;
  OMXClock                  *m_av_clock;
  COMXAudio                 m_omxAudio;
  std::string               m_codec_name;
  bool                      m_passthrough;
  AEAudioFormat             m_format;
  COMXAudioCodecOMX         *m_pAudioCodec;
  int                       m_speed;
  bool                      m_silence;
  double                    m_audioClock;

  bool                      m_stalled;
  IDVDStreamPlayer::ESyncState m_syncState;

  BitstreamStats            m_audioStats;

  bool                      m_buffer_empty;
  bool                      m_flush;
  bool                      m_DecoderOpen;

  bool                      m_bad_state;

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  void OpenStream(CDVDStreamInfo &hints, COMXAudioCodecOMX *codec);
private:
public:
  OMXPlayerAudio(OMXClock *av_clock, CDVDMessageQueue& parent, CProcessInfo &processInfo);
  ~OMXPlayerAudio();
  bool OpenStream(CDVDStreamInfo hints) override;
  void SendMessage(CDVDMsg* pMsg, int priority = 0) override { m_messageQueue.Put(pMsg, priority); }
  void FlushMessages()                              override { m_messageQueue.Flush(); }
  bool AcceptsData() const                          override { return !m_messageQueue.IsFull(); }
  bool HasData() const                              override { return m_messageQueue.GetDataSize() > 0; }
  bool IsInited() const                             override { return m_messageQueue.IsInited(); }
  int  GetLevel() const                             override { return m_messageQueue.GetLevel(); }
  bool IsStalled() const                            override { return m_stalled;  }
  bool IsEOS() override;
  void CloseStream(bool bWaitForBuffers) override;
  bool CodecChange();
  bool Decode(DemuxPacket *pkt, bool bDropPacket, bool bTrickPlay);
  void Flush(bool sync) override;
  AEAudioFormat GetDataFormat(CDVDStreamInfo hints);
  bool IsPassthrough() const override;
  bool OpenDecoder();
  void CloseDecoder();
  double GetCurrentPts() override { return m_audioClock; };
  void SubmitEOS();

  void SetVolume(float fVolume)                          override { m_omxAudio.SetVolume(fVolume); }
  void SetMute(bool bOnOff)                              override { m_omxAudio.SetMute(bOnOff); }
  void SetDynamicRangeCompression(long drc)              override { m_omxAudio.SetDynamicRangeCompression(drc); }
  float GetDynamicRangeAmplification() const             override { return m_omxAudio.GetDynamicRangeAmplification(); }
  void SetSpeed(int iSpeed) override;
  int GetAudioChannels() override;
  std::string GetPlayerInfo() override;
};

