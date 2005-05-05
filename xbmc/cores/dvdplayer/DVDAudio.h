
#pragma once

#include "..\mplayer\IDirectSoundRenderer.h"
#include "..\mplayer\IAudioCallback.h"
#include "..\..\utils\CriticalSection.h"

class CDVDAudio
{
public:
  CDVDAudio();
  ~CDVDAudio();

  void RegisterAudioCallback(IAudioCallback* pCallback);
  void UnRegisterAudioCallback();

  int GetVolume();
  void SetVolume(int iVolume);
  void Pause();
  void Resume();
  bool Create(int iChannels, int iBitrate, int iBitsPerSample, bool bPasstrough);
  void Destroy();
  DWORD AddPackets(unsigned char* data, DWORD len);
  void DoWork();
  __int64 GetDelay(); // returns the time it takes to play a packet if we add one at this time
  void Flush();
  void WaitForBuffer(int iPacketsLeft);

  IDirectSoundRenderer* m_pAudioDecoder;
protected:

  IAudioCallback* m_pCallback;
  BYTE* m_pBuffer; // should be [m_dwPacketSize]
  int m_iBufferSize;
  DWORD m_dwPacketSize;
  CCriticalSection m_critSection;

  int m_iChannels;
  int m_iBitrate;
  int m_iBitsPerSample;
  int m_iPackets;
};
