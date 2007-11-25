
#pragma once

#include "../mplayer/IDirectSoundRenderer.h"
#include "../mplayer/IAudioCallback.h"
#include "utils/CriticalSection.h"

enum CodecID;
typedef struct stDVDAudioFrame DVDAudioFrame;

class CDVDAudio
{
public:
  CDVDAudio(volatile bool& bStop);
  ~CDVDAudio();

  void RegisterAudioCallback(IAudioCallback* pCallback);
  void UnRegisterAudioCallback();

  void SetVolume(int iVolume);
  void SetDynamicRangeCompression(long drc);
  void Pause();
  void Resume();
  bool Create(const DVDAudioFrame &audioframe, CodecID codec);
  bool IsValidFormat(const DVDAudioFrame &audioframe);
  void Destroy();
  DWORD AddPackets(const DVDAudioFrame &audioframe);
  void DoWork();
  double GetDelay(); // returns the time it takes to play a packet if we add one at this time
  void Flush();

  void SetSpeed(int iSpeed);

  IDirectSoundRenderer* m_pAudioDecoder;
protected:
  DWORD AddPacketsRenderer(unsigned char* data, DWORD len);
  IAudioCallback* m_pCallback;
  BYTE* m_pBuffer; // should be [m_dwPacketSize]
  DWORD m_iBufferSize;
  DWORD m_dwPacketSize;
  CCriticalSection m_critSection;

  int m_iChannels;
  int m_iBitrate;
  int m_iBitsPerSample;
  bool m_bPassthrough;
  int m_iSpeed;

  volatile bool& m_bStop;
  //counter that will go from 0 to m_iSpeed-1 and reset, data will only be output when speedstep is 0
  //int m_iSpeedStep; 
};
