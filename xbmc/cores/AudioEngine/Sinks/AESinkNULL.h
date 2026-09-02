/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"

#include <chrono>

/**
 * @brief Sink that discards everything it is given.
 *
 * Takes over the configuration of the real device while that is handed over to an external
 * media pipeline. Not registered with CAESinkFactory, it never shows up as a device.
 */
class CAESinkNULL : public IAESink
{
public:
  const char* GetName() override { return "NULL"; }

  CAESinkNULL(std::chrono::duration<double> cacheTotal, std::chrono::duration<double> latency);

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override {}

  double GetCacheTotal() override { return m_cacheTotal.count(); }
  double GetLatency() override { return m_latency.count(); }

  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void GetDelay(AEDelayStatus& status) override;
  void Drain() override;

private:
  void UpdateBufferLevel();

  AEAudioFormat m_format{};
  std::chrono::duration<double> m_cacheTotal;
  std::chrono::duration<double> m_latency;

  std::chrono::duration<double> m_bufferLevel{};
  std::chrono::steady_clock::time_point m_lastUpdate{};
};
