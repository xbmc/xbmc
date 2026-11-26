/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "interfaces/generic/ScriptRunner.h"
#include "test/TestUtils.h"
#include "utils/StreamDetails.h"
#include "utils/URIUtils.h"
#include "video/Episode.h"
#include "video/VideoGeneratedImageFileLoader.h"
#include "video/VideoInfoScannerLookups.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iterator>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
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

// No more are looked up than the window holds, the one being handed back among them
TEST(TestVideoInfoScannerLookups, NoMoreAreLookedUpThanTheWindowHolds)
{
  constexpr size_t count{12};
  constexpr size_t window{3};
  std::atomic<size_t> started{0};
  size_t handedBack{0};
  size_t most{0};
  const std::atomic<bool> stop{false};

  CVideoLookups::Run(
      count, 4, [&started](size_t) { ++started; },
      [&started, &handedBack, &most](size_t)
      {
        // Slow, so that the threads look up as far ahead as they may meanwhile
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        most = std::max(most, started - handedBack);
        ++handedBack;
        return true;
      },
      stop, window);

  EXPECT_EQ(count, handedBack);
  EXPECT_EQ(window, most);
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

// Told before it waits for a video not yet looked up, so that it can let go of what it holds, and
// not while the next video is ready
TEST(TestVideoInfoScannerLookups, TheCallingThreadIsToldBeforeItWaitsForAVideo)
{
  constexpr size_t count{3};
  std::promise<void> told;
  const std::shared_future<void> waitingForTheSecond{told.get_future().share()};
  std::atomic<bool> toldBeforeTheSecond{false};
  std::vector<size_t> toldAfter;
  size_t handedBack{0};
  const std::atomic<bool> stop{false};

  CVideoLookups::Run(
      count, 2,
      [&](size_t i)
      {
        // The second is looked up once the calling thread has been told it waits for it
        if (i == 1)
          toldBeforeTheSecond =
              waitingForTheSecond.wait_for(std::chrono::seconds{5}) == std::future_status::ready;
      },
      [&](size_t i)
      {
        ++handedBack;
        // By when it is asked for, the third has long been looked up
        if (i == 1)
          std::this_thread::sleep_for(std::chrono::milliseconds{100});
        return true;
      },
      stop, 0,
      [&]
      {
        toldAfter.push_back(handedBack);
        if (handedBack == 1)
          told.set_value();
        return true;
      });

  EXPECT_EQ(count, handedBack);
  EXPECT_TRUE(toldBeforeTheSecond);
  EXPECT_EQ(1, std::ranges::count(toldAfter, 1));
  EXPECT_EQ(0, std::ranges::count(toldAfter, 2));
}

// One worker looks each video up on the calling thread, which it keeps waiting, and the calling
// thread may say before any of them to hand back no more
TEST(TestVideoInfoScannerLookups, OneWorkerTellsTheCallingThreadBeforeEachLookUp)
{
  std::vector<size_t> lookedUp;
  std::vector<size_t> toldAfter;
  const std::atomic<bool> stop{false};

  CVideoLookups::Run(
      3, 1, [&lookedUp](size_t i) { lookedUp.push_back(i); }, [](size_t) { return true; }, stop, 0,
      [&]
      {
        toldAfter.push_back(lookedUp.size());
        return lookedUp.size() < 2;
      });

  EXPECT_EQ((std::vector<size_t>{0, 1, 2}), toldAfter);
  EXPECT_EQ((std::vector<size_t>{0, 1}), lookedUp);
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

namespace
{
bool Eventually(const std::function<bool()>& condition)
{
  for (int i = 0; i < 500 && !condition(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return condition();
}
} // namespace

TEST(TestVideoInfoScannerLookups, DiscsAreReadInTheOrderTheyWereQueued)
{
  std::mutex lock;
  std::vector<std::string> read;
  CDiscReadAhead reads{4, 1};
  const std::vector<std::string> discs{"a", "b", "c", "d"};
  for (const std::string& disc : discs)
    reads.Queue(disc,
                [&lock, &read, disc]
                {
                  std::unique_lock guard(lock);
                  read.push_back(disc);
                });
  ASSERT_TRUE(Eventually(
      [&lock, &read]
      {
        std::unique_lock guard(lock);
        return read.size() == 4;
      }));

  for (const std::string& disc : discs)
  {
    EXPECT_TRUE(reads.Take(disc)) << disc;
    reads.Release(disc);
  }
  std::unique_lock guard(lock);
  EXPECT_EQ(discs, read);
}

TEST(TestVideoInfoScannerLookups, OnlyAFewDiscsAreReadAheadOfThoseTaken)
{
  std::atomic<int> read{0};
  CDiscReadAhead reads{2, 1};
  for (const std::string disc : {"a", "b", "c", "d"})
    reads.Queue(disc, [&read] { ++read; });

  ASSERT_TRUE(Eventually([&read] { return read == 2; }));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(2, read);

  EXPECT_TRUE(reads.Take("a"));
  reads.Release("a");
  EXPECT_TRUE(Eventually([&read] { return read == 3; }));
}

// A disc not yet read is left to the scan, and one being read is waited for. The scan may come back
// to a disc it passed while it was being read, as it does to a folder of extras after the folders
// beside it.
TEST(TestVideoInfoScannerLookups, ADiscBeingReadIsWaitedForAndOneNotYetReadIsLeftToTheScan)
{
  CDiscReadAhead reads{1, 1};
  std::promise<void> started;
  std::promise<void> carryOn;
  std::shared_future<void> carryingOn{carryOn.get_future()};
  std::atomic<bool> finished{false};
  std::atomic<bool> secondRead{false};
  reads.Queue("a",
              [&started, carryingOn, &finished]
              {
                started.set_value();
                carryingOn.wait();
                finished = true;
              });
  reads.Queue("b", [&secondRead] { secondRead = true; });
  started.get_future().wait();
  EXPECT_FALSE(reads.Take("b"));

  std::thread reading(
      [&carryOn]
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        carryOn.set_value();
      });
  EXPECT_TRUE(reads.Take("a"));
  EXPECT_TRUE(finished);
  reading.join();
  reads.Release("a");
  reads.Stop();
  EXPECT_FALSE(secondRead);
}

// A disc the scan turns out not to need, its movie added from its nfo, is not read. One being read
// already is read, and taken as before.
TEST(TestVideoInfoScannerLookups, ADiscDroppedBeforeItIsReadIsNotRead)
{
  CDiscReadAhead reads{2, 1};
  std::promise<void> started;
  std::promise<void> carryOn;
  std::shared_future<void> carryingOn{carryOn.get_future()};
  std::atomic<bool> firstRead{false};
  std::atomic<bool> secondRead{false};
  reads.Queue("a",
              [&started, carryingOn, &firstRead]
              {
                started.set_value();
                carryingOn.wait();
                firstRead = true;
              });
  reads.Queue("b", [&secondRead] { secondRead = true; });
  started.get_future().wait();

  reads.Drop("a");
  reads.Drop("b");
  carryOn.set_value();
  EXPECT_TRUE(reads.Take("a"));
  EXPECT_TRUE(firstRead);
  reads.Release("a");
  EXPECT_FALSE(reads.Take("b"));
  reads.Stop();
  EXPECT_FALSE(secondRead);
}

// A disc held for its movie's look-up is read only once the look-up allows it, and not at all if
// the look-up drops it, so that no reader takes it before the look-up knows whether it is needed
TEST(TestVideoInfoScannerLookups, ADiscHeldForItsLookUpIsReadOnlyOnceAllowed)
{
  CDiscReadAhead reads{4, 2};
  std::atomic<bool> allowedRead{false};
  std::atomic<bool> droppedRead{false};
  reads.Queue("a", [&allowedRead] { allowedRead = true; }, true);
  reads.Queue("b", [&droppedRead] { droppedRead = true; }, true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(allowedRead);

  reads.Allow("a");
  reads.Drop("b");
  EXPECT_TRUE(Eventually([&allowedRead] { return allowedRead.load(); }));
  EXPECT_TRUE(reads.Take("a"));
  reads.Release("a");
  EXPECT_FALSE(reads.Take("b"));
  reads.Stop();
  EXPECT_FALSE(droppedRead);
}

// A disc still held when the scan comes to it, its look-up not done, is left to the scan
TEST(TestVideoInfoScannerLookups, ADiscStillHeldIsLeftToTheScan)
{
  CDiscReadAhead reads{4, 2};
  std::atomic<bool> read{false};
  reads.Queue("a", [&read] { read = true; }, true);
  EXPECT_FALSE(reads.Take("a"));
  reads.Allow("a");
  reads.Stop();
  EXPECT_FALSE(read);
}

// Those let go of as a disc is taken leave room for another to be read while the scan waits for it
TEST(TestVideoInfoScannerLookups, ADiscIsReadWhileTheScanWaitsForTheOneItTook)
{
  CDiscReadAhead reads{2, 2};
  std::atomic<bool> firstRead{false};
  std::promise<void> started;
  std::promise<void> carryOn;
  std::shared_future<void> carryingOn{carryOn.get_future()};
  std::atomic<bool> thirdRead{false};
  reads.Queue("a", [&firstRead] { firstRead = true; });
  reads.Queue("b",
              [&started, carryingOn]
              {
                started.set_value();
                carryingOn.wait();
              });
  started.get_future().wait();
  ASSERT_TRUE(Eventually([&firstRead] { return firstRead.load(); }));
  // For "a" to count as read, not as being read, so that "c" has no room until "a" is let go of
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  reads.Queue("c", [&thirdRead] { thirdRead = true; });

  auto taking{std::async(std::launch::async, [&reads] { return reads.Take("b"); })};
  EXPECT_TRUE(Eventually([&thirdRead] { return thirdRead.load(); }));
  carryOn.set_value();
  EXPECT_TRUE(taking.get());
  reads.Release("b");
  reads.Stop();
}

TEST(TestVideoInfoScannerLookups, TakingADiscLetsThoseBeforeItGo)
{
  std::atomic<int> read{0};
  CDiscReadAhead reads{2, 1};
  for (const std::string disc : {"a", "b"})
    reads.Queue(disc, [&read] { ++read; });
  ASSERT_TRUE(Eventually([&read] { return read == 2; }));

  // A movie the scan added without its disc, as nothing was found for it
  EXPECT_TRUE(reads.Take("b"));
  reads.Release("b");

  for (const std::string disc : {"c", "d"})
    reads.Queue(disc, [&read] { ++read; });
  EXPECT_TRUE(Eventually([&read] { return read == 4; }));
}

TEST(TestVideoInfoScannerLookups, NoMoreDiscsAreReadOnceAbandoned)
{
  CDiscReadAhead reads{1, 1};
  std::atomic<int> read{0};
  std::promise<void> started;
  std::promise<void> carryOn;
  std::shared_future<void> carryingOn{carryOn.get_future()};
  reads.Queue("a",
              [&read, &started, carryingOn]
              {
                started.set_value();
                carryingOn.wait();
                ++read;
              });
  reads.Queue("b", [&read] { ++read; });
  started.get_future().wait();

  reads.Abandon();
  carryOn.set_value();
  reads.Stop();
  EXPECT_EQ(1, read);
}

TEST(TestVideoInfoScannerLookups, DiscsAreReadAsManyAtATimeAsThereAreReaders)
{
  CDiscReadAhead reads{4, 2};
  std::mutex lock;
  int reading{0};
  int most{0};
  std::promise<void> carryOn;
  std::shared_future<void> carryingOn{carryOn.get_future()};
  std::atomic<int> read{0};
  for (const std::string disc : {"a", "b", "c"})
    reads.Queue(disc,
                [&lock, &reading, &most, carryingOn, &read]
                {
                  {
                    std::unique_lock guard(lock);
                    most = std::max(most, ++reading);
                  }
                  carryingOn.wait();
                  {
                    std::unique_lock guard(lock);
                    --reading;
                  }
                  ++read;
                });

  const bool together{Eventually(
      [&lock, &reading]
      {
        std::unique_lock guard(lock);
        return reading == 2;
      })};
  carryOn.set_value();
  const bool allRead{Eventually([&read] { return read == 3; })};
  reads.Stop();
  ASSERT_TRUE(together);
  ASSERT_TRUE(allRead);

  std::unique_lock guard(lock);
  EXPECT_EQ(2, most);
}

// A disc's episodes are added together, so it is read once, in the turn of the first of them
TEST(TestVideoInfoScannerLookups, TheDiscsOfAShowAreReadInTheOrderItsEpisodesAreAdded)
{
  const auto episode{[](const std::string& path, int season, int number)
                     {
                       EPISODE file{season, number};
                       file.strPath = path;
                       return file;
                     }};
  const EPISODELIST files{episode("/tv/Show/Season 1/A S01E01E02.iso", 1, 1),
                          episode("/tv/Show/Season 1/A S01E01E02.iso", 1, 2),
                          episode("/tv/Show/Season 1/Show S01E03.mkv", 1, 3),
                          episode("/tv/Show/Season 2/Show.S02E01/BDMV/index.bdmv", 2, 1),
                          episode("/tv/Show/Specials/Show 2023-11-05.iso", -1, -1),
                          episode("/tv/Show/Season 2/B S02E02.iso", 2, 2),
                          episode("/tv/Show/Season 1/A S01E01E02.iso", 1, 1)};
  std::vector<const EPISODE*> toAdd;
  std::ranges::transform(files, std::back_inserter(toAdd),
                         [](const EPISODE& file) { return &file; });

  EXPECT_EQ((std::vector<std::string>{files[0].strPath, files[3].strPath, files[4].strPath,
                                      files[5].strPath}),
            EpisodeDiscs(toAdd));
}

// A scan's probe, which opens the file without the file cache, finds what any other probe does
TEST(TestVideoInfoScannerLookups, AScanProbeFindsTheStreamsAnotherProbeFinds)
{
  const std::string file{
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/h264_8bit_320x240.mkv")};
  CFileItem plain{file, false};
  CFileItem scanned{file, false};
  CStreamDetails probed;
  ASSERT_TRUE(CDVDFileInfo::GetFileStreamDetails(&plain));
  ASSERT_TRUE(CScanProbe::GetFileStreamDetails(&scanned, 0, &probed));

  const CStreamDetails& expected{plain.GetVideoInfoTag()->m_streamDetails};
  EXPECT_EQ(expected.GetVideoCodec(), scanned.GetVideoInfoTag()->m_streamDetails.GetVideoCodec());
  EXPECT_EQ(expected.GetVideoWidth(), probed.GetVideoWidth());
  EXPECT_EQ(expected.GetVideoDuration(), probed.GetVideoDuration());

  int plainDuration{0};
  int scannedDuration{0};
  ASSERT_TRUE(CDVDFileInfo::GetFileDuration(file, plainDuration));
  ASSERT_TRUE(CScanProbe::GetFileDuration(file, scannedDuration));
  EXPECT_EQ(plainDuration, scannedDuration);
}

TEST(TestVideoInfoScannerLookups, AFileReadThroughAClientOfOneCallAtATimeIsKnown)
{
  EXPECT_FALSE(CScanProbe::ReadsOneAtATime("nfs://server/export/Film.mkv"));
  EXPECT_FALSE(CScanProbe::ReadsOneAtATime("stack:///videos/Film CD1.mkv , /videos/Film CD2.mkv"));
  EXPECT_FALSE(CScanProbe::ReadsOneAtATime("/videos/Film.mkv"));
#if defined(TARGET_POSIX)
  EXPECT_TRUE(CScanProbe::ReadsOneAtATime("smb://server/share/Film.mkv"));
  EXPECT_TRUE(CScanProbe::ReadsOneAtATime(
      URIUtils::GetBlurayPlaylistPath("smb://server/share/Film/BDMV/index.bdmv", 800)));
  // A stack is read through the clients of its files
  EXPECT_TRUE(CScanProbe::ReadsOneAtATime(
      "stack:///videos/Film CD1.mkv , smb://server/share/Film CD2.mkv"));
#else
  EXPECT_FALSE(CScanProbe::ReadsOneAtATime("smb://server/share/Film.mkv"));
#endif
}

#if defined(TARGET_POSIX)
TEST(TestVideoInfoScannerLookups, ProbesOfFilesReadOneCallAtATimeTakeTurns)
{
  std::atomic<int> inside{0};
  std::atomic<int> most{0};
  const auto probe{[&]
                   {
                     const int now{++inside};
                     int seen{most};
                     while (now > seen && !most.compare_exchange_weak(seen, now))
                     {
                     }
                     std::this_thread::sleep_for(std::chrono::milliseconds(50));
                     --inside;
                     return true;
                   }};

  std::thread other{[&] { CScanProbe::Run("smb://server/share/a.mkv", probe); }};
  CScanProbe::Run("smb://server/share/b.mkv", probe);
  other.join();

  EXPECT_EQ(1, most);
}

// A look-up that has its turn to probe, and waits for another probe's calls through libsmbclient,
// gives up without probing where its scan stopped meanwhile
TEST(TestVideoInfoScannerLookups, AStoppedLookupDoesNotStartAnotherSmbProbe)
{
  std::atomic<bool> stop{false};
  std::atomic<bool> probed{false};
  std::promise<void> ownerEntered;
  std::promise<void> lookupEntered;
  std::promise<void> releaseOwner;
  const std::future<void> release{releaseOwner.get_future()};
  std::thread owner{[&]
                    {
                      CScanProbe::Run("smb://server/share/a.mkv",
                                      [&]
                                      {
                                        ownerEntered.set_value();
                                        release.wait();
                                        return true;
                                      });
                    }};
  ownerEntered.get_future().wait();
  std::thread lookup{[&]
                     {
                       CVideoLookups::Run(
                           1, 1,
                           [&](size_t)
                           {
                             lookupEntered.set_value();
                             CScanProbe::Run("smb://server/share/b.mkv",
                                             [&] { return probed = true; });
                           },
                           [](size_t) { return true; }, stop);
                     }};
  lookupEntered.get_future().wait();
  stop = true;
  releaseOwner.set_value();
  owner.join();
  lookup.join();

  EXPECT_FALSE(probed);
}
#endif

TEST(TestVideoInfoScannerLookups, AScanRunsForAsLongAsItsMarkLives)
{
  EXPECT_FALSE(CScanProbe::Scanning());
  {
    const CScanProbe::CScanning scanning;
    EXPECT_TRUE(CScanProbe::Scanning());
  }
  EXPECT_FALSE(CScanProbe::Scanning());
}

TEST(TestVideoInfoScannerLookups, ProbesOfOtherFilesRunAtOnce)
{
  std::mutex lock;
  std::condition_variable changed;
  int entered{0};
  // Each waits for the other to be inside too
  const auto probe{[&]
                   {
                     std::unique_lock guard(lock);
                     ++entered;
                     changed.notify_all();
                     return changed.wait_for(guard, std::chrono::seconds(5),
                                             [&] { return entered == 2; });
                   }};

  bool otherMet{false};
  std::thread other{[&] { otherMet = CScanProbe::Run("/videos/a.mkv", probe); }};
  const bool met{CScanProbe::Run("/videos/b.mkv", probe)};
  other.join();

  EXPECT_TRUE(met);
  EXPECT_TRUE(otherMet);
}

namespace
{
//! Probes of the video being added that take every turn, and hold them until let go
class CHeldProbes
{
public:
  CHeldProbes()
  {
    const size_t count{CScanProbe::ProbesAtOnce()};
    for (size_t probe = 0; probe < count; ++probe)
      m_probes.emplace_back([this]
                            { CScanProbe::Run("/videos/held.mkv", [this] { return Hold(); }); });
    std::unique_lock guard(m_lock);
    m_held = m_changed.wait_for(guard, std::chrono::seconds(5),
                                [this, count] { return m_holding == count; });
  }
  ~CHeldProbes()
  {
    LetGo(m_probes.size());
    for (std::thread& probe : m_probes)
      probe.join();
  }
  CHeldProbes(const CHeldProbes&) = delete;
  CHeldProbes& operator=(const CHeldProbes&) = delete;

  //! Whether every turn is held
  bool Held() const { return m_held; }
  //! Lets \p count of them end, and give their turns back
  void LetGo(size_t count = 1)
  {
    {
      std::unique_lock guard(m_lock);
      m_letGo += count;
    }
    m_changed.notify_all();
  }

private:
  bool Hold()
  {
    std::unique_lock guard(m_lock);
    ++m_holding;
    m_changed.notify_all();
    if (m_changed.wait_for(guard, std::chrono::seconds(30), [this] { return m_letGo > 0; }))
      --m_letGo;
    return true;
  }

  std::mutex m_lock;
  std::condition_variable m_changed;
  size_t m_holding{0};
  size_t m_letGo{0};
  bool m_held{false};
  std::vector<std::thread> m_probes;
};
} // namespace

TEST(TestVideoInfoScannerLookups, NoMoreProbesRunAtOnceThanThereAreTurns)
{
  const size_t turns{CScanProbe::ProbesAtOnce()};
  std::mutex lock;
  std::condition_variable changed;
  size_t inside{0};
  size_t most{0};
  // Each waits a while for the others to be inside too
  const auto probe{[&]
                   {
                     std::unique_lock guard(lock);
                     most = std::max(most, ++inside);
                     changed.notify_all();
                     changed.wait_for(guard, std::chrono::seconds(1),
                                      [&] { return inside >= turns; });
                     guard.unlock();
                     std::this_thread::sleep_for(std::chrono::milliseconds(20));
                     guard.lock();
                     --inside;
                     return true;
                   }};

  std::vector<std::thread> probes;
  for (size_t file = 0; file < 2 * turns; ++file)
    probes.emplace_back([&probe, file]
                        { CScanProbe::Run("/videos/" + std::to_string(file) + ".mkv", probe); });
  for (std::thread& running : probes)
    running.join();

  EXPECT_EQ(turns, most);
}

// Of the probes waiting, the one for the video being added goes first, then those of the videos
// looked up ahead of it in the order the scan adds them
TEST(TestVideoInfoScannerLookups, TheProbeForTheVideoNeededSoonestGoesFirst)
{
  std::mutex lock;
  std::vector<std::string> turns;
  std::vector<std::future<void>> waiting;
  CHeldProbes held;
  ASSERT_TRUE(held.Held());
  const auto probe{[&lock, &turns](std::optional<size_t> place, const std::string& name)
                   {
                     return std::async(std::launch::async,
                                       [&lock, &turns, place, name]
                                       {
                                         std::optional<CVideoLookups::CPlace> ahead;
                                         if (place)
                                           ahead.emplace(*place);
                                         CScanProbe::Run("/videos/" + name + ".mkv",
                                                         [&lock, &turns, &name]
                                                         {
                                                           std::unique_lock guard(lock);
                                                           turns.push_back(name);
                                                           return true;
                                                         });
                                       });
                   }};
  for (const auto& [place, name] : {std::pair<std::optional<size_t>, std::string>{8, "ninth"},
                                    {std::nullopt, "added"},
                                    {4, "fifth"},
                                    {1, "second"},
                                    {6, "seventh"}})
  {
    waiting.push_back(probe(place, name));
    ASSERT_TRUE(Eventually([&waiting] { return CScanProbe::Waiting() == waiting.size(); }));
  }

  // One turn, which each takes in its turn
  held.LetGo();
  for (std::future<void>& probed : waiting)
    EXPECT_EQ(std::future_status::ready, probed.wait_for(std::chrono::seconds(5)));

  std::unique_lock guard(lock);
  EXPECT_EQ((std::vector<std::string>{"added", "second", "fifth", "seventh", "ninth"}), turns);
}

// A probe that waits for its turn in a look-up of a scan that stops gives up, without probing, as the
// scan won't add the video, but one the scan thread makes, for a video it may add, waits on
TEST(TestVideoInfoScannerLookups, AProbeWaitingForItsTurnGivesUpOnceItsScanStopsOnlyInALookUp)
{
  std::atomic<bool> stop{false};
  std::atomic<bool> lookUpProbed{false};
  std::atomic<bool> scanProbed{false};
  // Made before the turns are held, so that they end once those are let go of
  std::future<bool> lookUp;
  std::future<bool> scanThread;
  CHeldProbes held;
  ASSERT_TRUE(held.Held());
  lookUp = std::async(std::launch::async,
                      [&stop, &lookUpProbed]
                      {
                        bool probed{false};
                        CVideoLookups::Run(
                            1, 1,
                            [&lookUpProbed, &probed](size_t)
                            {
                              probed = CScanProbe::Run("/videos/a.mkv",
                                                       [&lookUpProbed]
                                                       {
                                                         lookUpProbed = true;
                                                         return true;
                                                       });
                            },
                            [](size_t) { return true; }, stop);
                        return probed;
                      });
  ASSERT_TRUE(Eventually([] { return CScanProbe::Waiting() == 1; }));
  scanThread = std::async(std::launch::async,
                          [&stop, &scanProbed]
                          {
                            const CScriptRunner::CAbandonWhen abandonOnStop{stop};
                            return CScanProbe::Run("/videos/b.mkv",
                                                   [&scanProbed]
                                                   {
                                                     scanProbed = true;
                                                     return true;
                                                   });
                          });
  ASSERT_TRUE(Eventually([] { return CScanProbe::Waiting() == 2; }));

  stop = true;
  const bool gaveUp{lookUp.wait_for(std::chrono::seconds(5)) == std::future_status::ready};
  const bool scanWaits{scanThread.wait_for(std::chrono::milliseconds(300)) ==
                       std::future_status::timeout};
  held.LetGo();

  EXPECT_TRUE(gaveUp);
  EXPECT_FALSE(lookUp.get());
  EXPECT_FALSE(lookUpProbed);
  EXPECT_TRUE(scanWaits);
  EXPECT_TRUE(scanThread.get());
  EXPECT_TRUE(scanProbed);
}

// A thumb decoded from a video's frame while a scan runs, as the GUI asks for one, doesn't wait for
// the scan's probes
TEST(TestVideoInfoScannerLookups, AThumbDecodedDuringAScanDoesntWaitForItsProbes)
{
  const IMAGE_FILES::CImageFileURL thumb{IMAGE_FILES::URLFromFile("/videos/thumb.mkv", "video")};
  const CVideoGeneratedImageFileLoader loader;
  const CScanProbe::CScanning scanning;
  CHeldProbes held;
  ASSERT_TRUE(held.Held());
  std::future<void> decoding{
      std::async(std::launch::async, [&loader, &thumb] { static_cast<void>(loader.Load(thumb)); })};

  EXPECT_EQ(std::future_status::ready, decoding.wait_for(std::chrono::seconds(5)));
  held.LetGo(CScanProbe::ProbesAtOnce());
}

// A video refused ends the run, and the calling thread is told before it waits for the look-ups
// still running
TEST(TestVideoInfoScannerLookups, TheCallingThreadIsToldBeforeItWaitsForTheLookUpsLeft)
{
  std::promise<void> started;
  const std::shared_future<void> secondStarted{started.get_future().share()};
  std::promise<void> told;
  const std::shared_future<void> toldOnce{told.get_future().share()};
  std::atomic<bool> refused{false};
  std::atomic<bool> toldFirst{false};
  const std::atomic<bool> stop{false};

  CVideoLookups::Run(
      2, 2,
      [&](size_t i)
      {
        if (i != 1)
          return;
        started.set_value();
        toldFirst = toldOnce.wait_for(std::chrono::seconds{5}) == std::future_status::ready;
      },
      [&](size_t)
      {
        // Refused while the second is looked up
        refused = secondStarted.wait_for(std::chrono::seconds{5}) == std::future_status::ready;
        return false;
      },
      stop, 0,
      [&]
      {
        if (refused && toldOnce.wait_for(std::chrono::seconds{0}) != std::future_status::ready)
          told.set_value();
        return true;
      });

  EXPECT_TRUE(toldFirst);
}
