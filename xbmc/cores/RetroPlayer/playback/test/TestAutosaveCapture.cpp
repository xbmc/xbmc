/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/playback/AutosaveCapture.h"
#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

TEST(TestAutosaveCapture, ReusesCoreStateAndCapturesAchievementsOnMarkedFrame)
{
  CAutosaveCapture capture;
  CDeltaPairMemoryStream stream;
  stream.Init(4, 10);
  auto spare = std::make_unique<uint32_t[]>(1);
  unsigned int serializations = 0;
  unsigned int achievements = 0;
  unsigned int frame = 1;
  auto serialize = [&]()
  {
    ++serializations;
    return true;
  };
  auto metadata = [&]() { achievements = frame; };
  capture.Request();
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = frame;
  ++serializations;
  stream.SubmitFrame();
  EXPECT_FALSE(capture.CaptureFrame(&stream, true, spare, serialize, metadata));
  EXPECT_EQ(achievements, 1u);
  frame = 2;
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = frame;
  ++serializations;
  stream.SubmitFrame();
  EXPECT_TRUE(capture.CaptureFrame(&stream, true, spare, serialize, metadata));
  EXPECT_EQ(spare[0], 1u);
  EXPECT_EQ(achievements, 1u);
  EXPECT_EQ(serializations, 2u);
  EXPECT_FALSE(capture.IsPending());
}

TEST(TestAutosaveCapture, DiscChangeKeepsRetiredMemoryAndMetadataTogether)
{
  CAutosaveCapture capture;
  CDeltaPairMemoryStream stream;
  stream.Init(4, 10);
  auto buffer = std::make_unique<uint32_t[]>(1);
  uint32_t capturedDisc = 0;
  uint64_t capturedFrame = 0;
  auto metadata = [&]
  {
    capturedDisc = stream.GetDiscStateID();
    capturedFrame = stream.GetFrameCounter();
  };
  capture.Request();
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 17;
  stream.SubmitFrame(1, 17);
  EXPECT_FALSE(capture.CaptureFrame(&stream, true, buffer, [] { return false; }, metadata));
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 18;
  stream.SubmitFrame(2, 18);
  ASSERT_TRUE(capture.CaptureFrame(&stream, true, buffer, [] { return false; }, metadata));
  EXPECT_EQ(buffer[0], 17u);
  EXPECT_EQ(capturedDisc, 1u);
  EXPECT_EQ(capturedFrame, 17u);
  EXPECT_EQ(stream.GetDiscStateID(), 2u);
  ASSERT_EQ(stream.RewindFrames(1), 1u);
  EXPECT_EQ(stream.GetDiscStateID(), capturedDisc);
  EXPECT_EQ(stream.GetFrameCounter(), capturedFrame);
  EXPECT_EQ(*reinterpret_cast<const uint32_t*>(stream.CurrentFrame()), buffer[0]);
}

TEST(TestAutosaveCapture, RewindDisabledCoalescesAndSerializesIntoOwnedBuffer)
{
  CAutosaveCapture capture;
  auto buffer = std::make_unique<uint32_t[]>(1);
  const auto* address = buffer.get();
  unsigned int serializations = 0;
  unsigned int achievements = 0;
  auto serialize = [&]()
  {
    ++serializations;
    buffer[0] = 17;
    return true;
  };
  auto metadata = [&]() { achievements = 17; };
  for (unsigned int i = 0; i < 1000; ++i)
    capture.Request();
  EXPECT_TRUE(capture.CaptureFrame(nullptr, false, buffer, serialize, metadata));
  EXPECT_EQ(buffer.get(), address);
  EXPECT_EQ(buffer[0], 17u);
  EXPECT_EQ(achievements, 17u);
  EXPECT_EQ(serializations, 1u);
  EXPECT_FALSE(capture.CaptureFrame(nullptr, false, buffer, serialize, metadata));
  EXPECT_EQ(serializations, 1u);
}

TEST(TestAutosaveCapture, MissingRewindStreamRetriesPinnedCaptureWithDirectSerialization)
{
  CAutosaveCapture capture;
  CDeltaPairMemoryStream stream;
  stream.Init(4, 10);
  auto buffer = std::make_unique<uint32_t[]>(1);
  unsigned int serializations = 0;
  unsigned int metadataCaptures = 0;
  uint32_t frame = 1;
  uint32_t capturedFrame = 0;
  auto serialize = [&]()
  {
    ++serializations;
    buffer[0] = frame;
    return true;
  };
  auto metadata = [&]()
  {
    ++metadataCaptures;
    capturedFrame = frame;
  };

  capture.Request();
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = frame;
  stream.SubmitFrame();
  ASSERT_FALSE(capture.CaptureFrame(&stream, true, buffer, serialize, metadata));
  ASSERT_TRUE(capture.IsPending());
  EXPECT_EQ(serializations, 0u);
  EXPECT_EQ(metadataCaptures, 1u);
  EXPECT_EQ(capturedFrame, 1u);

  frame = 2;
  ASSERT_TRUE(capture.CaptureFrame(nullptr, false, buffer, serialize, metadata));
  EXPECT_EQ(serializations, 1u);
  EXPECT_EQ(buffer[0], 2u);
  EXPECT_EQ(metadataCaptures, 2u);
  EXPECT_EQ(capturedFrame, 2u);
  EXPECT_FALSE(capture.IsPending());
  EXPECT_FALSE(capture.CaptureFrame(nullptr, false, buffer, serialize, metadata));
  EXPECT_EQ(serializations, 1u);
  EXPECT_EQ(metadataCaptures, 2u);
}

TEST(TestAutosaveCapture, FailedSerializationRemainsPending)
{
  CAutosaveCapture capture;
  auto buffer = std::make_unique<uint32_t[]>(1);
  bool captured = false;
  capture.Request();
  EXPECT_FALSE(
      capture.CaptureFrame(nullptr, false, buffer, [] { return false; }, [&] { captured = true; }));
  EXPECT_FALSE(captured);
  EXPECT_TRUE(capture.IsPending());
}

TEST(TestAutosaveCapture, TimelineChangeDoesNotPairRetiredCoreWithOldAchievements)
{
  CAutosaveCapture capture;
  CDeltaPairMemoryStream stream;
  stream.Init(4, 10);
  auto buffer = std::make_unique<uint32_t[]>(1);
  unsigned int achievements = 1;
  capture.Request();
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 1;
  stream.SubmitFrame();
  EXPECT_FALSE(capture.CaptureFrame(&stream, true, buffer, [] { return true; }, [] {}));
  capture.Cancel();
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 10;
  stream.SubmitFrame();
  EXPECT_FALSE(
      capture.CaptureFrame(&stream, true, buffer, [] { return true; }, [&] { achievements = 10; }));
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 11;
  stream.SubmitFrame();
  EXPECT_TRUE(capture.CaptureFrame(&stream, true, buffer, [] { return true; }, [] {}));
  EXPECT_EQ(buffer[0], 10u);
  EXPECT_EQ(achievements, 10u);
}

TEST(TestAutosaveCapture, NoSpareDefersWithoutSerializing)
{
  CAutosaveCapture capture;
  std::unique_ptr<uint32_t[]> unavailable;
  capture.Request();
  EXPECT_FALSE(capture.CaptureFrame(
      nullptr, false, unavailable,
      []
      {
        ADD_FAILURE() << "No storage for serialization";
        return true;
      },
      [] { ADD_FAILURE() << "No core snapshot for achievements"; }));
  EXPECT_TRUE(capture.IsPending());
}

TEST(TestAutosaveCapture, NewRequestWhilePinnedCapturesAnotherFrame)
{
  CAutosaveCapture capture;
  CDeltaPairMemoryStream stream;
  stream.Init(4, 10);
  auto buffer = std::make_unique<uint32_t[]>(1);
  capture.Request();
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 1;
  stream.SubmitFrame();
  EXPECT_FALSE(capture.CaptureFrame(&stream, true, buffer, [] { return true; }, [] {}));
  capture.Request();
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 2;
  stream.SubmitFrame();
  EXPECT_TRUE(capture.CaptureFrame(&stream, true, buffer, [] { return true; }, [] {}));
  EXPECT_EQ(buffer[0], 1u);
  EXPECT_TRUE(capture.IsPending());
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 3;
  stream.SubmitFrame();
  EXPECT_FALSE(capture.CaptureFrame(&stream, true, buffer, [] { return true; }, [] {}));
  *reinterpret_cast<uint32_t*>(stream.BeginFrame()) = 4;
  stream.SubmitFrame();
  EXPECT_TRUE(capture.CaptureFrame(&stream, true, buffer, [] { return true; }, [] {}));
  EXPECT_EQ(buffer[0], 3u);
}

TEST(TestAutosaveCapture, CancellingUnusedStorageDoesNotRequestAutosave)
{
  struct Snapshot
  {
    bool discarded{false};
  };
  CAutosaveCapture capture;
  auto snapshot = std::make_unique<Snapshot>();
  bool ready = false;
  capture.Cancel(snapshot, ready);
  EXPECT_TRUE(snapshot->discarded);
  EXPECT_TRUE(ready);
  EXPECT_FALSE(capture.IsPending());
  capture.Cancel(snapshot, ready);
  EXPECT_FALSE(capture.IsPending());
}

TEST(TestAutosaveCapture, ResetClearsInvalidatedReadyRequest)
{
  struct Snapshot
  {
    bool discarded{false};
  };
  CAutosaveCapture capture;
  auto snapshot = std::make_unique<Snapshot>();
  auto buffer = std::make_unique<uint32_t[]>(1);
  capture.Request();
  bool ready = capture.CaptureFrame(nullptr, false, buffer, [] { return true; }, [] {});
  ASSERT_TRUE(ready);
  capture.Cancel(snapshot, ready);
  capture.Reset();
  EXPECT_FALSE(capture.IsPending());
  EXPECT_FALSE(capture.CaptureFrame(nullptr, false, buffer, [] { return true; }, [] {}));
}
