/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/playback/GameLoop.h"
#include "cores/RetroPlayer/playback/SavestateWorker.h"

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <thread>

#include <gtest/gtest.h>

using namespace KODI::RETRO;
using namespace std::chrono_literals;

namespace
{
class GameLoopCallback : public IGameLoopCallback
{
public:
  void FrameEvent() override
  {
    if (++frames == 1)
    {
      gameThread = std::this_thread::get_id();
      entered.set_value();
      release.wait();
    }
  }
  void RewindEvent() override { FrameEvent(); }
  void EndEvent() override
  {
    EXPECT_EQ(std::this_thread::get_id(), gameThread);
    contextDestroyed = true;
  }

  std::atomic<unsigned int> frames{0};
  std::atomic<bool> contextDestroyed{false};
  std::promise<void> entered;
  std::promise<void> finishFrame;
  std::future<void> release{finishFrame.get_future()};
  std::thread::id gameThread;
};
} // namespace

TEST(TestGameLoop, QuiescePreservesContextUntilFinalSaveCompletes)
{
  GameLoopCallback callback;
  CGameLoop loop(&callback, 60.0);
  loop.SetSpeed(1.0);
  loop.Start();
  callback.entered.get_future().wait();
  auto quiesced = std::async(std::launch::async, [&] { loop.Quiesce(); });
  EXPECT_EQ(quiesced.wait_for(0s), std::future_status::timeout);
  callback.finishFrame.set_value();
  quiesced.get();
  loop.Quiesce();
  EXPECT_FALSE(callback.contextDestroyed);
  const auto finalFrame = callback.frames.load();
  bool saved = false;
  CSavestateWorker<unsigned int> worker(std::make_unique<unsigned int>(finalFrame),
                                        [&](const unsigned int& frame)
                                        {
                                          EXPECT_FALSE(callback.contextDestroyed);
                                          EXPECT_EQ(callback.frames.load(), frame);
                                          saved = true;
                                        });
  auto snapshot = worker.Acquire();
  worker.Submit(snapshot);
  worker.Drain();
  EXPECT_TRUE(saved);
  EXPECT_EQ(callback.frames.load(), finalFrame);
  EXPECT_FALSE(callback.contextDestroyed);
  loop.Stop();
  EXPECT_TRUE(callback.contextDestroyed);
}

TEST(TestGameLoop, QuiesceHandlesPausedAndUnstartedLoops)
{
  class Callback : public IGameLoopCallback
  {
  public:
    void FrameEvent() override { ADD_FAILURE() << "Paused game advanced"; }
    void RewindEvent() override { ADD_FAILURE() << "Paused game rewound"; }
    void EndEvent() override { ++ends; }
    std::atomic<unsigned int> ends{0};
  } callback;
  CGameLoop loop(&callback, 60.0);
  loop.Quiesce();
  loop.Start();
  loop.Quiesce();
  EXPECT_EQ(callback.ends, 0u);
  loop.Stop();
  EXPECT_EQ(callback.ends, 1u);
  loop.Quiesce();
  loop.Stop();
  EXPECT_EQ(callback.ends, 1u);
}

TEST(TestGameLoop, LetsTheDisplayFollowOnlyAGameItWouldNotPace)
{
  using Clock = CDisplayPacing::Clock;

  class Callback : public IGameLoopCallback
  {
  public:
    explicit Callback(const CDisplayPacing& pacing) : m_pacing(pacing) {}
    void FrameEvent() override
    {
      std::unique_lock lock(m_mutex);
      m_followed = m_pacing.FollowedFrameTime() != Clock::duration::zero();
      m_frameRun.notify_all();
    }
    void RewindEvent() override {}
    void EndEvent() override {}

    bool WaitForFrame(bool followed)
    {
      std::unique_lock lock(m_mutex);
      return m_frameRun.wait_for(lock, 5s, [&] { return m_followed == followed; });
    }

  private:
    const CDisplayPacing& m_pacing;
    std::mutex m_mutex;
    std::condition_variable m_frameRun;
    std::optional<bool> m_followed;
  };

  CDisplayPacing pacing;
  pacing.SetRefreshRateSource([] { return 60.0; });
  pacing.SetVariableRefreshRange(40.0, 60.0);

  std::atomic<bool> rendering{true};
  std::thread renderer(
      [&]
      {
        while (rendering)
        {
          pacing.OnFrameTaken(Clock::now());
          std::this_thread::sleep_for(16ms);
        }
      });

  Callback callback(pacing);
  CGameLoop loop(&callback, 50.0, &pacing);
  loop.SetSpeed(1.0);
  loop.Start();

  // 50 fps is too far from the screen's 60 Hz to pace
  EXPECT_TRUE(callback.WaitForFrame(true));

  // 59.95 fps is paced, and the display keeps to its own rate
  loop.SetFrameRate(59.95);
  EXPECT_TRUE(callback.WaitForFrame(false));

  loop.Stop();
  rendering = false;
  renderer.join();
}
