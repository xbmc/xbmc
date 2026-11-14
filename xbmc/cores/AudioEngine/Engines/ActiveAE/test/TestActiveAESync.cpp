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
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <numbers>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
constexpr const char* DRIVER = "FAKESYNC";
constexpr const char* DEVICE = "speakers";
constexpr unsigned int SAMPLE_RATE = 48000;
constexpr unsigned int CHUNK = 1024;
// Above it, a stream in sync is adjusted at normal speed. At a tempo only above 1000 ms, so the
// tests check the error rather than whether the stream is adjusted.
constexpr double MAX_SYNC_ERROR = 200.0;

// Consumes packets in real time
class CPacedSink : public IAESink
{
public:
  const char* GetName() override { return DRIVER; }

  bool Initialize(AEAudioFormat& format, std::string& device) override
  {
    format.m_dataFormat = AE_FMT_FLOAT;
    format.m_sampleRate = SAMPLE_RATE;
    format.m_channelLayout = AE_CH_LAYOUT_2_0;
    format.m_frames = CHUNK;
    format.m_frameSize = format.m_channelLayout.Count() * sizeof(float);
    return true;
  }

  void Deinitialize() override {}
  double GetCacheTotal() override { return 0.0; }

  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override
  {
    std::this_thread::sleep_for(std::chrono::microseconds(frames * 1000000ULL / SAMPLE_RATE));
    return frames;
  }

  void GetDelay(AEDelayStatus& status) override { status.SetDelay(0.0); }

  static void Register()
  {
    AE::AESinkRegEntry entry;
    entry.sinkName = DRIVER;
    entry.createFunc = [](std::string& device, AEAudioFormat& format) -> std::unique_ptr<IAESink>
    {
      auto sink = std::make_unique<CPacedSink>();
      sink->Initialize(format, device);
      return sink;
    };
    entry.enumerateFunc = [](AEDeviceInfoList& list, bool force)
    {
      CAEDeviceInfo info;
      info.m_deviceName = DEVICE;
      info.m_displayName = DEVICE;
      info.m_deviceType = AE_DEVTYPE_PCM;
      info.m_channels = AE_CH_LAYOUT_2_0;
      info.m_sampleRates.push_back(SAMPLE_RATE);
      info.m_dataFormats.push_back(AE_FMT_FLOAT);
      info.m_wantsIECPassthrough = false;
      info.m_onlyPCM = true;
      list.clear();
      list.push_back(info);
    };
    entry.cleanupFunc = [] {};
    AE::CAESinkFactory::RegisterSink(entry);
  }
};

// A player's clock, in milliseconds, running at a speed the test sets
class CTestClock : public IAEClockCallback
{
public:
  double GetClock() override
  {
    std::unique_lock lock(m_mutex);
    return Now();
  }

  double GetClockSpeed() override
  {
    std::unique_lock lock(m_mutex);
    return m_speed;
  }

  void SetSpeed(double speed)
  {
    std::unique_lock lock(m_mutex);
    m_base = Now();
    m_start = std::chrono::steady_clock::now();
    m_speed = speed;
  }

  // Back to media time 0
  void Restart()
  {
    std::unique_lock lock(m_mutex);
    m_base = 0.0;
    m_start = std::chrono::steady_clock::now();
  }

private:
  double Now() const
  {
    const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - m_start;
    return m_base + elapsed.count() * m_speed;
  }

  std::mutex m_mutex;
  std::chrono::steady_clock::time_point m_start{std::chrono::steady_clock::now()};
  double m_base{0.0};
  double m_speed{1.0};
};

class TestActiveAESync : public ::testing::Test
{
protected:
  void SetUp() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_savedDevice = settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);

    AE::CAESinkFactory::ClearSinks();
    CPacedSink::Register();
    ASSERT_TRUE(settings->SetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE,
                                    std::string(DRIVER) + ":" + DEVICE));

    m_engine = std::make_unique<ActiveAE::CActiveAE>();
    m_engine->Start();
  }

  void TearDown() override
  {
    m_running = false;
    if (m_feeder.joinable())
      m_feeder.join();
    m_stream.reset();
    m_engine.reset();
    AE::CAESinkFactory::ClearSinks();

    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE, m_savedDevice);
  }

  // Plays a tone from media time 0 synced to the clock by resampling, as the video player does
  void Play()
  {
    AEAudioFormat format;
    format.m_dataFormat = AE_FMT_FLOAT;
    format.m_sampleRate = SAMPLE_RATE;
    format.m_channelLayout = AE_CH_LAYOUT_2_0;
    format.m_frames = CHUNK;
    format.m_frameSize = format.m_channelLayout.Count() * sizeof(float);

    m_stream = m_engine->MakeStream(format, AESTREAM_FORCE_RESAMPLE | AESTREAM_PAUSED, &m_clock);
    ASSERT_TRUE(m_stream);
    m_stream->SetResampleMode(1);

    m_running = true;
    m_feeder = std::thread(
        [this]
        {
          std::vector<float> samples(2 * CHUNK);
          const uint8_t* data[] = {reinterpret_cast<const uint8_t*>(samples.data())};
          for (uint64_t frame = 0; m_running; frame += CHUNK)
          {
            for (unsigned int i = 0; i < CHUNK; ++i)
            {
              const auto value = static_cast<float>(
                  0.1 * std::sin(2.0 * std::numbers::pi * 440.0 * (frame + i) / SAMPLE_RATE));
              samples[2 * i] = value;
              samples[2 * i + 1] = value;
            }

            unsigned int offset = 0;
            while (offset < CHUNK && m_running)
            {
              IAEStream::ExtData ext;
              if (offset == 0)
                ext.pts = frame * 1000.0 / SAMPLE_RATE;
              const unsigned int added = m_stream->AddData(data, offset, CHUNK - offset, &ext);
              offset += added;
              if (added == 0)
                std::this_thread::sleep_for(1ms);
            }
          }
        });

    m_clock.Restart();
    m_stream->Resume();
  }

  bool WaitForSync(std::chrono::milliseconds timeout)
  {
    const auto end = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < end)
    {
      if (m_stream->GetSyncInfo().state == CAESyncInfo::SYNC_INSYNC)
        return true;
      std::this_thread::sleep_for(20ms);
    }
    return false;
  }

  // Checks that the stream stays in sync with errors below maxError, in ms
  void ExpectInSync(std::chrono::milliseconds duration, double maxError)
  {
    double largestError = 0.0;
    const auto end = std::chrono::steady_clock::now() + duration;
    while (std::chrono::steady_clock::now() < end)
    {
      const CAESyncInfo info = m_stream->GetSyncInfo();
      ASSERT_EQ(info.state, CAESyncInfo::SYNC_INSYNC)
          << "the stream left sync, the last error was " << info.error << " ms";
      largestError = std::max(largestError, std::abs(info.error));
      std::this_thread::sleep_for(20ms);
    }
    EXPECT_LT(largestError, maxError);
  }

  std::unique_ptr<ActiveAE::CActiveAE> m_engine;
  IAE::StreamPtr m_stream;
  CTestClock m_clock;
  std::thread m_feeder;
  std::atomic<bool> m_running{false};
  std::string m_savedDevice;
};
} // namespace

TEST_F(TestActiveAESync, StaysInSyncAtDoubleTempo)
{
  m_clock.SetSpeed(2.0);
  Play();
  ASSERT_TRUE(WaitForSync(3s));

  ExpectInSync(2500ms, MAX_SYNC_ERROR);
}

TEST_F(TestActiveAESync, FollowsAChangeOfTempoRightAway)
{
  Play();
  ASSERT_TRUE(WaitForSync(3s));
  std::this_thread::sleep_for(1s);

  // Right after a check of the sync, so that waiting for the next one would leave audio at the
  // old tempo the longest
  const unsigned int checked = m_stream->GetSyncInfo().errortime;
  for (int i = 0; i < 400 && m_stream->GetSyncInfo().errortime == checked; ++i)
    std::this_thread::sleep_for(5ms);

  // The audio queued at the old tempo still plays at it and falls behind by as much. Audio
  // made after the change has to follow the new tempo.
  const double queued = m_stream->GetSyncInfo().delay * 1000.0;
  m_clock.SetSpeed(2.0);
  ExpectInSync(3s, queued * (2.0 - 1.0) + 100.0);
}
