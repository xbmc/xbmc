/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace RETRO;

namespace
{
void Submit(CDeltaPairMemoryStream& stream, const std::vector<uint8_t>& bytes, uint32_t id)
{
  ASSERT_EQ(bytes.size(), stream.FrameSize());
  std::memcpy(stream.BeginFrame(), bytes.data(), bytes.size());
  stream.SubmitFrame(id);
}

void ExpectCurrent(const CDeltaPairMemoryStream& stream,
                   const std::vector<uint8_t>& bytes,
                   uint32_t id,
                   uint64_t counter)
{
  ASSERT_NE(stream.CurrentFrame(), nullptr);
  EXPECT_TRUE(std::equal(bytes.begin(), bytes.end(), stream.CurrentFrame()));
  EXPECT_EQ(stream.GetDiscStateID(), id);
  EXPECT_EQ(stream.GetFrameCounter(), counter);
}
} // unnamed namespace

TEST(TestDeltaPairMemoryStream, RewindAndAdvanceRestoreExactFrameMetadata)
{
  CDeltaPairMemoryStream stream;
  stream.Init(5, 5);
  stream.SetFrameCounter(40);
  Submit(stream, {1, 2, 3, 4, 5}, 11);
  Submit(stream, {6, 7, 8, 9, 10}, 22);
  Submit(stream, {11, 12, 13, 14, 15}, 33);

  EXPECT_EQ(stream.RewindFrames(2), 2U);
  ExpectCurrent(stream, {1, 2, 3, 4, 5}, 11, 40);
  EXPECT_EQ(stream.PastFramesAvailable(), 0U);
  EXPECT_EQ(stream.FutureFramesAvailable(), 2U);
  EXPECT_EQ(stream.AdvanceFrames(1), 1U);
  ExpectCurrent(stream, {6, 7, 8, 9, 10}, 22, 41);
  EXPECT_EQ(stream.AdvanceFrames(5), 1U);
  ExpectCurrent(stream, {11, 12, 13, 14, 15}, 33, 42);
}

TEST(TestDeltaPairMemoryStream, SubmissionAfterRewindAbandonsFuture)
{
  CDeltaPairMemoryStream stream;
  stream.Init(3, 5);
  Submit(stream, {1, 1, 1}, 1);
  Submit(stream, {2, 2, 2}, 2);
  Submit(stream, {3, 3, 3}, 3);
  ASSERT_EQ(stream.RewindFrames(1), 1U);

  Submit(stream, {9, 9, 9}, 9);
  ExpectCurrent(stream, {9, 9, 9}, 9, 2);
  EXPECT_EQ(stream.FutureFramesAvailable(), 0U);
  EXPECT_EQ(stream.AdvanceFrames(1), 0U);
  ASSERT_EQ(stream.RewindFrames(1), 1U);
  ExpectCurrent(stream, {2, 2, 2}, 2, 1);
}

TEST(TestDeltaPairMemoryStream, ShrinkingCapacityPrunesPastThenDistantFuture)
{
  CDeltaPairMemoryStream stream;
  stream.Init(1, 5);
  Submit(stream, {1}, 1);
  Submit(stream, {2}, 2);
  Submit(stream, {3}, 3);
  Submit(stream, {4}, 4);
  Submit(stream, {5}, 5);
  ASSERT_EQ(stream.RewindFrames(3), 3U);

  stream.SetMaxFrameCount(3);
  EXPECT_EQ(stream.PastFramesAvailable(), 0U);
  EXPECT_EQ(stream.FutureFramesAvailable(), 2U);
  EXPECT_EQ(stream.AdvanceFrames(3), 2U);
  ExpectCurrent(stream, {4}, 4, 3);
}

TEST(TestDeltaPairMemoryStream, EvictionAndResetKeepMetadataSynchronized)
{
  CDeltaPairMemoryStream stream;
  stream.Init(1, 2);
  Submit(stream, {1}, 10);
  Submit(stream, {2}, 20);
  Submit(stream, {3}, 30);
  EXPECT_EQ(stream.PastFramesAvailable(), 1U);
  ASSERT_EQ(stream.RewindFrames(2), 1U);
  ExpectCurrent(stream, {2}, 20, 1);

  stream.Reset();
  EXPECT_EQ(stream.CurrentFrame(), nullptr);
  EXPECT_EQ(stream.GetDiscStateID(), 0U);
  EXPECT_EQ(stream.PastFramesAvailable(), 0U);
  EXPECT_EQ(stream.FutureFramesAvailable(), 0U);
}

TEST(TestDeltaPairMemoryStream, ExplicitFrameCountersSurviveSerializationGaps)
{
  CDeltaPairMemoryStream stream;
  stream.Init(1, 3);

  std::memcpy(stream.BeginFrame(), std::vector<uint8_t>{1}.data(), 1);
  stream.SubmitFrame(10, 1001);
  ExpectCurrent(stream, {1}, 10, 1001);

  std::memcpy(stream.BeginFrame(), std::vector<uint8_t>{5}.data(), 1);
  stream.SubmitFrame(50, 1005);
  ExpectCurrent(stream, {5}, 50, 1005);

  ASSERT_EQ(stream.RewindFrames(1), 1U);
  ExpectCurrent(stream, {1}, 10, 1001);
  ASSERT_EQ(stream.AdvanceFrames(1), 1U);
  ExpectCurrent(stream, {5}, 50, 1005);
}
