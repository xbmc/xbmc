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
#include "../../utils/Thread.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"
#include "BitstreamStats.h"
#include <samplerate.h>

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

#define MAXCONVSAMPLES 100000
#define RINGSIZE 1000000

#define SMALLPROPORTIONAL 20.0
#define MIDPROPORTIONAL 5.0
#define BIGPROPORTIONAL 2.0
#define INTEGRAL 100.0

typedef struct stDVDAudioFrame
{
  BYTE* data;
  double pts;
  double duration;
  unsigned int size;

  int channels;
  int bits_per_sample;
  int sample_rate;
  bool passthrough;
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

class CDVDPlayerResampler
{
  public:
    CDVDPlayerResampler();
    ~CDVDPlayerResampler();
  
    void Add(DVDAudioFrame audioframe, double pts);
    bool Retreive(DVDAudioFrame audioframe, double &pts);
    void SetRatio(double ratio);
  
  private:
  
    int NrChannels;
    SRC_STATE* Converter;
    SRC_DATA ConverterData;
  
    float* RingBuffer;  //ringbuffer for the audiosamples
    int RingBufferPos;  //where we are in the ringbuffer
    int RingBufferFill; //how many unread samples there are in the ringbuffer, starting at RingBufferPos
    double* PtsRingBuffer;  //ringbuffer for the pts value, each sample gets its own pts
  
    void CheckResampleBuffers(int channels);
    
    //this makes sure value is bewteen min and max
    template <typename A, typename B, typename C>
    inline A Clamp(A value, B min, C max){ return value < max ? (value > min ? value : min) : max; }
};

class CDVDPlayerAudio : public CThread
{
public:
  CDVDPlayerAudio(CDVDClock* pClock);
  virtual ~CDVDPlayerAudio();

  void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdAudio.RegisterAudioCallback(pCallback); }
  void UnRegisterAudioCallback()                        { m_dvdAudio.UnRegisterAudioCallback(); }

  bool OpenStream(CDVDStreamInfo &hints);
  void CloseStream(bool bWaitForBuffers);
  
  void SetSpeed(int speed);
  void Flush();

  // waits untill all available data has been rendered  
  void WaitForBuffers();
  bool AcceptsData()                                    { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg)                       { m_messageQueue.Put(pMsg); }
  
  void SetVolume(long nVolume)                          { m_dvdAudio.SetVolume(nVolume); }
  void SetDynamicRangeCompression(long drc)             { m_dvdAudio.SetDynamicRangeCompression(drc); }

  std::string GetPlayerInfo();
  int GetAudioBitrate();

  // holds stream information for current playing stream
  CDVDStreamInfo m_streaminfo;
  
  CDVDMessageQueue m_messageQueue;
  CPTSOutputQueue m_ptsOutput;
  CPTSInputQueue  m_ptsInput;

  double GetCurrentPts()                            { return m_ptsOutput.Current(); }

  bool IsStalled()                                  { return m_stalled;  }
protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  int DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket);

  // tries to open a decoder for the given data. 
  bool OpenDecoder(CDVDStreamInfo &hint, BYTE* buffer = NULL, unsigned int size = 0);

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
  CRITICAL_SECTION m_critCodecSection;
  
  void ResetErrorCounters();
  
  int PCMSynctype; //sync type for pcm
  int AC3DTSSynctype; //sync type for ac3/dts passthrough
  double CurrError; //current average error
  double AverageError; //place to store errors
  int ErrorCount; //amount of error stored
  int SkipDupCount; //whether to skip, duplicate or play normal
  bool PrevSkipped; //if the previous frame was skipped, don't skip the current one
  double MeasureTime; //timestamp when the last average error was measured
  bool SyncToVideoClock;
  CDVDPlayerResampler Resampler;
  double Integral;
};