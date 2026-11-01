/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/Event.h"
#include "threads/Timer.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
template<typename Predicate>
bool WaitFor(Predicate predicate)
{
  for (int i = 0; i < 5000; ++i)
  {
    if (predicate())
      return true;
    std::this_thread::sleep_for(1ms);
  }
  return false;
}

// A timer whose callback can use the timer itself
class CSelfTimer : public ITimerCallback
{
public:
  explicit CSelfTimer(std::function<void(CTimer&)> onTimeout) : m_onTimeout(std::move(onTimeout)) {}

  void OnTimeout() override { m_onTimeout(m_timer); }

  const std::function<void(CTimer&)> m_onTimeout;
  CTimer m_timer{this};
};
} // namespace

TEST(TestTimer, FiresOnce)
{
  std::atomic<int> calls{0};
  CTimer timer([&calls] { ++calls; });

  ASSERT_TRUE(timer.Start(1ms));
  ASSERT_TRUE(WaitFor([&] { return !timer.IsRunning(); }));
  std::this_thread::sleep_for(20ms);
  EXPECT_EQ(calls, 1);
}

TEST(TestTimer, FiresAtInterval)
{
  std::atomic<int> calls{0};
  CTimer timer([&calls] { ++calls; });

  ASSERT_TRUE(timer.Start(1ms, true));
  ASSERT_TRUE(WaitFor([&] { return calls >= 3; }));
  EXPECT_TRUE(timer.Stop(true));
  const int stoppedAt = calls;
  std::this_thread::sleep_for(20ms);
  EXPECT_EQ(calls, stoppedAt);
  EXPECT_FALSE(timer.IsRunning());
}

TEST(TestTimer, RestartPostponesTimeout)
{
  std::atomic<int> calls{0};
  std::atomic<std::chrono::steady_clock::time_point> firedAt;
  CTimer timer(
      [&]
      {
        firedAt = std::chrono::steady_clock::now();
        ++calls;
      });

  // A slow machine may let the timer fire before a Restart(), which then fails and isn't counted
  auto restartedAt = std::chrono::steady_clock::now();
  ASSERT_TRUE(timer.Start(100ms));
  for (int i = 0; i < 3; ++i)
  {
    std::this_thread::sleep_for(20ms);
    const auto now = std::chrono::steady_clock::now();
    if (timer.Restart())
      restartedAt = now;
  }
  ASSERT_TRUE(WaitFor([&] { return !timer.IsRunning(); }));
  EXPECT_EQ(calls, 1);
  EXPECT_GE(firedAt.load() - restartedAt, 100ms);
}

TEST(TestTimer, RestartAsyncShortensTimeout)
{
  std::atomic<int> calls{0};
  CTimer timer([&calls] { ++calls; });

  ASSERT_TRUE(timer.Start(10s));
  // Let the timer thread start waiting for the 10 s timeout
  std::this_thread::sleep_for(20ms);
  timer.RestartAsync(1ms);
  EXPECT_TRUE(WaitFor([&] { return calls == 1; }));
}

TEST(TestTimer, RestartAsyncFromCallback)
{
  std::atomic<int> calls{0};
  std::atomic<bool> runningInCallback{false};
  CSelfTimer self(
      [&](CTimer& timer)
      {
        runningInCallback = timer.IsRunning();
        if (++calls == 1)
          timer.RestartAsync(1ms);
      });

  ASSERT_TRUE(self.m_timer.Start(1ms));
  ASSERT_TRUE(WaitFor([&] { return calls == 2 && !self.m_timer.IsRunning(); }));
  EXPECT_TRUE(runningInCallback);
  std::this_thread::sleep_for(20ms);
  EXPECT_EQ(calls, 2);
}

TEST(TestTimer, SharesOneThreadInDueOrder)
{
  std::mutex mutex;
  std::vector<std::pair<char, std::thread::id>> calls;
  auto record = [&](char name)
  {
    std::unique_lock lock(mutex);
    calls.emplace_back(name, std::this_thread::get_id());
  };
  CTimer late([&] { record('L'); });
  CTimer early([&] { record('E'); });

  ASSERT_TRUE(late.Start(200ms));
  ASSERT_TRUE(early.Start(10ms));
  ASSERT_TRUE(WaitFor([&] { return !late.IsRunning() && !early.IsRunning(); }));

  ASSERT_EQ(calls.size(), 2u);
  EXPECT_EQ(calls[0].first, 'E');
  EXPECT_EQ(calls[1].first, 'L');
  EXPECT_EQ(calls[0].second, calls[1].second);
  EXPECT_NE(calls[0].second, std::this_thread::get_id());
}

TEST(TestTimer, StopWaitsForCallback)
{
  CEvent entered;
  std::atomic<bool> done{false};
  CTimer timer(
      [&]
      {
        entered.Set();
        std::this_thread::sleep_for(20ms);
        done = true;
      });

  ASSERT_TRUE(timer.Start(1ms));
  ASSERT_TRUE(entered.Wait(1s));
  EXPECT_TRUE(timer.Stop(true));
  EXPECT_TRUE(done);
}

TEST(TestTimer, StopWaitsOnlyForItsOwnCallback)
{
  CEvent entered;
  CEvent release;
  std::atomic<bool> released{false};
  CTimer busy(
      [&]
      {
        entered.Set();
        released = release.Wait(5s);
      });
  CTimer idle([] {});

  ASSERT_TRUE(idle.Start(10s));
  ASSERT_TRUE(busy.Start(1ms));
  ASSERT_TRUE(entered.Wait(1s));
  EXPECT_TRUE(idle.Stop(true));
  release.Set();
  ASSERT_TRUE(WaitFor([&] { return !busy.IsRunning(); }));
  EXPECT_TRUE(released);
}

TEST(TestTimer, StoppedTimerRunsUntilItsCallbackReturns)
{
  CEvent entered;
  CEvent release;
  std::atomic<bool> done{false};
  CTimer timer(
      [&]
      {
        entered.Set();
        release.Wait(5s);
        done = true;
      });

  ASSERT_TRUE(timer.Start(1ms));
  ASSERT_TRUE(entered.Wait(1s));
  EXPECT_TRUE(timer.Stop(false));
  EXPECT_TRUE(timer.IsRunning());
  std::thread releaser(
      [&]
      {
        std::this_thread::sleep_for(20ms);
        release.Set();
      });
  timer.Stop(true);
  EXPECT_TRUE(done);
  releaser.join();
}

TEST(TestTimer, StopFromOwnCallback)
{
  std::atomic<int> calls{0};
  CSelfTimer self(
      [&](CTimer& timer)
      {
        timer.Stop(true);
        ++calls;
      });

  ASSERT_TRUE(self.m_timer.Start(1ms, true));
  ASSERT_TRUE(WaitFor([&] { return calls == 1 && !self.m_timer.IsRunning(); }));
  std::this_thread::sleep_for(20ms);
  EXPECT_EQ(calls, 1);
}

TEST(TestTimer, ExceptionInCallbackDoesNotStopOtherTimers)
{
  std::atomic<bool> fired{false};
  CTimer throwing([] { throw std::runtime_error("test"); });
  CTimer other([&fired] { fired = true; });

  ASSERT_TRUE(throwing.Start(1ms));
  ASSERT_TRUE(WaitFor([&] { return !throwing.IsRunning(); }));
  ASSERT_TRUE(other.Start(1ms));
  EXPECT_TRUE(WaitFor([&] { return fired.load(); }));
}
