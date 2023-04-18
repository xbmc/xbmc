/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

#include <starfish-media-pipeline/StarfishMediaAPIs.h>

class CAESinkStarfish : public IAESink
{
public:
  const char* GetName() override { return "Starfish"; }
  CAESinkStarfish();
  ~CAESinkStarfish() override;

  static void Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList& list, bool force = false);

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override;
  double GetCacheTotal() override;
  double GetLatency() override;
  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void AddPause(unsigned int millis) override;
  void GetDelay(AEDelayStatus& status) override;
  void Drain() override;
  bool HasVolume() override;
  void SetVolume(float volume) override;

private:
  void PlayerCallback(const int32_t type, const int64_t numValue, const char* strValue);
  static void PlayerCallback(const int32_t type,
                             const int64_t numValue,
                             const char* strValue,
                             void* data);

  std::string_view AEFormatToStarfishFormat(AEDataFormat format);

  std::unique_ptr<StarfishMediaAPIs> m_starfishMediaAPI;
  AEAudioFormat m_format;
  int64_t m_pts{0};
  int64_t m_bufferSize{0};
  int64_t m_delay{0};
  int64_t m_playtime{0};
};
