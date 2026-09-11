/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/playback/DiscStateHistory.h"
#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"
#include "games/addons/disc/GameClientDiscModel.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace RETRO;

namespace
{
GAME::CGameClientDiscModel MakeModel()
{
  GAME::CGameClientDiscModel model;
  model.AddDisc("/games/disc1.chd", "Disc One");
  model.AddRemovedSlot();
  model.AddDisc("/games/disc2.chd", "Disc Two");
  model.SetSelectedDiscByIndex(2);
  model.SetEjected(true);
  return model;
}

struct TimelineFrame
{
  std::vector<uint8_t> memory;
  GAME::CGameClientDiscModel model;
};

void Submit(CDeltaPairMemoryStream& stream, CDiscStateHistory& history, const TimelineFrame& frame)
{
  ASSERT_EQ(frame.memory.size(), stream.FrameSize());
  std::memcpy(stream.BeginFrame(), frame.memory.data(), frame.memory.size());
  stream.SubmitFrame(history.Intern(frame.model));
}

void ExpectCurrent(const CDeltaPairMemoryStream& stream,
                   const CDiscStateHistory& history,
                   const TimelineFrame& expected)
{
  ASSERT_NE(stream.CurrentFrame(), nullptr);
  EXPECT_TRUE(std::equal(expected.memory.begin(), expected.memory.end(), stream.CurrentFrame()));

  const GAME::CGameClientDiscModel* restored = history.Get(stream.GetDiscStateID());
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(*restored, expected.model);
  EXPECT_EQ(restored->GetDiscs(), expected.model.GetDiscs());
  EXPECT_EQ(restored->GetSelectedDiscIndex(), expected.model.GetSelectedDiscIndex());
  EXPECT_EQ(restored->IsSelectedNoDisc(), expected.model.IsSelectedNoDisc());
  EXPECT_EQ(restored->IsEjected(), expected.model.IsEjected());
}

GAME::CGameClientDiscModel TwoDiscModel(size_t selected, bool ejected)
{
  GAME::CGameClientDiscModel model;
  model.AddDisc("/mgs/Metal Gear Solid (USA) (Disc 1).chd", "MGS Disc One");
  model.AddDisc("/mgs/Metal Gear Solid (USA) (Disc 2).chd", "MGS Disc Two");
  model.SetSelectedDiscByIndex(selected);
  model.SetEjected(ejected);
  return model;
}
} // unnamed namespace

TEST(TestDiscStateHistory, EqualStatesReuseImmutableSnapshot)
{
  CDiscStateHistory history;
  const GAME::CGameClientDiscModel model = MakeModel();
  const uint32_t firstId = history.Intern(model);

  EXPECT_NE(firstId, 0U);
  EXPECT_EQ(history.Intern(model), firstId);
  ASSERT_NE(history.Get(firstId), nullptr);
  EXPECT_EQ(*history.Get(firstId), model);
}

TEST(TestDiscStateHistory, DiscTopologySelectionAndTrayProduceDistinctStates)
{
  CDiscStateHistory history;
  const GAME::CGameClientDiscModel original = MakeModel();
  const uint32_t originalId = history.Intern(original);
  GAME::CGameClientDiscModel selection = original;
  selection.SetSelectedNoDisc();
  GAME::CGameClientDiscModel tray = original;
  tray.SetEjected(false);
  GAME::CGameClientDiscModel playlist = original;
  playlist.EraseDiscByIndex(1);

  EXPECT_NE(history.Intern(selection), originalId);
  EXPECT_NE(history.Intern(tray), originalId);
  EXPECT_NE(history.Intern(playlist), originalId);
}

TEST(TestDiscStateHistory, SnapshotsRemainImmutableAndClearInvalidatesIds)
{
  CDiscStateHistory history;
  GAME::CGameClientDiscModel model = MakeModel();
  const uint32_t id = history.Intern(model);
  model.Clear();

  ASSERT_NE(history.Get(id), nullptr);
  EXPECT_EQ(history.Get(id)->Size(), 3U);
  EXPECT_EQ(history.Get(0), nullptr);
  EXPECT_EQ(history.Get(999), nullptr);
  history.Clear();
  EXPECT_EQ(history.Get(id), nullptr);
}

TEST(TestDiscStateTimeline, RewindAndAdvanceRestoreMemoryAndFullDiscModels)
{
  CDiscStateHistory history;
  CDeltaPairMemoryStream stream;
  stream.Init(5, 10);

  GAME::CGameClientDiscModel disc1 = TwoDiscModel(0, false);
  GAME::CGameClientDiscModel disc2 = TwoDiscModel(1, false);
  GAME::CGameClientDiscModel ejected = disc2;
  ejected.SetEjected(true);
  GAME::CGameClientDiscModel noDisc = ejected;
  noDisc.SetSelectedNoDisc();
  GAME::CGameClientDiscModel added = disc2;
  added.AddDisc("/mgs/bonus.chd", "Bonus Disc");
  added.SetSelectedDiscByIndex(2);
  GAME::CGameClientDiscModel removed = added;
  removed.MarkRemovedByIndex(1);
  GAME::CGameClientDiscModel reordered;
  reordered.SetDiscs({removed.GetDiscs()[2], removed.GetDiscs()[1], removed.GetDiscs()[0]});
  reordered.SetSelectedDiscByIndex(2);
  GAME::CGameClientDiscModel reused;
  reused.SetDiscs(
      {reordered.GetDiscs()[0],
       {GAME::GameClientDiscEntry::DiscSlotType::Disc, "/mgs/vr.chd", "vr.chd", "VR Missions"},
       reordered.GetDiscs()[2]});
  reused.SetSelectedDiscByIndex(1);

  const std::vector<TimelineFrame> frames{{{1, 1, 1, 1, 1}, disc1},     {{2, 2, 2, 2, 2}, disc2},
                                          {{3, 3, 3, 3, 3}, ejected},   {{4, 4, 4, 4, 4}, noDisc},
                                          {{5, 5, 5, 5, 5}, added},     {{6, 6, 6, 6, 6}, removed},
                                          {{7, 7, 7, 7, 7}, reordered}, {{8, 8, 8, 8, 8}, reused}};

  for (const TimelineFrame& frame : frames)
    Submit(stream, history, frame);

  for (size_t index = frames.size() - 1; index > 0; --index)
  {
    ASSERT_EQ(stream.RewindFrames(1), 1U);
    ExpectCurrent(stream, history, frames[index - 1]);
  }
  for (size_t index = 1; index < frames.size(); ++index)
  {
    ASSERT_EQ(stream.AdvanceFrames(1), 1U);
    ExpectCurrent(stream, history, frames[index]);
  }
}

TEST(TestDiscStateTimeline, BranchingCannotRestoreAbandonedDiscState)
{
  CDiscStateHistory history;
  CDeltaPairMemoryStream stream;
  stream.Init(3, 5);
  const TimelineFrame disc1{{1, 1, 1}, TwoDiscModel(0, false)};
  const TimelineFrame disc2{{2, 2, 2}, TwoDiscModel(1, false)};
  GAME::CGameClientDiscModel oldFutureModel = disc2.model;
  oldFutureModel.AddDisc("/mgs/old-future.chd", "Old Future");
  const TimelineFrame oldFuture{{3, 3, 3}, oldFutureModel};

  Submit(stream, history, disc1);
  Submit(stream, history, disc2);
  Submit(stream, history, oldFuture);
  ASSERT_EQ(stream.RewindFrames(2), 2U);
  ExpectCurrent(stream, history, disc1);

  GAME::CGameClientDiscModel branchModel = disc1.model;
  branchModel.MarkRemovedByIndex(1);
  branchModel.AddDisc("/mgs/new-branch.chd", "New Branch");
  branchModel.SetSelectedDiscByIndex(2);
  const TimelineFrame branch{{9, 9, 9}, branchModel};
  Submit(stream, history, branch);

  EXPECT_EQ(stream.FutureFramesAvailable(), 0U);
  ExpectCurrent(stream, history, branch);
  ASSERT_EQ(stream.RewindFrames(1), 1U);
  ExpectCurrent(stream, history, disc1);
  ASSERT_EQ(stream.AdvanceFrames(1), 1U);
  ExpectCurrent(stream, history, branch);
}

TEST(TestDiscStateTimeline, CapacityEvictionAndResetKeepModelLookupSynchronized)
{
  CDiscStateHistory history;
  CDeltaPairMemoryStream stream;
  stream.Init(1, 3);
  const TimelineFrame disc1{{1}, TwoDiscModel(0, false)};
  const TimelineFrame ejected{{2}, TwoDiscModel(0, true)};
  const TimelineFrame disc2{{3}, TwoDiscModel(1, false)};
  GAME::CGameClientDiscModel noDiscModel = disc2.model;
  noDiscModel.SetSelectedNoDisc();
  const TimelineFrame noDisc{{4}, noDiscModel};

  Submit(stream, history, disc1);
  Submit(stream, history, ejected);
  Submit(stream, history, disc2);
  Submit(stream, history, noDisc);
  EXPECT_EQ(stream.PastFramesAvailable(), 2U);
  ASSERT_EQ(stream.RewindFrames(2), 2U);
  ExpectCurrent(stream, history, ejected);
  ASSERT_EQ(stream.AdvanceFrames(2), 2U);
  ExpectCurrent(stream, history, noDisc);

  stream.Reset();
  EXPECT_EQ(stream.GetDiscStateID(), 0U);
  EXPECT_EQ(history.Get(stream.GetDiscStateID()), nullptr);
  EXPECT_EQ(stream.PastFramesAvailable(), 0U);
  EXPECT_EQ(stream.FutureFramesAvailable(), 0U);
}
