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
#include "cores/AudioEngine/Utils/AEUtil.h"
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include "threads/CriticalSection.h"

class CAESinkPULSE : public IAESink
{
public:
  const char *GetName() override { return "PULSE"; }

  CAESinkPULSE();
  ~CAESinkPULSE() override;

  static bool Register();
  static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

  bool Initialize(AEAudioFormat &format, std::string &device) override;
  void Deinitialize() override;

  virtual double GetDelay() { return 0.0; }
  void GetDelay(AEDelayStatus& status) override;
  double GetCacheTotal() override;
  unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
  void Drain() override;

  bool HasVolume() override { return true; };
  void SetVolume(float volume) override;

  bool IsInitialized();
  void UpdateInternalVolume(const pa_cvolume* nVol);
  pa_stream* GetInternalStream();
  CCriticalSection m_sec;
private:
  void Pause(bool pause);
  static inline bool WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry);
  static bool SetupContext(const char *host, pa_context **context, pa_threaded_mainloop **mainloop);

  bool m_IsAllocated;
  bool m_passthrough;
  bool m_IsStreamPaused;

  AEAudioFormat m_format;
  unsigned int m_BytesPerSecond;
  unsigned int m_BufferSize;
  unsigned int m_Channels;

  pa_stream *m_Stream;
  pa_cvolume m_Volume;
  bool m_volume_needs_update;
  uint32_t m_periodSize;

  pa_context *m_Context;
  pa_threaded_mainloop *m_MainLoop;
};
