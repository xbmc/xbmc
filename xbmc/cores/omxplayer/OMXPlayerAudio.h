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

#ifndef _OMX_PLAYERAUDIO_H_
#define _OMX_PLAYERAUDIO_H_

#include <deque>
#include <sys/types.h>

#include "OMXClock.h"
#include "DVDStreamInfo.h"
#include "OMXAudio.h"
#include "OMXAudioCodecOMX.h"
#include "threads/Thread.h"
#include "IDVDPlayer.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDMessageQueue.h"
#include "utils/BitstreamStats.h"
#include "xbmc/linux/DllBCM.h"

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
  bool                      m_use_hw_decode;
  bool                      m_hw_decode;
  AEAudioFormat             m_format;
  COMXAudioCodecOMX         *m_pAudioCodec;
  int                       m_speed;
  bool                      m_silence;
  double                    m_audioClock;

  bool                      m_stalled;
  bool                      m_started;

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
  OMXPlayerAudio(OMXClock *av_clock, CDVDMessageQueue& parent);
  ~OMXPlayerAudio();
  bool OpenStream(CDVDStreamInfo &hints);
  void SendMessage(CDVDMsg* pMsg, int priority = 0) { m_messageQueue.Put(pMsg, priority); }
  void FlushMessages()                              { m_messageQueue.Flush(); }
  bool AcceptsData() const                          { return !m_messageQueue.IsFull(); }
  bool HasData() const                              { return m_messageQueue.GetDataSize() > 0; }
  bool IsInited() const                             { return m_messageQueue.IsInited(); }
  int  GetLevel() const                             { return m_messageQueue.GetLevel(); }
  bool IsStalled() const                            { return m_stalled;  }
  bool IsEOS();
  void WaitForBuffers();
  void CloseStream(bool bWaitForBuffers);
  bool CodecChange();
  bool Decode(DemuxPacket *pkt, bool bDropPacket);
  void Flush();
  bool AddPacket(DemuxPacket *pkt);
  AEDataFormat GetDataFormat(CDVDStreamInfo hints);
  bool IsPassthrough() const;
  bool OpenDecoder();
  void CloseDecoder();
  double GetDelay();
  double GetCacheTime();
  double GetCacheTotal();
  double GetCurrentPts() { return m_audioClock; };
  void SubmitEOS();

  void SetVolume(float fVolume)                          { m_omxAudio.SetVolume(fVolume); }
  void SetMute(bool bOnOff)                              { m_omxAudio.SetMute(bOnOff); }
  void SetDynamicRangeCompression(long drc)              { m_omxAudio.SetDynamicRangeCompression(drc); }
  float GetDynamicRangeAmplification() const             { return m_omxAudio.GetDynamicRangeAmplification(); }
  void SetSpeed(int iSpeed);
  int  GetAudioBitrate();
  int GetAudioChannels();
  std::string GetPlayerInfo();

  bool BadState() { return m_bad_state; }
};
#endif
