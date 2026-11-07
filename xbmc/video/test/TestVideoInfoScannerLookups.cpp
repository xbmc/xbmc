/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/VideoInfoScannerLookups.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO;

namespace
{
//! The later a video is given, the sooner its look-up finishes
void LookUpInReverse(size_t i, size_t count)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(2 * (count - i)));
}
} // namespace

TEST(TestVideoInfoScannerLookups, VideosAreHandedBackInTheOrderTheyWereGivenOnTheCallingThread)
{
  constexpr size_t count{20};
  const std::thread::id caller{std::this_thread::get_id()};
  std::mutex lock;
  std::set<std::thread::id> lookingUp;
  std::vector<size_t> handedBack;
  bool onCaller{true};
  const std::atomic<bool> stop{false};

  CVideoLookups::Run(
      count, 4,
      [&](size_t i)
      {
        LookUpInReverse(i, count);
        std::unique_lock guard(lock);
        lookingUp.insert(std::this_thread::get_id());
      },
      [&](size_t i)
      {
        handedBack.push_back(i);
        onCaller = onCaller && std::this_thread::get_id() == caller;
        return true;
      },
      stop);

  ASSERT_EQ(count, handedBack.size());
  for (size_t i = 0; i < count; ++i)
    EXPECT_EQ(i, handedBack[i]);
  EXPECT_TRUE(onCaller);
  EXPECT_GT(lookingUp.size(), 1U);
}

// Once a video is refused, none after it is handed back, and no more are looked up than are already
// on their way
TEST(TestVideoInfoScannerLookups, NoMoreAreLookedUpOnceOneIsRefused)
{
  constexpr size_t count{100};
  std::atomic<size_t> lookedUp{0};
  std::vector<size_t> handedBack;
  const std::atomic<bool> stop{false};

  CVideoLookups::Run(
      count, 4, [&lookedUp](size_t) { ++lookedUp; },
      [&handedBack](size_t i)
      {
        handedBack.push_back(i);
        return i < 2;
      },
      stop);

  EXPECT_EQ(3U, handedBack.size());
  EXPECT_LT(lookedUp.load(), count);
}

TEST(TestVideoInfoScannerLookups, NothingIsLookedUpOnceTheScanIsStopped)
{
  std::atomic<size_t> lookedUp{0};
  const std::atomic<bool> stop{true};

  CVideoLookups::Run(10, 4, [&lookedUp](size_t) { ++lookedUp; }, [](size_t) { return true; }, stop);

  EXPECT_EQ(0U, lookedUp.load());
}

TEST(TestVideoInfoScannerLookups, OneWorkerLooksEachUpBeforeTheNext)
{
  std::vector<int> events;
  const std::atomic<bool> stop{false};

  CVideoLookups::Run(
      3, 1, [&events](size_t i) { events.push_back(static_cast<int>(i)); },
      [&events](size_t i)
      {
        events.push_back(-1 - static_cast<int>(i));
        return true;
      },
      stop);

  EXPECT_EQ((std::vector<int>{0, -1, 1, -2, 2, -3}), events);
}

// The scan may be stopped at any moment, without anyone being woken
TEST(TestVideoInfoScannerLookups, AScanStoppedAtAnyMomentEnds)
{
  constexpr size_t count{16};
  for (int trial = 0; trial < 200; ++trial)
  {
    std::atomic<bool> stop{false};
    std::mutex lock;
    std::set<size_t> lookedUp;
    std::vector<size_t> handedBack;
    std::thread stopper(
        [&stop, trial]
        {
          // Spun rather than slept, as a sleep lasts at least a tick of the system's timer
          const auto until{std::chrono::steady_clock::now() + std::chrono::microseconds(trial * 5)};
          while (std::chrono::steady_clock::now() < until)
            std::this_thread::yield();
          stop = true;
        });

    CVideoLookups::Run(
        count, 4,
        [&](size_t i)
        {
          std::unique_lock guard(lock);
          lookedUp.insert(i);
        },
        [&handedBack](size_t i)
        {
          handedBack.push_back(i);
          return true;
        },
        stop);
    stopper.join();

    // Those handed back are the first, each looked up
    for (size_t i = 0; i < handedBack.size(); ++i)
    {
      EXPECT_EQ(i, handedBack[i]);
      EXPECT_TRUE(lookedUp.contains(i));
    }
  }
}

TEST(TestVideoInfoScannerLookups, AHandBackThatThrowsEndsTheLookUps)
{
  std::atomic<size_t> lookedUp{0};
  const std::atomic<bool> stop{false};

  EXPECT_THROW(CVideoLookups::Run(
                   100, 4, [&lookedUp](size_t) { ++lookedUp; },
                   [](size_t i)
                   {
                     if (i == 3)
                       throw std::runtime_error("hand-back failed");
                     return true;
                   },
                   stop),
               std::runtime_error);

  // No look-up is still running
  const size_t ended{lookedUp.load()};
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_EQ(ended, lookedUp.load());
  EXPECT_LT(ended, 100U);
}

// A look-up that throws is handed back in its turn, and those after it are looked up, on several
// threads or on one: a folder of one video, or a refresh
TEST(TestVideoInfoScannerLookups, ALookUpThatThrowsIsStillHandedBack)
{
  const auto run{[](size_t count, size_t workers)
                 {
                   std::vector<size_t> handedBack;
                   const std::atomic<bool> stop{false};
                   CVideoLookups::Run(
                       count, workers,
                       [](size_t i)
                       {
                         if (i == 0)
                           throw std::runtime_error("look-up failed");
                       },
                       [&handedBack](size_t i)
                       {
                         handedBack.push_back(i);
                         return true;
                       },
                       stop);
                   return handedBack;
                 }};

  EXPECT_EQ((std::vector<size_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}), run(10, 4));
  EXPECT_EQ(std::vector<size_t>{0}, run(1, 4));
  EXPECT_EQ((std::vector<size_t>{0, 1, 2}), run(3, 1));
}
