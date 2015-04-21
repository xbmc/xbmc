/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define INITGUID

#include <algorithm>

#include "AESinkDirectSound.h"
#include "utils/log.h"
#include <initguid.h>
#include <list>
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include <Audioclient.h>
#include <Mmreg.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Rpc.h>
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/StringUtils.h"
#pragma comment(lib, "Rpcrt4.lib")

extern HWND g_hWnd;

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason " - %s", __VA_ARGS__, WASAPIErrToStr(hr)); goto failed;}

#define ERRTOSTR(err) case err: return #err

#define DS_SPEAKER_COUNT 8
static const unsigned int DSChannelOrder[] = {SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_CENTER, SPEAKER_LOW_FREQUENCY, SPEAKER_BACK_LEFT, SPEAKER_BACK_RIGHT, SPEAKER_SIDE_LEFT, SPEAKER_SIDE_RIGHT};
static const enum AEChannel AEChannelNames[] = {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

static enum AEChannel layoutsByChCount[9][9] = {
    {AE_CH_NULL},
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_BC, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_LFE, AE_CH_NULL}};

struct DSDevice
{
  std::string name;
  LPGUID     lpGuid;
};

struct winEndpointsToAEDeviceType
{
  std::string winEndpointType;
  AEDeviceType aeDeviceType;
};

static const winEndpointsToAEDeviceType winEndpoints[EndpointFormFactor_enum_count] =
{
  {"Network Device - ",         AE_DEVTYPE_PCM},
  {"Speakers - ",               AE_DEVTYPE_PCM},
  {"LineLevel - ",              AE_DEVTYPE_PCM},
  {"Headphones - ",             AE_DEVTYPE_PCM},
  {"Microphone - ",             AE_DEVTYPE_PCM},
  {"Headset - ",                AE_DEVTYPE_PCM},
  {"Handset - ",                AE_DEVTYPE_PCM},
  {"Digital Passthrough - ", AE_DEVTYPE_IEC958},
  {"SPDIF - ",               AE_DEVTYPE_IEC958},
  {"HDMI - ",                  AE_DEVTYPE_HDMI},
  {"Unknown - ",                AE_DEVTYPE_PCM},
};

// implemented in AESinkWASAPI.cpp
extern std::string localWideToUtf(LPCWSTR wstr);

static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
  DSDevice dev;
  std::list<DSDevice> &enumerator = *static_cast<std::list<DSDevice>*>(lpContext);

  int bufSize = MultiByteToWideChar(CP_ACP, 0, lpcstrDescription, -1, NULL, 0);
  wchar_t *strW = new wchar_t[bufSize+1];
  if ( bufSize == 0 || MultiByteToWideChar(CP_ACP, 0, lpcstrDescription, -1, strW, bufSize) != bufSize )
    strW[0] = 0;
  else
    strW[bufSize] = 0;

  dev.name = localWideToUtf(strW);
  delete[] strW;

  dev.lpGuid = lpGuid;

  if (lpGuid)
    enumerator.push_back(dev);

  return TRUE;
}

CAESinkDirectSound::CAESinkDirectSound() :
  m_pBuffer       (NULL ),
  m_pDSound       (NULL ),
  m_encodedFormat (AE_FMT_INVALID),
  m_AvgBytesPerSec(0    ),
  m_dwChunkSize   (0    ),
  m_dwFrameSize   (0    ),
  m_dwBufferLen   (0    ),
  m_BufferOffset  (0    ),
  m_CacheLen      (0    ),
  m_LastCacheCheck(0    ),
  m_BufferTimeouts(0    ),
  m_running       (false),
  m_initialized   (false),
  m_isDirtyDS     (false)
{
  m_channelLayout.Reset();
}

CAESinkDirectSound::~CAESinkDirectSound()
{
  Deinitialize();
}

bool CAESinkDirectSound::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  LPGUID deviceGUID = NULL;
  RPC_CSTR wszUuid  = NULL;
  HRESULT hr = E_FAIL;
  std::string strDeviceGUID = device;
  std::list<DSDevice> DSDeviceList;
  std::string deviceFriendlyName;
  DirectSoundEnumerate(DSEnumCallback, &DSDeviceList);

  if(StringUtils::EndsWithNoCase(device, std::string("default")))
    strDeviceGUID = GetDefaultDevice();

  for (std::list<DSDevice>::iterator itt = DSDeviceList.begin(); itt != DSDeviceList.end(); ++itt)
  {
    if ((*itt).lpGuid)
    {
      hr = (UuidToString((*itt).lpGuid, &wszUuid));
      std::string sztmp = (char*)wszUuid;
      std::string szGUID = "{" + std::string(sztmp.begin(), sztmp.end()) + "}";
      if (strcasecmp(szGUID.c_str(), strDeviceGUID.c_str()) == 0)
      {
        deviceGUID = (*itt).lpGuid;
        deviceFriendlyName = (*itt).name.c_str();
        break;
      }
    }
    if (hr == RPC_S_OK) RpcStringFree(&wszUuid);
  }

  hr = DirectSoundCreate(deviceGUID, &m_pDSound, NULL);

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create the DirectSound device %s with error %s, trying the default device.", deviceFriendlyName.c_str(), dserr2str(hr));
    hr = DirectSoundCreate(NULL, &m_pDSound, NULL);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create the default DirectSound device with error %s.", dserr2str(hr));
      return false;
    }
  }

  HWND tmp_hWnd;

  /* Dodge the null handle on first init by using desktop handle */
  if (g_hWnd == NULL)
    tmp_hWnd = GetDesktopWindow();
  else
    tmp_hWnd = g_hWnd;

  CLog::Log(LOGDEBUG, __FUNCTION__": Using Window handle: %d", tmp_hWnd);

  hr = m_pDSound->SetCooperativeLevel(tmp_hWnd, DSSCL_PRIORITY);

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create the DirectSound device cooperative level.");
    CLog::Log(LOGERROR, __FUNCTION__": DSErr: %s", dserr2str(hr));
    m_pDSound->Release();
    return false;
  }

  WAVEFORMATEXTENSIBLE wfxex = {0};

  // clamp samplerate to a minimum
  if (format.m_sampleRate < 44100)
    format.m_sampleRate = 44100;

  //fill waveformatex
  ZeroMemory(&wfxex, sizeof(WAVEFORMATEXTENSIBLE));
  wfxex.Format.cbSize          = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = format.m_channelLayout.Count();
  wfxex.Format.nSamplesPerSec  = format.m_sampleRate;
  if (AE_IS_RAW(format.m_dataFormat))
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
    wfxex.Format.nChannels       = 2;
  }
  else
  {
    wfxex.dwChannelMask          = SpeakerMaskFromAEChannels(format.m_channelLayout);
    wfxex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    wfxex.Format.wBitsPerSample  = 32;
  }

  wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
  wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  unsigned int uiFrameCount = (int)(format.m_sampleRate * 0.015); //default to 15ms chunks
  m_dwFrameSize = wfxex.Format.nBlockAlign;
  m_dwChunkSize = m_dwFrameSize * uiFrameCount;
  m_dwBufferLen = m_dwChunkSize * 12; //180ms total buffer

  // fill in the secondary sound buffer descriptor
  DSBUFFERDESC dsbdesc;
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                  | DSBCAPS_TRUEPLAYPOSITION    /** Vista+ accurate position */
                  | DSBCAPS_GLOBALFOCUS;         /** Allows background playing */

  dsbdesc.dwBufferBytes = m_dwBufferLen;
  dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wfxex;

  // now create the stream buffer
  HRESULT res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
  if (res != DS_OK)
  {
    if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE)
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGDEBUG, __FUNCTION__": Couldn't create secondary buffer (%s). Trying without LOCHARDWARE.", dserr2str(res));
      // Try without DSBCAPS_LOCHARDWARE
      dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
      res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
    }
    if (res != DS_OK)
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGERROR, __FUNCTION__": cannot create secondary buffer (%s)", dserr2str(res));
      return false;
    }
  }
  CLog::Log(LOGDEBUG, __FUNCTION__": secondary buffer created");

  m_pBuffer->Stop();

  AEChannelsFromSpeakerMask(wfxex.dwChannelMask);
  format.m_channelLayout = m_channelLayout;
  m_encodedFormat = format.m_dataFormat;
  format.m_frames = uiFrameCount;
  format.m_frameSamples = format.m_frames * format.m_channelLayout.Count();
  format.m_frameSize = (AE_IS_RAW(format.m_dataFormat) ? wfxex.Format.wBitsPerSample >> 3 : sizeof(float)) * format.m_channelLayout.Count();
  format.m_dataFormat = AE_IS_RAW(format.m_dataFormat) ? AE_FMT_S16NE : AE_FMT_FLOAT;

  m_format = format;
  m_device = device;

  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_LastCacheCheck = XbmcThreads::SystemClockMillis();
  m_initialized = true;
  m_isDirtyDS = false;

  CLog::Log(LOGDEBUG, __FUNCTION__": Initializing DirectSound with the following parameters:");
  CLog::Log(LOGDEBUG, "  Audio Device    : %s", ((std::string)deviceFriendlyName).c_str());
  CLog::Log(LOGDEBUG, "  Sample Rate     : %d", wfxex.Format.nSamplesPerSec);
  CLog::Log(LOGDEBUG, "  Sample Format   : %s", CAEUtil::DataFormatToStr(format.m_dataFormat));
  CLog::Log(LOGDEBUG, "  Bits Per Sample : %d", wfxex.Format.wBitsPerSample);
  CLog::Log(LOGDEBUG, "  Valid Bits/Samp : %d", wfxex.Samples.wValidBitsPerSample);
  CLog::Log(LOGDEBUG, "  Channel Count   : %d", wfxex.Format.nChannels);
  CLog::Log(LOGDEBUG, "  Block Align     : %d", wfxex.Format.nBlockAlign);
  CLog::Log(LOGDEBUG, "  Avg. Bytes Sec  : %d", wfxex.Format.nAvgBytesPerSec);
  CLog::Log(LOGDEBUG, "  Samples/Block   : %d", wfxex.Samples.wSamplesPerBlock);
  CLog::Log(LOGDEBUG, "  Format cBSize   : %d", wfxex.Format.cbSize);
  CLog::Log(LOGDEBUG, "  Channel Layout  : %s", ((std::string)format.m_channelLayout).c_str());
  CLog::Log(LOGDEBUG, "  Channel Mask    : %d", wfxex.dwChannelMask);
  CLog::Log(LOGDEBUG, "  Frames          : %d", format.m_frames);
  CLog::Log(LOGDEBUG, "  Frame Samples   : %d", format.m_frameSamples);
  CLog::Log(LOGDEBUG, "  Frame Size      : %d", format.m_frameSize);

  return true;
}

void CAESinkDirectSound::Deinitialize()
{
  if (!m_initialized)
    return;

  CLog::Log(LOGDEBUG, __FUNCTION__": Cleaning up");

  if (m_pBuffer)
  {
    m_pBuffer->Stop();
    SAFE_RELEASE(m_pBuffer);
  }

  if (m_pDSound)
  {
    m_pDSound->Release();
  }

  m_initialized = false;
  m_pBuffer = NULL;
  m_pDSound = NULL;
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;
}

unsigned int CAESinkDirectSound::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_initialized)
    return 0;

  DWORD total = m_dwFrameSize * frames;
  DWORD len = total;
  unsigned char* pBuffer = (unsigned char*)data[0]+offset*m_format.m_frameSize;

  DWORD bufferStatus = 0;
  if (m_pBuffer->GetStatus(&bufferStatus) != DS_OK)
  {
    CLog::Log(LOGERROR, "%s: GetStatus() failed", __FUNCTION__);
    return 0;
  }
  if (bufferStatus & DSBSTATUS_BUFFERLOST)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__ ": Buffer allocation was lost. Restoring buffer.");
    m_pBuffer->Restore();
  }

  while (GetSpace() < total)
  {
    if(m_isDirtyDS)
      return INT_MAX;
    else
    {
      Sleep(total * 1000 / m_AvgBytesPerSec);
    }
  }

  while (len)
  {
    LPVOID start = NULL, startWrap = NULL;
    DWORD size = 0, sizeWrap = 0;
    if (m_BufferOffset >= m_dwBufferLen) // Wrap-around manually
      m_BufferOffset = 0;
    DWORD dwWriteBytes = std::min((int)m_dwChunkSize, (int)len);
    HRESULT res = m_pBuffer->Lock(m_BufferOffset, dwWriteBytes, &start, &size, &startWrap, &sizeWrap, 0);
    if (DS_OK != res)
    {
      CLog::Log(LOGERROR, __FUNCTION__ ": Unable to lock buffer at offset %u. HRESULT: 0x%08x", m_BufferOffset, res);
      m_isDirtyDS = true;
      return INT_MAX;
    }

    memcpy(start, pBuffer, size);

    pBuffer += size;
    len     -= size;

    m_BufferOffset += size;
    if (startWrap) // Write-region wraps to beginning of buffer
    {
      memcpy(startWrap, pBuffer, sizeWrap);
      m_BufferOffset = sizeWrap;

      pBuffer += sizeWrap;
      len     -= sizeWrap;
    }

    m_CacheLen += size + sizeWrap; // This data is now in the cache
    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
  }

  CheckPlayStatus();

  return (total - len) / m_dwFrameSize; // Frames used
}

void CAESinkDirectSound::Stop()
{
  if (m_pBuffer)
    m_pBuffer->Stop();
}

void CAESinkDirectSound::Drain()
{
  if (!m_initialized || m_isDirtyDS)
    return;

  m_pBuffer->Stop();
  HRESULT res = m_pBuffer->SetCurrentPosition(0);
  if (DS_OK != res)
  {
    CLog::Log(LOGERROR,__FUNCTION__ ": SetCurrentPosition failed. Unable to determine buffer status. HRESULT = 0x%08x", res);
    m_isDirtyDS = true;
    return;
  }
  m_BufferOffset = 0;
  UpdateCacheStatus();
}

void CAESinkDirectSound::GetDelay(AEDelayStatus& status)
{
  if (!m_initialized)
  {
    status.SetDelay(0);
    return;
  }

  /* Make sure we know how much data is in the cache */
  if (!UpdateCacheStatus())
    m_isDirtyDS = true;

  /** returns current cached data duration in seconds */
  status.SetDelay((double)m_CacheLen / (double)m_AvgBytesPerSec);
}

double CAESinkDirectSound::GetCacheTotal()
{
  /** returns total cache capacity in seconds */
  return (double)m_dwBufferLen / (double)m_AvgBytesPerSec;
}

void CAESinkDirectSound::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  CAEDeviceInfo        deviceInfo;

  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

  HRESULT                hr;

  std::string strDD = GetDefaultDevice();

  /* Windows Vista or later - supporting WASAPI device probing */
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %li", hr)

  UINT uiCount = 0;

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    deviceInfo.m_channels.Reset();
    deviceInfo.m_dataFormats.clear();
    deviceInfo.m_sampleRates.clear();

    hr = pEnumDevices->Item(i, &pDevice);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint failed.");
      goto failed;
    }

    hr = pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint properties failed.");
      SAFE_RELEASE(pDevice);
      goto failed;
    }

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint device name failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::string strFriendlyName = localWideToUtf(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint GUID failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::string strDevName = localWideToUtf(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint form factor failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }
    std::string strWinDevType = winEndpoints[(EndpointFormFactor)varName.uiVal].winEndpointType;
    AEDeviceType aeDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;

    PropVariantClear(&varName);

    /* In shared mode Windows tells us what format the audio must be in. */
    IAudioClient *pClient;
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pClient);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Activate device failed (%s)", WASAPIErrToStr(hr));
      goto failed;
    }

    //hr = pClient->GetMixFormat(&pwfxex);
    hr = pProperty->GetValue(PKEY_AudioEngine_DeviceFormat, &varName);
    if (SUCCEEDED(hr) && varName.blob.cbSize > 0)
    {
      WAVEFORMATEX* smpwfxex = (WAVEFORMATEX*)varName.blob.pBlobData;
      deviceInfo.m_channels = layoutsByChCount[std::max(std::min(smpwfxex->nChannels, (WORD) DS_SPEAKER_COUNT), (WORD) 2)];
      deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_FLOAT));
      if (aeDeviceType != AE_DEVTYPE_PCM)
      {
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_AC3));
        // DTS is played with the same infrastructure as AC3
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_DTS));
      }
      deviceInfo.m_sampleRates.push_back(std::min(smpwfxex->nSamplesPerSec, (DWORD) 192000));
    }
    else
    {
      CLog::Log(LOGERROR, __FUNCTION__": Getting DeviceFormat failed (%s)", WASAPIErrToStr(hr));
    }
    pClient->Release();

    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pProperty);

    deviceInfo.m_deviceName       = strDevName;
    deviceInfo.m_displayName      = strWinDevType.append(strFriendlyName);
    deviceInfo.m_displayNameExtra = std::string("DIRECTSOUND: ").append(strFriendlyName);
    deviceInfo.m_deviceType       = aeDeviceType;

    deviceInfoList.push_back(deviceInfo);

    // add the default device with m_deviceName = default
    if(strDD == strDevName)
    {
      deviceInfo.m_deviceName = std::string("default");
      deviceInfo.m_displayName = std::string("default");
      deviceInfo.m_displayNameExtra = std::string("");
      deviceInfoList.push_back(deviceInfo);
    }
  }

  return;

failed:

  if (FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__": Failed to enumerate WASAPI endpoint devices (%s).", WASAPIErrToStr(hr));

  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);

}

///////////////////////////////////////////////////////////////////////////////

void CAESinkDirectSound::CheckPlayStatus()
{
  DWORD status = 0;
  if (m_pBuffer->GetStatus(&status) != DS_OK)
  {
    CLog::Log(LOGERROR, "%s: GetStatus() failed", __FUNCTION__);
    return;
  }

  if (!(status & DSBSTATUS_PLAYING) && m_CacheLen != 0) // If we have some data, see if we can start playback
  {
    HRESULT hr = m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    CLog::Log(LOGDEBUG,__FUNCTION__ ": Resuming Playback");
    if (FAILED(hr))
      CLog::Log(LOGERROR, __FUNCTION__": Failed to play the DirectSound buffer: %s", dserr2str(hr));
  }
}

bool CAESinkDirectSound::UpdateCacheStatus()
{
  CSingleLock lock (m_runLock);

  DWORD playCursor = 0, writeCursor = 0;
  HRESULT res = m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor); // Get the current playback and safe write positions
  if (DS_OK != res)
  {
    CLog::Log(LOGERROR,__FUNCTION__ ": GetCurrentPosition failed. Unable to determine buffer status. HRESULT = 0x%08x", res);
    m_isDirtyDS = true;
    return false;
  }

  // Check the state of the ring buffer (P->O->W == underrun)
  // These are the logical situations that can occur
  // O: CurrentOffset  W: WriteCursor  P: PlayCursor
  // | | | | | | | | | |
  // ***O----W----P***** < underrun   P > W && O < W (1)
  // | | | | | | | | | |
  // ---P****O----W----- < underrun   O > P && O < W (2)
  // | | | | | | | | | |
  // ---W----P****O----- < underrun   P > W && P < O (3)
  // | | | | | | | | | |
  // ***W****O----P*****              P > W && P > O (4)
  // | | | | | | | | | |
  // ---P****W****O-----              P < W && O > W (5)
  // | | | | | | | | | |
  // ***O----P****W*****              P < W && O < P (6)

  // Check for underruns
  if ((playCursor > writeCursor && m_BufferOffset < writeCursor) ||    // (1)
      (playCursor < m_BufferOffset && m_BufferOffset < writeCursor) || // (2)
      (playCursor > writeCursor && playCursor <  m_BufferOffset))      // (3)
  {
    CLog::Log(LOGWARNING, "CWin32DirectSound::GetSpace - buffer underrun - W:%u, P:%u, O:%u.", writeCursor, playCursor, m_BufferOffset);
    m_BufferOffset = writeCursor; // Catch up
    //m_pBuffer->Stop(); // Wait until someone gives us some data to restart playback (prevents glitches)
    m_BufferTimeouts++;
    if (m_BufferTimeouts > 10)
    {
      m_isDirtyDS = true;
      return false;
    }
  }
  else 
    m_BufferTimeouts = 0;

  // Calculate available space in the ring buffer
  if (playCursor == m_BufferOffset && m_BufferOffset ==  writeCursor) // Playback is stopped and we are all at the same place
    m_CacheLen = 0;
  else if (m_BufferOffset > playCursor)
    m_CacheLen = m_BufferOffset - playCursor;
  else
    m_CacheLen = m_dwBufferLen - (playCursor - m_BufferOffset);

  return true;
}

unsigned int CAESinkDirectSound::GetSpace()
{
  CSingleLock lock (m_runLock);
  if (!UpdateCacheStatus())
    m_isDirtyDS = true;
  unsigned int space = m_dwBufferLen - m_CacheLen;

  // We can never allow the internal buffers to fill up complete
  // as we get confused between if the buffer is full or empty
  // so never allow the last chunk to be added
  if (space > m_dwChunkSize)
    return space - m_dwChunkSize;
  else
    return 0;
}

void CAESinkDirectSound::AEChannelsFromSpeakerMask(DWORD speakers)
{
  m_channelLayout.Reset();

  for (int i = 0; i < DS_SPEAKER_COUNT; i++)
  {
    if (speakers & DSChannelOrder[i])
      m_channelLayout += AEChannelNames[i];
  }
}

DWORD CAESinkDirectSound::SpeakerMaskFromAEChannels(const CAEChannelInfo &channels)
{
  DWORD mask = 0;

  for (unsigned int i = 0; i < channels.Count(); i++)
  {
    for (unsigned int j = 0; j < DS_SPEAKER_COUNT; j++)
      if (channels[i] == AEChannelNames[j])
        mask |= DSChannelOrder[j];
  }

  return mask;
}

const char *CAESinkDirectSound::dserr2str(int err)
{
  switch (err)
  {
    case DS_OK: return "DS_OK";
    case DS_NO_VIRTUALIZATION: return "DS_NO_VIRTUALIZATION";
    case DSERR_ALLOCATED: return "DS_NO_VIRTUALIZATION";
    case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
    case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
    case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
    case DSERR_GENERIC: return "DSERR_GENERIC";
    case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
    case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
    case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
    case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
    case DSERR_NODRIVER: return "DSERR_NODRIVER";
    case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
    case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
    case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
    case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
    case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
    case DSERR_NOINTERFACE: return "DSERR_NOINTERFACE";
    case DSERR_ACCESSDENIED: return "DSERR_ACCESSDENIED";
    case DSERR_BUFFERTOOSMALL: return "DSERR_BUFFERTOOSMALL";
    case DSERR_DS8_REQUIRED: return "DSERR_DS8_REQUIRED";
    case DSERR_SENDLOOP: return "DSERR_SENDLOOP";
    case DSERR_BADSENDBUFFERGUID: return "DSERR_BADSENDBUFFERGUID";
    case DSERR_OBJECTNOTFOUND: return "DSERR_OBJECTNOTFOUND";
    case DSERR_FXUNAVAILABLE: return "DSERR_FXUNAVAILABLE";
    default: return "unknown";
  }
}

const char *CAESinkDirectSound::WASAPIErrToStr(HRESULT err)
{
  switch(err)
  {
    ERRTOSTR(AUDCLNT_E_NOT_INITIALIZED);
    ERRTOSTR(AUDCLNT_E_ALREADY_INITIALIZED);
    ERRTOSTR(AUDCLNT_E_WRONG_ENDPOINT_TYPE);
    ERRTOSTR(AUDCLNT_E_DEVICE_INVALIDATED);
    ERRTOSTR(AUDCLNT_E_NOT_STOPPED);
    ERRTOSTR(AUDCLNT_E_BUFFER_TOO_LARGE);
    ERRTOSTR(AUDCLNT_E_OUT_OF_ORDER);
    ERRTOSTR(AUDCLNT_E_UNSUPPORTED_FORMAT);
    ERRTOSTR(AUDCLNT_E_INVALID_SIZE);
    ERRTOSTR(AUDCLNT_E_DEVICE_IN_USE);
    ERRTOSTR(AUDCLNT_E_BUFFER_OPERATION_PENDING);
    ERRTOSTR(AUDCLNT_E_THREAD_NOT_REGISTERED);
    ERRTOSTR(AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED);
    ERRTOSTR(AUDCLNT_E_ENDPOINT_CREATE_FAILED);
    ERRTOSTR(AUDCLNT_E_SERVICE_NOT_RUNNING);
    ERRTOSTR(AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED);
    ERRTOSTR(AUDCLNT_E_EXCLUSIVE_MODE_ONLY);
    ERRTOSTR(AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL);
    ERRTOSTR(AUDCLNT_E_EVENTHANDLE_NOT_SET);
    ERRTOSTR(AUDCLNT_E_INCORRECT_BUFFER_SIZE);
    ERRTOSTR(AUDCLNT_E_BUFFER_SIZE_ERROR);
    ERRTOSTR(AUDCLNT_E_CPUUSAGE_EXCEEDED);
    ERRTOSTR(AUDCLNT_E_BUFFER_ERROR);
    ERRTOSTR(AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED);
    ERRTOSTR(AUDCLNT_E_INVALID_DEVICE_PERIOD);
    ERRTOSTR(E_POINTER);
    ERRTOSTR(E_INVALIDARG);
    ERRTOSTR(E_OUTOFMEMORY);
    default: break;
  }
  return NULL;
}

std::string CAESinkDirectSound::GetDefaultDevice()
{
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDevice*           pDevice = NULL;
  IPropertyStore*      pProperty = NULL;
  HRESULT              hr;
  PROPVARIANT          varName;
  std::string          strDevName = "default";

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %s", WASAPIErrToStr(hr));
    goto failed;
  }

  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Retrieval of audio endpoint enumeration failed.");
    goto failed;
  }

  hr = pDevice->OpenPropertyStore(STGM_READ, &pProperty);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint properties failed.");
    goto failed;
  }

  PropVariantInit(&varName);
  hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint form factor failed.");
    goto failed;
  }
  AEDeviceType aeDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;
  PropVariantClear(&varName);

  hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Retrieval of DirectSound endpoint GUID failed.");    
    goto failed;
  }

  strDevName = localWideToUtf(varName.pwszVal);
  PropVariantClear(&varName);

failed:

  SAFE_RELEASE(pProperty);
  SAFE_RELEASE(pDevice);
  SAFE_RELEASE(pEnumerator);

  return strDevName;
}
