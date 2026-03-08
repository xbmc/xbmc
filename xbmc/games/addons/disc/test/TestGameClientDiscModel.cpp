/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscModel, AddDiscAndMarkRemovedKeepsSize)
{
  // Verify removing a disc marks a tombstone without shrinking the model
  CGameClientDiscModel model;

  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd");

  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_EQ(model.Size(), 2U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
}

TEST(TestGameClientDiscModel, RemovedSlotClearsPathAndLabel)
{
  // Verify removed slots no longer expose path/label information
  CGameClientDiscModel model;

  model.AddDisc("/roms/game/disc1.iso", "Disc One");
  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_EQ(model.GetPathByIndex(0), "");
  EXPECT_EQ(model.GetLabelByIndex(0), "");
}

TEST(TestGameClientDiscModel, LookupIgnoresRemovedSlots)
{
  // Verify removed slots are ignored by path/basename lookup
  CGameClientDiscModel model;

  model.AddDisc("/roms/game/disc1.iso");
  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_FALSE(model.GetDiscIndexByPath("/roms/game/disc1.iso").has_value());
  EXPECT_FALSE(model.GetDiscIndexByBasename("disc1.iso").has_value());
}

TEST(TestGameClientDiscModel, MultipleRemovedSlotsCanCoexist)
{
  // Verify multiple tombstones can coexist and preserve index slots
  CGameClientDiscModel model;

  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd");
  model.AddDisc("/roms/disc3.chd");

  ASSERT_TRUE(model.MarkRemovedByIndex(0));
  ASSERT_TRUE(model.MarkRemovedByIndex(2));

  EXPECT_EQ(model.Size(), 3U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(model.IsRemovedSlotByIndex(2));
}

TEST(TestGameClientDiscModel, SelectionReplacementSkipsRemovedSlots)
{
  // Verify selected replacement skips removed tombstones
  CGameClientDiscModel model;

  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd");
  model.AddDisc("/roms/disc3.chd");

  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.GetSelectedDiscIndex().has_value());
}

TEST(TestGameClientDiscModel, RemovingOnlySelectedDiscClearsSelection)
{
  CGameClientDiscModel model;

  model.AddDisc("/roms/disc1.chd");
  ASSERT_TRUE(model.SetSelectedDiscByIndex(0));

  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.GetSelectedDiscIndex().has_value());
}

TEST(TestGameClientDiscModel, MixedSlotModelBehavior)
{
  // Verify mixed Disc/RemovedSlot behavior for labels and paths
  CGameClientDiscModel model;

  model.AddDisc("/roms/disc1.chd");
  model.AddRemovedSlot();
  model.AddDisc("/roms/disc3.chd", "Disc 3");
  ASSERT_TRUE(model.MarkRemovedByIndex(2));

  EXPECT_EQ(model.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(model.GetLabelByIndex(0), "disc1.chd");
  EXPECT_EQ(model.GetPathByIndex(1), "");
  EXPECT_EQ(model.GetLabelByIndex(1), "");
  EXPECT_EQ(model.GetPathByIndex(2), "");
  EXPECT_EQ(model.GetLabelByIndex(2), "");
}

TEST(TestGameClientDiscModel, RemovedSlotsAreNotSelectable)
{
  CGameClientDiscModel model;

  model.AddDisc("/roms/disc1.chd");
  model.AddRemovedSlot();

  EXPECT_TRUE(model.IsSelectableSlotByIndex(0));
  EXPECT_FALSE(model.IsSelectableSlotByIndex(1));
}

TEST(TestGameClientDiscModel, ClearResetsEjectedState)
{
  CGameClientDiscModel model;

  model.SetEjected(true);
  ASSERT_TRUE(model.IsEjected());

  model.Clear();

  EXPECT_FALSE(model.IsEjected());
}
