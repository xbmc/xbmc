/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkXAudio.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/windows/AESinkFactoryWin.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"

#include <algorithm>
#include <stdint.h>

#include <ksmedia.h>

using namespace Microsoft::WRL;

namespace
{
constexpr int XAUDIO_BUFFERS_IN_QUEUE = 2;

HRESULT KXAudio2Create(IXAudio2** ppXAudio2,
                       UINT32 Flags X2DEFAULT(0),
                       XAUDIO2_PROCESSOR XAudio2Processor X2DEFAULT(XAUDIO2_DEFAULT_PROCESSOR))
{
  typedef HRESULT(__stdcall * XAudio2CreateInfoFunc)(_Outptr_ IXAudio2**, UINT32,
                                                     XAUDIO2_PROCESSOR);
  static HMODULE dll = NULL;
  static XAudio2CreateInfoFunc XAudio2CreateFn = nullptr;

  if (dll == NULL)
  {
    dll = LoadLibraryEx(L"xaudio2_9redist.dll", NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    if (dll == NULL)
    {
      dll = LoadLibraryEx(L"xaudio2_9.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

      if (dll == NULL)
      {
        // Windows 8 compatibility
        dll = LoadLibraryEx(L"xaudio2_8.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (dll == NULL)
          return HRESULT_FROM_WIN32(GetLastError());
      }
    }

    XAudio2CreateFn = (XAudio2CreateInfoFunc)(void*)GetProcAddress(dll, "XAudio2Create");
    if (!XAudio2CreateFn)
    {
      return HRESULT_FROM_WIN32(GetLastError());
    }
  }

  if (XAudio2CreateFn)
    return (*XAudio2CreateFn)(ppXAudio2, Flags, XAudio2Processor);
  else
    return E_FAIL;
}

} // namespace

template <class TVoice>
inline void SafeDestroyVoice(TVoice **ppVoice)
{
  if (*ppVoice)
  {
    (*ppVoice)->DestroyVoice();
    *ppVoice = nullptr;
  }
}

CAESinkXAudio::CAESinkXAudio()
{
  HRESULT hr = KXAudio2Create(m_xAudio2.ReleaseAndGetAddressOf(), 0);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "XAudio initialization failed, error {:X}.", hr);
  }
#ifdef _DEBUG
  else
  {
    XAUDIO2_DEBUG_CONFIGURATION config = {};
    config.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_STREAMING;
    config.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_STREAMING;
    config.LogTiming = true;
    config.LogFunctionName = true;
    m_xAudio2->SetDebugConfiguration(&config, 0);
  }
#endif // _DEBUG

  // Get performance counter frequency for latency calculations
  QueryPerformanceFrequency(&m_timerFreq);
}

CAESinkXAudio::~CAESinkXAudio()
{
  if (m_xAudio2)
    m_xAudio2.Reset();
}

void CAESinkXAudio::Register()
{
  AE::AESinkRegEntry reg;
  reg.sinkName = "XAUDIO";
  reg.createFunc = CAESinkXAudio::Create;
  reg.enumerateFunc = CAESinkXAudio::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(reg);
}

std::unique_ptr<IAESink> CAESinkXAudio::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkXAudio>();

  if (!sink->m_xAudio2)
  {
    CLog::LogF(LOGERROR, "XAudio2 not loaded.");
    return {};
  }

  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

bool CAESinkXAudio::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  /* Save requested format */
  AEDataFormat reqFormat = format.m_dataFormat;

  if (!InitializeInternal(device, format))
  {
    CLog::LogF(LOGINFO, "could not Initialize voices with format {}",
               CAEUtil::DataFormatToStr(reqFormat));
    CLog::LogF(LOGERROR, "XAudio initialization failed");
    return false;
  }

  m_initialized = true;
  m_isDirty = false;

  return true;
}

void CAESinkXAudio::Deinitialize()
{
  if (!m_initialized && !m_isDirty)
    return;

  if (m_running)
  {
    try
    {
      m_sourceVoice->Stop();
      m_sourceVoice->FlushSourceBuffers();

      // Stop and FlushSourceBuffers are async, wait for queued buffers to be released by XAudio2.
      // callbacks don't seem to be called otherwise, with memory leakage.
      XAUDIO2_VOICE_STATE state{};
      do
      {
        if (WAIT_OBJECT_0 != WaitForSingleObject(m_voiceCallback.mBufferEnd.get(), 500))
        {
          CLog::LogF(LOGERROR, "timeout waiting for buffer flush - possible buffer memory leak");
          break;
        }
        m_sourceVoice->GetState(&state, 0);
      } while (state.BuffersQueued > 0);

      m_sinkFrames = 0;
      m_framesInBuffers = 0;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "invalidated source voice - Releasing");
    }
  }
  m_running = false;

  SafeDestroyVoice(&m_sourceVoice);
  SafeDestroyVoice(&m_masterVoice);

  m_initialized = false;
}

void CAESinkXAudio::GetDelay(AEDelayStatus& status)
{
  if (!m_initialized)
  {
    status.SetDelay(0.0);
    return;
  }

  XAUDIO2_VOICE_STATE state;
  m_sourceVoice->GetState(&state, 0);

  status.SetDelay(static_cast<double>(m_sinkFrames - state.SamplesPlayed) / m_format.m_sampleRate);
  return;
}

double CAESinkXAudio::GetCacheTotal()
{
  if (!m_initialized)
    return 0.0;

  return static_cast<double>(XAUDIO_BUFFERS_IN_QUEUE * m_format.m_frames) / m_format.m_sampleRate;
}

double CAESinkXAudio::GetLatency()
{
  if (!m_initialized || !m_xAudio2)
    return 0.0;

  XAUDIO2_PERFORMANCE_DATA perfData;
  m_xAudio2->GetPerformanceData(&perfData);

  return static_cast<double>(perfData.CurrentLatencyInSamples) / m_format.m_sampleRate;
}

unsigned int CAESinkXAudio::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_initialized)
    return 0;

  HRESULT hr = S_OK;

  LARGE_INTEGER timerStart;
  LARGE_INTEGER timerStop;

  XAUDIO2_BUFFER xbuffer = BuildXAudio2Buffer(data, frames, offset);

  if (!m_running) //first time called, pre-fill buffer then start voice
  {
    m_sourceVoice->Stop();
    hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "voice submit buffer failed due to {}", CWIN32Util::FormatHRESULT(hr));
      delete xbuffer.pContext;
      return 0;
    }
    hr = m_sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "voice start failed due to {}", CWIN32Util::FormatHRESULT(hr));
      m_isDirty = true; //flag new device or re-init needed
      delete xbuffer.pContext;
      return INT_MAX;
    }
    m_sinkFrames += frames;
    m_framesInBuffers += frames;
    m_running = true; //signal that we're processing frames
    return frames;
  }

  /* Get clock time for latency checks */
  QueryPerformanceCounter(&timerStart);

  /* Wait for Audio Driver to tell us it's got a buffer available */
  while (m_format.m_frames * XAUDIO_BUFFERS_IN_QUEUE <= m_framesInBuffers.load())
  {
    DWORD eventAudioCallback;
    eventAudioCallback = WaitForSingleObjectEx(m_voiceCallback.mBufferEnd.get(), 1100, TRUE);
    if (eventAudioCallback != WAIT_OBJECT_0)
    {
      CLog::LogF(LOGERROR, "voice buffer timed out");
      delete xbuffer.pContext;
      return INT_MAX;
    }
  }

  if (!m_running)
    return 0;

  QueryPerformanceCounter(&timerStop);
  const LONGLONG timerDiff = timerStop.QuadPart - timerStart.QuadPart;
  const double timerElapsed = static_cast<double>(timerDiff) * 1000.0 / m_timerFreq.QuadPart;
  m_avgTimeWaiting += (timerElapsed - m_avgTimeWaiting) * 0.5;

  if (m_avgTimeWaiting < 3.0)
  {
    CLog::LogF(LOGDEBUG, "Possible AQ Loss: Avg. Time Waiting for Audio Driver callback : {}msec",
               (int)m_avgTimeWaiting);
  }

  hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "submitting buffer failed due to {}", CWIN32Util::FormatHRESULT(hr));
    delete xbuffer.pContext;
    return INT_MAX;
  }

  m_sinkFrames += frames;
  m_framesInBuffers += frames;

  return frames;
}

void CAESinkXAudio::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  HRESULT hr = S_OK;
  CAEDeviceInfo deviceInfo;
  CAEChannelInfo deviceChannels;
  WAVEFORMATEXTENSIBLE wfxex = {};
  UINT32 eflags = 0; // XAUDIO2_DEBUG_ENGINE;
  IXAudio2MasteringVoice* mMasterVoice = nullptr;
  IXAudio2SourceVoice* mSourceVoice = nullptr;
  Microsoft::WRL::ComPtr<IXAudio2> xaudio2;

  // ForegroundOnlyMedia/BackgroundCapableMedia replaced in Windows 10 by Movie/Media
  const AUDIO_STREAM_CATEGORY streamCategory{
      CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10)
          ? AudioCategory_Media
          : AudioCategory_ForegroundOnlyMedia};

  hr = KXAudio2Create(xaudio2.ReleaseAndGetAddressOf(), eflags);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "failed to activate XAudio for capability testing ({})",
               CWIN32Util::FormatHRESULT(hr));
    return;
  }

  for (RendererDetail& details : CAESinkFactoryWin::GetRendererDetailsWinRT())
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

    const std::wstring deviceId = KODI::PLATFORM::WINDOWS::ToW(details.strDeviceId);

    /* Test format for PCM format iteration */
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.Format.nSamplesPerSec = 48000;
    wfxex.Format.nChannels = 2;
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

    hr = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels,
                                       wfxex.Format.nSamplesPerSec, 0, deviceId.c_str(), nullptr,
                                       streamCategory);

    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "failed to create mastering voice (:X)", hr);
      return;
    }

    for (int p = AE_FMT_FLOAT; p > AE_FMT_INVALID; p--)
    {
      if (p < AE_FMT_FLOAT)
        wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
      wfxex.Format.wBitsPerSample = CAEUtil::DataFormatToBits((AEDataFormat)p);
      wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
      wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
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

      SafeDestroyVoice(&mSourceVoice);
      hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);

      if (SUCCEEDED(hr))
        deviceInfo.m_dataFormats.push_back((AEDataFormat)p);
    }

    /* Test format for sample rate iteration */
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    wfxex.Format.wBitsPerSample = 16;
    wfxex.Samples.wValidBitsPerSample = 16;
    wfxex.Format.nChannels = 2;
    wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
    wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

    for (int j = 0; j < WASAPISampleRateCount; j++)
    {
      if (WASAPISampleRates[j] < XAUDIO2_MIN_SAMPLE_RATE ||
          WASAPISampleRates[j] > XAUDIO2_MAX_SAMPLE_RATE)
        continue;

      SafeDestroyVoice(&mSourceVoice);
      SafeDestroyVoice(&mMasterVoice);

      wfxex.Format.nSamplesPerSec = WASAPISampleRates[j];
      wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      if (SUCCEEDED(xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels,
                                                  wfxex.Format.nSamplesPerSec, 0, deviceId.c_str(),
                                                  nullptr, streamCategory)))
      {
        hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);

        if (SUCCEEDED(hr))
          deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
      }
    }

    SafeDestroyVoice(&mSourceVoice);
    SafeDestroyVoice(&mMasterVoice);

    deviceInfo.m_deviceName = details.strDeviceId;
    deviceInfo.m_displayName = details.strWinDevType.append(details.strDescription);
    deviceInfo.m_displayNameExtra = std::string("XAudio: ").append(details.strDescription);
    deviceInfo.m_deviceType = details.eDeviceType;
    deviceInfo.m_channels = deviceChannels;

    /* Store the device info */
    deviceInfo.m_wantsIECPassthrough = true;
    deviceInfo.m_onlyPCM = true;

    if (!deviceInfo.m_streamTypes.empty())
      deviceInfo.m_dataFormats.push_back(AE_FMT_RAW);

    deviceInfoList.push_back(deviceInfo);

    if (details.bDefault)
    {
      deviceInfo.m_deviceName = std::string("default");
      deviceInfo.m_displayName = std::string("default");
      deviceInfo.m_displayNameExtra = std::string("");
      deviceInfo.m_wantsIECPassthrough = true;
      deviceInfo.m_onlyPCM = true;
      deviceInfoList.push_back(deviceInfo);
    }
  }
}

bool CAESinkXAudio::InitializeInternal(std::string deviceId, AEAudioFormat &format)
{
  const std::wstring device = KODI::PLATFORM::WINDOWS::ToW(deviceId);
  WAVEFORMATEXTENSIBLE wfxex = {};

  if ( format.m_dataFormat <= AE_FMT_FLOAT
    || format.m_dataFormat == AE_FMT_RAW)
    CAESinkFactoryWin::BuildWaveFormatExtensible(format, wfxex);
  else
  {
    // planar formats are currently not supported by this sink
    format.m_dataFormat = AE_FMT_FLOAT;
    CAESinkFactoryWin::BuildWaveFormatExtensible(format, wfxex);
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

  const bool bdefault = deviceId.find("default") != std::string::npos;

  HRESULT hr;
  IXAudio2MasteringVoice* pMasterVoice = nullptr;
  const wchar_t* pDevice = device.c_str();
  // ForegroundOnlyMedia/BackgroundCapableMedia replaced in Windows 10 by Movie/Media
  const AUDIO_STREAM_CATEGORY streamCategory{
      CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10)
          ? AudioCategory_Media
          : AudioCategory_ForegroundOnlyMedia};

  if (!bdefault)
  {
    hr = m_xAudio2->CreateMasteringVoice(&pMasterVoice, wfxex.Format.nChannels,
                                         wfxex.Format.nSamplesPerSec, 0, pDevice, nullptr,
                                         streamCategory);
  }

  if (!pMasterVoice)
  {
    if (!bdefault)
    {
      CLog::LogF(LOGINFO,
                 "could not locate the device named \"{}\" in the list of Xaudio endpoint devices. "
                 "Trying the default device...",
                 KODI::PLATFORM::WINDOWS::FromW(device));
    }
    // smartphone issue: providing device ID (even default ID) causes E_NOINTERFACE result
    // workaround: device = nullptr will initialize default audio endpoint
    pDevice = nullptr;
    hr = m_xAudio2->CreateMasteringVoice(&pMasterVoice, wfxex.Format.nChannels,
                                         wfxex.Format.nSamplesPerSec, 0, pDevice, nullptr,
                                         streamCategory);
    if (FAILED(hr) || !pMasterVoice)
    {
      CLog::LogF(LOGINFO, "Could not retrieve the default XAudio audio endpoint ({}).",
                 CWIN32Util::FormatHRESULT(hr));
      return false;
    }
  }

  m_masterVoice = pMasterVoice;

  int closestMatch = 0;
  unsigned int requestedChannels = 0;
  unsigned int noOfCh = 0;

  hr = m_xAudio2->CreateSourceVoice(&m_sourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_voiceCallback);
  if (SUCCEEDED(hr))
  {
    CLog::LogF(LOGINFO, "Format is Supported - will attempt to Initialize");
    goto initialize;
  }

  if (format.m_dataFormat == AE_FMT_RAW) //No sense in trying other formats for passthrough.
    return false;

  if (CServiceBroker::GetLogging().CanLogComponent(LOGAUDIO))
    CLog::LogFC(LOGDEBUG, LOGAUDIO,
                "CreateSourceVoice failed ({}) - trying to find a compatible format",
                CWIN32Util::FormatHRESULT(hr));

  requestedChannels = wfxex.Format.nChannels;

  /* The requested format is not supported by the device.  Find something that works */
  for (int layout = -1; layout <= (int)ARRAYSIZE(layoutsList); layout++)
  {
    // if requested layout is not supported, try standard layouts with at least
    // the number of channels as requested
    // as the last resort try stereo
    if (layout == ARRAYSIZE(layoutsList))
    {
      wfxex.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
      wfxex.Format.nChannels = 2;
    }
    else if (layout >= 0)
    {
      wfxex.dwChannelMask = CAESinkFactoryWin::ChLayoutToChMask(layoutsList[layout], &noOfCh);
      wfxex.Format.nChannels = noOfCh;
      if (noOfCh < requestedChannels)
        continue;
    }

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
        if (WASAPISampleRates[j] < XAUDIO2_MIN_SAMPLE_RATE ||
            WASAPISampleRates[j] > XAUDIO2_MAX_SAMPLE_RATE)
          continue;

        SafeDestroyVoice(&m_sourceVoice);
        SafeDestroyVoice(&m_masterVoice);

        wfxex.Format.nSamplesPerSec    = WASAPISampleRates[i];
        wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

        hr = m_xAudio2->CreateMasteringVoice(&m_masterVoice, wfxex.Format.nChannels,
                                             wfxex.Format.nSamplesPerSec, 0, pDevice, nullptr,
                                             streamCategory);
        if (SUCCEEDED(hr))
        {
          hr = m_xAudio2->CreateSourceVoice(&m_sourceVoice, &wfxex.Format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_voiceCallback);
          if (SUCCEEDED(hr))
          {
            /* If the current sample rate matches the source then stop looking and use it */
            if ((WASAPISampleRates[i] == format.m_sampleRate) && (testFormats[j].subFormatType <= format.m_dataFormat))
              goto initialize;
            /* If this rate is closer to the source then the previous one, save it */
            else if (closestMatch < 0 || abs((int)WASAPISampleRates[i] - (int)format.m_sampleRate) < abs((int)WASAPISampleRates[closestMatch] - (int)format.m_sampleRate))
              closestMatch = i;
          }
        }

        if (FAILED(hr))
          CLog::LogF(LOGERROR, "creating voices failed ({})", CWIN32Util::FormatHRESULT(hr));
      }

      if (closestMatch >= 0)
      {
        // Closest match may be different from the last successful sample rate tested
        SafeDestroyVoice(&m_sourceVoice);
        SafeDestroyVoice(&m_masterVoice);

        wfxex.Format.nSamplesPerSec    = WASAPISampleRates[closestMatch];
        wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

        if (SUCCEEDED(m_xAudio2->CreateMasteringVoice(&m_masterVoice, wfxex.Format.nChannels,
                                                      wfxex.Format.nSamplesPerSec, 0, pDevice,
                                                      nullptr, streamCategory)) &&
            SUCCEEDED(m_xAudio2->CreateSourceVoice(&m_sourceVoice, &wfxex.Format, 0,
                                                   XAUDIO2_DEFAULT_FREQ_RATIO, &m_voiceCallback)))
          goto initialize;
      }
    }
  }

  CLog::LogF(LOGERROR, "unable to locate a supported output format for the device. Check the "
                       "speaker settings in the control panel.");

  /* We couldn't find anything supported. This should never happen      */
  /* unless the user set the wrong speaker setting in the control panel */
  return false;

initialize:

  CAEChannelInfo channelLayout;
  CAESinkFactoryWin::AEChannelsFromSpeakerMask(channelLayout, wfxex.dwChannelMask);
  format.m_channelLayout = channelLayout;

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

  if (format.m_dataFormat == AE_FMT_RAW)
    format.m_dataFormat = AE_FMT_S16NE;

  hr = m_sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Voice start failed : {}", CWIN32Util::FormatHRESULT(hr));
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
    return false;
  }

  XAUDIO2_PERFORMANCE_DATA perfData;
  m_xAudio2->GetPerformanceData(&perfData);
  if (!perfData.TotalSourceVoiceCount)
  {
    CLog::LogF(LOGERROR, "GetPerformanceData Failed : {}", CWIN32Util::FormatHRESULT(hr));
    return false;
  }

  format.m_frames = static_cast<int>(format.m_sampleRate * 0.02); // 20 ms chunks

  m_format = format;

  CLog::LogF(LOGINFO, "XAudio Sink Initialized using: {}, {}, {}",
             CAEUtil::DataFormatToStr(format.m_dataFormat), wfxex.Format.nSamplesPerSec,
             wfxex.Format.nChannels);

  m_sourceVoice->Stop();

  CLog::LogF(LOGDEBUG, "Initializing XAudio with the following parameters:");
  CLog::Log(LOGDEBUG, "  Audio Device    : {}", KODI::PLATFORM::WINDOWS::FromW(device));
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

  return true;
}

void CAESinkXAudio::Drain()
{
  if(!m_sourceVoice)
    return;

  if (m_running)
  {
    try
    {
      // Contrary to MS doc, the voice must play a buffer with end of stream flag for the voice
      // SamplesPlayed counter to be reset.
      // Per MS doc, Discontinuity() may not take effect until after the entire buffer queue is
      // consumed, which wouldn't invoke the callback or reset the voice stats.
      // Solution: submit a 1 sample buffer with end of stream flag and wait for StreamEnd callback
      // The StreamEnd event is manual reset so that it cannot be missed even if raised before this
      // code starts waiting for it

      AddEndOfStreamPacket();

      constexpr uint32_t waitSafety = 100; // extra ms wait in case of scheduler issue
      DWORD waitRc =
          WaitForSingleObject(m_voiceCallback.m_StreamEndEvent,
                              m_framesInBuffers * 1000 / m_format.m_sampleRate + waitSafety);

      if (waitRc != WAIT_OBJECT_0)
      {
        if (waitRc == WAIT_FAILED)
        {
          //! @todo use FormatMessage for a human readable error message
          CLog::LogF(LOGERROR,
                     "error WAIT_FAILED while waiting for queued buffers to drain. GetLastError:{}",
                     GetLastError());
        }
        else
        {
          CLog::LogF(LOGERROR, "error {} while waiting for queued buffers to drain.", waitRc);
        }
      }

      m_sourceVoice->Stop();
      ResetEvent(m_voiceCallback.m_StreamEndEvent);

      m_sinkFrames = 0;
      m_framesInBuffers = 0;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "invalidated source voice - Releasing");
    }
  }
  m_running = false;
}

bool CAESinkXAudio::AddEndOfStreamPacket()
{
  constexpr unsigned int frames{1};

  XAUDIO2_BUFFER xbuffer = BuildXAudio2Buffer(nullptr, frames, 0);
  xbuffer.Flags = XAUDIO2_END_OF_STREAM;

  HRESULT hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);

  if (hr != S_OK)
  {
    CLog::LogF(LOGERROR, "SubmitSourceBuffer failed due to {:X}", hr);
    delete xbuffer.pContext;
    return false;
  }

  m_sinkFrames += frames;
  m_framesInBuffers += frames;
  return true;
}

XAUDIO2_BUFFER CAESinkXAudio::BuildXAudio2Buffer(uint8_t** data,
                                                 unsigned int frames,
                                                 unsigned int offset)
{
  const unsigned int dataLength{frames * m_format.m_frameSize};

  struct buffer_ctx* ctx = new buffer_ctx;
  ctx->data = new uint8_t[dataLength];
  ctx->frames = frames;
  ctx->sink = this;

  if (data)
    memcpy(ctx->data, data[0] + offset * m_format.m_frameSize, dataLength);
  else
    CAEUtil::GenerateSilence(m_format.m_dataFormat, m_format.m_frameSize, ctx->data, frames);

  XAUDIO2_BUFFER xbuffer{};
  xbuffer.AudioBytes = dataLength;
  xbuffer.pAudioData = ctx->data;
  xbuffer.pContext = ctx;

  return xbuffer;
}
