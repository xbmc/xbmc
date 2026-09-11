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
#include <future>
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
