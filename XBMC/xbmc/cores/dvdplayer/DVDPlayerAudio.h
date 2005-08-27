#pragma once
#include "../../utils/thread.h"

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

  void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdAudio.RegisterAudioCallback(pCallback); }
  void UnRegisterAudioCallback()                        { m_dvdAudio.UnRegisterAudioCallback(); }

  bool OpenStream(CodecID codecID, int iChannels, int iSampleRate);
  void CloseStream(bool bWaitForBuffers);

  void Pause()                                          { m_dvdAudio.Pause(); }
  void Resume()                                         { m_dvdAudio.Resume(); }
  void Flush();

  void DoWork()                                         { m_dvdAudio.DoWork(); }

  void SetVolume(long nVolume)                          { m_dvdAudio.SetVolume(nVolume); }
  int GetVolume()                                       { return m_dvdAudio.GetVolume(); }

  CodecID m_codec;    // codec id of the current active stream
  int m_iSourceChannels; // number of audio channels for the current active stream
  
  CDVDPacketQueue m_packetQueue;

protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  bool InitializeOutputDevice();
  int DecodeFrame(BYTE** audio_buf);

  bool m_bInitializedOutputDevice;
  __int64 m_audioClock;
  
  // for audio decoding
  CDVDDemux::DemuxPacket* pAudioPacket;
  BYTE* audio_pkt_data; // current audio packet
  int audio_pkt_size; // and current audio packet size
  
  CDVDAudio m_dvdAudio; // audio output device
  CDVDClock* m_pClock; // dvd master clock
  CDVDAudioCodec* m_pAudioCodec; // audio codec
  
  CRITICAL_SECTION m_critCodecSection;
};
