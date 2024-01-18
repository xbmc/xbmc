/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkWASAPI.h"

#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <stdint.h>

#include <Audioclient.h>
#include <Mmreg.h>

#ifdef TARGET_WINDOWS_DESKTOP
#  pragma comment(lib, "Avrt.lib")
#endif // TARGET_WINDOWS_DESKTOP

const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioClock = __uuidof(IAudioClock);
DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);
DEFINE_PROPERTYKEY(PKEY_Device_EnumeratorName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 24);

extern const char *WASAPIErrToStr(HRESULT err);
#define EXIT_ON_FAILURE(hr, reason) \
  if (FAILED(hr)) \
  { \
    CLog::LogF(LOGERROR, reason " - HRESULT = {} ErrorMessage = {}", hr, WASAPIErrToStr(hr)); \
    goto failed; \
  }

template<class T>
inline void SafeRelease(T **ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

using namespace Microsoft::WRL;

CAESinkWASAPI::CAESinkWASAPI()
{
  m_channelLayout.Reset();
}

CAESinkWASAPI::~CAESinkWASAPI()
{
}

void CAESinkWASAPI::Register()
{
  AE::AESinkRegEntry reg;
  reg.sinkName = "WASAPI";
  reg.createFunc = CAESinkWASAPI::Create;
  reg.enumerateFunc = CAESinkWASAPI::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(reg);
}

std::unique_ptr<IAESink> CAESinkWASAPI::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkWASAPI>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

bool CAESinkWASAPI::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  m_device = device;
  bool bdefault = false;
  HRESULT hr = S_FALSE;

  /* Save requested format */
  /* Clear returned format */
  sinkReqFormat = format.m_dataFormat;
  sinkRetFormat = AE_FMT_INVALID;

  if(StringUtils::EndsWithNoCase(device, std::string("default")))
    bdefault = true;

  if(!bdefault)
  {
    hr = CAESinkFactoryWin::ActivateWASAPIDevice(device, &m_pDevice);
    EXIT_ON_FAILURE(hr, "Retrieval of WASAPI endpoint failed.")
  }

  if (!m_pDevice)
  {
    if(!bdefault)
    {
      CLog::LogF(LOGINFO,
                 "Could not locate the device named \"{}\" in the list of WASAPI endpoint devices. "
                 " Trying the default device...",
                 device);
    }

    std::string defaultId = CAESinkFactoryWin::GetDefaultDeviceId();
    if (defaultId.empty())
    {
      CLog::LogF(LOGINFO, "Could not locate the default device id in the list of WASAPI endpoint devices.");
      goto failed;
    }

    hr = CAESinkFactoryWin::ActivateWASAPIDevice(defaultId, &m_pDevice);
    EXIT_ON_FAILURE(hr, "Could not retrieve the default WASAPI audio endpoint.")

    device = defaultId;
  }

  hr = m_pDevice->Activate(m_pAudioClient.ReleaseAndGetAddressOf());
  EXIT_ON_FAILURE(hr, "Activating the WASAPI endpoint device failed.")

  if (!InitializeExclusive(format))
  {
    CLog::LogF(LOGINFO, "Could not Initialize Exclusive with that format");
    goto failed;
  }

  /* get the buffer size and calculate the frames for AE */
  m_pAudioClient->GetBufferSize(&m_uiBufferLen);

  format.m_frames       = m_uiBufferLen;
  m_format              = format;
  sinkRetFormat         = format.m_dataFormat;

  hr = m_pAudioClient->GetService(IID_IAudioRenderClient, reinterpret_cast<void**>(m_pRenderClient.ReleaseAndGetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not initialize the WASAPI render client interface.")

  hr = m_pAudioClient->GetService(IID_IAudioClock, reinterpret_cast<void**>(m_pAudioClock.ReleaseAndGetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not initialize the WASAPI audio clock interface.")

  hr = m_pAudioClock->GetFrequency(&m_clockFreq);
  EXIT_ON_FAILURE(hr, "Retrieval of IAudioClock::GetFrequency failed.")

  m_needDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  hr = m_pAudioClient->SetEventHandle(m_needDataEvent);
  EXIT_ON_FAILURE(hr, "Could not set the WASAPI event handler.");

  m_initialized = true;
  m_isDirty     = false;

  // allow feeding less samples than buffer size
  // if the device is opened exclusive and event driven, provided samples must match buffersize
  // ActiveAE tries to align provided samples with buffer size but cannot guarantee (e.g. transcoding)
  // this can be avoided by dropping the event mode which has not much benefit; SoftAE polls anyway
  m_buffer.resize(format.m_frames * format.m_frameSize);
  m_bufferPtr = 0;

  return true;

failed:
  CLog::LogF(LOGERROR, "WASAPI initialization failed.");
  SafeRelease(&m_pDevice);
  if(m_needDataEvent)
  {
    CloseHandle(m_needDataEvent);
    m_needDataEvent = 0;
  }

  return false;
}

void CAESinkWASAPI::Deinitialize()
{
  if (!m_initialized && !m_isDirty)
    return;

  if (m_running)
  {
    try
    {
    m_pAudioClient->Stop();  //stop the audio output
    m_pAudioClient->Reset(); //flush buffer and reset audio clock stream position
    m_sinkFrames = 0;
    }
    catch (...)
    {
      CLog::LogF(LOGDEBUG, "Invalidated AudioClient - Releasing");
    }
  }
  m_running = false;

  CloseHandle(m_needDataEvent);

  m_pRenderClient = nullptr;
  m_pAudioClient = nullptr;
  m_pAudioClock = nullptr;
  SafeRelease(&m_pDevice);

  m_initialized = false;

  m_bufferPtr = 0;
}

/**
 * @brief rescale uint64_t without overflowing on large values
 */
static uint64_t rescale_u64(uint64_t val, uint64_t num, uint64_t den)
{
  return ((val / den) * num) + (((val % den) * num) / den);
}


void CAESinkWASAPI::GetDelay(AEDelayStatus& status)
{
  HRESULT hr;
  uint64_t pos, tick;
  int retries = 0;

  if (!m_initialized)
    goto failed;

  do {
    hr = m_pAudioClock->GetPosition(&pos, &tick);
  } while (hr != S_OK && ++retries < 100);
  EXIT_ON_FAILURE(hr, "Retrieval of IAudioClock::GetPosition failed.")

  status.delay = (double)(m_sinkFrames + m_bufferPtr) / m_format.m_sampleRate - (double)pos / m_clockFreq;
  status.tick  = rescale_u64(tick, CurrentHostFrequency(), 10000000); /* convert from 100ns back to qpc ticks */
  return;
failed:
  status.SetDelay(0);
}

double CAESinkWASAPI::GetCacheTotal()
{
  if (!m_initialized)
    return 0.0;

  return m_sinkLatency;
}

unsigned int CAESinkWASAPI::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
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

  unsigned int NumFramesRequested = m_format.m_frames;
  unsigned int FramesToCopy = std::min(m_format.m_frames - m_bufferPtr, frames);
  uint8_t *buffer = data[0]+offset*m_format.m_frameSize;
  if (m_bufferPtr != 0 || frames != m_format.m_frames)
  {
    memcpy(m_buffer.data() + m_bufferPtr * m_format.m_frameSize, buffer,
           FramesToCopy * m_format.m_frameSize);
    m_bufferPtr += FramesToCopy;
    if (m_bufferPtr != m_format.m_frames)
      return frames;
  }

  if (!m_running) //first time called, pre-fill buffer then start audio client
  {
    hr = m_pAudioClient->Reset();
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, " AudioClient reset failed due to {}", WASAPIErrToStr(hr));
      return 0;
    }
    hr = m_pRenderClient->GetBuffer(NumFramesRequested, &buf);
    if (FAILED(hr))
    {
      #ifdef _DEBUG
      CLog::LogF(LOGERROR, "GetBuffer failed due to {}", WASAPIErrToStr(hr));
#endif
      m_isDirty = true; //flag new device or re-init needed
      return INT_MAX;
    }

    memset(buf, 0, NumFramesRequested * m_format.m_frameSize); //fill buffer with silence

    hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
    if (FAILED(hr))
    {
      #ifdef _DEBUG
      CLog::LogF(LOGDEBUG, "ReleaseBuffer failed due to {}.", WASAPIErrToStr(hr));
#endif
      m_isDirty = true; //flag new device or re-init needed
      return INT_MAX;
    }
    m_sinkFrames += NumFramesRequested;

    hr = m_pAudioClient->Start(); //start the audio driver running
    if (FAILED(hr))
      CLog::LogF(LOGERROR, "AudioClient Start Failed");
    m_running = true; //signal that we're processing frames
    return 0U;
  }

#ifndef _DEBUG
  /* Get clock time for latency checks */
  QueryPerformanceFrequency(&timerFreq);
  QueryPerformanceCounter(&timerStart);
#endif

  /* Wait for Audio Driver to tell us it's got a buffer available */
  DWORD eventAudioCallback;
  eventAudioCallback = WaitForSingleObject(m_needDataEvent, 1100);

  if(eventAudioCallback != WAIT_OBJECT_0 || !&buf)
  {
    CLog::LogF(LOGERROR, "Endpoint Buffer timed out");
    return INT_MAX;
  }

  if (!m_running)
    return 0;

#ifndef _DEBUG
  QueryPerformanceCounter(&timerStop);
  LONGLONG timerDiff = timerStop.QuadPart - timerStart.QuadPart;
  double timerElapsed = (double) timerDiff * 1000.0 / (double) timerFreq.QuadPart;
  m_avgTimeWaiting += (timerElapsed - m_avgTimeWaiting) * 0.5;

  if (m_avgTimeWaiting < 3.0)
  {
    CLog::LogF(LOGDEBUG, "Possible AQ Loss: Avg. Time Waiting for Audio Driver callback : {}msec",
               (int)m_avgTimeWaiting);
  }
#endif

  hr = m_pRenderClient->GetBuffer(NumFramesRequested, &buf);
  if (FAILED(hr))
  {
#ifdef _DEBUG
    CLog::LogF(LOGERROR, "GetBuffer failed due to {}", WASAPIErrToStr(hr));
#endif
    return INT_MAX;
  }

  // fill buffer
  memcpy(buf, m_bufferPtr == 0 ? buffer : m_buffer.data(),
         NumFramesRequested * m_format.m_frameSize);
  m_bufferPtr = 0;

  hr = m_pRenderClient->ReleaseBuffer(NumFramesRequested, flags); //pass back to audio driver
  if (FAILED(hr))
  {
#ifdef _DEBUG
    CLog::LogF(LOGDEBUG, "ReleaseBuffer failed due to {}.", WASAPIErrToStr(hr));
#endif
    return INT_MAX;
  }
  m_sinkFrames += NumFramesRequested;

  if (FramesToCopy != frames)
  {
    m_bufferPtr = frames-FramesToCopy;
    memcpy(m_buffer.data(), buffer + FramesToCopy * m_format.m_frameSize,
           m_bufferPtr * m_format.m_frameSize);
  }

  return frames;
}

void CAESinkWASAPI::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  CAEDeviceInfo        deviceInfo;
  CAEChannelInfo       deviceChannels;
  bool                 add192 = false;
  bool add48 = false;

  WAVEFORMATEXTENSIBLE wfxex = {};
  HRESULT              hr;

  const bool onlyPT = (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::WindowsDeviceFamily::Xbox);

  for(RendererDetail& details : CAESinkFactoryWin::GetRendererDetails())
  {
    deviceInfo.m_channels.Reset();
    deviceInfo.m_dataFormats.clear();
    deviceInfo.m_sampleRates.clear();
    deviceChannels.Reset();

    for (unsigned int c = 0; c < WASAPI_SPEAKER_COUNT; c++)
    {
      if (details.uiChannelMask & WASAPIChannelOrder[c])
        deviceChannels += AEChannelNames[c];
    }

    IAEWASAPIDevice* pDevice;
    hr = CAESinkFactoryWin::ActivateWASAPIDevice(details.strDeviceId, &pDevice);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of WASAPI endpoint failed.");
      goto failed;
    }

    ComPtr<IAudioClient> pClient = nullptr;
    hr = pDevice->Activate(pClient.GetAddressOf());
    if (SUCCEEDED(hr))
    {
      /* Test format DTS-HD-HR */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.Format.nSamplesPerSec       = 192000;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_5POINT1;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
      wfxex.Format.wBitsPerSample       = 16;
      wfxex.Samples.wValidBitsPerSample = 16;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
      {
        CLog::LogF(LOGINFO,
                   "Exclusive mode is not allowed on device \"{}\", check device settings.",
                   details.strDescription);
        SafeRelease(&pDevice);
        continue;
      }
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if(FAILED(hr))
        {
          CLog::LogF(LOGINFO, "stream type \"{}\" on device \"{}\" seems to be not supported.",
                     CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_DTSHD),
                     details.strDescription);
        }

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
        add192 = true;
      }

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
      if (hr == AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED)
      {
        CLog::LogF(LOGINFO,
                   "Exclusive mode is not allowed on device \"{}\", check device settings.",
                   details.strDescription);
        SafeRelease(&pDevice);
        continue;
      }
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if(FAILED(hr))
        {
          CLog::LogF(LOGINFO, "stream type \"{}\" on device \"{}\" seems to be not supported.",
                     CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_DTSHD_MA),
                     details.strDescription);
        }

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_MA);
        add192 = true;
      }

      /* Test format Dolby TrueHD */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if(FAILED(hr))
        {
          CLog::LogF(LOGINFO, "stream type \"{}\" on device \"{}\" seems to be not supported.",
                     CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_TRUEHD),
                     details.strDescription);
        }

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
        add192 = true;
      }

      /* Test format Dolby EAC3 */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
      wfxex.Format.nChannels            = 2;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if(FAILED(hr))
        {
          CLog::LogF(LOGINFO, "stream type \"{}\" on device \"{}\" seems to be not supported.",
                     CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_EAC3),
                     details.strDescription);
        }

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
        add192 = true;
      }

      /* Test format DTS */
      wfxex.Format.nSamplesPerSec       = 48000;
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_5POINT1;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
      wfxex.Format.nBlockAlign          = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec      = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if(FAILED(hr))
        {
          CLog::LogF(LOGINFO, "stream type \"{}\" on device \"{}\" seems to be not supported.",
                     "STREAM_TYPE_DTS", details.strDescription);
        }

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
        add48 = true;
      }

      /* Test format Dolby AC3 */
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;
      hr = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);
      if (SUCCEEDED(hr) || details.eDeviceType == AE_DEVTYPE_HDMI)
      {
        if(FAILED(hr))
        {
          CLog::LogF(LOGINFO, "stream type \"{}\" on device \"{}\" seems to be not supported.",
                     CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_AC3),
                     details.strDescription);
        }

        deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
        add48 = true;
      }

      /* Test format for PCM format iteration */
      wfxex.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_STEREO;
      wfxex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
      wfxex.SubFormat                   = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

      for (int p = AE_FMT_FLOAT; p > AE_FMT_INVALID; p--)
      {
        if (p < AE_FMT_FLOAT)
          wfxex.SubFormat               = KSDATAFORMAT_SUBTYPE_PCM;
        wfxex.Format.wBitsPerSample     = CAEUtil::DataFormatToBits((AEDataFormat) p);
        wfxex.Format.nBlockAlign        = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
        wfxex.Format.nAvgBytesPerSec    = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        if (p == AE_FMT_S24NE4MSB)
        {
          wfxex.Samples.wValidBitsPerSample = 24;
        }
        else if (p <= AE_FMT_S24NE4 && p >= AE_FMT_S24BE4)
        {
          // not supported
          continue;
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
      wfxex.dwChannelMask               = KSAUDIO_SPEAKER_STEREO;
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
        else if (wfxex.Format.nSamplesPerSec == 192000 && add192)
        {
          deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
          CLog::LogF(LOGINFO, "sample rate 192khz on device \"{}\" seems to be not supported.",
                     details.strDescription);
        }
        else if (wfxex.Format.nSamplesPerSec == 48000 && add48)
        {
          deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
          CLog::LogF(LOGINFO, "sample rate 48khz on device \"{}\" seems to be not supported.",
                     details.strDescription);
        }
      }
      pClient = nullptr;
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Failed to activate device for passthrough capability testing.");
    }

    deviceInfo.m_deviceName       = details.strDeviceId;
    deviceInfo.m_displayName      = details.strWinDevType.append(details.strDescription);
    deviceInfo.m_displayNameExtra = std::string("WASAPI: ").append(details.strDescription);
    deviceInfo.m_deviceType       = details.eDeviceType;
    deviceInfo.m_channels         = deviceChannels;

    /* Store the device info */
    deviceInfo.m_wantsIECPassthrough = true;
    deviceInfo.m_onlyPassthrough = onlyPT;

    if (!deviceInfo.m_streamTypes.empty())
      deviceInfo.m_dataFormats.push_back(AE_FMT_RAW);

    deviceInfoList.push_back(deviceInfo);

    if (details.bDefault)
    {
      deviceInfo.m_deviceName = std::string("default");
      deviceInfo.m_displayName = std::string("default");
      deviceInfo.m_displayNameExtra = std::string("");
      deviceInfo.m_wantsIECPassthrough = true;
      deviceInfo.m_onlyPassthrough = onlyPT;
      deviceInfoList.push_back(deviceInfo);
    }

    SafeRelease(&pDevice);
  }
  return;

failed:

  if (FAILED(hr))
    CLog::LogF(LOGERROR, "Failed to enumerate WASAPI endpoint devices ({}).", WASAPIErrToStr(hr));
}

//Private utility functions////////////////////////////////////////////////////

void CAESinkWASAPI::BuildWaveFormatExtensibleIEC61397(AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex)
{
  /* Fill the common structure */
  CAESinkFactoryWin::BuildWaveFormatExtensible(format, wfxex.FormatExt);

  /* Code below kept for future use - preferred for later Windows versions */
  /* but can cause problems on older Windows versions and drivers          */
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

  if (format.m_dataFormat <= AE_FMT_FLOAT)
    CAESinkFactoryWin::BuildWaveFormatExtensible(format, wfxex);
  else if (format.m_dataFormat == AE_FMT_RAW)
    BuildWaveFormatExtensibleIEC61397(format, wfxex_iec61937);
  else
  {
    // planar formats are currently not supported by this sink
    format.m_dataFormat = AE_FMT_FLOAT;
    CAESinkFactoryWin::BuildWaveFormatExtensible(format, wfxex);
  }

  // Prevents NULL speaker mask. To do: debug exact cause.
  // When this happens requested AE format is AE_FMT_FLOAT + channel layout
  // RAW, RAW, RAW... (6 channels). Only happens at end of playback PT
  // stream, force to defaults does not affect functionality or user
  // experience. Only avoids crash.
  if (!wfxex.dwChannelMask && format.m_dataFormat <= AE_FMT_FLOAT)
  {
    CLog::LogF(LOGWARNING, "NULL Channel Mask detected. Default values are enforced.");
    format.m_sampleRate = 0; // force defaults in following code
  }

  /* Test for incomplete format and provide defaults */
  if (format.m_sampleRate == 0 ||
      format.m_channelLayout == CAEChannelInfo(nullptr) ||
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

  HRESULT hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

  int closestMatch = 0;
  unsigned int requestedChannels = 0;
  unsigned int noOfCh = 0;
  uint64_t desired_map = 0;
  bool matchNoChannelsOnly = false;

  if (SUCCEEDED(hr))
  {
    CLog::LogF(LOGINFO, "Format is Supported - will attempt to Initialize");
    goto initialize;
  }
  else if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT) //It failed for a reason unrelated to an unsupported format.
  {
    CLog::LogF(LOGERROR, "IsFormatSupported failed ({})", WASAPIErrToStr(hr));
    return false;
  }
  else if (format.m_dataFormat == AE_FMT_RAW) //No sense in trying other formats for passthrough.
    return false;

  CLog::Log(LOGWARNING,
            "AESinkWASAPI: IsFormatSupported failed ({}) - trying to find a compatible format",
            WASAPIErrToStr(hr));

  requestedChannels = wfxex.Format.nChannels;
  desired_map = CAESinkFactoryWin::SpeakerMaskFromAEChannels(format.m_channelLayout);

  /* The requested format is not supported by the device.  Find something that works */
  CLog::Log(LOGWARNING,
            "AESinkWASAPI: Input channels are [{}] - Trying to find a matching output layout",
            std::string(format.m_channelLayout));
  for (int layout = -1; layout <= (int)ARRAYSIZE(layoutsList); layout++)
  {
    // if requested layout is not supported, try standard layouts which contain
    // at least the same channels as the input source
    // as the last resort try stereo
    if (layout == ARRAYSIZE(layoutsList))
    {
      if (matchNoChannelsOnly)
      {
        wfxex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
        wfxex.Format.nChannels = 2;
      }
      else
      {
        matchNoChannelsOnly = true;
        layout = -1;
        CLog::Log(LOGWARNING, "AESinkWASAPI: Match only number of audio channels as fallback");
        continue;
      }
    }
    else if (layout >= 0)
    {
      wfxex.dwChannelMask = CAESinkFactoryWin::ChLayoutToChMask(layoutsList[layout], &noOfCh);
      wfxex.Format.nChannels = noOfCh;
      int res = desired_map & wfxex.dwChannelMask;
      if (matchNoChannelsOnly)
      {
        if (noOfCh < requestedChannels)
          continue; // number of channels doesn't match requested channels
      }
      else
      {
        if (res != desired_map)
          continue; // output channel layout doesn't match input channels
      }
    }
    CAEChannelInfo foundChannels;
    CAESinkFactoryWin::AEChannelsFromSpeakerMask(foundChannels, wfxex.dwChannelMask);
    CLog::Log(LOGDEBUG, "AESinkWASAPI: Trying matching channel layout [{}]",
              std::string(foundChannels));

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

        /* Trace format match iteration loop via log */
#if 0
        CLog::Log(LOGDEBUG, "WASAPI: Trying Format: {}, {}, {}, {}", CAEUtil::DataFormatToStr(testFormats[j].subFormatType),
          wfxex.Format.nSamplesPerSec,
          wfxex.Format.wBitsPerSample,
          wfxex.Samples.wValidBitsPerSample);
#endif

        hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxex.Format, NULL);

        if (SUCCEEDED(hr))
        {
          /* If the current sample rate matches the source then stop looking and use it */
          if ((WASAPISampleRates[i] == format.m_sampleRate) && (testFormats[j].subFormatType <= format.m_dataFormat))
            goto initialize;
          /* If this rate is closer to the source then the previous one, save it */
          else if (closestMatch < 0 || abs((int)WASAPISampleRates[i] - (int)format.m_sampleRate) < abs((int)WASAPISampleRates[closestMatch] - (int)format.m_sampleRate))
            closestMatch = i;
        }
        else if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT)
          CLog::LogF(LOGERROR, "IsFormatSupported failed ({})", WASAPIErrToStr(hr));
      }

      if (closestMatch >= 0)
      {
        wfxex.Format.nSamplesPerSec    = WASAPISampleRates[closestMatch];
        wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        goto initialize;
      }
    }
    CLog::Log(LOGDEBUG, "AESinkWASAPI: Format [{}] not supported by driver",
              std::string(foundChannels));
  }

  CLog::LogF(LOGERROR, "Unable to locate a supported output format for the device.  "
                                   "Check the speaker settings in the control panel.");

  /* We couldn't find anything supported. This should never happen      */
  /* unless the user set the wrong speaker setting in the control panel */
  return false;

initialize:

  CAESinkFactoryWin::AEChannelsFromSpeakerMask(m_channelLayout, wfxex.dwChannelMask);
  format.m_channelLayout = m_channelLayout;

  /* When the stream is raw, the values in the format structure are set to the link    */
  /* parameters, so store the encoded stream values here for the IsCompatible function */
  m_encodedChannels   = wfxex.Format.nChannels;
  m_encodedSampleRate = (format.m_dataFormat == AE_FMT_RAW) ? format.m_streamInfo.m_sampleRate : format.m_sampleRate;
  wfxex_iec61937.dwEncodedChannelCount = wfxex.Format.nChannels;
  wfxex_iec61937.dwEncodedSamplesPerSec = m_encodedSampleRate;

  /* Set up returned sink format for engine */
  if (format.m_dataFormat != AE_FMT_RAW)
  {
    if (wfxex.Format.wBitsPerSample == 32)
    {
      if (wfxex.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        format.m_dataFormat = AE_FMT_FLOAT;
      else if (wfxex.Samples.wValidBitsPerSample == 32)
        format.m_dataFormat = AE_FMT_S32NE;
      else
        format.m_dataFormat = AE_FMT_S24NE4MSB;
    }
    else if (wfxex.Format.wBitsPerSample == 24)
      format.m_dataFormat = AE_FMT_S24NE3;
    else
      format.m_dataFormat = AE_FMT_S16NE;
  }

  format.m_sampleRate    = wfxex.Format.nSamplesPerSec; //PCM: Sample rate.  RAW: Link speed
  format.m_frameSize     = (wfxex.Format.wBitsPerSample >> 3) * wfxex.Format.nChannels;

  REFERENCE_TIME audioSinkBufferDurationMsec, hnsLatency;

  audioSinkBufferDurationMsec = (REFERENCE_TIME)500000;
  if (IsUSBDevice())
  {
    CLog::LogF(LOGDEBUG, "detected USB device, increasing buffer size");
    audioSinkBufferDurationMsec = (REFERENCE_TIME)1000000;
  }
  audioSinkBufferDurationMsec = (REFERENCE_TIME)((audioSinkBufferDurationMsec / format.m_frameSize) * format.m_frameSize); //even number of frames

  if (format.m_dataFormat == AE_FMT_RAW)
    format.m_dataFormat = AE_FMT_S16NE;

  hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                    audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, NULL);

  if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
  {
    /* WASAPI requires aligned buffer */
    /* Get the next aligned frame     */
    hr = m_pAudioClient->GetBufferSize(&m_uiBufferLen);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "GetBufferSize Failed : {}", WASAPIErrToStr(hr));
      return false;
    }

    audioSinkBufferDurationMsec = (REFERENCE_TIME) ((10000.0 * 1000 / wfxex.Format.nSamplesPerSec * m_uiBufferLen) + 0.5);

    /* Release the previous allocations */
    /* Create a new audio client */
    hr = m_pDevice->Activate(m_pAudioClient.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Device Activation Failed : {}", WASAPIErrToStr(hr));
      return false;
    }

    /* Open the stream and associate it with an audio session */
    hr = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                      audioSinkBufferDurationMsec, audioSinkBufferDurationMsec, &wfxex.Format, NULL);
  }
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Failed to initialize WASAPI in exclusive mode {} - ({}).", HRESULT(hr),
               WASAPIErrToStr(hr));
    CLog::Log(LOGDEBUG, "  Sample Rate     : {}", wfxex.Format.nSamplesPerSec);
    CLog::Log(LOGDEBUG, "  Sample Format   : {}", CAEUtil::DataFormatToStr(format.m_dataFormat));
    CLog::Log(LOGDEBUG, "  Bits Per Sample : {}", wfxex.Format.wBitsPerSample);
    CLog::Log(LOGDEBUG, "  Valid Bits/Samp : {}", wfxex.Samples.wValidBitsPerSample);
    CLog::Log(LOGDEBUG, "  Channel Count   : {}", wfxex.Format.nChannels);
    CLog::Log(LOGDEBUG, "  Block Align     : {}", wfxex.Format.nBlockAlign);
    CLog::Log(LOGDEBUG, "  Avg. Bytes Sec  : {}", wfxex.Format.nAvgBytesPerSec);
    CLog::Log(LOGDEBUG, "  Samples/Block   : {}", wfxex.Samples.wSamplesPerBlock);
    CLog::Log(LOGDEBUG, "  Format cBSize   : {}", wfxex.Format.cbSize);
    CLog::Log(LOGDEBUG, "  Channel Layout  : {}", ((std::string)format.m_channelLayout));
    CLog::Log(LOGDEBUG, "  Enc. Channels   : {}", wfxex_iec61937.dwEncodedChannelCount);
    CLog::Log(LOGDEBUG, "  Enc. Samples/Sec: {}", wfxex_iec61937.dwEncodedSamplesPerSec);
    CLog::Log(LOGDEBUG, "  Channel Mask    : {}", wfxex.dwChannelMask);
    CLog::Log(LOGDEBUG, "  Periodicty      : {}", audioSinkBufferDurationMsec);
    return false;
  }

  /* Latency of WASAPI buffers in event-driven mode is equal to the returned value  */
  /* of GetStreamLatency converted from 100ns intervals to seconds then multiplied  */
  /* by two as there are two equally-sized buffers and playback starts when the     */
  /* second buffer is filled. Multiplying the returned 100ns intervals by 0.0000002 */
  /* is handles both the unit conversion and twin buffers.                          */
  hr = m_pAudioClient->GetStreamLatency(&hnsLatency);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "GetStreamLatency Failed : {}", WASAPIErrToStr(hr));
    return false;
  }

  m_sinkLatency = hnsLatency * 0.0000002;

  CLog::LogF(LOGINFO, "WASAPI Exclusive Mode Sink Initialized using: {}, {}, {}",
             CAEUtil::DataFormatToStr(format.m_dataFormat), wfxex.Format.nSamplesPerSec,
             wfxex.Format.nChannels);
  return true;
}

void CAESinkWASAPI::Drain()
{
  if(!m_pAudioClient)
    return;

  AEDelayStatus status;
  GetDelay(status);

  KODI::TIME::Sleep(std::chrono::milliseconds(static_cast<int>(status.GetDelay() * 500)));

  if (m_running)
  {
    try
    {
      m_pAudioClient->Stop();  //stop the audio output
      m_pAudioClient->Reset(); //flush buffer and reset audio clock stream position
      m_sinkFrames = 0;
    }
    catch (...)
    {
      CLog::LogF(LOGDEBUG, "Invalidated AudioClient - Releasing");
    }
  }
  m_running = false;
}

bool CAESinkWASAPI::IsUSBDevice()
{
  return m_pDevice && m_pDevice->IsUSBDevice();
}
