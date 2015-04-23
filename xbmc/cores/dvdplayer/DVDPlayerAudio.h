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

#pragma once
#include "threads/Thread.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDStreamInfo.h"
#include "utils/BitstreamStats.h"
#include "IDVDPlayer.h"

#include <list>

class CDVDPlayer;
class CDVDAudioCodec;
class CDVDAudioCodec;

#define DECODE_FLAG_DROP    1
#define DECODE_FLAG_RESYNC  2
#define DECODE_FLAG_ERROR   4
#define DECODE_FLAG_ABORT   8
#define DECODE_FLAG_TIMEOUT 16

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

class CDVDErrorAverage
{
public:
  CDVDErrorAverage()
  {
    Flush();
  }
  void Add(double error)
  {
    m_buffer += error;
    m_count++;
  }

  void Flush(int interval = 100)
  {
    m_buffer = 0.0f;
    m_count  = 0;
    m_timer.Set(interval);
  }

  double Get()
  {
    if(m_count)
      return m_buffer / m_count;
    else
      return 0.0;
  }

  bool Get(double& error, int interval = 100)
  {
    if(m_timer.IsTimePast())
    {
      error = Get();
      Flush(interval);
      return true;
    }
    else
      return false;
  }

  double m_buffer; //place to store average errors
  int m_count;  //number of errors stored
  XbmcThreads::EndTime m_timer;
};

class CDVDPlayerAudio : public CThread, public IDVDStreamPlayerAudio
{
public:
  CDVDPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent);
  virtual ~CDVDPlayerAudio();

  bool OpenStream(CDVDStreamInfo &hints);
  void CloseStream(bool bWaitForBuffers);

  void SetSpeed(int speed);
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers();
  bool AcceptsData() const                              { return !m_messageQueue.IsFull(); }
  bool HasData() const                                  { return m_messageQueue.GetDataSize() > 0; }
  int  GetLevel() const                                 { return m_messageQueue.GetLevel(); }
  bool IsInited() const                                 { return m_messageQueue.IsInited(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0)     { m_messageQueue.Put(pMsg, priority); }
  void FlushMessages()                                  { m_messageQueue.Flush(); }

  void SetVolume(float fVolume)                         { m_dvdAudio.SetVolume(fVolume); }
  void SetMute(bool bOnOff)                             { }
  void SetDynamicRangeCompression(long drc)             { m_dvdAudio.SetDynamicRangeCompression(drc); }
  float GetDynamicRangeAmplification() const            { return 0.0f; }


  std::string GetPlayerInfo();
  int GetAudioBitrate();
  int GetAudioChannels();

  // holds stream information for current playing stream
  CDVDStreamInfo m_streaminfo;

  CPTSInputQueue  m_ptsInput;

  double GetCurrentPts()                            { CSingleLock lock(m_info_section); return m_info.pts; }

  bool IsStalled() const                            { return m_stalled;  }
  bool IsEOS()                                      { return false; }
  bool IsPassthrough() const;
  double GetDelay() { return 0.0; }
  double GetCacheTotal() { return 0.0; }
protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  int DecodeFrame(DVDAudioFrame &audioframe);

  void UpdatePlayerInfo();
  void OpenStream(CDVDStreamInfo &hints, CDVDAudioCodec* codec);
  //! Switch codec if needed. Called when the sample rate gotten from the
  //! codec changes, in which case we may want to switch passthrough on/off.
  bool SwitchCodecIfNeeded();
  float GetCurrentAttenuation()                         { return m_dvdAudio.GetCurrentAttenuation(); }

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
    uint8_t*               data;
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
  bool    m_stalled;
  bool    m_started;
  bool    m_silence;

  bool OutputPacket(DVDAudioFrame &audioframe);

  //SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
  int    m_synctype;
  int    m_setsynctype;
  int    m_prevsynctype; //so we can print to the log

  double m_error;    //last average error

  void   SetSyncType(bool passthrough);
  void   HandleSyncError(double duration);
  CDVDErrorAverage m_errors;
  bool   m_syncclock;

  double m_integral; //integral correction for resampler
  bool   m_prevskipped;
  double m_maxspeedadjust;
  double m_resampleratio; //resample ratio when using SYNC_RESAMPLE, used for the codec info

  struct SInfo
  {
    SInfo()
    : pts(DVD_NOPTS_VALUE)
    , passthrough(false)
    {}

    std::string      info;
    double           pts;
    bool             passthrough;
  };

  CCriticalSection m_info_section;
  SInfo            m_info;
};

