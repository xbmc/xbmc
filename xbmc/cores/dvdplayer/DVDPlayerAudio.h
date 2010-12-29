/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once
#include "utils/Thread.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"
#include "BitstreamStats.h"
#include "DVDPlayerAudioResampler.h"

#include "AudioEngine/AE.h"
#include "AudioEngine/AEAudioFormat.h"
#include "AudioEngine/AEUtil.h"

#include <list>
#include <queue>

class CDVDPlayer;
class CDVDAudioCodec;
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
  AEChLayout        channel_layout;
  enum AEDataFormat data_format;
  int               bits_per_sample;
  int               sample_rate;
  bool              passthrough;
} DVDAudioFrame;

class CPTSOutputQueue
{
private:
  typedef struct {double pts; double timestamp; double duration;} TPTSItem;
  TPTSItem m_current;
  std::queue<TPTSItem> m_queue;
  CCriticalSection m_sync;

public:
  CPTSOutputQueue();
  void Add(double pts, double delay, double duration);
  void Flush();
  double Current();
};

class CPTSInputQueue
{
private:
  typedef std::list<std::pair<__int64, double> >::iterator IT;
  std::list<std::pair<__int64, double> > m_list;
  CCriticalSection m_sync;
public:
  void   Add(__int64 bytes, double pts);
  double Get(__int64 bytes, bool consume);
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

  void SetSpeed(int speed);
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers();
  bool AcceptsData()                                    { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0)     { m_messageQueue.Put(pMsg, priority); }

  void SetVolume(float volume)                          { m_dvdAudio.SetVolume(volume); }

  std::string GetPlayerInfo();
  int GetAudioBitrate();

  // holds stream information for current playing stream
  CDVDStreamInfo m_streaminfo;

  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;
  CPTSOutputQueue m_ptsOutput;
  CPTSInputQueue  m_ptsInput;

  double GetCurrentPts()                            { return m_ptsOutput.Current(); }

  bool IsStalled()                                  { return m_stalled;  }
  bool IsPassthrough() const;
protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  int DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket);

  double m_audioClock;

  // data for audio decoding
  struct
  {
    CDVDMsgDemuxerPacket*  msg;
    BYTE*                  data;
    int                    size;
    double                 dts;

    void Attach(CDVDMsgDemuxerPacket* msg2)
    {
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

  CDVDPlayerResampler m_resampler;

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

