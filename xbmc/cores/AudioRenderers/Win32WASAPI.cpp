/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "system.h" // WIN32INCLUDES needed for the WASAPI stuff below

#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <initguid.h>
#include <Mmreg.h>
#include "Win32WASAPI.h"
#include "AudioContext.h"
#include "Settings.h"
#include "SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "CharsetConverter.h"

#pragma comment(lib, "Avrt.lib")

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_PCM, WAVE_FORMAT_PCM, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

const int wasapi_channel_mask[] = 
{
  SPEAKER_FRONT_CENTER,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT   | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER | SPEAKER_SIDE_LEFT    | SPEAKER_SIDE_RIGHT     | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER | SPEAKER_SIDE_LEFT    | SPEAKER_SIDE_RIGHT     | SPEAKER_BACK_CENTER | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT     | SPEAKER_SIDE_LEFT   | SPEAKER_SIDE_RIGHT | SPEAKER_LOW_FREQUENCY
};

const enum PCMChannels wasapi_channel_order[] = {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_FRONT_LEFT_OF_CENTER, PCM_FRONT_RIGHT_OF_CENTER, PCM_BACK_CENTER, PCM_SIDE_LEFT, PCM_SIDE_RIGHT};

#define WASAPI_CHANNEL_MASK_COUNT 8
#define WASAPI_TOTAL_CHANNELS 11

#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason, __VA_ARGS__); goto failed;}

//This needs to be static since only one exclusive stream can exist at one time.
bool CWin32WASAPI::m_bIsAllocated = false;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CWin32WASAPI::CWin32WASAPI() :
  m_bPassthrough(false),
  m_uiAvgBytesPerSec(0),
  m_CacheLen(0),
  m_uiChunkSize(0),
  m_uiBufferLen(0),
  m_PreCacheSize(0),
  m_LastCacheCheck(0),
  m_pAudioClient(NULL),
  m_pRenderClient(NULL),
  m_pDevice(NULL)
{
}

bool CWin32WASAPI::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bAudioPassthrough)
{
  //First check if the version of Windows we are running on even supports WASAPI.
  OSVERSIONINFO winVersion;
  winVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  GetVersionEx(&winVersion);

  if(winVersion.dwMajorVersion < 6)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Version %i.%i of Windows detected.  WASAPI output requires version 6.0 (Vista) or higher.", winVersion.dwMajorVersion, winVersion.dwMinorVersion);
    return false;
  }

  //If there is a listing of required channels, build the speaker mask and mapping from that.
  //Otherwise, use the default.
  if(channelMap)
    BuildChannelMapping(iChannels, channelMap);
  else
  {
    m_uiSpeakerMask = wasapi_channel_mask[iChannels-1];
    for(int i = 0; i < iChannels; i++)
      m_SpeakerOrder[i] = wasapi_channel_order[i];
  }

  m_remap.SetInputFormat (iChannels, channelMap, uiBitsPerSample / 8);
  m_remap.SetOutputFormat(iChannels, m_SpeakerOrder);

  //Only one exclusive stream may be initialized at one time.
  if(m_bIsAllocated)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Cannot create more then one WASAPI stream at one time.");
    return false;
  }

  //Make sure we support the correct number of channels.
  if(iChannels > WASAPI_CHANNEL_MASK_COUNT)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Unsupported number of channels. %i specified while max is %i", iChannels, WASAPI_CHANNEL_MASK_COUNT);
    return false;
  }

  m_bPlaying = false;
  m_bPause = false;
  m_bMuting = false;
  m_uiChannels = iChannels;
  m_uiBitsPerSample = uiBitsPerSample;
  m_bPassthrough = bAudioPassthrough;

  m_nCurrentVolume = g_settings.m_nVolumeLevel;
  m_fVolAdjustFactor = 1.0f - ((float)m_nCurrentVolume / -6000.0f);
  
  WAVEFORMATEXTENSIBLE wfxex = {0};

  //fill waveformatex
  ZeroMemory(&wfxex, sizeof(WAVEFORMATEXTENSIBLE));
  wfxex.Format.cbSize          =  sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = iChannels;
  wfxex.Format.nSamplesPerSec  = uiSamplesPerSec;
  if (bAudioPassthrough == true) 
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
    wfxex.Format.nChannels       = 2;
  } 
  else
  {
    wfxex.dwChannelMask          = m_uiSpeakerMask;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat              = KSDATAFORMAT_SUBTYPE_PCM;
    wfxex.Format.wBitsPerSample  = uiBitsPerSample == 24 ? 32 : uiBitsPerSample;
  }

  wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
  wfxex.Format.nBlockAlign       = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

  m_uiAvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  m_uiBytesPerFrame = wfxex.Format.nBlockAlign;

  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

  //Shut down Directsound.
  g_audioContext.SetActiveDevice(CAudioContext::NONE);

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %i", hr)

  //Get our device.
  //First try to find the named device.
  UINT uiCount = 0;

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  for(UINT i = 0; i < uiCount; i++)
  {
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;

    hr = pEnumDevices->Item(i, &m_pDevice);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint failed.")

    hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if(FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint device name failed.");
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    CStdStringW strRawDevName(varName.pwszVal);
    CStdString strDevName;
    g_charsetConverter.ucs2CharsetToStringCharset(strRawDevName, strDevName);

    if(device == strDevName)
      i = uiCount;
    else
      SAFE_RELEASE(m_pDevice);

    PropVariantClear(&varName);
    SAFE_RELEASE(pProperty);
  }

  SAFE_RELEASE(pEnumDevices);

  if(!m_pDevice)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Could not locate the device named \"%s\" in the list of WASAPI endpoint devices.  Trying the default device...", device.c_str());
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Could not retrieve the default WASAPI audio endpoint.")
  }

  //We are done with the enumerator.
  SAFE_RELEASE(pEnumerator);

  hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Activating the WASAPI endpoint device failed.")

  hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Audio format not supported by the WASAPI device.  Channels: %i, Rate: %i, Bits/sample: %i.", iChannels, uiSamplesPerSec, uiBitsPerSample)

  REFERENCE_TIME hnsRequestedDuration, hnsPeriodicity;
  hr = m_pAudioClient->GetDevicePeriod(NULL, &hnsRequestedDuration);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not retrieve the WASAPI endpoint device period.");

  hnsPeriodicity = hnsRequestedDuration; 

  //PAPlayer seems to need a larger buffer and chunk size then what some devices give by default.
  if(bIsMusic && hnsRequestedDuration < 60000)
    hnsRequestedDuration = 60000;

  // now create the stream buffer
  hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, 0, hnsRequestedDuration*4, hnsPeriodicity, &wfxex.Format, NULL);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not initialize the WASAPI endpoint device.")

  hr = m_pAudioClient->GetBufferSize(&m_uiBufferLen);
  m_uiBufferLen *= m_uiBytesPerFrame;

  //Chunk sizes are 1/4 the buffer size.
  //WASAPI chunk sizes need to be evenly divisable into the buffer size or pops and clicks will result.
  m_uiChunkSize = m_uiBufferLen / 4;

  if(bIsMusic)
    m_PreCacheSize = m_uiBufferLen / 2; //Start music only when the buffer is half filled.
  else
    m_PreCacheSize = m_uiChunkSize;

  CLog::Log(LOGDEBUG, __FUNCTION__": Packet Size = %d. Avg Bytes Per Second = %d.", m_uiChunkSize, m_uiAvgBytesPerSec);

  hr = m_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&m_pRenderClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not initialize the WASAPI render client interface.")

  m_bIsAllocated = true;
  m_CacheLen = 0;
  m_LastCacheCheck = CTimeUtils::GetTimeMS();
  
  return m_bIsAllocated;

failed:
  CLog::Log(LOGERROR, __FUNCTION__": WASAPI initialization failed.");
  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);
  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pDevice);

  //Restart Directsound
  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return false;
}

//***********************************************************************************************
CWin32WASAPI::~CWin32WASAPI()
{
  Deinitialize();
}

//***********************************************************************************************
bool CWin32WASAPI::Deinitialize()
{
  if (m_bIsAllocated)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Cleaning up");

    m_pAudioClient->Stop();

    SAFE_RELEASE(m_pRenderClient)
    SAFE_RELEASE(m_pAudioClient)
    SAFE_RELEASE(m_pDevice)

    m_CacheLen = 0;
    m_uiChunkSize = 0;
    m_uiBufferLen = 0;

    m_bIsAllocated = false;

    //Restart Directsound for the interface sounds.
    g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  }
  return true;
}

//***********************************************************************************************
bool CWin32WASAPI::Pause()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return false;

  if (m_bPause) // Already paused
    return true;

  m_bPause = true;
  m_pAudioClient->Stop();

  return true;
}

//***********************************************************************************************
bool CWin32WASAPI::Resume()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return false;

  if(!m_bPause) // Already playing
    return true;

  m_bPause = false;

  UpdateCacheStatus();
  if(m_CacheLen >= m_PreCacheSize) // Make sure we have some data to play (if not, playback will start when we add some)
    m_pAudioClient->Start();
  else
    m_bPlaying = false;  // Trigger playback restart the next time data is added to the buffer.

  return true;
}

//***********************************************************************************************
bool CWin32WASAPI::Stop()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return false;

  // Stop and reset WASAPI buffer
  m_pAudioClient->Stop();
  m_pAudioClient->Reset();

  // Reset buffer management members
  m_CacheLen = 0;
  m_bPause = false;
  m_bPlaying = false;

  return true;
}

//***********************************************************************************************
long CWin32WASAPI::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CWin32WASAPI::Mute(bool bMute)
{
  CSingleLock lock (m_critSection);

  m_bMuting = bMute;
}

//***********************************************************************************************
bool CWin32WASAPI::SetCurrentVolume(long nVolume)
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return false;

  m_nCurrentVolume = nVolume;
  m_fVolAdjustFactor = 1.0f - ((float)nVolume / -6000.0f);
  return true;
}

//***********************************************************************************************
unsigned int CWin32WASAPI::AddPackets(const void* data, unsigned int len)
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return 0;

  DWORD dwFlags = m_bMuting ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

  unsigned int uiBytesToWrite;
  BYTE* pBuffer = NULL;

  uiBytesToWrite = std::min(GetSpace(), len);
  uiBytesToWrite /= m_uiChunkSize;
  uiBytesToWrite *= m_uiChunkSize;

  if(uiBytesToWrite == 0)
    return 0;

  // Get the buffer
  m_pRenderClient->GetBuffer(uiBytesToWrite/m_uiBytesPerFrame, &pBuffer);

  // Write data into the buffer
  AddDataToBuffer((unsigned char*)data, uiBytesToWrite, pBuffer);

  // Release the buffer
  m_pRenderClient->ReleaseBuffer(uiBytesToWrite/m_uiBytesPerFrame, dwFlags);

  m_CacheLen += uiBytesToWrite;

  CheckPlayStatus();

  return uiBytesToWrite; // Bytes used
}

void CWin32WASAPI::UpdateCacheStatus()
{
  unsigned int time = CTimeUtils::GetTimeMS();
  if (time == m_LastCacheCheck)
    return; // Don't recalc more frequently than once/ms (that is our max resolution anyway)

  m_LastCacheCheck = time;

  m_pAudioClient->GetCurrentPadding(&m_CacheLen);
  m_CacheLen *= m_uiBytesPerFrame;
}

void CWin32WASAPI::CheckPlayStatus()
{
  if(!m_bPause && !m_bPlaying && m_CacheLen >= m_PreCacheSize) // If we have some data, see if we can start playback
  {
	  m_pAudioClient->Start();
	  m_bPlaying = true;
  }
}

unsigned int CWin32WASAPI::GetSpace()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return 0;

  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  return m_uiBufferLen - m_CacheLen;
}

//***********************************************************************************************
float CWin32WASAPI::GetDelay()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return 0.0f;

  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  return (float)m_CacheLen / (float)m_uiAvgBytesPerSec;
}

//***********************************************************************************************
float CWin32WASAPI::GetCacheTime()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return 0.0f;

  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  return (float)m_CacheLen / (float)m_uiAvgBytesPerSec;
}

//***********************************************************************************************
float CWin32WASAPI::GetCacheTotal()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return 0.0f;

  return (float)m_uiBufferLen / (float)m_uiAvgBytesPerSec;
}

//***********************************************************************************************
unsigned int CWin32WASAPI::GetChunkLen()
{
  return m_uiChunkSize;
}

//***********************************************************************************************
int CWin32WASAPI::SetPlaySpeed(int iSpeed)
{
  return 0;
}

//***********************************************************************************************
void CWin32WASAPI::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

//***********************************************************************************************
void CWin32WASAPI::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

//***********************************************************************************************

void CWin32WASAPI::EnumerateAudioSinks(AudioSinkList &vAudioSinks, bool passthrough)
{
  //First check if the version of Windows we are running on even supports WASAPI.
  OSVERSIONINFO winVersion;
  winVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  GetVersionEx(&winVersion);

  if(winVersion.dwMajorVersion < 6)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Version %i.%i of Windows detected.  WASAPI enumeration requires version 6.0 (Vista) or higher.", winVersion.dwMajorVersion, winVersion.dwMinorVersion);
    return;
  }

  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %i", hr)

  UINT uiCount = 0;

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  for(UINT i = 0; i < uiCount; i++)
  {
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;

    pEnumDevices->Item(i, &pDevice);
    if(FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint failed.");

      goto failed;
    }

    hr = pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    if(FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.");
      SAFE_RELEASE(pDevice)

      goto failed;
    }

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if(FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint device name failed.");
      SAFE_RELEASE(pDevice)
      SAFE_RELEASE(pProperty)

      goto failed;
    }

    CStdStringW strRawDevName(varName.pwszVal);
    CStdString strDevName;
    g_charsetConverter.wToUTF8(strRawDevName, strDevName);

    vAudioSinks.push_back(AudioSink(CStdString("WASAPI: ").append(strDevName), CStdString("wasapi:").append(strDevName)));

    SAFE_RELEASE(pDevice)

    PropVariantClear(&varName);
    SAFE_RELEASE(pProperty)
  }

failed:

  if(FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__": Failed to enumerate WASAPI endpoint devices.");

  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);
}

//***********************************************************************************************
void CWin32WASAPI::WaitCompletion()
{
  CSingleLock lock (m_critSection);

  if (!m_bIsAllocated)
    return;

  DWORD dwTimeRemaining;

  if(!m_bPlaying)
    return; // We weren't playing anyway

  //Calculate the remaining cache time and wait for it to finish.
  dwTimeRemaining = (DWORD)(1000 * GetDelay());
  Sleep(dwTimeRemaining);

  m_pAudioClient->Stop();
  m_pAudioClient->Reset();

  m_CacheLen = 0;
  m_bPause = false;
  m_bPlaying = false;
}

//***********************************************************************************************
void CWin32WASAPI::AddDataToBuffer(unsigned char* pData, unsigned int len, unsigned char* pOut)
{
  // Adjust the volume if necessary.
  if(m_nCurrentVolume != 0 && !m_bPassthrough) 
  { 
    if(m_uiBitsPerSample == 16)
    {
      short* pOutSample = (short*)pData; 
      for(unsigned int i = 0; i < len >> 1; i++) 
      { 
        pOutSample[i] = (short)(((float)pOutSample[i] * m_fVolAdjustFactor) + 0.5f); 
      }
    }
    else if(m_uiBitsPerSample == 32)
    {
      int* pOutSample = (int*)pData; 
      for(unsigned int i = 0; i < len >> 1; i++) 
      { 
        pOutSample[i] = (int)(((float)pOutSample[i] * m_fVolAdjustFactor) + 0.5f); 
      }
    }
  }

  // Remap the data to the correct channels
  if (m_remap.CanRemap())
    m_remap.Remap((void*)pData, pOut, len / m_uiBytesPerFrame);
  else
    memcpy(pOut, pData, len);
}

//***********************************************************************************************
void CWin32WASAPI::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
  return;
}

//***********************************************************************************************
void CWin32WASAPI::BuildChannelMapping(int channels, enum PCMChannels* map)
{
  bool usedChannels[WASAPI_TOTAL_CHANNELS];

  memset(usedChannels, false, sizeof(usedChannels));

  m_uiSpeakerMask = 0;

  //Build the speaker mask and note which are used.
  for(int i = 0; i < channels; i++)
  {
    switch(map[i])
    {
    case PCM_FRONT_LEFT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_LEFT;
      break;
    case PCM_FRONT_RIGHT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_RIGHT;
      break;
    case PCM_FRONT_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_CENTER;
      break;
    case PCM_LOW_FREQUENCY:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_LOW_FREQUENCY;
      break;
    case PCM_BACK_LEFT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_BACK_LEFT;
      break;
    case PCM_BACK_RIGHT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_BACK_RIGHT;
      break;
    case PCM_FRONT_LEFT_OF_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_LEFT_OF_CENTER;
      break;
    case PCM_FRONT_RIGHT_OF_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_FRONT_RIGHT_OF_CENTER;
      break;
    case PCM_BACK_CENTER:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_BACK_CENTER;
      break;
    case PCM_SIDE_LEFT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_SIDE_LEFT;
      break;
    case PCM_SIDE_RIGHT:
      usedChannels[map[i]] = true;
      m_uiSpeakerMask |= SPEAKER_SIDE_RIGHT;
      break;
    }
  }

  //Assemble a compacted channel set.
  for(int i = 0, j = 0; i < WASAPI_TOTAL_CHANNELS; i++)
  {
    if(usedChannels[i])
    {
      m_SpeakerOrder[j] = wasapi_channel_order[i];
      j++;
    }
  }
}

