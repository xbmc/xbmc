/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioDevice.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

class AERingBuffer;
struct AEDelayStatus;

class CAESinkDARWINOSX : public IAESink
{
public:
  const char* GetName() override { return "DARWINOSX"; }

  CAESinkDARWINOSX();
  ~CAESinkDARWINOSX() override;

  static void Register();
  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force);
  static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override;

  void GetDelay(AEDelayStatus& status) override;
  double GetCacheTotal() override;
  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void Drain() override;

private:
  static OSStatus renderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData);
  void SetHogMode(bool on);

  CAEDeviceInfo      m_info;

  CCoreAudioDevice   m_device;
  CCoreAudioStream   m_outputStream;
  unsigned int       m_latentFrames;
  unsigned int       m_outputBufferIndex;

  bool               m_outputBitstream;   ///< true if we're bistreaming into a LinearPCM stream rather than AC3 stream.
  unsigned int       m_planes;            ///< number of audio planes (1 if non-planar)
  unsigned int       m_frameSizePerPlane; ///< frame size (per plane) in bytes
  unsigned int       m_framesPerSecond;   ///< sample rate

  AERingBuffer      *m_buffer;
  volatile bool      m_started;     // set once we get a callback from CoreAudio, which can take a little while.

  CAESpinSection         m_render_locker;
  volatile int64_t       m_render_tick;
  volatile double        m_render_delay;
};
