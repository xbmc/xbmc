#pragma once
#include "utils/Thread.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"

class CDVDPlayer;
class CDVDAudioCodec;
class IAudioCallback;
class CDVDAudioCodec;

enum CodecID;

#define DECODE_FLAG_DROP    1
#define DECODE_FLAG_RESYNC  2
#define DECODE_FLAG_ERROR   4
#define DECODE_FLAG_ABORT   8

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

class CPTSQueue
{
private:
  typedef struct {double pts; double timestamp; double duration;} TPTSItem;
  TPTSItem m_current;
  std::queue<TPTSItem> m_queue;

public:
  CPTSQueue();
  void Add(double pts, double delay, double duration);
  void Flush();
  double Current();
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
  
  void DoWork()                                         { m_dvdAudio.DoWork(); }

  void SetVolume(long nVolume)                          { m_dvdAudio.SetVolume(nVolume); }
  void SetDynamicRangeCompression(long drc)             { m_dvdAudio.SetDynamicRangeCompression(drc); }

  string GetPlayerInfo();

  // holds stream information for current playing stream
  CDVDStreamInfo m_streaminfo;
  
  CDVDMessageQueue m_messageQueue;
  CPTSQueue m_ptsQueue;

  double GetCurrentPts()                            { return m_ptsQueue.Current(); }

  bool IsStalled()                                  { return m_Stalled;  }
protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  int DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket);

  // tries to open a decoder for the given data. 
  bool OpenDecoder(CDVDStreamInfo &hint, BYTE* buffer = NULL, unsigned int size = 0);

  double m_audioClock;
  
  // for audio decoding
  CDVDDemux::DemuxPacket* pAudioPacket;
  BYTE* audio_pkt_data; // current audio packet
  int audio_pkt_size; // and current audio packet size
  
  double m_bps_i;  // input bytes per second
  double m_bps_o;  // output bytes per second

  CDVDAudio m_dvdAudio; // audio output device
  CDVDClock* m_pClock; // dvd master clock
  CDVDAudioCodec* m_pAudioCodec; // audio codec

  int m_speed;  // wanted playback speed. if playback speed!=DVD_PLAYSPEED_NORMAL, don't sync clock as it will loose track of position after seek
  double m_droptime;

  bool m_Stalled;
  CRITICAL_SECTION m_critCodecSection;
};

