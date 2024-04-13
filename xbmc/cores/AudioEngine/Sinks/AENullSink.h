#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

class CAENullSink : public IAESink
{
public:
  static bool Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList& list, bool force = false);
  static void Destroy();

  const char* GetName() override { return "NULL"; }

  CAENullSink() = default;
  ~CAENullSink() override = default;

  bool Initialize(AEAudioFormat& format, std::string& device) override;

  void Deinitialize() override {}

  double GetCacheTotal() override { return 0.0; }

  double GetLatency() override { return 0.0; }

  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;

  void AddPause(unsigned int millis) override {}

  void GetDelay(AEDelayStatus& status) override {}

  void Drain() override {}

  bool HasVolume() override { return false; }

  void SetVolume(float volume) override {}

private:
  AEAudioFormat m_format;
};
