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

#ifndef _OMX_PLAYERAUDIO_H_
#define _OMX_PLAYERAUDIO_H_

#include <deque>
#include <sys/types.h>

#include "utils/StdString.h"

#include "OMXClock.h"
#include "DVDStreamInfo.h"
#include "OMXAudio.h"
#include "OMXAudioCodecOMX.h"
#include "threads/Thread.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDMessageQueue.h"
#include "utils/BitstreamStats.h"
#include "xbmc/linux/DllBCM.h"

using namespace std;

class OMXPlayerAudio : public CThread
{
protected:
  CDVDMessageQueue      m_messageQueue;
  CDVDMessageQueue      &m_messageParent;

  CDVDStreamInfo            m_hints_current;
  CDVDStreamInfo            m_hints;
  OMXClock                  *m_av_clock;
  COMXAudio                 m_omxAudio;
  std::string               m_codec_name;
  bool                      m_use_passthrough;
  bool                      m_passthrough;
  bool                      m_use_hw_decode;
  bool                      m_hw_decode;
  AEAudioFormat             m_format;
  CAEChannelInfo            m_channelLayout;
  COMXAudioCodecOMX         *m_pAudioCodec;
  unsigned int              m_speed;
  bool                      m_silence;
  double                    m_audioClock;
  double m_error;    //last average error

  int64_t m_errortime; //timestamp of last time we measured
  int64_t m_freq;

  void   HandleSyncError(double duration);
  double m_errorbuff; //place to store average errors
  int    m_errorcount;//number of errors stored
  bool   m_syncclock;

  double m_integral; //integral correction for resampler
  int    m_skipdupcount; //counter for skip/duplicate synctype
  bool   m_prevskipped;

  bool                      m_stalled;
  bool                      m_started;

  BitstreamStats            m_audioStats;

  struct timespec           m_starttime, m_endtime;
  bool                      m_buffer_empty;
  bool                      m_flush;
  //SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
  int                       m_synctype;
  int                       m_nChannels;
  bool                      m_DecoderOpen;

  DllBcmHost                m_DllBcmHost;
  bool                      m_send_eos;

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
private:
public:
  OMXPlayerAudio(OMXClock *av_clock, CDVDMessageQueue& parent);
  ~OMXPlayerAudio();
  bool OpenStream(CDVDStreamInfo &hints);
  void OpenStream(CDVDStreamInfo &hints, COMXAudioCodecOMX *codec);
  void SendMessage(CDVDMsg* pMsg, int priority = 0) { m_messageQueue.Put(pMsg, priority); }
  bool AcceptsData() const                          { return !m_messageQueue.IsFull(); }
  bool HasData() const                              { return m_messageQueue.GetDataSize() > 0; }
  bool IsInited() const                             { return m_messageQueue.IsInited(); }
  int  GetLevel() const                             { return m_messageQueue.GetLevel(); }
  bool IsStalled()                                  { return m_stalled;  }
  bool IsEOS()                                      { return m_send_eos; };
  void WaitForBuffers();
  bool CloseStream(bool bWaitForBuffers);
  bool CodecChange();
  bool Decode(DemuxPacket *pkt, bool bDropPacket);
  void Flush();
  bool AddPacket(DemuxPacket *pkt);
  AEDataFormat GetDataFormat(CDVDStreamInfo hints);
  bool Passthrough() const;
  bool OpenDecoder();
  void CloseDecoder();
  double GetDelay();
  double GetCacheTime();
  double GetCurrentPTS() { return m_audioClock; };
  void WaitCompletion();
  void  RegisterAudioCallback(IAudioCallback* pCallback);
  void  UnRegisterAudioCallback();
  void SetCurrentVolume(float fVolume);
  void SetSpeed(int iSpeed);
  int  GetAudioBitrate();
  std::string GetPlayerInfo();
};
#endif
