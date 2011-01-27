/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "AudioContext.h"
#include "GUIAudioManager.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#ifdef _WIN32
#include "WINDirectSound.h"
#endif
extern HWND g_hWnd;

#ifdef _WIN32
static GUID g_digitaldevice;
BOOL CALLBACK DSEnumCallback(
  LPGUID lpGuid,
  LPCSTR lpcstrDescription,
  LPCSTR lpcstrModule,
  LPVOID lpContext
)
{
  if(strstr(lpcstrDescription, "Digital Output") != NULL)
  {
    g_digitaldevice = *lpGuid;
    return false;
  }
  return true;
}
#endif

CAudioContext::CAudioContext()
{
  m_bAC3EncoderActive=false;
  m_iDevice=DEFAULT_DEVICE;
  m_strDevice.clear();
#ifdef HAS_AUDIO
#ifdef HAS_AUDIO_PASS_THROUGH
  m_pAC97Device=NULL;
#endif
  m_pDirectSoundDevice=NULL;
#endif
}

CAudioContext::~CAudioContext()
{
}

// \brief Create a new device by type (DEFAULT_DEVICE, DIRECTSOUND_DEVICE, AC97_DEVICE)
void CAudioContext::SetActiveDevice(int iDevice)
{
  CStdString strAudioDev = g_guiSettings.GetString("audiooutput.audiodevice");

  /* if device is the same, no need to bother */
#ifdef _WIN32

  HRESULT hr;
  int iPos = strAudioDev.Find(':');
  if(iPos != CStdString::npos)
    strAudioDev.erase(0, iPos+1);

  if (iDevice == DEFAULT_DEVICE)
    iDevice = DIRECTSOUND_DEVICE; // default device on win32 is directsound device
  if(m_iDevice == iDevice && strAudioDev.Equals(m_strDevice))
  {
    if (iDevice != NONE && m_pDirectSoundDevice)
    {
      DSCAPS devCaps = {0};
      devCaps.dwSize = sizeof(devCaps);
      HRESULT hr = m_pDirectSoundDevice->GetCaps(&devCaps);
      if (SUCCEEDED(hr)) // Make sure the DirectSound interface is still valid.
        return;
      CLog::Log(LOGDEBUG, "%s - DirectSoundDevice no longer valid and is going to be recreated (0x%08x)", __FUNCTION__, hr);
    }
  }

#else
  if(m_iDevice == iDevice)
    return;
#endif

  CLog::Log(LOGDEBUG, "%s - SetActiveDevice from %i to %i", __FUNCTION__, m_iDevice, iDevice);
  /* deinit current device */
  RemoveActiveDevice();

  m_iDevice=iDevice;
  m_strDevice=strAudioDev;

#ifdef HAS_AUDIO
  memset(&g_digitaldevice, 0, sizeof(GUID));
  hr = DirectSoundEnumerate(DSEnumCallback, this);
  if (FAILED(hr))
    CLog::Log(LOGERROR, "%s - failed to enumerate output devices (0x%08X)", __FUNCTION__, hr);

  if (iDevice==DIRECTSOUND_DEVICE
  ||  iDevice==DIRECTSOUND_DEVICE_DIGITAL)
  {
    LPGUID guid = NULL;
#ifdef _WIN32
    CWDSound p_dsound;
    std::vector<DSDeviceInfo > deviceList = p_dsound.GetSoundDevices();
    if (deviceList.size() == 0)
      CLog::Log(LOGDEBUG, "%s - no output devices found.", __FUNCTION__);

    std::vector<DSDeviceInfo >::const_iterator iter = deviceList.begin();
    for (int i=0; iter != deviceList.end(); i++)
    {
      DSDeviceInfo dev = *iter;

      if (strAudioDev.Equals(dev.strDescription))
      {
        guid = dev.lpGuid;
        CLog::Log(LOGDEBUG, "%s - selecting %s as output devices", __FUNCTION__, dev.strDescription.c_str());
        break;
      }

      ++iter;
    }
    if (guid == NULL)
      CLog::Log(LOGDEBUG, "%s - (default playback device).", __FUNCTION__);
#else
    if(iDevice == DIRECTSOUND_DEVICE_DIGITAL
    && ( g_digitaldevice.Data1 || g_digitaldevice.Data2
      || g_digitaldevice.Data3 || g_digitaldevice.Data4 ))
      guid = &g_digitaldevice;
#endif

    // Create DirectSound
    hr = DirectSoundCreate( guid, &m_pDirectSoundDevice, NULL );
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "DirectSoundCreate() Failed (0x%08X)", hr);
      return;
    }
    hr = m_pDirectSoundDevice->SetCooperativeLevel(g_hWnd, DSSCL_PRIORITY);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "DirectSoundDevice::SetCooperativeLevel() Failed (0x%08X)", hr);
      return;
    }
  }
  else if (iDevice == DIRECTSOUND_DEVICE_DIGITAL)
  {

  }
#ifdef HAS_AUDIO_PASS_THROUGH
  else if (iDevice==AC97_DEVICE)
  {
    // Create AC97 Device
    if (FAILED(Ac97CreateMediaObject(DSAC97_CHANNEL_DIGITAL, NULL, NULL, &m_pAC97Device)))
    {
      CLog::Log(LOGERROR, "Failed to create digital Ac97CreateMediaObject()");
      return;
    }
  }
#endif
  // Don't log an error if the caller specifically asked for no device
  // externalplayer does this to ensure all audio devices are closed
  // during external playback
  else if (iDevice != NONE)
  {
    CLog::Log(LOGERROR, "Failed to create audio device");
    return;
  }
#endif
  g_audioManager.Initialize(m_iDevice);
}

// \brief Return the active device type (NONE, DEFAULT_DEVICE, DIRECTSOUND_DEVICE, AC97_DEVICE)
int CAudioContext::GetActiveDevice()
{
  return m_iDevice;
}

// \brief Remove the current sound device, eg. to setup new speaker config
void CAudioContext::RemoveActiveDevice()
{
  CLog::Log(LOGDEBUG, "%s - Removing device %i", __FUNCTION__, m_iDevice);
  g_audioManager.DeInitialize(m_iDevice);
  m_iDevice=NONE;

#ifdef HAS_AUDIO
#ifdef HAS_AUDIO_PASS_THROUGH
  SAFE_RELEASE(m_pAC97Device);
#endif
  SAFE_RELEASE(m_pDirectSoundDevice);
#endif
}

// \brief set a new speaker config
void CAudioContext::SetupSpeakerConfig(int iChannels, bool& bAudioOnAllSpeakers, bool bIsMusic)
{
  m_bAC3EncoderActive = false;
  bAudioOnAllSpeakers = false;

#ifdef HAS_AUDIO
  return; //not implemented
  DWORD spconfig = DSSPEAKER_USE_DEFAULT;
  if (AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode")))
  {
    if (g_settings.m_currentVideoSettings.m_OutputToAllSpeakers && !bIsMusic)
    {
      bAudioOnAllSpeakers = true;
      m_bAC3EncoderActive = true;
      spconfig = DSSPEAKER_USE_DEFAULT; //Allows ac3 encoder should it be enabled
    }
    else
    {
      if (iChannels == 1)
        spconfig = DSSPEAKER_MONO;
      else if (iChannels == 2)
        spconfig = DSSPEAKER_STEREO;
      else
        spconfig = DSSPEAKER_USE_DEFAULT; //Allows ac3 encoder should it be enabled
    }
  }
  else // We don't want to use the Dolby Digital Encoder output. Downmix to surround instead.
  {
    if (iChannels == 1)
      spconfig = DSSPEAKER_MONO;
    else
    {
      // check if surround mode is allowed, if not then use normal stereo
      // don't always set it to default as that enabled ac3 encoder if that is allowed in dash
      // ruining quality
      spconfig = DSSPEAKER_STEREO;
    }
  }

  DWORD spconfig_old = DSSPEAKER_USE_DEFAULT;
  if(m_pDirectSoundDevice)
  {
    m_pDirectSoundDevice->GetSpeakerConfig(&spconfig_old);
    spconfig_old = DSSPEAKER_CONFIG(spconfig_old);
  }

  /* speaker config identical, no need to do anything */
  if(spconfig == spconfig_old) return;
  CLog::Log(LOGDEBUG, "%s - Speakerconfig changed from %i to %i", __FUNCTION__, spconfig_old, spconfig);
#endif

  /* speaker config has changed, caller need to recreate it */
  RemoveActiveDevice();
}

bool CAudioContext::IsAC3EncoderActive() const
{
  return m_bAC3EncoderActive;
}

bool CAudioContext::IsPassthroughActive() const
{
  return (m_iDevice == DIRECTSOUND_DEVICE_DIGITAL);
}

