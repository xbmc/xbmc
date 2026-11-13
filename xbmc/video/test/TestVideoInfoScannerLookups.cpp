/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "test/TestUtils.h"
#include "utils/StreamDetails.h"
#include "utils/URIUtils.h"
#include "video/Episode.h"
#include "video/VideoInfoScannerLookups.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iterator>
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

namespace
{
//! Waits until the condition holds, for a while at most
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

// Kodi's nfs:// client, and its smb:// client but on Windows, make one network call at a time
TEST(TestVideoInfoScannerLookups, AFileReadThroughAClientOfOneCallAtATimeIsKnown)
{
  EXPECT_TRUE(CScanProbe::ReadsOneAtATime("nfs://server/export/Film.mkv"));
  EXPECT_TRUE(CScanProbe::ReadsOneAtATime(
      URIUtils::GetBlurayPlaylistPath("nfs://server/export/Film/BDMV/index.bdmv", 800)));
  // A stack is read through the clients of its files
  EXPECT_TRUE(CScanProbe::ReadsOneAtATime(
      "stack:///videos/Film CD1.mkv , nfs://server/export/Film CD2.mkv"));
  EXPECT_FALSE(CScanProbe::ReadsOneAtATime("stack:///videos/Film CD1.mkv , /videos/Film CD2.mkv"));
  EXPECT_FALSE(CScanProbe::ReadsOneAtATime("/videos/Film.mkv"));
#if defined(TARGET_POSIX)
  EXPECT_TRUE(CScanProbe::ReadsOneAtATime("smb://server/share/Film.mkv"));
#else
  EXPECT_FALSE(CScanProbe::ReadsOneAtATime("smb://server/share/Film.mkv"));
#endif
}

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

  std::thread other{[&] { CScanProbe::Run("nfs://server/export/a.mkv", probe); }};
  CScanProbe::Run("nfs://server/export/b.mkv", probe);
  other.join();

  EXPECT_EQ(1, most);
}

// A thumbnail's frame is taken in turn with a scan's probes only while a scan runs
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
