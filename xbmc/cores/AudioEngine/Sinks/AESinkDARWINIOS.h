/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#define DO_440HZ_TONE_TEST 0

#if DO_440HZ_TONE_TEST
typedef struct {
  float currentPhase;
  float phaseIncrement;
} SineWaveGenerator;
#endif

class AERingBuffer;
class CAAudioUnitSink;

class CAESinkDARWINIOS : public IAESink
{
public:
  const char* GetName() override { return "DARWINIOS"; }

  CAESinkDARWINIOS();
  ~CAESinkDARWINIOS() override = default;

  static void Register();
  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force);
  static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override;

  void GetDelay(AEDelayStatus& status) override;
  double GetCacheTotal() override;
  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void Drain() override;
  bool HasVolume() override;

private:
  static AEDeviceInfoList m_devices;
  CAEDeviceInfo      m_info;
  AEAudioFormat      m_format;

  CAAudioUnitSink   *m_audioSink;
#if DO_440HZ_TONE_TEST
  SineWaveGenerator  m_SineWaveGenerator;
#endif
};
