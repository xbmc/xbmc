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
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"

#include <algorithm>
#include <stdint.h>

#include <ksmedia.h>
#include <mfapi.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL;

extern const char *WASAPIErrToStr(HRESULT err);

#define EXIT_ON_FAILURE(hr, reason, ...) \
  if (FAILED(hr)) \
  { \
    CLog::Log(LOGERROR, reason " - {}", __VA_ARGS__, WASAPIErrToStr(hr)); \
    goto failed; \
  }
#define XAUDIO_BUFFERS_IN_QUEUE 2

template <class TVoice>
inline void SafeDestroyVoice(TVoice **ppVoice)
{
  if (*ppVoice)
  {
    (*ppVoice)->DestroyVoice();
    *ppVoice = nullptr;
  }
}

///  ----------------- CAESinkXAudio ------------------------

CAESinkXAudio::CAESinkXAudio() :
  m_masterVoice(nullptr),
  m_sourceVoice(nullptr),
  m_encodedChannels(0),
  m_encodedSampleRate(0),
  sinkReqFormat(AE_FMT_INVALID),
  sinkRetFormat(AE_FMT_INVALID),
  m_AvgBytesPerSec(0),
  m_dwChunkSize(0),
  m_dwFrameSize(0),
  m_dwBufferLen(0),
  m_sinkFrames(0),
  m_framesInBuffers(0),
  m_running(false),
  m_initialized(false),
  m_isSuspended(false),
  m_isDirty(false),
  m_uiBufferLen(0),
  m_avgTimeWaiting(50)
{
  m_channelLayout.Reset();

  HRESULT hr = XAudio2Create(m_xAudio2.ReleaseAndGetAddressOf(), 0);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "XAudio initialization failed.");
  }
#ifdef  _DEBUG
  else
  {
    XAUDIO2_DEBUG_CONFIGURATION config = {};
    config.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_STREAMING;
    config.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_STREAMING;
    config.LogTiming = true;
    config.LogFunctionName = true;
    m_xAudio2->SetDebugConfiguration(&config, 0);
  }
#endif //  _DEBUG
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
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

bool CAESinkXAudio::Initialize(AEAudioFormat &format, std::string &device)
{
  if (m_initialized)
    return false;

  m_device = device;
  bool bdefault = false;
  HRESULT hr = S_OK;

  /* Save requested format */
  /* Clear returned format */
  sinkReqFormat = format.m_dataFormat;
  sinkRetFormat = AE_FMT_INVALID;

  if (!InitializeInternal(device, format))
  {
    CLog::Log(LOGINFO, __FUNCTION__": Could not Initialize voices with that format");
    goto failed;
  }

  format.m_frames       = m_uiBufferLen;
  m_format              = format;
  sinkRetFormat         = format.m_dataFormat;

  m_initialized = true;
  m_isDirty     = false;

  return true;

failed:
  CLog::Log(LOGERROR, __FUNCTION__": XAudio initialization failed.");
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
      m_sinkFrames = 0;
      m_framesInBuffers = 0;
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "{}: Invalidated source voice - Releasing", __FUNCTION__);
    }
  }
  m_running = false;

  SafeDestroyVoice(&m_sourceVoice);
  SafeDestroyVoice(&m_masterVoice);

  m_initialized = false;
}

/**
 * @brief rescale uint64_t without overflowing on large values
 */
static uint64_t rescale_u64(uint64_t val, uint64_t num, uint64_t den)
{
  return ((val / den) * num) + (((val % den) * num) / den);
}

void CAESinkXAudio::GetDelay(AEDelayStatus& status)
{
  HRESULT hr = S_OK;
  uint64_t pos = 0, tick = 0;
  int retries = 0;

  if (!m_initialized)
  {
    status.SetDelay(0.0);
    return;
  }

  XAUDIO2_VOICE_STATE state;
  m_sourceVoice->GetState(&state, 0);

  double delay = (double)(m_sinkFrames - state.SamplesPlayed) / m_format.m_sampleRate;
  status.SetDelay(delay);
  return;
}

double CAESinkXAudio::GetCacheTotal()
{
  if (!m_initialized)
    return 0.0;

  return XAUDIO_BUFFERS_IN_QUEUE * m_format.m_frames / (double)m_format.m_sampleRate;
}

double CAESinkXAudio::GetLatency()
{
  if (!m_initialized || !m_xAudio2)
    return 0.0;

  XAUDIO2_PERFORMANCE_DATA perfData;
  m_xAudio2->GetPerformanceData(&perfData);

  return perfData.CurrentLatencyInSamples / (double) m_format.m_sampleRate;
}

unsigned int CAESinkXAudio::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_initialized)
    return 0;

  HRESULT hr = S_OK;
  DWORD flags = 0;

#ifndef _DEBUG
  LARGE_INTEGER timerStart;
  LARGE_INTEGER timerStop;
  LARGE_INTEGER timerFreq;
#endif
  size_t dataLenght = frames * m_format.m_frameSize;

  struct buffer_ctx *ctx = new buffer_ctx;
  ctx->data = new uint8_t[dataLenght];
  ctx->frames = frames;
  ctx->sink = this;
  memcpy(ctx->data, data[0] + offset * m_format.m_frameSize, dataLenght);

  XAUDIO2_BUFFER xbuffer = {};
  xbuffer.AudioBytes = dataLenght;
  xbuffer.pAudioData = ctx->data;
  xbuffer.pContext = ctx;

  if (!m_running) //first time called, pre-fill buffer then start voice
  {
    m_sourceVoice->Stop();
    hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "voice submit buffer failed due to {}", WASAPIErrToStr(hr));
      delete ctx;
      return 0;
    }
    hr = m_sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "voice start failed due to {}", WASAPIErrToStr(hr));
      m_isDirty = true; //flag new device or re-init needed
      delete ctx;
      return INT_MAX;
    }
    m_sinkFrames += frames;
    m_framesInBuffers += frames;
    m_running = true; //signal that we're processing frames
    return frames;
  }

#ifndef _DEBUG
  /* Get clock time for latency checks */
  QueryPerformanceFrequency(&timerFreq);
  QueryPerformanceCounter(&timerStart);
#endif

  /* Wait for Audio Driver to tell us it's got a buffer available */
  //XAUDIO2_VOICE_STATE state;
  //while (m_sourceVoice->GetState(&state), state.BuffersQueued >= XAUDIO_BUFFERS_IN_QUEUE)
  while (m_format.m_frames * XAUDIO_BUFFERS_IN_QUEUE <= m_framesInBuffers.load())
  {
    DWORD eventAudioCallback;
    eventAudioCallback = WaitForSingleObjectEx(m_voiceCallback.mBufferEnd.get(), 1100, TRUE);
    if (eventAudioCallback != WAIT_OBJECT_0)
    {
      CLog::LogF(LOGERROR, "voice buffer timed out");
      delete ctx;
      return INT_MAX;
    }
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

  hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);
  if (FAILED(hr))
  {
    #ifdef _DEBUG
    CLog::LogF(LOGERROR, "submiting buffer failed due to {}", WASAPIErrToStr(hr));
#endif
    delete ctx;
    return INT_MAX;
  }

  m_sinkFrames += frames;
  m_framesInBuffers += frames;

  return frames;
}

void CAESinkXAudio::EnumerateDevicesEx(AEDeviceInfoList &deviceInfoList, bool force)
{
  HRESULT hr = S_OK, hr2 = S_OK;
  CAEDeviceInfo deviceInfo;
  CAEChannelInfo deviceChannels;
  WAVEFORMATEXTENSIBLE wfxex = {};
  bool add192 = false;

  UINT32 eflags = 0;// XAUDIO2_DEBUG_ENGINE;

  IXAudio2MasteringVoice* mMasterVoice = nullptr;
  IXAudio2SourceVoice* mSourceVoice = nullptr;
  Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
  hr = XAudio2Create(xaudio2.ReleaseAndGetAddressOf(), eflags);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Failed to activate XAudio for capability testing.");
    goto failed;
  }

  for(RendererDetail& details : CAESinkFactoryWin::GetRendererDetails())
  {
    deviceInfo.m_channels.Reset();
    deviceInfo.m_dataFormats.clear();
    deviceInfo.m_sampleRates.clear();

    std::wstring deviceId = KODI::PLATFORM::WINDOWS::ToW(details.strDeviceId);

    /* Test format DTS-HD-MA */
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.Format.nSamplesPerSec = 192000;
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
    wfxex.Format.wBitsPerSample = 16;
    wfxex.Samples.wValidBitsPerSample = 16;
    wfxex.Format.nChannels = 8;
    wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
    wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

    hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                        0, deviceId.c_str(), nullptr, AudioCategory_Media);
    hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);

    if (FAILED(hr))
    {
      CLog::Log(
          LOGINFO, __FUNCTION__ ": stream type \"{}\" on device \"{}\" seems to be not supported.",
          CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_DTSHD_MA), details.strDescription);
    }
    else
    {
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_MA);
      add192 = true;
    }
    SafeDestroyVoice(&mSourceVoice);

    /* Test format DTS-HD-HR */
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.Format.nSamplesPerSec = 192000;
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
    wfxex.Format.wBitsPerSample = 16;
    wfxex.Samples.wValidBitsPerSample = 16;
    wfxex.Format.nChannels = 2;
    wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
    wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

    hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                        0, deviceId.c_str(), nullptr, AudioCategory_Media);
    hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);

    if (FAILED(hr))
    {
      CLog::Log(LOGINFO,
                __FUNCTION__ ": stream type \"{}\" on device \"{}\" seems to be not supported.",
                CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_DTSHD), details.strDescription);
    }
    else
    {
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
      add192 = true;
    }
    SafeDestroyVoice(&mSourceVoice);

    /* Test format Dolby TrueHD */
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
    wfxex.Format.nChannels = 8;
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;

    hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);
    if (FAILED(hr))
    {
      CLog::Log(
          LOGINFO, __FUNCTION__ ": stream type \"{}\" on device \"{}\" seems to be not supported.",
          CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_TRUEHD), details.strDescription);
    }
    else
    {
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_TRUEHD);
      add192 = true;
    }

    /* Test format Dolby EAC3 */
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
    wfxex.Format.nChannels = 2;
    wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
    wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

    SafeDestroyVoice(&mSourceVoice);
    SafeDestroyVoice(&mMasterVoice);
    hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                        0, deviceId.c_str(), nullptr, AudioCategory_Media);
    hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);

    if (FAILED(hr))
    {
      CLog::Log(LOGINFO,
                __FUNCTION__ ": stream type \"{}\" on device \"{}\" seems to be not supported.",
                CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_EAC3), details.strDescription);
    }
    else
    {
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
      add192 = true;
    }

    /* Test format DTS */
    wfxex.Format.nSamplesPerSec = 48000;
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS;
    wfxex.Format.nBlockAlign = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
    wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

    SafeDestroyVoice(&mSourceVoice);
    SafeDestroyVoice(&mMasterVoice);
    hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                        0, deviceId.c_str(), nullptr, AudioCategory_Media);
    hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);
    if (FAILED(hr))
    {
      CLog::Log(LOGINFO,
                __FUNCTION__ ": stream type \"{}\" on device \"{}\" seems to be not supported.",
                "STREAM_TYPE_DTS", details.strDescription);
    }
    else
    {
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
    }
    SafeDestroyVoice(&mSourceVoice);

    /* Test format Dolby AC3 */
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL;

    hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);
    if (FAILED(hr))
    {
      CLog::Log(LOGINFO,
                __FUNCTION__ ": stream type \"{}\" on device \"{}\" seems to be not supported.",
                CAEUtil::StreamTypeToStr(CAEStreamInfo::STREAM_TYPE_AC3), details.strDescription);
    }
    else
    {
      deviceInfo.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
    }

    /* Test format for PCM format iteration */
    wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfxex.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxex.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

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
      SafeDestroyVoice(&mSourceVoice);
      SafeDestroyVoice(&mMasterVoice);

      wfxex.Format.nSamplesPerSec = WASAPISampleRates[j];
      wfxex.Format.nAvgBytesPerSec = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

      hr2 = xaudio2->CreateMasteringVoice(&mMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                          0, deviceId.c_str(), nullptr, AudioCategory_Media);
      hr = xaudio2->CreateSourceVoice(&mSourceVoice, &wfxex.Format);
      if (SUCCEEDED(hr))
        deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
      else if (wfxex.Format.nSamplesPerSec == 192000 && add192)
      {
        deviceInfo.m_sampleRates.push_back(WASAPISampleRates[j]);
        CLog::Log(LOGINFO,
                  __FUNCTION__ ": sample rate 192khz on device \"{}\" seems to be not supported.",
                  details.strDescription);
      }
    }
    SafeDestroyVoice(&mSourceVoice);
    SafeDestroyVoice(&mMasterVoice);

    deviceInfo.m_deviceName = details.strDeviceId;
    deviceInfo.m_displayName = details.strWinDevType.append(details.strDescription);
    deviceInfo.m_displayNameExtra = std::string("XAudio: ").append(details.strDescription);
    deviceInfo.m_deviceType = details.eDeviceType;
    deviceInfo.m_channels = layoutsByChCount[details.nChannels];

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

failed:

  if (FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__ ": Failed to enumerate XAudio endpoint devices ({}).",
              WASAPIErrToStr(hr));
}

/// ------------------- Private utility functions -----------------------------------

bool CAESinkXAudio::InitializeInternal(std::string deviceId, AEAudioFormat &format)
{
  std::wstring device = KODI::PLATFORM::WINDOWS::ToW(deviceId);
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

  bool bdefault = StringUtils::EndsWithNoCase(deviceId, std::string("default"));

  HRESULT hr;
  IXAudio2MasteringVoice* pMasterVoice = nullptr;

  if (!bdefault)
  {
    hr = m_xAudio2->CreateMasteringVoice(&pMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                          0, device.c_str(), nullptr, AudioCategory_Media);
  }

  if (!pMasterVoice)
  {
    if (!bdefault)
    {
      CLog::Log(LOGINFO,
                __FUNCTION__ ": Could not locate the device named \"{}\" in the list of Xaudio "
                             "endpoint devices. Trying the default device...",
                KODI::PLATFORM::WINDOWS::FromW(device));
    }

    // smartphone issue: providing device ID (even default ID) causes E_NOINTERFACE result
    // workaround: device = nullptr will initialize default audio endpoint
    hr = m_xAudio2->CreateMasteringVoice(&pMasterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                          0, 0, nullptr, AudioCategory_Media);
    if (FAILED(hr) || !pMasterVoice)
    {
      CLog::Log(LOGINFO,
                __FUNCTION__ ": Could not retrieve the default XAudio audio endpoint ({}).",
                WASAPIErrToStr(hr));
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
    CLog::Log(LOGINFO, __FUNCTION__": Format is Supported - will attempt to Initialize");
    goto initialize;
  }

  if (format.m_dataFormat == AE_FMT_RAW) //No sense in trying other formats for passthrough.
    return false;

  if (CServiceBroker::GetLogging().CanLogComponent(LOGAUDIO))
    CLog::Log(LOGDEBUG,
              __FUNCTION__ ": CreateSourceVoice failed ({}) - trying to find a compatible format",
              WASAPIErrToStr(hr));

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
        wfxex.Format.nSamplesPerSec    = WASAPISampleRates[i];
        wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

        hr = m_xAudio2->CreateMasteringVoice(&m_masterVoice, wfxex.Format.nChannels, wfxex.Format.nSamplesPerSec,
                                             0, device.c_str(), nullptr, AudioCategory_Media);
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
          CLog::Log(LOGERROR, __FUNCTION__ ": creating voices failed ({})", WASAPIErrToStr(hr));
      }

      if (closestMatch >= 0)
      {
        wfxex.Format.nSamplesPerSec    = WASAPISampleRates[closestMatch];
        wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;
        goto initialize;
      }
    }
  }

  CLog::Log(LOGERROR, __FUNCTION__": Unable to locate a supported output format for the device.  Check the speaker settings in the control panel.");

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
    CLog::Log(LOGERROR, __FUNCTION__ ": Voice start failed : {}", WASAPIErrToStr(hr));
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
    CLog::Log(LOGERROR, __FUNCTION__ ": GetPerformanceData Failed : {}", WASAPIErrToStr(hr));
    return false;
  }

  m_uiBufferLen = (int)(format.m_sampleRate * 0.02);
  m_dwFrameSize = wfxex.Format.nBlockAlign;
  m_dwChunkSize = m_dwFrameSize * m_uiBufferLen;
  m_dwBufferLen = m_dwChunkSize * 4;
  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  CLog::Log(LOGINFO, __FUNCTION__ ": XAudio Sink Initialized using: {}, {}, {}",
            CAEUtil::DataFormatToStr(format.m_dataFormat), wfxex.Format.nSamplesPerSec,
            wfxex.Format.nChannels);

  m_sourceVoice->Stop();

  return true;
}

void CAESinkXAudio::Drain()
{
  if(!m_sourceVoice)
    return;

  AEDelayStatus status;
  GetDelay(status);

  KODI::TIME::Sleep(std::chrono::milliseconds(static_cast<int>(status.GetDelay() * 500)));

  if (m_running)
  {
    try
    {
      m_sourceVoice->Stop();
      m_sourceVoice->FlushSourceBuffers();
      m_sinkFrames = 0;
      m_framesInBuffers = 0;
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "{}: Invalidated source voice - Releasing", __FUNCTION__);
    }
  }
  m_running = false;
}

bool CAESinkXAudio::IsUSBDevice()
{
#if 0 // TODO
  IPropertyStore *pProperty = nullptr;
  PROPVARIANT varName;
  PropVariantInit(&varName);
  bool ret = false;

  HRESULT hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
  if (!SUCCEEDED(hr))
    return ret;
  hr = pProperty->GetValue(PKEY_Device_EnumeratorName, &varName);

  std::string str = localWideToUtf(varName.pwszVal);
  StringUtils::ToUpper(str);
  ret = (str == "USB");
  PropVariantClear(&varName);
  if (pProperty)
    pProperty->Release();
#endif
  return false;
}
