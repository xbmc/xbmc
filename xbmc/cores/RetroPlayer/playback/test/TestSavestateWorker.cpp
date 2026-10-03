/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/playback/AutosaveCapture.h"
#include "cores/RetroPlayer/playback/SavestateWorker.h"

#include <future>
#include <memory>
#include <stdexcept>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

TEST(TestSavestateWorker, BusyWorkerKeepsOneOwnedSnapshot)
{
  std::promise<void> started;
  std::promise<void> finish;
  auto finished = finish.get_future();
  auto buffer = std::make_unique<int>(42);
  const int* address = buffer.get();
  CSavestateWorker<int> worker(std::move(buffer),
                               [&](const int& value)
                               {
                                 EXPECT_EQ(value, 42);
                                 started.set_value();
                                 finished.wait();
                               });
  auto snapshot = worker.Acquire();
  EXPECT_EQ(snapshot.get(), address);
  worker.Submit(snapshot);
  EXPECT_FALSE(snapshot);
  started.get_future().wait();
  for (unsigned int i = 0; i < 1000; ++i)
    EXPECT_FALSE(worker.TryAcquire());
  finish.set_value();
  worker.Drain();
  snapshot = worker.Acquire();
  EXPECT_EQ(snapshot.get(), address);
  EXPECT_EQ(*snapshot, 42);
}

TEST(TestSavestateWorker, DrainsPendingWorkOnDestruction)
{
  int saved = 0;
  {
    CSavestateWorker<int> worker(std::make_unique<int>(7),
                                 [&](const int& value) { saved = value; });
    auto snapshot = worker.Acquire();
    worker.Submit(snapshot);
  }
  EXPECT_EQ(saved, 7);
}

TEST(TestSavestateWorker, ReturnsStorageAfterFailure)
{
  CSavestateWorker<int> worker(std::make_unique<int>(9),
                               [](const int&) { throw std::runtime_error("save failed"); });
  auto snapshot = worker.Acquire();
  worker.Submit(snapshot);
  EXPECT_NE(worker.Drain(), nullptr);
  snapshot = worker.Acquire();
  EXPECT_EQ(*snapshot, 9);
}

TEST(TestSavestateWorker, BusySavesCoalesceToLatestFrame)
{
  struct Snapshot
  {
    std::unique_ptr<uint32_t[]> memory = std::make_unique<uint32_t[]>(1);
  };
  std::promise<void> started;
  std::promise<void> finish;
  auto finished = finish.get_future();
  unsigned int commits = 0;
  uint32_t lastSaved = 0;
  CSavestateWorker<Snapshot> worker(std::make_unique<Snapshot>(),
                                    [&](const Snapshot& snapshot)
                                    {
                                      lastSaved = snapshot.memory[0];
                                      if (++commits == 1)
                                      {
                                        started.set_value();
                                        finished.wait();
                                      }
                                    });
  auto snapshot = worker.Acquire();
  snapshot->memory[0] = 1;
  worker.Submit(snapshot);
  started.get_future().wait();
  CAutosaveCapture capture;
  unsigned int serializations = 0;
  for (unsigned int frame = 2; frame <= 1000; ++frame)
  {
    capture.Request();
    snapshot = worker.TryAcquire();
    EXPECT_FALSE(snapshot);
  }
  finish.set_value();
  worker.Drain();
  snapshot = worker.Acquire();
  EXPECT_TRUE(capture.CaptureFrame(
      nullptr, false, snapshot->memory,
      [&]
      {
        ++serializations;
        snapshot->memory[0] = 1001;
        return true;
      },
      [] {}));
  worker.Submit(snapshot);
  worker.Drain();
  EXPECT_EQ(serializations, 1u);
  EXPECT_EQ(commits, 2u);
  EXPECT_EQ(lastSaved, 1001u);
  EXPECT_FALSE(capture.IsPending());
}

TEST(TestSavestateWorker, ExplicitSaveWaitsForCapacityAndIsNotDropped)
{
  std::promise<void> started;
  std::promise<void> finish;
  auto finished = finish.get_future();
  int saved = 0;
  CSavestateWorker<int> worker(std::make_unique<int>(1),
                               [&](const int& value)
                               {
                                 saved = value;
                                 if (value == 1)
                                 {
                                   started.set_value();
                                   finished.wait();
                                 }
                               });
  auto snapshot = worker.Acquire();
  worker.Submit(snapshot);
  started.get_future().wait();
  auto explicitSave = std::async(std::launch::async,
                                 [&]
                                 {
                                   auto next = worker.Acquire();
                                   *next = 2;
                                   worker.Submit(next);
                                   worker.Drain();
                                 });
  EXPECT_EQ(explicitSave.wait_for(std::chrono::seconds(0)), std::future_status::timeout);
  finish.set_value();
  explicitSave.get();
  EXPECT_EQ(saved, 2);
}

namespace KODI::RETRO
{
class CSavestateWorkerTestAccess
{
public:
  template<typename Snapshot>
  static std::unique_lock<std::mutex> Lock(CSavestateWorker<Snapshot>& worker)
  {
    return std::unique_lock(worker.m_mutex);
  }
};
} // namespace KODI::RETRO

TEST(TestSavestateWorker, ReadySnapshotInvalidatedBeforeSubmissionRetriesOnce)
{
  struct Snapshot
  {
    std::unique_ptr<uint32_t[]> memory = std::make_unique<uint32_t[]>(1);
    bool discarded{false};
  };
  unsigned int writes = 0;
  uint32_t saved = 0;
  CSavestateWorker<Snapshot> worker(std::make_unique<Snapshot>(),
                                    [&](const Snapshot& snapshot)
                                    {
                                      if (!snapshot.discarded)
                                      {
                                        ++writes;
                                        saved = snapshot.memory[0];
                                      }
                                    });
  auto snapshot = worker.Acquire();
  CAutosaveCapture capture;
  capture.Request();
  bool ready = capture.CaptureFrame(
      nullptr, false, snapshot->memory,
      [&]
      {
        snapshot->memory[0] = 17;
        return true;
      },
      [] {});
  ASSERT_TRUE(ready);

  std::promise<void> locked;
  std::promise<void> unlock;
  auto release = unlock.get_future();
  std::thread holder(
      [&]
      {
        auto lock = CSavestateWorkerTestAccess::Lock(worker);
        locked.set_value();
        release.wait();
      });
  locked.get_future().wait();
  EXPECT_FALSE(worker.TrySubmit(snapshot));
  capture.Cancel(snapshot, ready);
  EXPECT_TRUE(snapshot->discarded);
  EXPECT_TRUE(ready);
  EXPECT_TRUE(capture.IsPending());
  capture.Cancel(snapshot, ready);
  unlock.set_value();
  holder.join();

  worker.Submit(snapshot);
  worker.Drain();
  EXPECT_EQ(writes, 0u);

  snapshot = worker.Acquire();
  auto serialize = [&]
  {
    snapshot->memory[0] = 23;
    return true;
  };
  auto metadata = [&] { snapshot->discarded = false; };
  EXPECT_TRUE(capture.CaptureFrame(nullptr, false, snapshot->memory, serialize, metadata));
  EXPECT_FALSE(snapshot->discarded);
  EXPECT_FALSE(capture.IsPending());
  EXPECT_FALSE(capture.CaptureFrame(nullptr, false, snapshot->memory, serialize, metadata));
  worker.Submit(snapshot);
  worker.Drain();
  EXPECT_EQ(writes, 1u);
  EXPECT_EQ(saved, 23u);
}

TEST(TestSavestateWorker, DrainReportsTheFinalTaskRatherThanAnEarlierResult)
{
  CSavestateWorker<bool> worker(std::make_unique<bool>(false),
                                [](const bool& success)
                                {
                                  if (!success)
                                    throw std::runtime_error("write failed");
                                });
  auto snapshot = worker.Acquire();
  worker.Submit(snapshot);
  EXPECT_NE(worker.Drain(), nullptr);

  snapshot = worker.Acquire();
  *snapshot = true;
  worker.Submit(snapshot);
  EXPECT_EQ(worker.Drain(), nullptr);

  snapshot = worker.Acquire();
  *snapshot = false;
  worker.Submit(snapshot);
  EXPECT_NE(worker.Drain(), nullptr);
}
