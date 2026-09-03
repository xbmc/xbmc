/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#include <cstdint>
#include <memory>

class CAESinkWasmAudioWorklet : public IAESink
{
public:
  const char* GetName() override { return "WasmAudioWorklet"; }

  CAESinkWasmAudioWorklet() = default;
  ~CAESinkWasmAudioWorklet() override = default;

  static void Register();
  static void Cleanup();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList& list, bool force = false);

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override;

  double GetCacheTotal() override;
  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void GetDelay(AEDelayStatus& status) override;
  void Drain() override;

private:
  void DrainUnderrunLog();

  bool m_initialized{false};
  uint64_t m_pendingUnderrunFrames{0};
};
