/*!
\file AudioContext.h
\brief 
*/

#pragma once
#include "IAudioDeviceChangedCallback.h"

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

          void                        SetupSpeakerConfig(int iChannels, bool& bAudioOnAllSpeakers);

  enum AUDIO_DEVICE {NONE=0, DEFAULT_DEVICE, DIRECTSOUND_DEVICE, AC97_DEVICE };
protected:
          LPAC97MEDIAOBJECT            m_pAC97Device;
          LPDIRECTSOUND8               m_pDirectSoundDevice;

          int                          m_iDevice;
          IAudioDeviceChangedCallback* m_pCallback;
};

extern CAudioContext g_audioContext;