/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "AESinkWasmAudioWorklet.h"

#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "platform/wasm/WasmAudioWorkletManager.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <algorithm>

using KODI::PLATFORM::WASM::CWasmAudioWorkletManager;

namespace
{
// Web Audio only defines speaker mixing for 1, 2, 4 and 6 channels; other counts are
// mixed as "discrete", dropping every channel the destination lacks.
AEStdChLayout ResolveSinkLayout(unsigned int requestedChannels, unsigned int maxOutputChannels)
{
  if (requestedChannels > 6 && maxOutputChannels >= 8)
    return AE_CH_LAYOUT_7_1;
  if (requestedChannels > 2 && maxOutputChannels >= 6)
    return AE_CH_LAYOUT_5_1;
  return AE_CH_LAYOUT_2_0;
}
} // namespace

void CAESinkWasmAudioWorklet::Register()
{
  AE::AESinkRegEntry entry;
  entry.sinkName = "WASM";
  entry.createFunc = CAESinkWasmAudioWorklet::Create;
  entry.enumerateFunc = CAESinkWasmAudioWorklet::EnumerateDevicesEx;
  entry.cleanupFunc = CAESinkWasmAudioWorklet::Cleanup;
  AE::CAESinkFactory::RegisterSink(entry);
}

void CAESinkWasmAudioWorklet::Cleanup()
{
  CWasmAudioWorkletManager::Instance().Shutdown();
}

std::unique_ptr<IAESink> CAESinkWasmAudioWorklet::Create(std::string& device,
                                                         AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkWasmAudioWorklet>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

void CAESinkWasmAudioWorklet::EnumerateDevicesEx(AEDeviceInfoList& list, bool)
{
  list.clear();

  CAEDeviceInfo info;
  info.m_deviceName = "default";
  info.m_displayName = "Browser Audio";
  info.m_displayNameExtra = "Wasm Audio Worklet";
  info.m_deviceType = AE_DEVTYPE_PCM;
  info.m_channels = ResolveSinkLayout(
      CWasmAudioWorkletManager::kMaxChannels,
      CWasmAudioWorkletManager::Instance().GetMaxOutputChannels());
  info.m_sampleRates = {44100, 48000};
  info.m_dataFormats = {AE_FMT_FLOATP};
  info.m_wantsIECPassthrough = false;
  info.m_onlyPCM = true;
  list.push_back(info);
}

bool CAESinkWasmAudioWorklet::Initialize(AEAudioFormat& format, std::string& device)
{
  if (format.m_dataFormat == AE_FMT_RAW)
  {
    CLog::Log(LOGERROR, "CAESinkWasmAudioWorklet::Initialize - passthrough is not supported");
    return false;
  }

  const unsigned int maxOutputChannels =
      CWasmAudioWorkletManager::Instance().GetMaxOutputChannels();
  format.m_channelLayout =
      CAEChannelInfo(ResolveSinkLayout(format.m_channelLayout.Count(), maxOutputChannels));
  const unsigned int channels = format.m_channelLayout.Count();

  if (!CWasmAudioWorkletManager::Instance().Initialize(channels, format.m_sampleRate))
    return false;

  device = "default";
  format.m_dataFormat = AE_FMT_FLOATP;
  format.m_sampleRate = CWasmAudioWorkletManager::Instance().GetSampleRate();

  constexpr unsigned int TARGET_PERIOD_MS = 20;
  const unsigned int quantum = CWasmAudioWorkletManager::Instance().GetQuantumSize();
  const unsigned int targetFrames = (format.m_sampleRate * TARGET_PERIOD_MS + 999U) / 1000U;
  unsigned int periodFrames = targetFrames;
  if (quantum > 0)
    periodFrames = ((targetFrames + quantum - 1U) / quantum) * quantum;
  format.m_frames = std::max<unsigned int>(periodFrames, quantum);

  format.m_frameSize = channels * static_cast<unsigned int>(sizeof(float));
  m_initialized = true;

  CLog::Log(LOGINFO,
            "CAESinkWasmAudioWorklet: sink period set to {} frames ({:.2f} ms) "
            "from worklet quantum {} @ {} Hz",
            format.m_frames,
            static_cast<double>(format.m_frames) * 1000.0 /
                static_cast<double>(format.m_sampleRate),
            quantum, format.m_sampleRate);

  return true;
}

void CAESinkWasmAudioWorklet::Deinitialize()
{
  if (!m_initialized)
    return;

  CWasmAudioWorkletManager::Instance().Shutdown();
  m_initialized = false;
}

double CAESinkWasmAudioWorklet::GetCacheTotal()
{
  return CWasmAudioWorkletManager::Instance().GetBufferCapacitySeconds();
}

unsigned int CAESinkWasmAudioWorklet::AddPackets(uint8_t** data, unsigned int frames, unsigned int offset)
{
  if (!m_initialized || !data || !data[0])
    return 0;

  const unsigned int channels = CWasmAudioWorkletManager::Instance().GetChannels();
  if (channels == 0)
    return 0;

  // Build a plane-pointer array for the manager. ActiveAE passes planar data
  // as one pointer per channel in data[0..channels-1] when the sink format is
  // AE_FMT_FLOATP, and a single interleaved pointer in data[0] otherwise.
  // Since we advertise AE_FMT_FLOATP we always take the planar path.
  const float* planes[CWasmAudioWorkletManager::kMaxChannels]{};
  for (unsigned int ch = 0; ch < channels; ++ch)
  {
    if (!data[ch])
      return 0;
    planes[ch] = reinterpret_cast<const float*>(data[ch]);
  }

  const unsigned int written =
      CWasmAudioWorkletManager::Instance().WritePlanar(planes, channels, frames, offset);
  DrainUnderrunLog();
  return written;
}

void CAESinkWasmAudioWorklet::GetDelay(AEDelayStatus& status)
{
  const double totalDelay = CWasmAudioWorkletManager::Instance().GetTotalDelaySeconds();
  status.SetDelay(totalDelay);
  status.maxcorrection = totalDelay;
  status.tick = CurrentHostCounter();
}

void CAESinkWasmAudioWorklet::Drain()
{
  if (!m_initialized)
    return;

  CWasmAudioWorkletManager::Instance().Drain();
  DrainUnderrunLog();
}

void CAESinkWasmAudioWorklet::DrainUnderrunLog()
{
  m_pendingUnderrunFrames += CWasmAudioWorkletManager::Instance().ConsumeUnderrunFrames();
  if (m_pendingUnderrunFrames == 0)
    return;

  const unsigned int sampleRate = CWasmAudioWorkletManager::Instance().GetSampleRate();
  if (sampleRate == 0)
    return;

  constexpr uint64_t LOG_THRESHOLD_MS = 10;
  const uint64_t thresholdFrames = (static_cast<uint64_t>(sampleRate) * LOG_THRESHOLD_MS) / 1000;
  if (thresholdFrames == 0 || m_pendingUnderrunFrames < thresholdFrames)
    return;

  const double missingMs =
      static_cast<double>(m_pendingUnderrunFrames) * 1000.0 / static_cast<double>(sampleRate);
  CLog::Log(LOGWARNING,
            "CAESinkWasmAudioWorklet: worklet underrun, {} frames ({:.2f} ms) of silence "
            "emitted since last report",
            m_pendingUnderrunFrames, missingMs);
  m_pendingUnderrunFrames = 0;
}
