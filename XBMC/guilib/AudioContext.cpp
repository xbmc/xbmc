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

#include "include.h"
#include "AudioContext.h"
#include "GUIAudioManager.h"
#include "IAudioDeviceChangedCallback.h"
#include "Settings.h"
#include "GUISettings.h"
#include "XBAudioConfig.h"
#ifdef _WIN32PC
#include "WINDirectSound.h"
#endif
extern HWND g_hWnd;


CAudioContext g_audioContext;

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
// Note: DEFAULT_DEVICE is created by the IAudioDeviceChangedCallback
void CAudioContext::SetActiveDevice(int iDevice)
{
  /* if device is the same, no need to bother */
#ifdef _WIN32PC
  if(m_iDevice == iDevice && g_guiSettings.GetString("audiooutput.audiodevice").Equals(m_strDevice))
#else
  if(m_iDevice == iDevice)
#endif
    return;

  if (iDevice==DEFAULT_DEVICE)
  {
    /* we just tell callbacks to init, it will setup audio */
    g_audioManager.Initialize(iDevice);
    return;
  }

  /* deinit current device */
  RemoveActiveDevice();

  m_iDevice=iDevice;
  m_strDevice=g_guiSettings.GetString("audiooutput.audiodevice");

#ifdef HAS_AUDIO
  memset(&g_digitaldevice, 0, sizeof(GUID));
  if (FAILED(DirectSoundEnumerate(DSEnumCallback, this)))
    CLog::Log(LOGERROR, "%s - failed to enumerate output devices", __FUNCTION__);

  if (iDevice==DIRECTSOUND_DEVICE
  ||  iDevice==DIRECTSOUND_DEVICE_DIGITAL)
  {
    LPGUID guid = NULL;
#ifdef _WIN32PC
    CWDSound p_dsound;
    std::vector<DSDeviceInfo > deviceList = p_dsound.GetSoundDevices();
    std::vector<DSDeviceInfo >::const_iterator iter = deviceList.begin();
    for (int i=0; iter != deviceList.end(); i++)
    {
      DSDeviceInfo dev = *iter;

      if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(dev.strDescription))
      {
        guid = dev.lpGuid;
        CLog::Log(LOGDEBUG, "%s - selecting %s as output devices", __FUNCTION__, dev.strDescription.c_str());
        break;
      }

      ++iter;
    }
#else
    if(iDevice == DIRECTSOUND_DEVICE_DIGITAL 
    && ( g_digitaldevice.Data1 || g_digitaldevice.Data2 
      || g_digitaldevice.Data3 || g_digitaldevice.Data4 ))
      guid = &g_digitaldevice;
#endif

    // Create DirectSound
    if (FAILED(DirectSoundCreate( guid, &m_pDirectSoundDevice, NULL )))
    {
      CLog::Log(LOGERROR, "DirectSoundCreate() Failed");
      return;
    }
    if (FAILED(m_pDirectSoundDevice->SetCooperativeLevel(g_hWnd, DSSCL_PRIORITY)))
    {
      CLog::Log(LOGERROR, "DirectSoundDevice::SetCooperativeLevel() Failed");
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
  else
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
  DWORD spconfig = DSSPEAKER_USE_DEFAULT;  
  if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL)
  {
    if (((g_guiSettings.GetBool("musicplayer.outputtoallspeakers")) && (bIsMusic)) || (g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers && !bIsMusic))
    {
      if( g_audioConfig.GetAC3Enabled() )
      {
        bAudioOnAllSpeakers = true;      
        m_bAC3EncoderActive = true;
        spconfig = DSSPEAKER_USE_DEFAULT; //Allows ac3 encoder should it be enabled
      }
      else
      {
        if (iChannels == 1)
          spconfig = DSSPEAKER_MONO;
        else
        { 
          spconfig = DSSPEAKER_STEREO;
        }
      }        
    }
    else
    {
      if (iChannels == 1)
        spconfig = DSSPEAKER_MONO;
      else if (iChannels == 2)
        spconfig = DSSPEAKER_STEREO;
      else
      {
        spconfig = DSSPEAKER_USE_DEFAULT; //Allows ac3 encoder should it be enabled
        m_bAC3EncoderActive = g_audioConfig.GetAC3Enabled();
      }
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
    spconfig_old = DSSPEAKER_USE_DEFAULT;
  }

  /* speaker config identical, no need to do anything */
  if(spconfig == spconfig_old) return;
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

