/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscMergeUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{

bool IsRealDiscByIndex(const CGameClientDiscModel& model, size_t index)
{
  // Treat a "real" disc as a selectable slot with a non-empty path identity.
  // This intentionally flags suspicious cases where a slot is marked as Disc
  // but has no path.
  return model.IsSelectableSlotByIndex(index) && !model.GetPathByIndex(index).empty();
}

} // namespace

TEST(TestGameClientDiscMergeUtils, StartupEmptyCoreMetadataKeepsFrontendDiscIdentityAndSelection)
{
  // Startup symptom: frontend already knows disc 1, but the core reports a
  // selected index before reporting that slot's path/label metadata.
  //
  // Expected behavior: keep the frontend disc identity for that slot so the
  // UI does not lose the disc, and keep selection on index 0.
  CGameClientDiscModel frontendDiscs;
  frontendDiscs.AddDisc("/roms/disc1.chd", "Frontend Disc One");
  ASSERT_TRUE(frontendDiscs.SetSelectedDiscByIndex(0));

  CGameClientDiscModel coreDiscs;
  coreDiscs.AddDisc("", "");
  ASSERT_TRUE(coreDiscs.SetSelectedDiscByIndex(0));

  const CGameClientDiscModel mergedDiscs =
      CGameClientDiscMergeUtils::ReconcileModels(frontendDiscs, coreDiscs);

  ASSERT_EQ(mergedDiscs.Size(), 1U);
  EXPECT_TRUE(IsRealDiscByIndex(mergedDiscs, 0));
  EXPECT_FALSE(mergedDiscs.IsRemovedSlotByIndex(0));
  EXPECT_EQ(mergedDiscs.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(mergedDiscs.GetLabelByIndex(0), "Frontend Disc One");

  EXPECT_FALSE(mergedDiscs.IsSelectedNoDisc());
  ASSERT_TRUE(mergedDiscs.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*mergedDiscs.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(mergedDiscs.GetSelectedDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscMergeUtils, StartupCorePathWithEmptyLabelShouldPreserveFrontendLabelByPath)
{
  // Startup symptom: core reports path first, but label arrives later.
  //
  // Intended behavior from in-code comments is to preserve frontend label by
  // path when available, so the UI keeps a stable, user-friendly identity.
  CGameClientDiscModel frontendDiscs;
  frontendDiscs.AddDisc("/roms/disc1.chd", "Disc One Friendly Name");

  CGameClientDiscModel coreDiscs;
  coreDiscs.AddDisc("/roms/disc1.chd", "");
  ASSERT_TRUE(coreDiscs.SetSelectedDiscByIndex(0));

  const CGameClientDiscModel mergedDiscs =
      CGameClientDiscMergeUtils::ReconcileModels(frontendDiscs, coreDiscs);

  ASSERT_EQ(mergedDiscs.Size(), 1U);
  EXPECT_TRUE(IsRealDiscByIndex(mergedDiscs, 0));
  EXPECT_FALSE(mergedDiscs.IsRemovedSlotByIndex(0));
  EXPECT_EQ(mergedDiscs.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(mergedDiscs.GetLabelByIndex(0), "Disc One Friendly Name");

  EXPECT_FALSE(mergedDiscs.IsSelectedNoDisc());
  ASSERT_TRUE(mergedDiscs.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*mergedDiscs.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(mergedDiscs.GetSelectedDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscMergeUtils, ZombieEmptyCoreSlotRespectsFrontendTombstone)
{
  // Zombie-slot symptom: core keeps an empty slot after removal and frontend
  // already has a tombstone at the same index.
  //
  // Expected behavior: keep the removed slot instead of creating a fake disc.
  CGameClientDiscModel frontendDiscs;
  frontendDiscs.AddRemovedSlot();

  CGameClientDiscModel coreDiscs;
  coreDiscs.AddDisc("", "");
  coreDiscs.SetSelectedNoDisc();

  const CGameClientDiscModel mergedDiscs =
      CGameClientDiscMergeUtils::ReconcileModels(frontendDiscs, coreDiscs);

  ASSERT_EQ(mergedDiscs.Size(), 1U);
  EXPECT_FALSE(IsRealDiscByIndex(mergedDiscs, 0));
  EXPECT_TRUE(mergedDiscs.IsRemovedSlotByIndex(0));
  EXPECT_EQ(mergedDiscs.GetPathByIndex(0), "");
  EXPECT_EQ(mergedDiscs.GetLabelByIndex(0), "");

  EXPECT_TRUE(mergedDiscs.IsSelectedNoDisc());
  EXPECT_FALSE(mergedDiscs.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(mergedDiscs.GetSelectedDiscPath(), "");
}

TEST(TestGameClientDiscMergeUtils, CompactingCoreAfterRemovalDoesNotCreateBogusEmptyPathDisc)
{
  // Compacting-core symptom: frontend has [disc1, tombstone], but core has
  // compacted to [disc1] after removal.
  //
  // Expected behavior: merged model follows core size and does not keep/create
  // a bogus real-disc entry with empty path.
  CGameClientDiscModel frontendDiscs;
  frontendDiscs.AddDisc("/roms/disc1.chd", "Disc 1");
  frontendDiscs.AddRemovedSlot();
  ASSERT_TRUE(frontendDiscs.SetSelectedDiscByIndex(0));

  CGameClientDiscModel coreDiscs;
  coreDiscs.AddDisc("/roms/disc1.chd", "Disc 1");
  ASSERT_TRUE(coreDiscs.SetSelectedDiscByIndex(0));

  const CGameClientDiscModel mergedDiscs =
      CGameClientDiscMergeUtils::ReconcileModels(frontendDiscs, coreDiscs);

  ASSERT_EQ(mergedDiscs.Size(), 1U);
  EXPECT_TRUE(IsRealDiscByIndex(mergedDiscs, 0));
  EXPECT_FALSE(mergedDiscs.IsRemovedSlotByIndex(0));
  EXPECT_EQ(mergedDiscs.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(mergedDiscs.GetLabelByIndex(0), "Disc 1");

  EXPECT_FALSE(mergedDiscs.IsSelectedNoDisc());
  ASSERT_TRUE(mergedDiscs.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*mergedDiscs.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(mergedDiscs.GetSelectedDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscMergeUtils, InvalidCoreSelectionIndexCanUnexpectedlyForceNoDisc)
{
  // Stress case: core reports a selected index that cannot be selected in the
  // merged model (index points to a removed slot).
  //
  // This verifies whether selection becomes "No disc" unexpectedly, and catches
  // any accidental creation of a bogus selectable disc with empty path.
  CGameClientDiscModel frontendDiscs;
  frontendDiscs.AddRemovedSlot();

  CGameClientDiscModel coreDiscs;
  coreDiscs.AddDisc("", "");
  ASSERT_TRUE(coreDiscs.SetSelectedDiscByIndex(0));

  const CGameClientDiscModel mergedDiscs =
      CGameClientDiscMergeUtils::ReconcileModels(frontendDiscs, coreDiscs);

  ASSERT_EQ(mergedDiscs.Size(), 1U);
  EXPECT_FALSE(IsRealDiscByIndex(mergedDiscs, 0));
  EXPECT_TRUE(mergedDiscs.IsRemovedSlotByIndex(0));
  EXPECT_EQ(mergedDiscs.GetPathByIndex(0), "");
  EXPECT_EQ(mergedDiscs.GetLabelByIndex(0), "");

  EXPECT_TRUE(mergedDiscs.IsSelectedNoDisc());
  EXPECT_FALSE(mergedDiscs.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(mergedDiscs.GetSelectedDiscPath(), "");
}
