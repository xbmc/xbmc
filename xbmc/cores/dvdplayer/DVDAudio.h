
#pragma once

#include "..\mplayer\IDirectSoundRenderer.h"
#include "..\mplayer\IAudioCallback.h"
#include "..\..\utils\CriticalSection.h"

enum CodecID;

class CDVDAudio
{
public:
  CDVDAudio();
  ~CDVDAudio();

  void RegisterAudioCallback(IAudioCallback* pCallback);
  void UnRegisterAudioCallback();

  void SetVolume(int iVolume);
  void SetDynamicRangeCompression(long drc);
  void Pause();
  void Resume();
  bool Create(int iChannels, int iBitrate, int iBitsPerSample, bool bPasstrough, CodecID codec);
  void Destroy();
  DWORD AddPackets(unsigned char* data, DWORD len);
  void DoWork();
  __int64 GetDelay(); // returns the time it takes to play a packet if we add one at this time
  void Flush();

  void SetSpeed(int iSpeed);

  IDirectSoundRenderer* m_pAudioDecoder;
protected:
  DWORD AddPacketsRenderer(unsigned char* data, DWORD len);
  IAudioCallback* m_pCallback;
  BYTE* m_pBuffer; // should be [m_dwPacketSize]
  int m_iBufferSize;
  DWORD m_dwPacketSize;
  CCriticalSection m_critSection;

  int m_iChannels;
  int m_iBitrate;
  int m_iBitsPerSample;
  int m_iSpeed;

  //counter that will go from 0 to m_iSpeed-1 and reset, data will only be output when speedstep is 0
  //int m_iSpeedStep; 
};
