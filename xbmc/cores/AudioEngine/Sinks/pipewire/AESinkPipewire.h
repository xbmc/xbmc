/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#include <chrono>

namespace KODI
{
namespace PIPEWIRE
{
class CPipewireStream;
}
} // namespace KODI

namespace AE
{
namespace SINK
{

class CAESinkPipewire : public IAESink
{
public:
  CAESinkPipewire() = default;
  ~CAESinkPipewire() override = default;

  static bool Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList& list, bool force = false);
  static void Destroy();

  // overrides via IAESink
  const char* GetName() override { return "PIPEWIRE"; }

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override;

  double GetCacheTotal() override;
  void GetDelay(AEDelayStatus& status) override;

  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void Drain() override;

private:
  AEAudioFormat m_format;
  std::chrono::duration<double, std::ratio<1>> m_latency;

  std::unique_ptr<KODI::PIPEWIRE::CPipewireStream> m_stream;
};

} // namespace SINK
} // namespace AE
