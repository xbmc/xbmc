/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkNULL.h"

#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

#include <algorithm>
#include <thread>

using namespace std::chrono_literals;

namespace
{
constexpr auto DEFAULT_BUFFER_TIME = 250ms;
constexpr auto DEFAULT_PERIOD_TIME = 50ms;
} // namespace

CAESinkNULL::CAESinkNULL(const std::chrono::duration<double> cacheTotal,
                         const std::chrono::duration<double> latency)
  : m_cacheTotal(cacheTotal),
    m_latency(latency)
{
  if (m_cacheTotal <= 0s)
    m_cacheTotal = DEFAULT_BUFFER_TIME;
}

bool CAESinkNULL::Initialize(AEAudioFormat& format, std::string& device)
{
  m_format = format;

  if (m_format.m_sampleRate == 0)
  {
    CLog::LogF(LOGERROR, "invalid sample rate");
    return false;
  }

  // a raw stream is IEC packed before it reaches the sink, report back what is written
  if (m_format.m_dataFormat == AE_FMT_RAW)
    m_format.m_dataFormat = AE_FMT_S16NE;

  if (m_format.m_frameSize == 0)
  {
    m_format.m_frameSize =
        m_format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3);
    if (m_format.m_frameSize == 0)
    {
      CLog::LogF(LOGERROR, "invalid frame size");
      return false;
    }
  }

  if (m_format.m_frames == 0)
  {
    m_format.m_frames = static_cast<unsigned int>(
        m_format.m_sampleRate * std::chrono::duration<double>(DEFAULT_PERIOD_TIME).count());
  }

  format = m_format;

  m_bufferLevel = 0s;
  m_lastUpdate = std::chrono::steady_clock::now();

  CLog::LogF(LOGDEBUG, "discarding {} at {} Hz, buffer {} ms",
             CAEUtil::DataFormatToStr(m_format.m_dataFormat), m_format.m_sampleRate,
             std::chrono::duration_cast<std::chrono::milliseconds>(m_cacheTotal).count());

  return true;
}

void CAESinkNULL::UpdateBufferLevel()
{
  const auto now = std::chrono::steady_clock::now();
  const auto elapsed = now - m_lastUpdate;
  m_lastUpdate = now;

  m_bufferLevel -= elapsed;
  m_bufferLevel = std::max(m_bufferLevel, std::chrono::duration<double>::zero());
}

unsigned int CAESinkNULL::AddPackets(uint8_t** data, unsigned int frames, unsigned int offset)
{
  UpdateBufferLevel();

  const std::chrono::duration<double> duration(static_cast<double>(frames) / m_format.m_sampleRate);

  if (m_bufferLevel + duration > m_cacheTotal)
  {
    std::this_thread::sleep_for(m_bufferLevel + duration - m_cacheTotal);
    UpdateBufferLevel();
  }

  m_bufferLevel += duration;

  return frames;
}

void CAESinkNULL::GetDelay(AEDelayStatus& status)
{
  UpdateBufferLevel();
  status.SetDelay((m_bufferLevel + m_latency).count());
}

void CAESinkNULL::Drain()
{
  UpdateBufferLevel();
  std::this_thread::sleep_for(m_bufferLevel);
  UpdateBufferLevel();
}
