/*
 *  Copyright (C) 2010-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"

class CAESinkDummy : public IAESink
{
public:
  CAESinkDummy();
  ~CAESinkDummy() override;

  const char* GetName() override;
  bool Initialize(AEAudioFormat &format, std::string &device) override;
  void Deinitialize() override;
  double GetCacheTotal() override;
  double GetLatency() override;
  void Drain() override;
  unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
  void GetDelay(AEDelayStatus& status) override;
  bool HasVolume() override;
  void SetVolume(float volume) override;
};
