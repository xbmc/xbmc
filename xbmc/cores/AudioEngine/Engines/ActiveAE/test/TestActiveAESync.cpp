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
// The sink counts a packet as played as it returns it, the engine a moment later
constexpr double PACKET_MS = CHUNK * 1000.0 / SAMPLE_RATE;

// Consumes packets in real time, and keeps the playback time the tests measure in. A busy machine
// slows it down along with the audio, rather than leaving the audio behind a clock in real time.
class CPacedSink : public IAESink
{
public:
  static double PlayedMs() { return played * 1000.0 / SAMPLE_RATE; }

  static inline std::atomic<uint64_t> played{0};

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
    played += frames;
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

// A player's clock, in milliseconds, running at a speed the test sets over the playback time
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
    // The sink plays on meanwhile, so a single reading for both
    const double played = CPacedSink::PlayedMs();
    m_base = Now(played);
    m_start = played;
    m_speed = speed;
  }

  // Back to media time 0
  void Restart()
  {
    std::unique_lock lock(m_mutex);
    m_base = 0.0;
    m_start = CPacedSink::PlayedMs();
  }

private:
  double Now() const { return Now(CPacedSink::PlayedMs()); }
  double Now(double played) const { return m_base + (played - m_start) * m_speed; }

  std::mutex m_mutex;
  double m_start{0.0};
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
    CPacedSink::played = 0;
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
              {
                m_filled = true;
                std::this_thread::sleep_for(1ms);
              }
            }
          }
        });

    // As the player, resume once the stream's cache is full. The engine then holds the most audio
    // when its sync starts.
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!m_filled && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(1ms);
    ASSERT_TRUE(m_filled) << "the stream doesn't take data";

    m_clock.Restart();
    m_stream->Resume();
  }

  // Calls check with the stream's sync every 20 ms, until it returns true or the sink has played
  // duration more. Returns whether check returned true.
  template<typename Check>
  bool Watch(std::chrono::milliseconds duration, Check check)
  {
    const double end = CPacedSink::PlayedMs() + static_cast<double>(duration.count());
    // Only against a hang, playback takes about duration
    const auto deadline = std::chrono::steady_clock::now() + 10 * duration + 10s;
    while (CPacedSink::PlayedMs() < end)
    {
      if (check(m_stream->GetSyncInfo()))
        return true;
      if (std::chrono::steady_clock::now() > deadline)
      {
        ADD_FAILURE() << "playback stalled";
        return false;
      }
      std::this_thread::sleep_for(20ms);
    }
    return false;
  }

  bool WaitForSync(std::chrono::milliseconds timeout)
  {
    return Watch(timeout,
                 [](const CAESyncInfo& info) { return info.state == CAESyncInfo::SYNC_INSYNC; });
  }

  // Checks that the stream stays in sync for duration, and returns the largest error, in ms
  double LargestErrorInSync(std::chrono::milliseconds duration)
  {
    double largestError = 0.0;
    const bool left = Watch(duration,
                            [&](const CAESyncInfo& info)
                            {
                              largestError = std::max(largestError, std::abs(info.error));
                              return info.state != CAESyncInfo::SYNC_INSYNC;
                            });
    EXPECT_FALSE(left) << "the stream left sync";
    return largestError;
  }

  std::unique_ptr<ActiveAE::CActiveAE> m_engine;
  IAE::StreamPtr m_stream;
  CTestClock m_clock;
  std::thread m_feeder;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_filled{false};
  std::string m_savedDevice;
};
} // namespace

TEST_F(TestActiveAESync, StaysInSyncAtDoubleTempo)
{
  m_clock.SetSpeed(2.0);
  ASSERT_NO_FATAL_FAILURE(Play());
  ASSERT_TRUE(WaitForSync(3s));

  EXPECT_LT(LargestErrorInSync(3s), MAX_SYNC_ERROR);
}

TEST_F(TestActiveAESync, FollowsAChangeOfTempoRightAway)
{
  ASSERT_NO_FATAL_FAILURE(Play());
  ASSERT_TRUE(WaitForSync(3s));
  EXPECT_LT(LargestErrorInSync(1s), MAX_SYNC_ERROR);

  // Only the audio the engine holds by then plays at the old tempo, and falls behind the clock by
  // as much times the change of speed. Following the new tempo at the next check of the sync
  // instead would leave the audio made until then behind too.
  m_clock.SetSpeed(2.0);
  const double maxLag = m_stream->GetMaxDelay() * 1000.0 * (2.0 - 1.0) + PACKET_MS;
  EXPECT_LT(LargestErrorInSync(6s), maxLag);

  // By then the sync took the lag back
  EXPECT_LT(LargestErrorInSync(2s), MAX_SYNC_ERROR);
}
