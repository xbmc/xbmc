/*!
\file AudioContext.h
\brief 
*/

#pragma once
#include "IAudioDeviceChangedCallback.h"

#define DSMIXBINTYPE_STANDARD 1
#define DSMIXBINTYPE_DMO 2
#define DSMIXBINTYPE_AAC 3
#define DSMIXBINTYPE_OGG 4
#define DSMIXBINTYPE_CUSTOM 5
#define DSMIXBINTYPE_STEREOALL 6
#define DSMIXBINTYPE_STEREOLEFT 7
#define DSMIXBINTYPE_STEREORIGHT 8

class CAudioContext
{
public:
  CAudioContext();
  virtual ~CAudioContext();

  void                        SetSoundDeviceCallback(IAudioDeviceChangedCallback* pCallback);

  void                        SetActiveDevice(int iDevice);
  int                         GetActiveDevice();
  void                        RemoveActiveDevice();

  LPDIRECTSOUND8              GetDirectSoundDevice() { return m_pDirectSoundDevice; }
  LPAC97MEDIAOBJECT           GetAc97Device() { return m_pAC97Device; }
  
  void                        SetupSpeakerConfig(int iChannels, bool& bAudioOnAllSpeakers, bool bIsMusic=true);
  bool                        IsAC3EncoderActive();

  bool                        GetMixBin(DSMIXBINVOLUMEPAIR* dsmbvp, int* MixBinCount, DWORD* dwChannelMask, int Type, int Channels);

  enum AUDIO_DEVICE {NONE=0, DEFAULT_DEVICE, DIRECTSOUND_DEVICE, AC97_DEVICE };
protected:
  LPAC97MEDIAOBJECT            m_pAC97Device;
  LPDIRECTSOUND8               m_pDirectSoundDevice;

  int                          m_iDevice;
  IAudioDeviceChangedCallback* m_pCallback;
  bool                         m_bAC3EncoderActive;
};

extern CAudioContext g_audioContext;