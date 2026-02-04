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
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <algorithm>
#include <chrono>
#include <stdint.h>

#include <Audioclient.h>
#include <Mmreg.h>

#ifdef TARGET_WINDOWS_DESKTOP
#  pragma comment(lib, "Avrt.lib")
#endif // TARGET_WINDOWS_DESKTOP

using namespace Microsoft::WRL;
using namespace std::chrono_literals;

namespace
{
constexpr auto minPcmPeriod{20ms};
constexpr auto minPassthroughPeriod{50ms};
} // namespace

const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioClock = __uuidof(IAudioClock);
DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);
DEFINE_PROPERTYKEY(PKEY_Device_EnumeratorName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 24);

#define EXIT_ON_FAILURE(hr, reason) \
  if (FAILED(hr)) \
  { \
    CLog::LogF(LOGERROR, reason " - error {}", hr, CWIN32Util::FormatHRESULT(hr)); \
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

CAESinkWASAPI::CAESinkWASAPI()
{
  m_channelLayout.Reset();

  // Get performance counter frequency for latency calculations
  QueryPerformanceFrequency(&m_timerFreq);
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
  HRESULT hr = S_FALSE;

  const bool bdefault = device.find("default") != std::string::npos;

  if (!bdefault)
  {
    hr = CAESinkFactoryWin::ActivateWASAPIDevice(device, &m_pDevice);
    EXIT_ON_FAILURE(hr, "Retrieval of WASAPI endpoint failed.")
  }

  if (!m_pDevice)
  {
    if (!bdefault)
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

  m_format = format;

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

void CAESinkWASAPI::GetDelay(AEDelayStatus& status)
{
  HRESULT hr;
  uint64_t pos;
  int retries = 0;

  if (!m_initialized)
    goto failed;

  do {
    hr = m_pAudioClock->GetPosition(&pos, NULL);
  } while (hr != S_OK && ++retries < 100);
  EXIT_ON_FAILURE(hr, "Retrieval of IAudioClock::GetPosition failed.")

  status.SetDelay((static_cast<double>(m_sinkFrames + m_bufferPtr) / m_format.m_sampleRate) -
                  (static_cast<double>(pos) / m_clockFreq));
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

  const unsigned int framesToCopy = std::min(m_format.m_frames - m_bufferPtr, frames);
  uint8_t* buffer = data[0] + offset * m_format.m_frameSize;

  // if there are older frames to write or the number of frames received does
  // not match the nominal, use the aux buffer to realign the frames
  if (m_bufferPtr != 0 || frames != m_format.m_frames)
  {
    memcpy(m_buffer.data() + m_bufferPtr * m_format.m_frameSize, buffer,
           framesToCopy * m_format.m_frameSize);
    m_bufferPtr += framesToCopy;
    if (m_bufferPtr != m_format.m_frames)
      return frames; // not enough frames to start write yet
  }

  // wait for Audio Driver to tell us it's got a buffer available
  if (m_running)
  {
    LARGE_INTEGER timerStart{};
    QueryPerformanceCounter(&timerStart);

    if (WaitForSingleObject(m_needDataEvent, 1100) != WAIT_OBJECT_0)
    {
      CLog::LogF(LOGERROR, "Endpoint Buffer timed out");
      m_isDirty = true;
      return INT_MAX;
    }

    LARGE_INTEGER timerStop{};
    QueryPerformanceCounter(&timerStop);
    const LONGLONG timerDiff = timerStop.QuadPart - timerStart.QuadPart;
    const double timerElapsed = static_cast<double>(timerDiff) * 1000.0 / m_timerFreq.QuadPart;
    m_avgTimeWaiting += (timerElapsed - m_avgTimeWaiting) * 0.5;

    if (m_avgTimeWaiting < 3.0)
    {
      CLog::LogF(LOGDEBUG, "Possible AQ Loss: Avg. Time Waiting for Audio Driver callback : {}msec",
                 static_cast<int>(m_avgTimeWaiting));
    }
  }

  // get buffer to write data
  BYTE* buf;
  HRESULT hr = m_pRenderClient->GetBuffer(m_format.m_frames, &buf);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "GetBuffer failed due to {}", CWIN32Util::FormatHRESULT(hr));
    m_isDirty = true;
    return INT_MAX;
  }

  // fill buffer
  memcpy(buf, m_bufferPtr == 0 ? buffer : m_buffer.data(),
         m_format.m_frames * m_format.m_frameSize);
  m_bufferPtr = 0;

  // pass back to the audio driver
  hr = m_pRenderClient->ReleaseBuffer(m_format.m_frames, 0);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "ReleaseBuffer failed due to {}.", CWIN32Util::FormatHRESULT(hr));
    m_isDirty = true;
    return INT_MAX;
  }

  m_sinkFrames += m_format.m_frames;

  // if not running start the audio driver
  if (!m_running)
  {
    hr = m_pAudioClient->Start();
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "AudioClient Start Failed");
      m_isDirty = true;
      return INT_MAX;
    }
    m_running = true;
  }

  // if not all received frames have been written, save the pending ones in the aux buffer
  if (framesToCopy != frames)
  {
    m_bufferPtr = frames - framesToCopy;
    memcpy(m_buffer.data(), buffer + framesToCopy * m_format.m_frameSize,
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
    deviceInfo.m_streamTypes.clear();
    deviceChannels.Reset();
    add192 = false;
    add48 = false;

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

      // 16 bits is most widely supported and likely to have the widest range of sample rates
      if (deviceInfo.m_dataFormats.empty() ||
          std::find(deviceInfo.m_dataFormats.cbegin(), deviceInfo.m_dataFormats.cend(),
                    AE_FMT_S16NE) != deviceInfo.m_dataFormats.cend())
      {
        wfxex.Format.wBitsPerSample = 16;
        wfxex.Samples.wValidBitsPerSample = 16;
      }
      else
      {
        const AEDataFormat fmt = deviceInfo.m_dataFormats.front();
        wfxex.Format.wBitsPerSample = CAEUtil::DataFormatToBits(fmt);
        wfxex.Samples.wValidBitsPerSample =
            (fmt == AE_FMT_S24NE4MSB ? 24 : wfxex.Format.wBitsPerSample);
      }

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
    CLog::LogF(LOGERROR, "Failed to enumerate WASAPI endpoint devices ({}).",
               CWIN32Util::FormatHRESULT(hr));
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
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.Format.nChannels = 2;
    wfxex.Format.nSamplesPerSec = 48000L;
    wfxex.Format.wBitsPerSample = 16;
    wfxex.Format.nBlockAlign = 4;
    wfxex.Samples.wValidBitsPerSample = 16;
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.Format.nAvgBytesPerSec = wfxex.Format.nBlockAlign * wfxex.Format.nSamplesPerSec;
    wfxex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
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
    CLog::LogF(LOGERROR, "IsFormatSupported failed ({})", CWIN32Util::FormatHRESULT(hr));
    return false;
  }
  else if (format.m_dataFormat == AE_FMT_RAW) //No sense in trying other formats for passthrough.
    return false;

  CLog::LogF(LOGWARNING,
             "format {} not supported by the device - trying to find a compatible format",
             CAEUtil::DataFormatToStr(format.m_dataFormat));

  requestedChannels = wfxex.Format.nChannels;
  desired_map = CAESinkFactoryWin::SpeakerMaskFromAEChannels(format.m_channelLayout);

  /* The requested format is not supported by the device.  Find something that works */
  CLog::LogF(LOGWARNING, "Input channels are [{}] - Trying to find a matching output layout",
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
          CLog::LogF(LOGERROR, "IsFormatSupported failed ({})", CWIN32Util::FormatHRESULT(hr));
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

  ComPtr<IAudioClient2> audioClient2;
  if (SUCCEEDED(m_pAudioClient.As(&audioClient2)))
  {
    AudioClientProperties props = {};
    props.cbSize = sizeof(props);
    // ForegroundOnlyMedia/BackgroundCapableMedia replaced in Windows 10 by Movie/Media
    props.eCategory = CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10)
                          ? AudioCategory_Media
                          : AudioCategory_ForegroundOnlyMedia;

    if (FAILED(hr = audioClient2->SetClientProperties(&props)))
      CLog::LogF(LOGERROR, "unable to set audio category, {}", CWIN32Util::FormatHRESULT(hr));
  }

  const bool isPassthrough = (format.m_dataFormat == AE_FMT_RAW);
  const auto targetPeriod = isPassthrough ? minPassthroughPeriod : minPcmPeriod;

  REFERENCE_TIME defaultDevicePeriodHns{};

  if (FAILED(hr = m_pAudioClient->GetDevicePeriod(&defaultDevicePeriodHns, nullptr)))
  {
    CLog::LogF(LOGERROR, "unable to retrieve the device's default period ({})",
               CWIN32Util::FormatHRESULT(hr));
    return false;
  }

  CLog::LogF(LOGDEBUG, "Default period: {:.1f}ms",
             static_cast<float>(defaultDevicePeriodHns) / 10000.0f);

  // Find the first multiple of the default device period larger or equal to the desired minimum
  // buffer duration
  // note: the default device period is meant for shared mode but offers a strong guarantee of
  // compatibility and we're not a pro audio app that requires the min latency possible.

  const auto devicePeriod = std::chrono::nanoseconds(defaultDevicePeriodHns * 100);
  const int multiplier =
      (targetPeriod / devicePeriod) + ((targetPeriod % devicePeriod == 0ns) ? 0 : 1);
  const auto period = multiplier * devicePeriod;

  if (isPassthrough)
    format.m_dataFormat = AE_FMT_S16NE;

  auto AudioClientInitialize = [this](REFERENCE_TIME periodHns, WAVEFORMATEX* format,
                                      bool needNewClient) -> HRESULT
  {
    if (needNewClient && m_pAudioClient != nullptr)
    {
      if (HRESULT hr = m_pDevice->Activate(m_pAudioClient.ReleaseAndGetAddressOf()); FAILED(hr))
      {
        CLog::LogF(LOGERROR, "Device Activation Failed : {}", CWIN32Util::FormatHRESULT(hr));
        return hr;
      }
    }

    const AUDCLNT_SHAREMODE mode = AUDCLNT_SHAREMODE_EXCLUSIVE;
    const DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST;
    return m_pAudioClient->Initialize(mode, flags, periodHns, periodHns, format, NULL);
  };

  REFERENCE_TIME bufferHns{period.count() / 100};
  bool needNewClient = false;

  do
  {
    hr = AudioClientInitialize(bufferHns, &wfxex.Format, needNewClient);

    if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
    {
      // WASAPI requires aligned buffer. Get the next aligned frame
      UINT32 numBufferFrames;
      if (FAILED(hr = m_pAudioClient->GetBufferSize(&numBufferFrames)))
      {
        CLog::LogF(LOGERROR, "GetBufferSize Failed : {}", CWIN32Util::FormatHRESULT(hr));
        return false;
      }

      bufferHns = static_cast<REFERENCE_TIME>(
          (10000.0 * 1000 / wfxex.Format.nSamplesPerSec * numBufferFrames) + 0.5);
      needNewClient = true;
    }
    else if (hr == AUDCLNT_E_BUFFER_SIZE_ERROR || hr == AUDCLNT_E_INVALID_DEVICE_PERIOD ||
             hr == E_OUTOFMEMORY)
    {
      // Requested buffer was too large, try again with a duration reduced by the min device period
      bufferHns -= defaultDevicePeriodHns;
      needNewClient = false;
    }
    else
    {
      // Success or HRESULT without dedicated handling
      break;
    }
  } while (bufferHns >= defaultDevicePeriodHns);

  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Failed to initialize WASAPI in exclusive mode - {}.",
               CWIN32Util::FormatHRESULT(hr));
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
    CLog::Log(LOGDEBUG, "  Periodicity (ms): {:.1f}", static_cast<float>(bufferHns) / 10000.0f);
    return false;
  }

  // Latency of WASAPI buffers in event-driven mode is equal to the returned value
  // of GetStreamLatency converted from 100ns intervals to seconds then multiplied
  // by two as there are two equally-sized buffers and playback starts when the
  // second buffer is filled.
  // m_sinkLatency should match with nominal delay when all is stabilized:
  // e.g: if period is 20ms, delay is 40 ms and latency also 40 ms
  REFERENCE_TIME hnsLatency{};
  hr = m_pAudioClient->GetStreamLatency(&hnsLatency);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "GetStreamLatency Failed : {}", CWIN32Util::FormatHRESULT(hr));
    return false;
  }

  m_sinkLatency = static_cast<double>(hnsLatency * 2) / 10000000; // 100ns intervals to s

  // Get the buffer size and calculate the frames for AE
  UINT32 numBufferFrames{0};
  hr = m_pAudioClient->GetBufferSize(&numBufferFrames);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "GetBufferSize Failed : {}", CWIN32Util::FormatHRESULT(hr));
    return false;
  }

  format.m_frames = numBufferFrames;

  CLog::LogF(LOGINFO, "WASAPI Exclusive Mode Sink Initialized using: {}, {}, {}",
             CAEUtil::DataFormatToStr(format.m_dataFormat), wfxex.Format.nSamplesPerSec,
             wfxex.Format.nChannels);

  CLog::LogF(LOGDEBUG, "WASAPI Exclusive Mode Sink Initialized with the following parameters:");
  CLog::Log(LOGDEBUG, "  Audio Device    : {}", m_pDevice->deviceId);
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
  CLog::Log(LOGDEBUG, "  Channel Mask    : {}", wfxex.dwChannelMask);
  CLog::Log(LOGDEBUG, "  Frames          : {}", format.m_frames);
  CLog::Log(LOGDEBUG, "  Frame Size      : {}", format.m_frameSize);
  CLog::Log(LOGDEBUG, "  Periodicity (ms): {:.1f}", static_cast<float>(bufferHns) / 10000.0f);
  CLog::Log(LOGDEBUG, "  Latency (s)     : {:.3f}", m_sinkLatency);

  return true;
}

void CAESinkWASAPI::Drain()
{
  if(!m_pAudioClient)
    return;

  WriteLastBuffer();

  // Wait for last data in buffer to play before stopping (current buffer + WriteLastBuffer).
  // If WriteLastBuffer() fails, it doesn't matter wait for the time of two buffers anyway.
  KODI::TIME::Sleep(std::chrono::milliseconds(static_cast<int>(m_sinkLatency * 1000)));

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

void CAESinkWASAPI::WriteLastBuffer()
{
  if (!m_initialized || !m_running)
    return;

  // complete pending m_buffer with silence or generate one new m_buffer of silence
  uint8_t* buffer = m_buffer.data();
  unsigned int frames = m_format.m_frames;
  if (m_bufferPtr != 0)
  {
    frames = m_format.m_frames - m_bufferPtr;
    buffer += m_bufferPtr * m_format.m_frameSize;
  }
  CAEUtil::GenerateSilence(m_format.m_dataFormat, m_format.m_frameSize, buffer, frames);

  LARGE_INTEGER timerStart{};
  QueryPerformanceCounter(&timerStart);

  // wait for buffer available
  if (WaitForSingleObject(m_needDataEvent, 1100) != WAIT_OBJECT_0)
  {
    CLog::LogF(LOGERROR, "Endpoint Buffer timed out");
    return;
  }

  LARGE_INTEGER timerStop{};
  QueryPerformanceCounter(&timerStop);
  const LONGLONG timerDiff = timerStop.QuadPart - timerStart.QuadPart;
  const double timerElapsed = static_cast<double>(timerDiff) * 1000.0 / m_timerFreq.QuadPart;

  if (timerElapsed < 3.0)
  {
    CLog::LogF(LOGWARNING, "Dropped last buffer data because there is a possible buffer underrun");
    return;
  }

  // get buffer to write data
  BYTE* buf;
  HRESULT hr = m_pRenderClient->GetBuffer(m_format.m_frames, &buf);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "GetBuffer failed due to {}", CWIN32Util::FormatHRESULT(hr));
    return;
  }

  // fill buffer
  memcpy(buf, m_buffer.data(), m_format.m_frames * m_format.m_frameSize);

  // pass back to the audio driver
  hr = m_pRenderClient->ReleaseBuffer(m_format.m_frames, 0);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "ReleaseBuffer failed due to {}.", CWIN32Util::FormatHRESULT(hr));
    return;
  }

  m_sinkFrames += m_format.m_frames;
  m_bufferPtr = 0;
}
