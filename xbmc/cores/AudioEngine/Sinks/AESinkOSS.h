/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "threads/CriticalSection.h"

#include <stdint.h>

class CAESinkOSS : public IAESink
{
public:
  const char *GetName() override { return "OSS"; }

  CAESinkOSS();
  ~CAESinkOSS() override;

  static void Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

  bool Initialize(AEAudioFormat &format, std::string &device) override;
  void Deinitialize() override;

  virtual void Stop();
  void GetDelay(AEDelayStatus& status) override;
  double GetCacheTotal() override { return 0.0; } /* FIXME */
  unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
  void Drain() override;
private:
  int m_fd;
  std::string m_device;
  AEAudioFormat m_initFormat;
  AEAudioFormat m_format;

  CAEChannelInfo GetChannelLayout(const AEAudioFormat& format);
  std::string GetDeviceUse(const AEAudioFormat& format, const std::string &device);
};

