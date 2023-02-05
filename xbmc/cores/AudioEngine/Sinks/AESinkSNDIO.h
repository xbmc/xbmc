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

#include <sndio.h>

class CAESinkSNDIO : public IAESink
{
public:
  const char *GetName() override { return "sndio"; }

  CAESinkSNDIO();
  ~CAESinkSNDIO() override;

  static void Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

  bool Initialize(AEAudioFormat &format, std::string &device) override;
  void Deinitialize() override;

  virtual void Stop();
  void GetDelay(AEDelayStatus& status) override;
  double GetCacheTotal() override { return 0.0; }
  unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
  void Drain() override;
private:
  void AudioFormatToPar(AEAudioFormat& format);
  bool ParToAudioFormat(AEAudioFormat& format);
  static void OnmoveCb(void *arg, int delta);

  struct sio_hdl *m_hdl;
  struct sio_par m_par;
  ssize_t m_played;
  ssize_t m_written;
};

