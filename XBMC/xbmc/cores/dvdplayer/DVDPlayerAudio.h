#pragma once
#include "../../utils/thread.h"
#include "../../utils/event.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDDemuxers\DVDPacketQueue.h"
#include "DVDDemuxers\DVDDemuxUtils.h"

class CDVDPlayer;
class CDVDAudioCodec;
class IAudioCallback;
enum CodecID;

class CDVDPlayerAudio : public CThread
{
public:
  CDVDPlayerAudio(CDVDClock* pClock);
  virtual ~CDVDPlayerAudio();

  void		RegisterAudioCallback(IAudioCallback* pCallback);
  void		UnRegisterAudioCallback();
  
  bool    OpenStream(CodecID codecID, int iChannels, int iSampleRate);
  void    CloseStream(bool bWaitForBuffers);
  
  void    Pause();
  void    Resume();
  void    Flush();
  
  void    DoWork();
  
  CDVDPacketQueue m_packetQueue;

  CodecID m_codec;    // codec id of the current active stream
  int     m_iSourceChannels; // number of audio channels for the current active stream
  
protected:

  virtual void    OnStartup();
  virtual void    OnExit();
  virtual void    Process();

  bool            InitializeOutputDevice();
  int             DecodeFrame(BYTE** audio_buf);
  
  CRITICAL_SECTION m_critCodecSection;
  CDVDClock*      m_pClock; // dvd master clock
  CDVDAudio       m_dvdAudio; // audio output device
  
  bool            m_bInitializedOutputDevice;
  
  CDVDAudioCodec* m_pAudioCodec; // audio codec
  
  
  // for audio decoding
  CDVDDemux::DemuxPacket* pAudioPacket; 
  BYTE* audio_pkt_data; // current audio packet
  int audio_pkt_size; // and current audio packet size
  
  __int64 m_audioClock;
  
};
