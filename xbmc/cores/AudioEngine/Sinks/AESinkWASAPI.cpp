/*
 *      Copyright (C) 2010-2012 Team XBMC
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
#include <Audioclient.h>
#include <avrt.h>
#include <initguid.h>
#include <Mmreg.h>
#include <stdint.h>

#include "../Utils/AEUtil.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/StdString.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "../Utils/AEDeviceInfo.h"
#include <Mmreg.h>
#include <mmdeviceapi.h>

#pragma comment(lib, "Avrt.lib")

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

static const unsigned int WASAPISampleRateCount = 10;
static const unsigned int WASAPISampleRates[] = {384000, 192000, 176400, 96000, 88200, 48000, 44100, 32000, 22050, 11025};

#define WASAPI_SPEAKER_COUNT 21
static const unsigned int WASAPIChannelOrder[] = {AE_CH_RAW,
                                                  SPEAKER_FRONT_LEFT,           SPEAKER_FRONT_RIGHT,           SPEAKER_FRONT_CENTER,
                                                  SPEAKER_LOW_FREQUENCY,        SPEAKER_BACK_LEFT,             SPEAKER_BACK_RIGHT,
                                                  SPEAKER_FRONT_LEFT_OF_CENTER, SPEAKER_FRONT_RIGHT_OF_CENTER,
                                                  SPEAKER_BACK_CENTER,          SPEAKER_SIDE_LEFT,             SPEAKER_SIDE_RIGHT,
                                                  SPEAKER_TOP_FRONT_LEFT,       SPEAKER_TOP_FRONT_RIGHT,       SPEAKER_TOP_FRONT_CENTER,
                                                  SPEAKER_TOP_CENTER,           SPEAKER_TOP_BACK_LEFT,         SPEAKER_TOP_BACK_RIGHT,
                                                  SPEAKER_TOP_BACK_CENTER,      SPEAKER_RESERVED,              SPEAKER_RESERVED};

static const enum AEChannel AEChannelNames[]   = {AE_CH_RAW,
                                                  AE_CH_FL,                     AE_CH_FR,                      AE_CH_FC,
                                                  AE_CH_LFE,                    AE_CH_BL,                      AE_CH_BR,
                                                  AE_CH_FLOC,                   AE_CH_FROC,
                                                  AE_CH_BC,                     AE_CH_SL,                      AE_CH_SR,
                                                  AE_CH_TFL,                    AE_CH_TFR,                     AE_CH_TFC ,
                                                  AE_CH_TC  ,                   AE_CH_TBL,                     AE_CH_TBR,
                                                  AE_CH_TBC,                    AE_CH_BLOC,                    AE_CH_BROC};

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

struct sampleFormat
{
  GUID subFormat;
  unsigned int bitsPerSample;
  unsigned int validBitsPerSample;
  AEDataFormat subFormatType;
};

//Sample formats go from float -> 32 bit int -> 24 bit int (packed in 32) -> 16 bit int
static const sampleFormat testFormats[] = { {KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 32, 32, AE_FMT_FLOAT},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 32, 32, AE_FMT_S32NE},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 32, 24, AE_FMT_S24NE4},
                                            {KSDATAFORMAT_SUBTYPE_PCM, 16, 16, AE_FMT_S16NE} };

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

AEDeviceInfoList DeviceInfoList;

#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason " - %s", __VA_ARGS__, WASAPIErrToStr(hr)); goto failed;}

#define ERRTOSTR(err) case err: return #err

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);

CAESinkWASAPI::CAESinkWASAPI() :
  m_pAudioClient(NULL),
  m_pRenderClient(NULL),
  m_needDataEvent(0),
  m_pDevice(NULL),
  m_initialized(false),
  m_running(false),
  m_encodedFormat(AE_FMT_INVALID),
  m_encodedChannels(0),
  m_encodedSampleRate(0),
  m_uiBufferLen(0),
  m_avgTimeWaiting(50)
{
  m_channelLayout.Reset();
}

CAESinkWASAPI::~CAESinkWASAPI()
{

}

bool CAESinkWASAPI::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  CLog::Log(LOGDEBUG, __FUNCTION__": Initializing WASAPI Sink Rev. 1.0.5");

  m_device = device;

  /* Save requested format */
  /* Clear returned format */
  sinkReqFormat = format.m_dataFormat;
  sinkRetFormat = AE_FMT_INVALID;

  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %li", hr)

  //Get our device.
  //First try to find the named device.
  UINT uiCount = 0;

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;

    hr = pEnumDevices->Item(i, &m_pDevice);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint failed.")

    hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint GUID failed.");
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::wstring strRawDevName(varName.pwszVal);
    std::string strDevName = std::string(strRawDevName.begin(), strRawDevName.end());
    //g_charsetConverter.ucs2CharsetToStringCharset(strRawDevName, strDevName.c_str());

    if (device == strDevName)
      i = uiCount;
    else
      SAFE_RELEASE(m_pDevice);

    PropVariantClear(&varName);
    SAFE_RELEASE(pProperty);
  }

  SAFE_RELEASE(pEnumDevices);

  if (!m_pDevice)
  {
    CLog::Log(LOGINFO, __FUNCTION__": Could not locate the device named \"%s\" in the list of WASAPI endpoint devices.  Trying the default device...", device.c_str());
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Could not retrieve the default WASAPI audio endpoint.")

    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;

    hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);

    std::wstring strRawDevName(varName.pwszVal);
    std::string strDevName = std::string(strRawDevName.begin(), strRawDevName.end());

    PropVariantClear(&varName);
    SAFE_RELEASE(pProperty);
  }

  //We are done with the enumerator.
  SAFE_RELEASE(pEnumerator);

  hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Activating the WASAPI endpoint device failed.")

  if (!InitializeExclusive(format))
  {
    CLog::Log(LOGINFO, __FUNCTION__": Could not Initialize Exclusive with that format");
    goto failed;
  }

  /* get the buffer size and calculate the frames for AE */
  m_pAudioClient->GetBufferSize(&m_uiBufferLen);

  format.m_frames       = m_uiBufferLen;
  format.m_frameSamples = format.m_frames * format.m_channelLayout.Count();
  m_format              = format;
  sinkRetFormat         = format.m_dataFormat;

  CLog::Log(LOGDEBUG, __FUNCTION__": Buffer Size     = %d Bytes", m_uiBufferLen * format.m_frameSize);

  hr = m_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&m_pRenderClient);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not initialize the WASAPI render client interface.")

  m_needDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  hr = m_pAudioClient->SetEventHandle(m_needDataEvent);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not set the WASAPI event handler.");

  m_initialized = true;

  return true;

failed:
  CLog::Log(LOGERROR, __FUNCTION__": WASAPI initialization failed.");
  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);
  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pDevice);
  CloseHandle(m_needDataEvent);

  return false;
}

void CAESinkWASAPI::Deinitialize()
{
  if (!m_initialized)
    return;


  if (m_running)
    m_pAudioClient->Stop();
  m_running = false;


  SAFE_RELEASE(m_pRenderClient);
  SAFE_RELEASE(m_pAudioClient);
  SAFE_RELEASE(m_pDevice);
  CloseHandle(m_needDataEvent);

  m_initialized = false;
}

bool CAESinkWASAPI::IsCompatible(const AEAudioFormat format, const std::string device)
{
  if (!m_initialized)
    return false;

  u_int notCompatible         = 0;
  const u_int numTests        = 5;
  std::string strDiffBecause ("");
  static const char* compatibleParams[numTests] = {":Devices",
                                                   ":Channels",
                                                   ":Sample Rates",
                                                   ":Data Formats",
                                                   ":Passthrough Formats"};

  notCompatible = (notCompatible  +!((AE_IS_RAW(format.m_dataFormat)  == AE_IS_RAW(m_encodedFormat))        ||
                                     (!AE_IS_RAW(format.m_dataFormat) == !AE_IS_RAW(m_encodedFormat))))     << 1;
  notCompatible = (notCompatible  +!((sinkReqFormat                   == format.m_dataFormat)               &&
                                     (sinkRetFormat                   == m_format.m_dataFormat)))           << 1;
  notCompatible = (notCompatible  + !(format.m_sampleRate             == m_format.m_sampleRate))            << 1;
  notCompatible = (notCompatible  + !(format.m_channelLayout.Count()  == m_format.m_channelLayout.Count())) << 1;
  notCompatible = (notCompatible  + !(m_device                        == device));

  if (!notCompatible)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Formats compatible - reusing existing sink");
    return true;
  }

  for (int i = 0; i < numTests ; i++)
  {
    strDiffBecause += (notCompatible & 0x01) ? (std::string) compatibleParams[i] : "";
    notCompatible    = notCompatible >> 1;
  }

  CLog::Log(LOGDEBUG, __FUNCTION__": Formats Incompatible due to different %s", strDiffBecause.c_str());
  return false;
}

double CAESinkWASAPI::GetDelay()
{
  HRESULT hr;
  if (!m_initialized)
    return 0.0;

  hr = m_pAudioClient->GetBufferSize(&m_uiBufferLen);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": GetBufferSize Failed : %s", WASAPIErrToStr(hr));
    return false;
  }
  return (double)m_uiBufferLen / (double)m_format.m_sampleRate;
}

double CAESinkWASAPI::GetCacheTime()
{
  if (!m_initialized)
    return 0.0;

  REFERENCE_TIME hnsLatency;
  HRESULT hr = m_pAudioClient->GetStreamLatency(&hnsLatency);

  /** returns buffer duration in seconds */
  return hnsLatency / 10.0;
}

double CAESinkWASAPI::GetCacheTotal()
{
  if (!m_initialized)
    return 0.0;

  REFERENCE_TIME hnsLatency;
  HRESULT hr = m_pAudioClient->GetStreamLatency(&hnsLatency);

  /** returns buffer duration in seconds */
  return hnsLatency / 10.0;
}

unsigned int CAESinkWASAPI::AddPackets(uint8_t *data, unsigned int frames)
{
  if (!m_initialized)
    return 0;

  HRESULT hr;
  BYTE *buf;
  DWORD flags = 0;

#ifndef _DEBUG
  LARGE_INTEGER timerStart;
  LARGE_INTEGER timerStop;
  LARGE_INTEGER timerFreq;
#endif

  unsigned int NumFramesRequested = frames;

  if (!m_running) //first time called, pre-fill buffer then start audio client
  {
    hr = m_pRenderClient->GetBuffer(NumFramesRequested, &buf);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": GetBuffer failed due to %s", WASAPIErrToStr(hr));
      return 0;
    }
    memcpy(buf, data, NumFramesRequested * m_format.m_frameSize); //fill buffer
    hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
    if (FAILED(hr))
    {
      CLog::Log(LOGDEBUG, __FUNCTION__": ReleaseBuffer failed due to %s.", WASAPIErrToStr(hr));
      return 0;
    }
    hr = m_pAudioClient->Start(); //start the audio driver running
    if FAILED(hr)
      CLog::Log(LOGERROR, __FUNCTION__": AudioClient Start Failed");
    m_running = true; //signal that we're processing frames
    return NumFramesRequested;
  }

#ifndef _DEBUG
  /* Get clock time for latency checks */
  QueryPerformanceFrequency(&timerFreq);
  QueryPerformanceCounter(&timerStart);
#endif

  /* Wait for Audio Driver to tell us it's got a buffer available */
  DWORD eventAudioCallback = WaitForSingleObject(m_needDataEvent, 1100);
  if (eventAudioCallback != WAIT_OBJECT_0)
  {
    // Event handle timed out - stop audio device
    m_pAudioClient->Stop();
    CLog::Log(LOGERROR, __FUNCTION__": Endpoint Buffer timed out");
    m_running = false;
    m_pAudioClient->Stop(); //stop processing - we're done
    return 0; //need better handling here
  }

#ifndef _DEBUG
  QueryPerformanceCounter(&timerStop);
  LONGLONG timerDiff = timerStop.QuadPart - timerStart.QuadPart;
  double timerElapsed = (double) timerDiff * 1000.0 / (double) timerFreq.QuadPart;
  m_avgTimeWaiting += (timerElapsed - m_avgTimeWaiting) * 0.5;

  if (m_avgTimeWaiting < 3.0)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Possible AQ Loss: Avg. Time Waiting for Audio Driver callback : %dmsec", (int)m_avgTimeWaiting);
  }
#endif

  hr = m_pRenderClient->GetBuffer(NumFramesRequested, &buf);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": GetBuffer failed due to %s", WASAPIErrToStr(hr));
    return 0;
  }
  memcpy(buf, data, NumFramesRequested * m_format.m_frameSize); //fill buffer
  hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": ReleaseBuffer failed due to %s.", WASAPIErrToStr(hr));
    return 0;
  }

  return NumFramesRequested;
}

void CAESinkWASAPI::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList)
{
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;
  CAEDeviceInfo        deviceInfo;
  CAEChannelInfo       deviceChannels;

  WAVEFORMATEXTENSIBLE wfxex = {0};
  WAVEFORMATEX*        pwfxex = NULL;
  HRESULT              hr;

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
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint failed.");
      goto failed;
    }

    hr = pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.");
      SAFE_RELEASE(pDevice);
      goto failed;
    }

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint device name failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::wstring strRawFriendlyName(varName.pwszVal);
    std::string strFriendlyName = std::string(strRawFriendlyName.begin(), strRawFriendlyName.end());

    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if(FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint GUID failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::wstring strRawDevName(varName.pwszVal);
    std::string strDevName = std::string(strRawDevName.begin(), strRawDevName.end());

    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint form factor failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }
    std::string strWinDevType = winEndpoints[(EndpointFormFactor)varName.uiVal].winEndpointType;
    AEDeviceType aeDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;

    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint speaker layout failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }
    unsigned int uiChannelMask = std::max(varName.uintVal, (unsigned int) AE_CH_FL | AE_CH_FR);

    deviceChannels.Reset();

    for (unsigned int c = 0; c < WASAPI_SPEAKER_COUNT; c++)
    {
      if (uiChannelMask & WASAPIChannelOrder[c])
        deviceChannels += AEChannelNames[c];
    }

    PropVariantClear(&varName);

    IAudioClient *pClient;
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pClient);
    if (SUCCEEDED(hr))
    {
      /* Test format DTS-HD */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.Format.nSamplesPerSec       = 192000;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_7POINT1_SURROUND;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = 8;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_DTSHD));

      /* Test format Dolby TrueHD */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_TRUEHD));

      /* Test format Dolby EAC3 */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_EAC3));

      /* Test format DTS */
      wfxex.Format.nSamplesPerSec       = 48000;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_5POINT1;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_DTS));

      /* Test format Dolby AC3 */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_AC3));

      /* Test format AAC */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_AAC;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back(AEDataFormat(AE_FMT_AAC));

      /* Test format for PCM format iteration */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask               = AE_CH_FL | AE_CH_FR;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

      for (int p = AE_FMT_FLOAT; p > AE_FMT_INVALID; p--)
      {
        if (p < AE_FMT_FLOAT)
          wfxex.SubFormat               = KSDATAFORMAT_SUBTYPE_PCM;
        wfxex.Format.wBitsPerSample     = CAEUtil::DataFormatToBits((AEDataFormat) p);
        wfxex.Format.nBlockAlign        = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        if (p <= AE_FMT_S24NE4 && p >= AE_FMT_S24BE4)
        {
          wfxex.Samples.wValidBitsPerSample = 24;
        }
        else
        {
          wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
        }

        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
          deviceInfo.m_dataFormats.push_back((AEDataFormat) p);
      }

      /* Test format for sample rate iteration */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask               = AE_CH_FL | AE_CH_FR;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      for (int j = 0; j < WASAPISampleRateCount; j++)
      {
        wfxex.Format.nSamplesPerSec     = WASAPISampleRates[j];
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
          deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
      }

      /* Test format for channels iteration */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask               = AE_CH_FL | AE_CH_FR;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;
      wfxex.Format.nSamplesPerSec       = 48000;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      //bool mpcmFlagged                  = false;

      for (int k = AE_CH_LAYOUT_MAX; k > 0; k--)
      {
        DWORD mask = 0;
        for (int c = 0; c < WASAPI_SPEAKER_COUNT; c++)
        {
          if (uiChannelMask & WASAPIChannelOrder[c])
            mask |= WASAPIChannelOrder[c];
        }

        wfxex.dwChannelMask             = mask;
        wfxex.Format.nChannels          = k;
        wfxex.Format.nBlockAlign        = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
        if (SUCCEEDED(hr))
        {
          deviceChannels                = layoutsByChCount[k];
          if (k > AE_CH_LAYOUT_2_1) // && !mpcmFlagged)
          {
            deviceInfo.m_dataFormats.push_back(AE_FMT_LPCM);
            //mpcmFlagged = true;
          }
          break;
        }
      }
      pClient->Release();
    }
    else
    {
      CLog::Log(LOGDEBUG, __FUNCTION__": Failed to activate device for passthrough capability testing.");
    }

    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pProperty);

    deviceInfo.m_deviceName       = strDevName;
    deviceInfo.m_displayName      = strWinDevType.append(strFriendlyName);
    deviceInfo.m_displayNameExtra = std::string("WASAPI: ").append(strFriendlyName);
    deviceInfo.m_deviceType       = aeDeviceType;
    deviceInfo.m_channels         = deviceChannels;

    /* Now logged by AESinkFactory on startup */
    //CLog::Log(LOGDEBUG,"Audio Device %d:    %s", i, ((std::string)deviceInfo).c_str());

    deviceInfoList.push_back(deviceInfo);
  }
  return;

failed:

  if (FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__": Failed to enumerate WASAPI endpoint devices (%s).", WASAPIErrToStr(hr));

  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);
}

//Private utility functions////////////////////////////////////////////////////

void CAESinkWASAPI::BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex)
{
  wfxex.Format.wFormatTag        = WAVE_FORMAT_EXTENSIBLE;
  wfxex.Format.cbSize            = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);


  if (!AE_IS_RAW(format.m_dataFormat)) // PCM data
  {
    wfxex.dwChannelMask          = SpeakerMaskFromAEChannels(format.m_channelLayout);
    wfxex.Format.nChannels       = (WORD)format.m_channelLayout.Count();
    wfxex.Format.nSamplesPerSec  = format.m_sampleRate;
    wfxex.Format.wBitsPerSample  = format.m_dataFormat <= AE_FMT_S16NE ? 16 : 32;
    wfxex.SubFormat              = format.m_dataFormat <= AE_FMT_FLOAT ? KSDATAFORMAT_SUBTYPE_PCM : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
  }
  else //Raw bitstream
  {
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    if (format.m_dataFormat == AE_FMT_AC3 || format.m_dataFormat == AE_FMT_DTS)
    {
      wfxex.dwChannelMask          = bool (format.m_channelLayout.Count() == 2) ? KSAUDIO_SPEAKER_STEREO : KSAUDIO_SPEAKER_5POINT1;

      if (format.m_dataFormat == AE_FMT_AC3)
      {
        wfxex.SubFormat            = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
        //wfxex.Format.wFormatTag    = WAVE_FORMAT_DOLBY_AC3_SPDIF;
      }
      else
      {
        wfxex.SubFormat            = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL; //KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
        //wfxex.Format.wFormatTag    = WAVE_FORMAT_DTS;
      }

      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = (WORD)format.m_channelLayout.Count();
      wfxex.Format.nSamplesPerSec       = format.m_sampleRate;
    }
    else if (format.m_dataFormat == AE_FMT_EAC3 || format.m_dataFormat == AE_FMT_TRUEHD || format.m_dataFormat == AE_FMT_DTSHD)
    {
      //IEC 61937 transmissions.
      //Currently these formats only run over HDMI.
      wfxex.Format.nSamplesPerSec       = 192000L; // Link runs at 192 KHz.
      wfxex.Format.wBitsPerSample       = 16; // Always at 16 bits over IEC 60958.
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_7POINT1_SURROUND;

      switch (format.m_dataFormat)
      {
        case AE_FMT_EAC3:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
          wfxex.Format.nChannels      = 2; // One IEC 60958 Line.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_5POINT1;
          break;
        case AE_FMT_TRUEHD:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
          wfxex.Format.nChannels      = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
          break;
        case AE_FMT_DTSHD:
          wfxex.SubFormat             = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
          wfxex.Format.nChannels      = 8; // Four IEC 60958 Lines.
          wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
          break;
      }

      if (format.m_channelLayout.Count() == 8)
        wfxex.dwChannelMask         = KSAUDIO_SPEAKER_7POINT1_SURROUND;
      else
        wfxex.dwChannelMask         = KSAUDIO_SPEAKER_5POINT1;
    }
  }

  if (wfxex.Format.wBitsPerSample == 32 && wfxex.SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
    wfxex.Samples.wValidBitsPerSample = 24;
  else
    wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;

  wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
}

void CAESinkWASAPI::BuildWaveFormatExtensibleIEC61397(AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex)
{
  //Fill the common structure.
  BuildWaveFormatExtensible(format, wfxex.FormatExt);
  /*
  wfxex.FormatExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE_IEC61937)-sizeof(WAVEFORMATEX);
  wfxex.dwEncodedChannelCount   = format.m_channelLayout.Count();
  wfxex.dwEncodedSamplesPerSec  = bool(format.m_dataFormat == AE_FMT_TRUEHD ||
                                       format.m_dataFormat == AE_FMT_DTSHD  ||
                                       format.m_dataFormat == AE_FMT_EAC3) ? 96000L : 48000L;
  wfxex.dwAverageBytesPerSec    = 0; //Ignored */
}

bool CAESinkWASAPI::InitializeExclusive(AEAudioFormat &format)
{
  WAVEFORMATEXTENSIBLE_IEC61937 wfxex_iec61937;
  WAVEFORMATEXTENSIBLE &wfxex = wfxex_iec61937.FormatExt;

  if (format.m_dataFormat <= AE_FMT_FLOAT) //DDDamian was AE_FMT_DTS
    BuildWaveFormatExtensible(format, wfxex);
  else
    BuildWaveFormatExtensibleIEC61397(format, wfxex_iec61937);

  //test for incomplete or startup format and provide default
  if (format.m_sampleRate == 0 ||
      format.m_channelLayout == NULL ||
      format.m_dataFormat <= AE_FMT_INVALID ||
      format.m_dataFormat >= AE_FMT_MAX ||
      format.m_channelLayout.Count() == 0)
  {
    wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    wfxex.Format.nChannels            = 2;
    wfxex.Format.nSamplesPerSec       = 44100L;
    wfxex.Format.wBitsPerSample       = 16;
    wfxex.Format.nBlockAlign          = 4;
    wfxex.Samples.wValidBitsPerSample = 16;
    wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nBlockAlign * wfxex.Format.nSamplesPerSec;
    wfxex.dwChannelMask               = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;
  }

  CLog::Log(LOGDEBUG, __FUNCTION__": Checking IsFormatSupported with the following parameters:");
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

  if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_PCM");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DTS");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD");
  else
    CLog::Log(LOGDEBUG, "  SubFormat       : NO SUBFORMAT SPECIFIED");

  HRESULT hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Format is Supported - will attempt to Initialize");
    goto initialize;
  }
  else if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT) //It failed for a reason unrelated to an unsupported format.
  {
    CLog::Log(LOGERROR, __FUNCTION__": IsFormatSupported failed (%s)", WASAPIErrToStr(hr));
    return false;
  }
  else if (AE_IS_RAW(format.m_dataFormat)) //No sense in trying other formats for passthrough.
    return false;

  CLog::Log(LOGERROR, __FUNCTION__": IsFormatSupported failed (%s) - trying to find a compatible format", WASAPIErrToStr(hr));

  int closestMatch;

  //The requested format is not supported by the device.  Find something that works.
  //Try other formats
  for (int j = 0; j < sizeof(testFormats)/sizeof(sampleFormat); j++)
  {
    closestMatch = -1;

    wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat                   = testFormats[j].subFormat;
    wfxex.Format.wBitsPerSample       = testFormats[j].bitsPerSample;
    wfxex.Samples.wValidBitsPerSample = testFormats[j].validBitsPerSample;
    wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);

    for (int i = 0 ; i < WASAPISampleRateCount; i++)
    {
      wfxex.Format.nSamplesPerSec    = WASAPISampleRates[i];
      wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      /* Uncomment to trace format match iteration loop via log */
      CLog::Log(LOGDEBUG, "WASAPI: Trying Sample Format    : %s", CAEUtil::DataFormatToStr(testFormats[j].subFormatType));
      CLog::Log(LOGDEBUG, "WASAPI: Trying Sample Rate      : %d", wfxex.Format.nSamplesPerSec);
      CLog::Log(LOGDEBUG, "WASAPI: Trying Bits/Sample      : %d", wfxex.Format.wBitsPerSample);
      CLog::Log(LOGDEBUG, "WASAPI: Trying Valid Bits/Sample: %d", wfxex.Samples.wValidBitsPerSample);

      hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

      if (SUCCEEDED(hr))
      {
        //If the current sample rate matches the source then stop looking and use it.
        if ((WASAPISampleRates[i] == format.m_sampleRate) && (testFormats[j].subFormatType <= format.m_dataFormat))
          goto initialize;
        //If this rate is closer to the source then the previous one, save it.
        else if (closestMatch < 0 || abs((int)WASAPISampleRates[i] - (int)format.m_sampleRate) < abs((int)WASAPISampleRates[closestMatch] - (int)format.m_sampleRate))
          closestMatch = i;
      }
      else if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT)
          CLog::Log(LOGERROR, __FUNCTION__": IsFormatSupported failed (%s)", WASAPIErrToStr(hr));
    }

    if (closestMatch >= 0)
    {
      wfxex.Format.nSamplesPerSec    = WASAPISampleRates[closestMatch];
      wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      goto initialize;
    }
  }

  CLog::Log(LOGERROR, __FUNCTION__": Unable to locate a supported output format for the device.  Check the speaker settings in the control panel.");

  //We couldn't find anything supported.  This should never happen unless the user set the wrong
  //speaker setting in the control panel.
  return false;

initialize:

  AEChannelsFromSpeakerMask(wfxex.dwChannelMask);

  //When the stream is raw, the values in the format structure are set to the link
  //parameters, so store the encoded stream values here for the IsCompatible function.
  m_encodedFormat     = format.m_dataFormat;
  m_encodedChannels   = wfxex.Format.nChannels;
  m_encodedSampleRate = format.m_encodedRate;// bool(format.m_dataFormat == AE_FMT_TRUEHD || format.m_dataFormat == AE_FMT_DTSHD) ? 96000L : 48000L;
  wfxex_iec61937.dwEncodedChannelCount = wfxex.Format.nChannels;
  wfxex_iec61937.dwEncodedSamplesPerSec = m_encodedSampleRate;

  if (!AE_IS_RAW(format.m_dataFormat))
  {
    if (wfxex.Format.wBitsPerSample == 32)
    {
      if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        format.m_dataFormat = AE_FMT_FLOAT;
      else if (wfxex.Samples.wValidBitsPerSample == 32)
        format.m_dataFormat = AE_FMT_S32NE;
      else
        format.m_dataFormat = AE_FMT_S24NE4;
    }
    else
    {
      format.m_dataFormat = AE_FMT_S16NE;
    }
  }

  format.m_sampleRate    = wfxex.Format.nSamplesPerSec; //PCM: Sample rate.  RAW: Link speed

  format.m_frameSize     = (wfxex.Format.wBitsPerSample >> 3) * wfxex.Format.nChannels;

  REFERENCE_TIME audioSinkBufferDurationMsec, hnsLatency;

  /* Get m_audioSinkBufferSizeMsec from advancedsettings.xml */
  audioSinkBufferDurationMsec = (REFERENCE_TIME)g_advancedSettings.m_audioSinkBufferDurationMsec * 10000;

  //use user's advancedsetting value for buffer size as long as it's over minimum set above
  audioSinkBufferDurationMsec = (REFERENCE_TIME)std::max(audioSinkBufferDurationMsec, (REFERENCE_TIME)500000);
  audioSinkBufferDurationMsec = (REFERENCE_TIME)((audioSinkBufferDurationMsec / format.m_frameSize) * format.m_frameSize); //even number of frames

  CLog::Log(LOGDEBUG, __FUNCTION__": Initializing WASAPI exclusive mode with the following parameters:");
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
  CLog::Log(LOGDEBUG, "  Enc. Channels   : %d", wfxex_iec61937.dwEncodedChannelCount);
  CLog::Log(LOGDEBUG, "  Enc. Samples/Sec: %d", wfxex_iec61937.dwEncodedSamplesPerSec);
  CLog::Log(LOGDEBUG, "  Channel Mask    : %d", wfxex.dwChannelMask);
  CLog::Log(LOGDEBUG, "  Periodicty      : %d", audioSinkBufferDurationMsec);

  if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_PCM");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DTS");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP");
  else if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD)
    CLog::Log(LOGDEBUG, "  SubFormat       : KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD");
  else
    CLog::Log(LOGDEBUG, "  SubFormat       : NO SUBFORMAT SPECIFIED");

  if (AE_IS_RAW(format.m_dataFormat))
    format.m_dataFormat = AE_FMT_S16NE;

  hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                    audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, NULL);

  if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Re-aligning WASAPI sink buffer due to %s.", WASAPIErrToStr(hr));
    // Get the next aligned frame.
    hr = m_pAudioClient->GetBufferSize(&m_uiBufferLen);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": GetBufferSize Failed : %s", WASAPIErrToStr(hr));
      return false;
    }

    audioSinkBufferDurationMsec = (REFERENCE_TIME) ((10000.0 * 1000 / wfxex.Format.nSamplesPerSec * m_uiBufferLen) + 0.5);
    CLog::Log(LOGDEBUG, __FUNCTION__": Number of Frames in Buffer   : %d", m_uiBufferLen);
    CLog::Log(LOGDEBUG, __FUNCTION__": Requested Duration of Buffer : %d", audioSinkBufferDurationMsec);

    // Release the previous allocations.
    SAFE_RELEASE(m_pAudioClient);

    // Create a new audio client.
    hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Device Activation Failed : %s", WASAPIErrToStr(hr));
      return false;
    }

    // Open the stream and associate it with an audio session.
    hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                      audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, NULL);
  }
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Unable to initialize WASAPI in exclusive mode %d - (%s).", HRESULT(hr), WASAPIErrToStr(hr));
    return false;
  }
  hr = m_pAudioClient->GetStreamLatency(&hnsLatency);
  CLog::Log(LOGDEBUG,  __FUNCTION__": Requested Duration of Buffer : %fmsec", hnsLatency / 10000.0);
  CLog::Log(LOGNOTICE, __FUNCTION__": WASAPI Exclusive Mode Sink Initialized Successfully!!!");
  return true;
}

void CAESinkWASAPI::AEChannelsFromSpeakerMask(DWORD speakers)
{
  m_channelLayout.Reset();

  for (int i = 0; i < WASAPI_SPEAKER_COUNT; i++)
  {
    if (speakers & WASAPIChannelOrder[i])
      m_channelLayout += AEChannelNames[i];
  }
}

DWORD CAESinkWASAPI::SpeakerMaskFromAEChannels(const CAEChannelInfo &channels)
{
  DWORD mask = 0;

  for (unsigned int i = 0; i < channels.Count(); i++)
  {
    for (unsigned int j = 0; j < WASAPI_SPEAKER_COUNT; j++)
      if (channels[i] == AEChannelNames[j])
        mask |= WASAPIChannelOrder[j];
  }
  return mask;
}

const char *CAESinkWASAPI::WASAPIErrToStr(HRESULT err)
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
