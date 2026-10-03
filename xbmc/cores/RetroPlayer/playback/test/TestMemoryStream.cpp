/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/playback/AutosaveCapture.h"
#include "cores/RetroPlayer/playback/SavestateWorker.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"
#include "filesystem/File.h"
#include "test/TestUtils.h"

#include <array>
#include <cstring>
#include <memory>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

TEST(TestMemoryStream, RetiredSnapshotSurvivesRewindAndBufferReuse)
{
  CDeltaPairMemoryStream stream;
  stream.Init(5, 10);
  const std::array<uint8_t, 5> first{1, 2, 3, 4, 5};
  const std::array<uint8_t, 5> second{9, 8, 7, 6, 5};
  const std::array<uint8_t, 5> third{0, 0, 0, 0, 0};
  std::memcpy(stream.BeginFrame(), first.data(), first.size());
  stream.SubmitFrame();
  const uint8_t* pinned = stream.CurrentFrame();
  std::memcpy(stream.BeginFrame(), second.data(), second.size());
  stream.SubmitFrame();
  auto spare = std::make_unique<uint32_t[]>(2);
  const auto* replacement = spare.get();
  ASSERT_TRUE(stream.ExchangeRetiredFrame(spare));
  EXPECT_EQ(reinterpret_cast<const uint8_t*>(spare.get()), pinned);
  EXPECT_EQ(std::memcmp(spare.get(), first.data(), first.size()), 0);
  EXPECT_EQ(stream.BeginFrame(), reinterpret_cast<const uint8_t*>(replacement));
  std::memcpy(stream.BeginFrame(), third.data(), third.size());
  stream.SubmitFrame();
  ASSERT_EQ(stream.RewindFrames(1), 1u);
  EXPECT_EQ(std::memcmp(stream.CurrentFrame(), second.data(), second.size()), 0);
  ASSERT_EQ(stream.RewindFrames(1), 1u);
  EXPECT_EQ(std::memcmp(stream.CurrentFrame(), first.data(), first.size()), 0);
  EXPECT_EQ(std::memcmp(spare.get(), first.data(), first.size()), 0);
}

TEST(TestMemoryStream, CannotDetachWithoutAReplacementOrRetiredFrame)
{
  CDeltaPairMemoryStream stream;
  stream.Init(4, 10);
  auto spare = std::make_unique<uint32_t[]>(1);
  EXPECT_FALSE(stream.ExchangeRetiredFrame(spare));
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 123;
  stream.SubmitFrame();
  EXPECT_FALSE(stream.ExchangeRetiredFrame(spare));
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 456;
  stream.SubmitFrame();
  std::unique_ptr<uint32_t[]> empty;
  EXPECT_FALSE(stream.ExchangeRetiredFrame(empty));
  EXPECT_EQ(*reinterpret_cast<const uint32_t*>(stream.CurrentFrame()), 456u);
}

TEST(TestMemoryStream, WorkerPersistsDetachedCoreAndAchievementsForReload)
{
  constexpr size_t stateSize = 4099;
  struct Snapshot
  {
    std::unique_ptr<uint32_t[]> memory = std::make_unique<uint32_t[]>(1025);
    std::array<uint8_t, 3> achievements{};
  };
  auto deleteTemp = [](XFILE::CFile* file) { XBMC_DELETETEMPFILE(file); };
  std::unique_ptr<XFILE::CFile, decltype(deleteTemp)> temp(XBMC_CREATETEMPFILE(".sav"), deleteTemp);
  ASSERT_NE(temp, nullptr);
  const auto path = XBMC_TEMPFILEPATH(temp.get());
  temp->Close();
  CSavestateDatabase database;
  CSavestateWorker<Snapshot> worker(
      std::make_unique<Snapshot>(),
      [&](const Snapshot& snapshot)
      {
        auto save = CSavestateDatabase::AllocateSavestate();
        std::memcpy(save->GetMemoryBuffer(stateSize), snapshot.memory.get(), stateSize);
        std::memcpy(save->GetAchievementBuffer(snapshot.achievements.size()),
                    snapshot.achievements.data(), snapshot.achievements.size());
        save->SetType(SAVE_TYPE::AUTO);
        save->SetTimestampFrames(17);
        save->Finalize();
        EXPECT_TRUE(database.AddSavestate(path, "game.rom", *save));
      });
  CDeltaPairMemoryStream stream;
  stream.Init(stateSize, 10);
  CAutosaveCapture capture;
  capture.Request();
  auto snapshot = worker.Acquire();
  std::memset(stream.BeginFrame(), 17, stateSize);
  stream.SubmitFrame();
  EXPECT_FALSE(capture.CaptureFrame(
      &stream, true, snapshot->memory, [] { return false; },
      [&] { snapshot->achievements = {17, 1, 2}; }));
  std::memset(stream.BeginFrame(), 18, stateSize);
  stream.SubmitFrame();
  ASSERT_TRUE(capture.CaptureFrame(&stream, true, snapshot->memory, [] { return false; }, [] {}));
  worker.Submit(snapshot);
  worker.Drain();
  auto loaded = CSavestateDatabase::AllocateSavestate();
  ASSERT_TRUE(database.GetSavestate(path, *loaded));
  ASSERT_TRUE(loaded->PrepareMemoryData(stateSize));
  ASSERT_EQ(loaded->GetMemorySize(), stateSize);
  EXPECT_EQ(loaded->Type(), SAVE_TYPE::AUTO);
  EXPECT_EQ(loaded->TimestampFrames(), 17u);
  for (size_t i = 0; i < stateSize; ++i)
    ASSERT_EQ(loaded->GetMemoryData()[i], 17);
  const std::array<uint8_t, 3> expected{17, 1, 2};
  ASSERT_EQ(loaded->GetAchievementSize(), expected.size());
  EXPECT_EQ(std::memcmp(loaded->GetAchievementData(), expected.data(), expected.size()), 0);
  ASSERT_EQ(stream.RewindFrames(1), 1u);
  EXPECT_EQ(std::memcmp(stream.CurrentFrame(), loaded->GetMemoryData(), stateSize), 0);
}

TEST(TestMemoryStream, PadsByteSizeToOneWordBoundary)
{
  class MemoryStream : public CDeltaPairMemoryStream
  {
  public:
    size_t WordCount() const { return m_paddedFrameSize; }
  } stream;
  stream.Init(4099, 10);
  ASSERT_EQ(stream.WordCount(), 1025u);
  EXPECT_EQ(stream.FrameSize(), 4099u);
  auto* first = stream.BeginFrame();
  std::memset(first, 0x17, 4099);
  EXPECT_EQ(first[4099], 0);
  stream.SubmitFrame();
  auto* second = stream.BeginFrame();
  std::memset(second, 0x28, 4099);
  EXPECT_EQ(second[4099], 0);
  stream.SubmitFrame();
  ASSERT_EQ(stream.RewindFrames(1), 1u);
  for (size_t i = 0; i < 4099; ++i)
    ASSERT_EQ(stream.CurrentFrame()[i], 0x17);
  EXPECT_EQ(stream.CurrentFrame()[4099], 0);
}
