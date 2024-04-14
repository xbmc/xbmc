/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkStarfish.h"

#include "CompileInfo.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "xbmc/cores/AudioEngine/AESinkFactory.h"

#include <thread>

using namespace std::chrono_literals;

namespace
{
constexpr unsigned int STARFISH_AUDIO_BUFFERS = 8;
constexpr unsigned int AC3_SYNCFRAME_SIZE = 2560;
constexpr unsigned int DTDHD_MA_MIN_SYNCFRAME_SIZE = 2012 + 2764;

static constexpr auto ms_audioCodecMap = make_map<CAEStreamInfo::DataType, std::string_view>({
    {CAEStreamInfo::STREAM_TYPE_AC3, "AC3"},
    {CAEStreamInfo::STREAM_TYPE_EAC3, "AC3 PLUS"},
    {CAEStreamInfo::STREAM_TYPE_DTS_512, "DTS"},
    {CAEStreamInfo::STREAM_TYPE_DTS_1024, "DTS"},
    {CAEStreamInfo::STREAM_TYPE_DTS_2048, "DTS"},
    {CAEStreamInfo::STREAM_TYPE_DTSHD, "DTS"},
    {CAEStreamInfo::STREAM_TYPE_DTSHD_CORE, "DTS"},
    {CAEStreamInfo::STREAM_TYPE_DTSHD_MA, "DTS"},
});

} // namespace

void CAESinkStarfish::Register()
{
  AE::AESinkRegEntry entry;
  entry.sinkName = "Starfish";
  entry.createFunc = CAESinkStarfish::Create;
  entry.enumerateFunc = CAESinkStarfish::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(entry);
}

std::unique_ptr<IAESink> CAESinkStarfish::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkStarfish>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

void CAESinkStarfish::EnumerateDevicesEx(AEDeviceInfoList& list, bool force)
{
  CAEDeviceInfo info;
  info.m_deviceName = "Starfish";
  info.m_displayName = "Starfish (Passthrough only)";
  info.m_channels = AE_CH_LAYOUT_2_0;
  info.m_wantsIECPassthrough = false;
  info.m_onlyPassthrough = true;

  // PCM disabled for now as the latency is just too high, needs more research
  // Thankfully, ALSA or PulseAudio do work as an alternative for PCM content
  info.m_dataFormats.emplace_back(AE_FMT_RAW);

  info.m_deviceType = AE_DEVTYPE_IEC958;
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_AC3);
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_EAC3);
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_DTSHD);
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
  info.m_streamTypes.emplace_back(CAEStreamInfo::STREAM_TYPE_DTSHD_MA);

  info.m_sampleRates.emplace_back(32000);
  info.m_sampleRates.emplace_back(44100);
  info.m_sampleRates.emplace_back(48000);

  list.emplace_back(info);
}

CAESinkStarfish::CAESinkStarfish() : m_starfishMediaAPI(std::make_unique<StarfishMediaAPIs>())
{
}

CAESinkStarfish::~CAESinkStarfish() = default;

bool CAESinkStarfish::Initialize(AEAudioFormat& format, std::string& device)
{
  m_format = format;
  m_pts = 0ns;

  if (m_format.m_dataFormat != AE_FMT_RAW)
  {
    CLog::LogF(LOGERROR, "CAESinkStarfish: Unsupported format PCM");
    return false;
  }
  m_format.m_frameSize = 1;

  CVariant payload;
  payload["isAudioOnly"] = true;
  payload["mediaTransportType"] = "BUFFERSTREAM";
  payload["option"]["appId"] = CCompileInfo::GetPackage();
  payload["option"]["needAudio"] = true;
  payload["option"]["queryPosition"] = true;
  payload["option"]["externalStreamingInfo"]["contents"]["esInfo"]["pauseAtDecodeTime"] = true;
  payload["option"]["externalStreamingInfo"]["contents"]["esInfo"]["seperatedPTS"] = true;
  payload["option"]["externalStreamingInfo"]["contents"]["esInfo"]["ptsToDecode"] = 0;
  payload["option"]["externalStreamingInfo"]["contents"]["format"] = "RAW";
  payload["option"]["transmission"]["contentsType"] = "LIVE"; // "LIVE", "WEBRTC"

  switch (m_format.m_streamInfo.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
    {
      if (!m_format.m_streamInfo.m_frameSize)
        m_format.m_streamInfo.m_frameSize = AC3_SYNCFRAME_SIZE;
      m_format.m_frames = m_format.m_streamInfo.m_frameSize;
      m_bufferSize = m_format.m_frames * STARFISH_AUDIO_BUFFERS;
      break;
    }
    case CAEStreamInfo::STREAM_TYPE_EAC3:
    {
      payload["option"]["externalStreamingInfo"]["contents"]["ac3PlusInfo"]["channels"] = 8;
      payload["option"]["externalStreamingInfo"]["contents"]["ac3PlusInfo"]["frequency"] =
          static_cast<double>(m_format.m_streamInfo.m_sampleRate) / 1000;

      if (!m_format.m_streamInfo.m_frameSize)
        m_format.m_streamInfo.m_frameSize = AC3_SYNCFRAME_SIZE;
      m_format.m_frames = m_format.m_streamInfo.m_frameSize;
      m_bufferSize = m_format.m_frames * STARFISH_AUDIO_BUFFERS;
      break;
    }
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
    {
      payload["option"]["externalStreamingInfo"]["contents"]["dtsInfo"]["channels"] =
          m_format.m_streamInfo.m_channels;
      payload["option"]["externalStreamingInfo"]["contents"]["dtsInfo"]["frequency"] =
          static_cast<double>(m_format.m_streamInfo.m_sampleRate) / 1000;

      m_format.m_frames = m_format.m_streamInfo.m_frameSize;

      // DTSHD_MA has dynamic frame sizes but we need to ensure that the buffer is large enough
      if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_MA)
        m_format.m_frames = DTDHD_MA_MIN_SYNCFRAME_SIZE * 2 - 1;
      m_bufferSize = m_format.m_frames * STARFISH_AUDIO_BUFFERS;
      break;
    }
    default:
      CLog::LogF(LOGDEBUG, "CAESinkStarfish: Unsupported format {}", m_format.m_streamInfo.m_type);
      return false;
  }
  payload["option"]["externalStreamingInfo"]["contents"]["codec"]["audio"] =
      ms_audioCodecMap.at(m_format.m_streamInfo.m_type).data();

  payload["option"]["externalStreamingInfo"]["bufferingCtrInfo"]["preBufferByte"] = 0;
  payload["option"]["externalStreamingInfo"]["bufferingCtrInfo"]["bufferMinLevel"] = 0;
  payload["option"]["externalStreamingInfo"]["bufferingCtrInfo"]["bufferMaxLevel"] = 100;
  // This is the size after which the sink starts blocking
  payload["option"]["externalStreamingInfo"]["bufferingCtrInfo"]["qBufferLevelAudio"] =
      m_bufferSize;
  // Internal buffer?
  payload["option"]["externalStreamingInfo"]["bufferingCtrInfo"]["srcBufferLevelAudio"]["minimum"] =
      m_format.m_frames;
  payload["option"]["externalStreamingInfo"]["bufferingCtrInfo"]["srcBufferLevelAudio"]["maximum"] =
      m_bufferSize;

  CVariant payloadArgs;
  payloadArgs["args"] = CVariant(CVariant::VariantTypeArray);
  payloadArgs["args"].push_back(std::move(payload));

  std::string json;
  CJSONVariantWriter::Write(payloadArgs, json, true);

  m_starfishMediaAPI->notifyForeground();
  CLog::LogFC(LOGDEBUG, LOGAUDIO, "CAESinkStarfish: Sending Load payload {}", json);
  if (!m_starfishMediaAPI->Load(json.c_str(), &CAESinkStarfish::PlayerCallback, this))
  {
    CLog::LogF(LOGERROR, "CAESinkStarfish: Load failed");
    return false;
  }

  format = m_format;

  return true;
}

void CAESinkStarfish::Deinitialize()
{
  m_starfishMediaAPI->Unload();
}

double CAESinkStarfish::GetCacheTotal()
{
  if (m_format.m_dataFormat == AE_FMT_RAW)
  {
    auto frameTimeSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::duration<double, std::milli>(m_format.m_streamInfo.GetDuration()));
    return STARFISH_AUDIO_BUFFERS * frameTimeSeconds.count() * 4;
  }
  else
    return 0.0;
}

double CAESinkStarfish::GetLatency()
{
  return 0.0;
}

unsigned int CAESinkStarfish::AddPackets(uint8_t** data, unsigned int frames, unsigned int offset)
{
  auto frameTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::milli>(m_format.m_streamInfo.GetDuration()));

  std::chrono::nanoseconds pts = 0ns;
  if (!m_firstFeed && offset == 0)
    pts += m_pts + frameTime;

  CVariant payload;
  uint8_t* buffer = data[0] + offset * m_format.m_frameSize;
  payload["bufferAddr"] = fmt::format("{:#x}", reinterpret_cast<std::uintptr_t>(buffer));
  payload["bufferSize"] = frames * m_format.m_frameSize;
  payload["pts"] = pts.count();
  payload["esData"] = 2;

  std::string json;
  CJSONVariantWriter::Write(payload, json, true);

  std::string result = m_starfishMediaAPI->Feed(json.c_str());
  while (result.find("BufferFull") != std::string::npos)
  {
    std::this_thread::sleep_for(std::chrono::nanoseconds(frameTime));
    result = m_starfishMediaAPI->Feed(json.c_str());
  }

  if (result.find("Ok") != std::string::npos)
  {
    m_pts = pts;
    m_firstFeed = false;
    return frames;
  }

  CLog::LogF(LOGWARNING, "CAESinkStarfish: Buffer submit returned error: {}", result);
  return 0;
}

void CAESinkStarfish::AddPause(unsigned int millis)
{
  m_starfishMediaAPI->Pause();
  std::this_thread::sleep_for(std::chrono::milliseconds(millis));
  m_starfishMediaAPI->Play();
}

void CAESinkStarfish::GetDelay(AEDelayStatus& status)
{
  auto delay = m_pts - std::chrono::nanoseconds(m_starfishMediaAPI->getCurrentPlaytime());
  status.SetDelay(std::chrono::duration_cast<std::chrono::duration<double>>(delay).count());
}

void CAESinkStarfish::Drain()
{
  m_starfishMediaAPI->pushEOS();
}

void CAESinkStarfish::PlayerCallback(const int32_t type,
                                     const int64_t numValue,
                                     const char* strValue)
{
  switch (type)
  {
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
      m_starfishMediaAPI->Play();
      break;
    default:
      std::string logstr = strValue != nullptr ? strValue : "";
      CLog::LogF(LOGDEBUG, "CAESinkStarfish: type: {}, numValue: {}, strValue: {}", type, numValue,
                 logstr);
  }
}

void CAESinkStarfish::PlayerCallback(const int32_t type,
                                     const int64_t numValue,
                                     const char* strValue,
                                     void* data)
{
  static_cast<CAESinkStarfish*>(data)->PlayerCallback(type, numValue, strValue);
}
