/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscModel.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscModel, AddFirstDiscAutoSelectsFirstIndex)
{
  // Verify adding the very first disc selects it by default.
  CGameClientDiscModel model;
  model.AddDisc("/roms/disc1.chd");

  ASSERT_TRUE(model.HasSelectedDisc());
  ASSERT_TRUE(model.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscModel, RemovedSlotsAreNotSelectable)
{
  // Verify removed slots are tracked but can never be selected as active discs.
  CGameClientDiscModel model;
  model.AddRemovedSlot();

  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsSelectableSlotByIndex(0));
  EXPECT_FALSE(model.SetSelectedDiscByIndex(0));
}

TEST(TestGameClientDiscModel, MarkRemovedSelectedDiscClearsSelection)
{
  // Verify removing the currently selected disc transitions selection to "No disc".
  CGameClientDiscModel model;
  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd");

  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.GetSelectedDiscIndex().has_value());
  EXPECT_TRUE(model.IsRemovedSlotByIndex(1));
}

TEST(TestGameClientDiscModel, EraseBeforeSelectedDiscShiftsSelectedIndex)
{
  // Verify erasing a lower slot keeps the same selected disc by shifting its index.
  CGameClientDiscModel model;
  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd");
  model.AddDisc("/roms/disc3.chd");

  ASSERT_TRUE(model.SetSelectedDiscByIndex(2));
  ASSERT_TRUE(model.EraseDiscByIndex(1));

  ASSERT_TRUE(model.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*model.GetSelectedDiscIndex(), 1U);
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc3.chd");
}

TEST(TestGameClientDiscModel, LabelFallsBackFromCachedLabelToBasename)
{
  // Verify labels prefer cached labels and otherwise use path-derived basename.
  CGameClientDiscModel model;
  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd", "Disc Two");

  EXPECT_EQ(model.GetLabelByIndex(0), "disc1.chd");
  EXPECT_EQ(model.GetLabelByIndex(1), "Disc Two");
}

TEST(TestGameClientDiscModel, DeriveBasenameHandlesUnixAndWindowsPaths)
{
  // Verify basename extraction normalizes both slash styles and trailing separators.
  EXPECT_EQ(CGameClientDiscModel::DeriveBasename("/roms/disc1.chd"), "disc1.chd");
  EXPECT_EQ(CGameClientDiscModel::DeriveBasename("C:\\roms\\disc2.chd"), "disc2.chd");
  EXPECT_EQ(CGameClientDiscModel::DeriveBasename("/roms/subdir/"), "subdir");
}

TEST(TestGameClientDiscModel, SnapshotEqualityIncludesAllMachineDiscState)
{
  CGameClientDiscModel first;
  first.AddDisc("/roms/disc1.chd", "One");
  CGameClientDiscModel second = first;
  EXPECT_TRUE(first == second);
  second.SetEjected(true);
  EXPECT_FALSE(first == second);
  second = first;
  second.SetSelectedNoDisc();
  EXPECT_FALSE(first == second);
  second = first;
  second.AddRemovedSlot();
  EXPECT_FALSE(first == second);
  second = first;
  second.UpdateCachedLabel("/roms/disc1.chd", "Changed");
  EXPECT_FALSE(first == second);
}

TEST(TestGameClientDiscModel, IndexedReplacementReusesSlotAndPreservesOtherSelection)
{
  const std::string oldPath = "/roms/old.chd";
  const std::string newPath = "/roms/new.chd";
  CGameClientDiscModel model;
  model.AddDisc("/roms/first.chd");
  model.AddDisc(oldPath, "Old disc");
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  ASSERT_TRUE(model.SetDiscByIndex(1, newPath, "Replacement"));
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_EQ(model.GetSelectedDiscIndex(), 0U);
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_EQ(model.GetPathByIndex(1), newPath);
  EXPECT_EQ(model.GetLabelByIndex(1), "Replacement");
  EXPECT_EQ(model.GetDiscByIndex(1)->basename, CGameClientDiscModel::DeriveBasename(newPath));
}

TEST(TestGameClientDiscModel, IndexedReplacementPreservesNoDiscAndSelectedSlot)
{
  CGameClientDiscModel model;
  model.AddRemovedSlot();
  model.SetEjected(true);
  ASSERT_TRUE(model.SetDiscByIndex(0, "/roms/first.chd"));
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_TRUE(model.IsEjected());

  ASSERT_TRUE(model.SetSelectedDiscByIndex(0));
  const std::string path = "/roms/replacement.chd";
  ASSERT_TRUE(model.SetDiscByIndex(0, path));
  EXPECT_EQ(model.Size(), 1U);
  EXPECT_EQ(model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(model.GetSelectedDiscPath(), path);
  EXPECT_TRUE(model.IsEjected());
}

TEST(TestGameClientDiscModel, InvalidIndexedReplacementLeavesModelUnchanged)
{
  CGameClientDiscModel model;
  model.AddDisc("/roms/first.chd");
  const CGameClientDiscModel previous = model;
  const std::string newPath = "/roms/new.chd";

  EXPECT_FALSE(model.SetDiscByIndex(model.Size(), newPath));
  EXPECT_FALSE(model.SetDiscByIndex(0, ""));
  EXPECT_TRUE(model == previous);
}
