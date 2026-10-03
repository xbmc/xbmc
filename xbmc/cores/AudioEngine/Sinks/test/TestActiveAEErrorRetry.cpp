/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{

constexpr const char* DRIVER = "FAKE";
constexpr const char* RECEIVER = "receiver";
constexpr const char* SPEAKERS = "speakers";

// A sink that accepts whatever it is given and consumes packets in real time.
class CFakeSink : public IAESink
{
public:
  const char* GetName() override { return DRIVER; }

  bool Initialize(AEAudioFormat& format, std::string& device) override
  {
    format.m_dataFormat = AE_FMT_FLOAT;
    format.m_sampleRate = 48000;
    format.m_channelLayout = AE_CH_LAYOUT_2_0;
    format.m_frames = 2400;
    format.m_frameSize = format.m_channelLayout.Count() * sizeof(float);
    m_sampleRate = format.m_sampleRate;
    return true;
  }

  void Deinitialize() override {}
  double GetCacheTotal() override { return 0.0; }

  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(frames * 1000 / m_sampleRate));
    return frames;
  }

  void GetDelay(AEDelayStatus& status) override { status.SetDelay(0.0); }

private:
  unsigned int m_sampleRate{48000};
};

// A driver exposing two devices. The receiver can be switched off, in which case opening it
// fails the way a WASAPI or ALSA open fails for an endpoint that is listed but not usable.
// The speakers always open. Every open attempt is recorded so a test can observe the engine's
// retry timing from outside.
class CFakeDriver
{
public:
  struct Attempt
  {
    std::string device;
    std::chrono::steady_clock::time_point when;
  };

  static void Register()
  {
    AE::AESinkRegEntry entry;
    entry.sinkName = DRIVER;
    entry.createFunc = Create;
    entry.enumerateFunc = Enumerate;
    entry.cleanupFunc = [] {};
    AE::CAESinkFactory::RegisterSink(entry);
  }

  static void Reset()
  {
    std::lock_guard lock(s_mutex);
    s_attempts.clear();
    s_receiverAvailable = false;
  }

  static void SetReceiverAvailable(bool available)
  {
    std::lock_guard lock(s_mutex);
    s_receiverAvailable = available;
  }

  static size_t AttemptCount()
  {
    std::lock_guard lock(s_mutex);
    return s_attempts.size();
  }

  static bool WaitForAttempts(size_t count, std::chrono::milliseconds timeout)
  {
    std::unique_lock lock(s_mutex);
    return s_cv.wait_for(lock, timeout, [count] { return s_attempts.size() >= count; });
  }

  static bool WaitForAttemptOn(const std::string& device,
                               size_t after,
                               std::chrono::milliseconds timeout)
  {
    std::unique_lock lock(s_mutex);
    return s_cv.wait_for(lock, timeout,
                         [&device, after]
                         {
                           for (size_t i = after; i < s_attempts.size(); ++i)
                           {
                             if (s_attempts[i].device == device)
                               return true;
                           }
                           return false;
                         });
  }

private:
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& format)
  {
    std::unique_lock lock(s_mutex);
    s_attempts.push_back({device, std::chrono::steady_clock::now()});
    const bool available = device != RECEIVER || s_receiverAvailable;
    lock.unlock();
    s_cv.notify_all();

    if (!available)
      return {};

    auto sink = std::make_unique<CFakeSink>();
    if (!sink->Initialize(format, device))
      return {};
    return sink;
  }

  static void Enumerate(AEDeviceInfoList& list, bool force)
  {
    list.clear();
    list.push_back(Device(RECEIVER, AE_DEVTYPE_HDMI));
    list.push_back(Device(SPEAKERS, AE_DEVTYPE_PCM));
  }

  static CAEDeviceInfo Device(const char* name, AEDeviceType type)
  {
    CAEDeviceInfo info;
    info.m_deviceName = name;
    info.m_displayName = name;
    info.m_deviceType = type;
    info.m_channels = AE_CH_LAYOUT_2_0;
    info.m_sampleRates.push_back(48000);
    info.m_dataFormats.push_back(AE_FMT_FLOAT);
    info.m_wantsIECPassthrough = false;
    info.m_onlyPCM = true;
    return info;
  }

  static std::mutex s_mutex;
  static std::condition_variable s_cv;
  static std::vector<Attempt> s_attempts;
  static bool s_receiverAvailable;
};

std::mutex CFakeDriver::s_mutex;
std::condition_variable CFakeDriver::s_cv;
std::vector<CFakeDriver::Attempt> CFakeDriver::s_attempts;
bool CFakeDriver::s_receiverAvailable = false;

std::string DeviceString(const char* name)
{
  return std::string(DRIVER) + ":" + name;
}

// Opening the configured device and, when that fails, the first enumerated device are two
// attempts per configure cycle. The engine's first cycle is the INIT; every later one is a
// retry from the error state.
constexpr size_t ATTEMPTS_PER_CYCLE = 2;

class TestActiveAEErrorRetry : public ::testing::Test
{
protected:
  void SetUp() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_savedDevice = settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);
    m_savedPassthroughDevice =
        settings->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE);

    AE::CAESinkFactory::ClearSinks();
    CFakeDriver::Reset();
    CFakeDriver::Register();

    ASSERT_TRUE(
        settings->SetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE, DeviceString(RECEIVER)));
    ASSERT_TRUE(settings->SetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE,
                                    DeviceString(RECEIVER)));
  }

  void TearDown() override
  {
    m_engine.reset();
    AE::CAESinkFactory::ClearSinks();

    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE, m_savedDevice);
    settings->SetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE, m_savedPassthroughDevice);
  }

  // Starts the engine against the unavailable receiver and lets it fail `cycles` configure
  // cycles, so the retry interval has grown to whatever the engine applies after that many.
  void StartAndFailCycles(size_t cycles)
  {
    m_engine = std::make_unique<ActiveAE::CActiveAE>();
    m_engine->Start();
    ASSERT_TRUE(CFakeDriver::WaitForAttempts(cycles * ATTEMPTS_PER_CYCLE, 10s))
        << "engine stopped retrying after " << CFakeDriver::AttemptCount() << " open attempts";
  }

  std::unique_ptr<ActiveAE::CActiveAE> m_engine;
  std::string m_savedDevice;
  std::string m_savedPassthroughDevice;
};

} // namespace

// The receiver is off and the engine has backed off. The user picks the speakers in the audio
// settings. That is a RECONFIGURE to the engine, and it must open the speakers promptly rather
// than when the next backed-off retry happens to fire.
TEST_F(TestActiveAEErrorRetry, SettingChangeWakesTheErrorRetry)
{
  // Four failed cycles: INIT, then retries after 500 ms, 500 ms and 1 s. The next retry is
  // two seconds out, so a wake-up inside one second can only come from the setting change.
  StartAndFailCycles(4);
  const size_t before = CFakeDriver::AttemptCount();

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  ASSERT_TRUE(
      settings->SetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE, DeviceString(SPEAKERS)));
  // The settings manager only notifies its callbacks once settings have been loaded, which the
  // test environment never does. Deliver the change the way the engine's settings handler would.
  m_engine->OnSettingsChange();

  EXPECT_TRUE(CFakeDriver::WaitForAttemptOn(SPEAKERS, before, 1s))
      << "the engine did not try the newly selected device within a second of the change";
}
