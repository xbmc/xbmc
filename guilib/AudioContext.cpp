#include "stdafx.h"
#include "AudioContext.h"
#include "../xbmc/Settings.h"

CAudioContext g_audioContext;

CAudioContext::CAudioContext()
{
  m_iDevice=DEFAULT_DEVICE;
  m_pAC97Device=NULL;
  m_pDirectSoundDevice=NULL;
}

CAudioContext::~CAudioContext()
{
}

void CAudioContext::SetSoundDeviceCallback(IAudioDeviceChangedCallback* pCallback)
{
  m_pCallback=pCallback;
}

// \brief Create a new device by type (DEFAULT_DEVICE, DIRECTSOUND_DEVICE, AC97_DEVICE)
// Note: DEFAULT_DEVICE is created by the IAudioDeviceChangedCallback
void CAudioContext::SetActiveDevice(int iDevice)
{
  m_iDevice=iDevice;

  if (iDevice==DIRECTSOUND_DEVICE)
  {
    SAFE_RELEASE(m_pAC97Device);
    SAFE_RELEASE(m_pDirectSoundDevice);

    // Create DirectSound
    if (FAILED(DirectSoundCreate( NULL, &m_pDirectSoundDevice, NULL )))
    {
      CLog::Log(LOGERROR, "DirectSoundCreate() Failed");
      return;
    }
  }
  else if (iDevice==AC97_DEVICE)
  {
    if (m_pCallback)
      m_pCallback->DeInitialize();

    SAFE_RELEASE(m_pDirectSoundDevice);
    SAFE_RELEASE(m_pAC97Device);

    // Create AC97 Device
    if (FAILED(Ac97CreateMediaObject(DSAC97_CHANNEL_DIGITAL, NULL, NULL, &m_pAC97Device)))
    {
      CLog::Log(LOGERROR, "Failed to create digital Ac97CreateMediaObject()");
      return;
    }
  }

  if (m_pCallback)
    m_pCallback->Initialize();
}

// \brief Return the active device type (NONE, DEFAULT_DEVICE, DIRECTSOUND_DEVICE, AC97_DEVICE)
int CAudioContext::GetActiveDevice()
{
  return m_iDevice;
}

// \brief Remove the current sound device, eg. to setup new speaker config
void CAudioContext::RemoveActiveDevice()
{
  m_iDevice=NONE;

  if (m_pCallback)
    m_pCallback->DeInitialize();

  SAFE_RELEASE(m_pAC97Device);
  SAFE_RELEASE(m_pDirectSoundDevice);
}

// \brief set a new speaker config
void CAudioContext::SetupSpeakerConfig(int iChannels, bool& bAudioOnAllSpeakers)
{
  if (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL)
  {
    if (g_guiSettings.GetBool("AudioOutput.OutputToAllSpeakers"))
    {
      bAudioOnAllSpeakers = true;
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);
    }
    else
    {
      if (iChannels == 1)
        DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
      else if (iChannels == 2)
        DirectSoundOverrideSpeakerConfig(DSSPEAKER_STEREO);
      else
        DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);
    }
  }
  else // We don't want to use the Dolby Digital Encoder output. Downmix to surround instead.
  {
    if (iChannels == 1)
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
    else
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT);
  }
}
