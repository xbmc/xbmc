/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AESinkWASAPI.h"
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <initguid.h>
#include <Mmreg.h>
#include <stdint.h>

#include "GUISettings.h"
#include "StdString.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "CharsetConverter.h"

#pragma comment(lib, "Avrt.lib")

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

static const unsigned int WASAPISampleRateCount = 9;
static const unsigned int WASAPISampleRates[] = {192000, 176400, 96000, 88200, 48000, 44100, 32000, 22050, 11025};

#define SPEAKER_COUNT 8
static const unsigned int WASAPIChannelOrder[] = {SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_CENTER, SPEAKER_LOW_FREQUENCY, SPEAKER_BACK_LEFT, SPEAKER_BACK_RIGHT, SPEAKER_SIDE_LEFT, SPEAKER_SIDE_RIGHT};
static const enum AEChannel AEChannelNames[] = {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL};

#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason, __VA_ARGS__); goto failed;}


CAESinkWASAPI::CAESinkWASAPI() :
  m_pAudioClient(NULL),
  m_pRenderClient(NULL),
  m_pDevice(NULL),
  m_initialized(false),
  m_running(false)
{
  m_channelLayout[0] = AE_CH_NULL;
}

CAESinkWASAPI::~CAESinkWASAPI()
{
  Deinitialize();
}

bool CAESinkWASAPI::Initialize(AEAudioFormat &format, CStdString &device)
{
  CSingleLock lock(m_runLock);

  if(m_initialized) return false;

  m_device = device;

  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

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

  if(g_guiSettings.GetBool("audiooutput.useexclusivemode") || format.m_dataFormat == AE_FMT_RAW)
  {
    if(!InitializeExclusive(format))
      goto failed;
  }
  else
  {
    if(!InitializeShared(format))
      goto failed;
  }

  UINT32 m_uiBufferLen;
  hr = m_pAudioClient->GetBufferSize(&m_uiBufferLen);
  format.m_frames = m_uiBufferLen/8;
  format.m_frameSamples = format.m_frames * format.m_channelCount;
  m_format = format;

  hr = m_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&m_pRenderClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not initialize the WASAPI render client interface.")

  m_initialized = true;
  
  return true;

failed:
  CLog::Log(LOGERROR, __FUNCTION__": WASAPI initialization failed.");
  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);
  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pDevice);

  return false;
}

void CAESinkWASAPI::Deinitialize()
{
  if(!m_initialized) return;

  Stop();

  SAFE_RELEASE(m_pRenderClient)
  SAFE_RELEASE(m_pAudioClient)
  SAFE_RELEASE(m_pDevice)
}

bool CAESinkWASAPI::IsCompatible(const AEAudioFormat format, const CStdString device)
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return false;

  if(m_device == device &&
     m_format.m_sampleRate   == format.m_sampleRate  &&
     ((format.m_dataFormat   == AE_FMT_RAW           && 
       m_format.m_dataFormat == AE_FMT_RAW)          ||
      (format.m_dataFormat   == AE_FMT_FLOAT         &&
       m_format.m_dataFormat != AE_FMT_RAW))         &&
     m_format.m_channelCount == format.m_channelCount)
     return true;

  return false;
}

void CAESinkWASAPI::Stop()
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return;

  if(m_running)
    m_pAudioClient->Stop();

  m_running = false;
}

float CAESinkWASAPI::GetDelay()
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return 0.0f;

  UINT32 frames;
  m_pAudioClient->GetCurrentPadding(&frames);
  return (float)frames / (float)m_format.m_sampleRate;
}

unsigned int CAESinkWASAPI::AddPackets(uint8_t *data, unsigned int frames)
{
  CSingleLock lock(m_runLock);
  if(!m_initialized) return 0;

  UINT32 total, waitFor, count, copied;
  count = frames;
  copied = 0;

  m_pAudioClient->GetBufferSize(&total);
  m_pAudioClient->GetCurrentPadding(&waitFor);

  while(total - waitFor < frames)
  {
    Sleep(1);
    m_pAudioClient->GetBufferSize(&total);
    m_pAudioClient->GetCurrentPadding(&waitFor);
  }

  BYTE *buf;
  HRESULT hr = m_pRenderClient->GetBuffer(frames, &buf);

  memcpy(buf, data, frames*8);

  m_pRenderClient->ReleaseBuffer(frames, 0);

  if(!m_running)
  {
    m_pAudioClient->Start();
    m_running = true;
  }

  return frames;
}

void CAESinkWASAPI::EnumerateDevices(AEDeviceList &devices, bool passthrough)
{
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

  WAVEFORMATEXTENSIBLE wfxex = {0};

  wfxex.Format.cbSize          = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nSamplesPerSec  = 48000;
  wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
  wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
  wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
  wfxex.Format.wBitsPerSample  = 16;
  wfxex.Samples.wValidBitsPerSample = 16;
  wfxex.Format.nChannels       = 2;
  wfxex.Format.nBlockAlign     = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

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

    CLog::Log(LOGDEBUG, __FUNCTION__": found endpoint device: %s", strDevName.c_str());

    if(passthrough)
    {
      IAudioClient *pClient;
      hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pClient);
      if(SUCCEEDED(hr))
      {
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

        if(SUCCEEDED(hr))
          devices.push_back(AEDevice(strDevName, CStdString("WASAPI:").append(strDevName)));

        pClient->Release();
      }
      else
      {
        CLog::Log(LOGDEBUG, __FUNCTION__": Failed to activate device for passthrough capability testing.");
      }
      
    }
    else
      devices.push_back(AEDevice(strDevName, CStdString("WASAPI:").append(strDevName)));

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

//Private utility functions////////////////////////////////////////////////////

bool CAESinkWASAPI::InitializeShared(AEAudioFormat &format)
{
  WAVEFORMATEXTENSIBLE *wfxex;

  //In shared mode Windows tells us what format the audio must be in.
  HRESULT hr = m_pAudioClient->GetMixFormat((WAVEFORMATEX **)&wfxex);
  if(FAILED(hr))
    return false;

  AEChannelsFromSpeakerMask(wfxex->dwChannelMask);

  format.m_channelCount = wfxex->Format.nChannels;
  format.m_channelLayout = m_channelLayout;
  format.m_dataFormat = AE_FMT_FLOAT; //The windows mixer always uses floats.
  format.m_frameSize = sizeof(float) * format.m_channelCount;
  format.m_sampleRate = wfxex->Format.nSamplesPerSec;

  REFERENCE_TIME hnsRequestedDuration, hnsPeriodicity;
  hr = m_pAudioClient->GetDevicePeriod(NULL, &hnsPeriodicity);
  
  //The default periods of some devices are VERY low (less than 3ms).
  //For audio stability make sure we have at least an 8ms buffer.
  if(hnsPeriodicity < 80000) hnsPeriodicity = 80000;

  hnsRequestedDuration = hnsPeriodicity*8;

  hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsRequestedDuration, 0, &wfxex->Format, NULL);

  return true;
}

bool CAESinkWASAPI::InitializeExclusive(AEAudioFormat &format)
{
  WAVEFORMATEXTENSIBLE wfxex = {0};

  wfxex.Format.cbSize          =  sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = format.m_channelCount;
  wfxex.Format.nSamplesPerSec  = format.m_sampleRate;
  if (format.m_dataFormat == AE_FMT_RAW) 
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
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
  wfxex.Format.nBlockAlign       = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

  HRESULT hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

  if(SUCCEEDED(hr))
    goto initialize;
  else if(format.m_dataFormat == AE_FMT_RAW) //No sense in trying other formats for passthrough.
    return false;

  int closestMatch;

  //The requested format is not supported by the device.  Find something that works.
  //Try float -> 24 bit int -> 16 bit int
  for(int j = 0; j < 3; j++)
  {
    closestMatch = -1;

    for(int i = 0 ; i < WASAPISampleRateCount; i++)
    {
      wfxex.Format.nSamplesPerSec    = WASAPISampleRates[i];
      wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

      if(SUCCEEDED(hr))
      {
        //If the current sample rate matches the source then stop looking and use it.
        if(WASAPISampleRates[i] == format.m_sampleRate)
          goto initialize;
        //If this rate is closer to the source then the previous one, save it.
        else if(closestMatch < 0 || abs((int)WASAPISampleRates[i] - (int)format.m_sampleRate) < abs((int)WASAPISampleRates[closestMatch] - (int)format.m_sampleRate))
          closestMatch = i;
      }
    }

    if(closestMatch >= 0)
    {
      wfxex.Format.nSamplesPerSec    = WASAPISampleRates[closestMatch];
      wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      goto initialize;
    }

    //The device doesn't like floats so try ints.
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    wfxex.Format.wBitsPerSample       = wfxex.Samples.wValidBitsPerSample == 32 ? 32 : 16;
    wfxex.Samples.wValidBitsPerSample -= 8;
    wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  }

  CLog::Log(LOGERROR, __FUNCTION__": Unable to locate a supported output format for the device.  Check the speaker settings in the control panel."); 

  //We couldn't find anything supported.  This should never happen unless the user set the wrong
  //speaker setting in the control panel.
  return false;

initialize:

  AEChannelsFromSpeakerMask(wfxex.dwChannelMask);

  format.m_dataFormat = wfxex.Samples.wValidBitsPerSample == 24 ? AE_FMT_S24NE4 : AE_FMT_S16NE;
  format.m_sampleRate = wfxex.Format.nSamplesPerSec;
  format.m_channelLayout = m_channelLayout;
  format.m_frameSize = (wfxex.Format.wBitsPerSample >> 3) * wfxex.Format.nChannels;

  REFERENCE_TIME hnsRequestedDuration, hnsPeriodicity;
  hr = m_pAudioClient->GetDevicePeriod(NULL, &hnsPeriodicity);
  
  //The default periods of some devices are VERY low (less than 3ms).
  //For audio stability make sure we have at least an 8ms buffer.
  if(hnsPeriodicity < 80000) hnsPeriodicity = 80000;

  hnsRequestedDuration = hnsPeriodicity * 8;

  hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, 0, hnsRequestedDuration, hnsPeriodicity, &wfxex.Format, NULL);

  if(FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__": Unable to initialize WASAPI in exclusive mode.");
  return true;
}

void CAESinkWASAPI::AEChannelsFromSpeakerMask(DWORD speakers)
{
  int j = 0;
  for(int i = 0; i < SPEAKER_COUNT; i++)
  {
    if(speakers & WASAPIChannelOrder[i])
      m_channelLayout[j++] = AEChannelNames[i];
  }

  m_channelLayout[j] = AE_CH_NULL;
}

DWORD CAESinkWASAPI::SpeakerMaskFromAEChannels(AEChLayout channels)
{
  DWORD mask = 0;

  for(int i = 0; channels[i] != AE_CH_NULL; i++)
  {
    for(int j = 0; j < SPEAKER_COUNT; j++)
      if(channels[i] == AEChannelNames[j])
        mask |= WASAPIChannelOrder[j];
  }

  return mask;
}
