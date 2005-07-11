#include "include.h"
#include "AudioContext.h"
#include "../xbmc/Settings.h"
#include "../xbmc/XBAudioConfig.h"

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
void CAudioContext::SetupSpeakerConfig(int iChannels, bool& bAudioOnAllSpeakers, bool bIsMusic)
{
  m_bAC3EncoderActive = false;

  if (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL)
  {
    if (((g_guiSettings.GetBool("AudioMusic.OutputToAllSpeakers")) && (bIsMusic)) || (g_guiSettings.GetBool("AudioVideo.OutputToAllSpeakers") && !bIsMusic))
    {
      bAudioOnAllSpeakers = true;
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT); //Allows ac3 encoder should it be enabled
      m_bAC3EncoderActive = g_audioConfig.GetAC3Enabled();
    }
    else
    {
      if (iChannels == 1)
        DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
      else if (iChannels == 2)
        DirectSoundOverrideSpeakerConfig(DSSPEAKER_STEREO);
      else
      {
        DirectSoundOverrideSpeakerConfig(DSSPEAKER_USE_DEFAULT); //Allows ac3 encoder should it be enabled
        m_bAC3EncoderActive = g_audioConfig.GetAC3Enabled();
         
      }
    }
  }
  else // We don't want to use the Dolby Digital Encoder output. Downmix to surround instead.
  {
    if (iChannels == 1)
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_MONO);
    else
      DirectSoundOverrideSpeakerConfig(DSSPEAKER_SURROUND); //Will turn of ac3 encoder and force downmix
  }
}

bool CAudioContext::IsAC3EncoderActive()
{
  return m_bAC3EncoderActive;
}

bool CAudioContext::GetMixBin(DSMIXBINVOLUMEPAIR* dsmbvp, int* MixBinCount, DWORD* dwChannelMask, int Type, int Channels)
{
  //3, 5, >6 channel are invalid XBOX wav formats thus can not be processed at this stage

  *MixBinCount = Channels;

  if (Channels == 6) //Handle 6 channels.
  {
    *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;

    switch (Type)
    {
    case DSMIXBINTYPE_DMO:  //FL, FR, C, LFE, SL, SR
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_AAC:  //C, FL, FR, SL, SR, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_OGG:  //FL, C, FR, SL, SR, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_STANDARD:  //FL, FR, SL, SR, C, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    }
    //Didn't manage to get anything
    CLog::Log(LOGERROR, "Invalid Mixbin type specified, reverting to standard");
    GetMixBin(dsmbvp, MixBinCount, dwChannelMask, DSMIXBINTYPE_STANDARD, Channels);
    return true;
  }
  else if (Channels == 4)
  {
    DSMIXBINVOLUMEPAIR dsm[4] = { DSMIXBINVOLUMEPAIRS_DEFAULT_4CHANNEL };
    memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
    *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    return true;
  }
  else if (Channels == 2)
  {
    if ( Type == DSMIXBINTYPE_STEREOALL )
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_FRONT_RIGHT, 0},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_BACK_RIGHT, 0},
          // left and right both to center and LFE, but attenuate each 3dB first
          // so they're the same level.
          // attenuate the center another 3dB so that it is a total 6dB lower
          // so that stereo effect is not lost.
          {DSMIXBIN_LOW_FREQUENCY, -301},
          {DSMIXBIN_LOW_FREQUENCY, -301},
          {DSMIXBIN_FRONT_CENTER, -602},
          {DSMIXBIN_FRONT_CENTER, -602}
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else if (Type == DSMIXBINTYPE_STEREOLEFT)
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          // left route to 4 channels
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_FRONT_RIGHT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_RIGHT, 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else if (Type == DSMIXBINTYPE_STEREORIGHT)
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          // right route to 4 channels
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_FRONT_RIGHT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_RIGHT, 0},
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else
    {
      DSMIXBINVOLUMEPAIR dsm[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    }
    return true;
  }
  else if (Channels == 1)
  {
    *MixBinCount = 2;
    DSMIXBINVOLUMEPAIR dsm[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_MONO };
    memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
    *dwChannelMask = SPEAKER_FRONT_LEFT;
    return true;
  }
  CLog::Log(LOGERROR, "Invalid Mixbin channels specified, get MixBins failed");
  return false;
}
