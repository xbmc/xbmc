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

#pragma once
#include "threads/Thread.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"
#include "utils/BitstreamStats.h"

#include "cores/AudioEngine/AEAudioFormat.h"

#include <list>
#include <queue>

class CDVDPlayer;
class CDVDAudioCodec;
class IAudioCallback;
class CDVDAudioCodec;

enum CodecID;

#define DECODE_FLAG_DROP    1
#define DECODE_FLAG_RESYNC  2
#define DECODE_FLAG_ERROR   4
#define DECODE_FLAG_ABORT   8
#define DECODE_FLAG_TIMEOUT 16

typedef struct stDVDAudioFrame
{
  BYTE* data;
  double pts;
  double duration;
  unsigned int size;

  int               channel_count;
  int               encoded_channel_count;
  CAEChannelInfo    channel_layout;
  enum AEDataFormat data_format;
  int               bits_per_sample;
  int               sample_rate;
  int               encoded_sample_rate;
  bool              passthrough;
} DVDAudioFrame;

class CPTSInputQueue
{
private:
  typedef std::list<std::pair<int64_t, double> >::iterator IT;
  std::list<std::pair<int64_t, double> > m_list;
  CCriticalSection m_sync;
public:
  void   Add(int64_t bytes, double pts);
  double Get(int64_t bytes, bool consume);
  void   Flush();
};

class CDVDPlayerAudio : public CThread
{
public:
  CDVDPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent);
  virtual ~CDVDPlayerAudio();

  bool OpenStream(CDVDStreamInfo &hints);
  void OpenStream(CDVDStreamInfo &hints, CDVDAudioCodec* codec);
  void CloseStream(bool bWaitForBuffers);

  void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdAudio.RegisterAudioCallback(pCallback); }
  void UnRegisterAudioCallback()                        { m_dvdAudio.UnRegisterAudioCallback(); }

  void SetSpeed(int speed);
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers();
  bool AcceptsData() const                              { return !m_messageQueue.IsFull(); }
  bool HasData() const                                  { return m_messageQueue.GetDataSize() > 0; }
  int  GetLevel() const                                 { return m_messageQueue.GetLevel(); }
  bool IsInited() const                                 { return m_messageQueue.IsInited(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0)     { m_messageQueue.Put(pMsg, priority); }

  //! Switch codec if needed. Called when the sample rate gotten from the
  //! codec changes, in which case we may want to switch passthrough on/off.
  bool SwitchCodecIfNeeded();

  void SetVolume(float fVolume)                         { m_dvdAudio.SetVolume(fVolume); }
  void SetDynamicRangeCompression(long drc)             { m_dvdAudio.SetDynamicRangeCompression(drc); }
  float GetCurrentAttenuation()                         { return m_dvdAudio.GetCurrentAttenuation(); }

  std::string GetPlayerInfo();
  int GetAudioBitrate();

  // holds stream information for current playing stream
  CDVDStreamInfo m_streaminfo;

  CPTSOutputQueue m_ptsOutput;
  CPTSInputQueue  m_ptsInput;

  double GetCurrentPts()                            { return m_dvdAudio.GetPlayingPts(); }

  bool IsStalled()                                  { return m_stalled;  }
  bool IsPassthrough() const;
protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  int DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket);

  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;

  double m_audioClock;

  // data for audio decoding
  struct PacketStatus
  {
    PacketStatus()
    {
        msg = NULL;
        Release();
    }

   ~PacketStatus()
    {
        Release();
    }

    CDVDMsgDemuxerPacket*  msg;
    BYTE*                  data;
    int                    size;
    double                 dts;

    void Attach(CDVDMsgDemuxerPacket* msg2)
    {
      if(msg) msg->Release();
      msg = msg2;
      msg->Acquire();
      DemuxPacket* p = msg->GetPacket();
      data = p->pData;
      size = p->iSize;
      dts = p->dts;

    }
    void Release()
    {
      if(msg) msg->Release();
      msg  = NULL;
      data = NULL;
      size = 0;
      dts  = DVD_NOPTS_VALUE;
    }
  } m_decode;

  CDVDAudio m_dvdAudio; // audio output device
  CDVDClock* m_pClock; // dvd master clock
  CDVDAudioCodec* m_pAudioCodec; // audio codec
  BitstreamStats m_audioStats;

  int     m_speed;
  double  m_droptime;
  bool    m_stalled;
  bool    m_started;
  double  m_duration; // last packets duration
  bool    m_silence;

  bool OutputPacket(DVDAudioFrame &audioframe);

  //SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
  int    m_synctype;
  int    m_setsynctype;
  int    m_prevsynctype; //so we can print to the log

  double m_error;    //last average error

  int64_t m_errortime; //timestamp of last time we measured
  int64_t m_freq;

  void   SetSyncType(bool passthrough);
  void   HandleSyncError(double duration);
  double m_errorbuff; //place to store average errors
  int    m_errorcount;//number of errors stored
  bool   m_syncclock;

  double m_integral; //integral correction for resampler
  int    m_skipdupcount; //counter for skip/duplicate synctype
  bool   m_prevskipped;
  double m_maxspeedadjust;
  double m_resampleratio; //resample ratio when using SYNC_RESAMPLE, used for the codec info
};

