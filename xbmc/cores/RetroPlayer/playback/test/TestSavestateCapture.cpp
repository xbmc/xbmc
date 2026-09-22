/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/playback/SavestateCapture.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
struct Snapshot
{
  std::unique_ptr<uint32_t[]> memory = std::make_unique<uint32_t[]>(1);
  uint32_t achievements{0};
};

class Core
{
public:
  std::unique_lock<std::mutex> LockForSnapshot() { return std::unique_lock(m_mutex); }

  bool Serialize(uint8_t* data, size_t size)
  {
    ++serializations;
    EXPECT_EQ(size, sizeof(state));
    if (!success || size != sizeof(state))
      return false;
    std::memcpy(data, &state, size);
    return true;
  }

  uint32_t state{17};
  unsigned int serializations{0};
  bool success{true};

private:
  std::mutex m_mutex;
};
} // namespace

TEST(TestSavestateCapture, ExplicitSavesSerializeCoreMutationWithoutRunningFrame)
{
  Core core;
  std::vector<uint32_t> saved;
  std::vector<uint32_t> achievements;
  auto storage = std::make_unique<Snapshot>();
  const auto* address = storage->memory.get();
  CSavestateWorker<Snapshot> worker(std::move(storage),
                                    [&](const Snapshot& snapshot)
                                    {
                                      saved.push_back(snapshot.memory[0]);
                                      achievements.push_back(snapshot.achievements);
                                    });
  auto metadata = [&](Snapshot& snapshot) { snapshot.achievements = core.state; };
  auto snapshot = CaptureSavestate(worker, core, sizeof(core.state), metadata);
  ASSERT_NE(snapshot, nullptr);
  EXPECT_EQ(snapshot->memory.get(), address);
  worker.Submit(snapshot);
  worker.Drain();

  core.state = 23;
  snapshot = CaptureSavestate(worker, core, sizeof(core.state), metadata);
  ASSERT_NE(snapshot, nullptr);
  EXPECT_EQ(snapshot->memory.get(), address);
  worker.Submit(snapshot);
  worker.Drain();

  EXPECT_EQ(core.serializations, 2u);
  EXPECT_EQ(saved, (std::vector<uint32_t>{17, 23}));
  EXPECT_EQ(achievements, (std::vector<uint32_t>{17, 23}));
}

TEST(TestSavestateCapture, FailedExplicitSerializationReleasesStorageWithoutCapturingMetadata)
{
  Core core;
  auto storage = std::make_unique<Snapshot>();
  const auto* address = storage->memory.get();
  CSavestateWorker<Snapshot> worker(std::move(storage), [](const Snapshot&) {});
  unsigned int metadataCaptures = 0;
  auto metadata = [&](Snapshot&) { ++metadataCaptures; };
  auto snapshot = CaptureSavestate(worker, core, sizeof(core.state), metadata);
  ASSERT_NE(snapshot, nullptr);
  worker.Submit(snapshot);
  worker.Drain();

  core.success = false;
  snapshot = CaptureSavestate(worker, core, sizeof(core.state), metadata);
  EXPECT_EQ(snapshot, nullptr);
  EXPECT_EQ(core.serializations, 2u);
  EXPECT_EQ(metadataCaptures, 1u);
  if (snapshot)
    worker.Release(snapshot);
  snapshot = worker.TryAcquire();
  ASSERT_NE(snapshot, nullptr);
  EXPECT_EQ(snapshot->memory.get(), address);
  worker.Release(snapshot);
}
