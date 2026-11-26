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
#include <future>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
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

// The videos of a run are numbered on from one batch to the next, and handed back in that order
TEST(TestVideoInfoScannerLookups, VideosAddedInBatchesAreHandedBackInOrder)
{
  constexpr size_t count{5};
  std::mutex lock;
  std::set<std::pair<size_t, size_t>> lookedUp;
  std::vector<size_t> handedBack;
  const std::atomic<bool> stop{false};
  const auto lookUp{[&lock, &lookedUp](size_t i, size_t index)
                    {
                      LookUpInReverse(i, count);
                      std::unique_lock guard(lock);
                      lookedUp.emplace(i, index);
                    }};
  const auto handBack{[&handedBack](size_t index)
                      {
                        handedBack.push_back(index);
                        return true;
                      }};

  CVideoLookups::CRun run{4, stop};
  EXPECT_EQ(0U, run.Add(count, lookUp));
  EXPECT_EQ(count, run.Add(count, lookUp));
  EXPECT_TRUE(run.HandBack(count, handBack));
  EXPECT_EQ(count, handedBack.size());
  EXPECT_TRUE(run.HandBack(2 * count, handBack));

  std::set<std::pair<size_t, size_t>> each;
  for (size_t index = 0; index < 2 * count; ++index)
  {
    EXPECT_EQ(index, handedBack[index]);
    each.emplace(index % count, index);
  }
  std::unique_lock guard(lock);
  EXPECT_EQ(each, lookedUp);
}

// A batch added before the one before it is handed back is looked up meanwhile, so that a slow
// look-up at the end of a batch doesn't hold up the next. The threads that had nothing left to look
// up wait for it.
TEST(TestVideoInfoScannerLookups, ABatchIsLookedUpWhileTheOneBeforeItWaits)
{
  std::promise<void> lastStarted;
  std::promise<void> nextStarted;
  const std::shared_future<void> nextLookedUp{nextStarted.get_future().share()};
  std::atomic<bool> waitedForTheNext{false};
  std::vector<size_t> handedBack;
  const std::atomic<bool> stop{false};
  const auto handBack{[&handedBack](size_t index)
                      {
                        handedBack.push_back(index);
                        return true;
                      }};

  CVideoLookups::CRun run{4, stop};
  run.Add(2,
          [&](size_t i, size_t)
          {
            if (i == 1)
            {
              lastStarted.set_value();
              waitedForTheNext =
                  nextLookedUp.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
            }
          });
  lastStarted.get_future().wait();
  // Long enough for the other threads to have looked the first up, and to wait for more
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  run.Add(2,
          [&nextStarted](size_t i, size_t)
          {
            if (i == 0)
              nextStarted.set_value();
          });
  EXPECT_TRUE(run.HandBack(2, handBack));
  EXPECT_TRUE(run.HandBack(4, handBack));

  EXPECT_TRUE(waitedForTheNext);
  EXPECT_EQ((std::vector<size_t>{0, 1, 2, 3}), handedBack);
}

// A batch added once the scan has stopped, as the one before is handed back, is neither looked up
// nor handed back, and the run ends
TEST(TestVideoInfoScannerLookups, ABatchAddedOnceTheScanStoppedIsNotLookedUp)
{
  std::atomic<bool> stop{false};
  std::atomic<size_t> lookedUp{0};
  std::vector<size_t> handedBack;
  const auto lookUp{[&lookedUp](size_t, size_t) { ++lookedUp; }};
  const auto handBack{[&handedBack](size_t index)
                      {
                        handedBack.push_back(index);
                        return true;
                      }};

  CVideoLookups::CRun run{4, stop};
  run.Add(3, lookUp);
  ASSERT_TRUE(run.HandBack(3, handBack));
  stop = true;
  run.Add(3, lookUp);
  const auto start{std::chrono::steady_clock::now()};
  EXPECT_FALSE(run.HandBack(6, handBack));
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(5));

  EXPECT_EQ(3U, lookedUp.load());
  EXPECT_EQ((std::vector<size_t>{0, 1, 2}), handedBack);
  EXPECT_FALSE(run.HandBack(6, handBack));
}

// One worker looks each video up on the calling thread as it is handed back, from batch to batch
TEST(TestVideoInfoScannerLookups, OneWorkerLooksEachUpAsItIsHandedBackFromBatchToBatch)
{
  const std::thread::id caller{std::this_thread::get_id()};
  std::mutex lock;
  std::vector<std::string> events;
  bool onCaller{true};
  const std::atomic<bool> stop{false};
  const auto lookUp{
      [&](size_t i, size_t index)
      {
        std::unique_lock guard(lock);
        events.push_back("look up " + std::to_string(i) + " " + std::to_string(index));
        onCaller = onCaller && std::this_thread::get_id() == caller;
      }};
  const auto handBack{[&](size_t index)
                      {
                        std::unique_lock guard(lock);
                        events.push_back("hand back " + std::to_string(index));
                        return true;
                      }};

  CVideoLookups::CRun run{1, stop};
  run.Add(2, lookUp);
  run.Add(2, lookUp);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  {
    std::unique_lock guard(lock);
    EXPECT_TRUE(events.empty());
  }
  EXPECT_TRUE(run.HandBack(2, handBack));
  EXPECT_TRUE(run.HandBack(4, handBack));

  std::unique_lock guard(lock);
  EXPECT_EQ((std::vector<std::string>{"look up 0 0", "hand back 0", "look up 1 1", "hand back 1",
                                      "look up 0 2", "hand back 2", "look up 1 3", "hand back 3"}),
            events);
  EXPECT_TRUE(onCaller);
}
