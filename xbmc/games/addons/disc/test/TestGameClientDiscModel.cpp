/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

class TestGameClientDiscModel : public testing::Test
{
protected:
  std::string CreateDisc()
  {
    XFILE::CFile* file = XBMC_CREATETEMPFILE(".chd");
    EXPECT_NE(file, nullptr);
    if (!file)
      return {};
    file->Close();
    m_files.push_back(file);
    return XBMC_TEMPFILEPATH(file);
  }

  std::string CreateDiscWithSameName(const std::string& path)
  {
    const std::string directory = CreateDisc() + ".dir";
    EXPECT_TRUE(XFILE::CDirectory::Create(directory));
    m_directories.push_back(directory);
    const std::string otherPath =
        URIUtils::AddFileToFolder(directory, CGameClientDiscModel::DeriveBasename(path));
    XFILE::CFile file;
    EXPECT_TRUE(file.OpenForWrite(otherPath, true));
    file.Close();
    m_additionalFiles.push_back(otherPath);
    return otherPath;
  }

  void TearDown() override
  {
    for (const std::string& path : m_additionalFiles)
      XFILE::CFile::Delete(path);
    for (const std::string& directory : m_directories)
      EXPECT_TRUE(XFILE::CDirectory::Remove(directory));
    for (XFILE::CFile* file : m_files)
      XBMC_DELETETEMPFILE(file);
  }

private:
  std::vector<XFILE::CFile*> m_files;
  std::vector<std::string> m_additionalFiles;
  std::vector<std::string> m_directories;
};

TEST_F(TestGameClientDiscModel, AddFirstDiscAutoSelectsFirstIndex)
{
  CGameClientDiscModel model;
  model.AddDisc("/roms/disc1.chd");

  ASSERT_TRUE(model.HasSelectedDisc());
  ASSERT_TRUE(model.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc1.chd");
}

TEST_F(TestGameClientDiscModel, RemovedSlotsAreNotSelectable)
{
  CGameClientDiscModel model;
  model.AddRemovedSlot();

  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsSelectableSlotByIndex(0));
  EXPECT_FALSE(model.SetSelectedDiscByIndex(0));
}

TEST_F(TestGameClientDiscModel, MarkRemovedSelectedDiscClearsSelection)
{
  CGameClientDiscModel model;
  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd");

  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.GetSelectedDiscIndex().has_value());
  EXPECT_TRUE(model.IsRemovedSlotByIndex(1));
}

TEST_F(TestGameClientDiscModel, EraseBeforeSelectedDiscShiftsSelectedIndex)
{
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

TEST_F(TestGameClientDiscModel, LabelFallsBackFromCachedLabelToBasename)
{
  CGameClientDiscModel model;
  model.AddDisc("/roms/disc1.chd");
  model.AddDisc("/roms/disc2.chd", "Disc Two");

  EXPECT_EQ(model.GetLabelByIndex(0), "disc1.chd");
  EXPECT_EQ(model.GetLabelByIndex(1), "Disc Two");
}

TEST_F(TestGameClientDiscModel, DeriveBasenameHandlesUnixAndWindowsPaths)
{
  EXPECT_EQ(CGameClientDiscModel::DeriveBasename("/roms/disc1.chd"), "disc1.chd");
  EXPECT_EQ(CGameClientDiscModel::DeriveBasename("C:\\roms\\disc2.chd"), "disc2.chd");
  EXPECT_EQ(CGameClientDiscModel::DeriveBasename("/roms/subdir/"), "subdir");
}

TEST_F(TestGameClientDiscModel, SavestateResolvesHistoricalOrderAndSelectionByFilename)
{
  const std::string disc1 = CreateDisc();
  const std::string disc2 = CreateDisc();
  CGameClientDiscModel saved;
  saved.AddDisc("/old/" + CGameClientDiscModel::DeriveBasename(disc1), "One");
  saved.AddRemovedSlot();
  saved.AddDisc("/old/" + CGameClientDiscModel::DeriveBasename(disc2), "Two");
  ASSERT_TRUE(saved.SetSelectedDiscByIndex(2));
  saved.SetEjected(true);

  const auto state = saved.GetState();
  EXPECT_EQ(state.slots[0].fileName, CGameClientDiscModel::DeriveBasename(disc1));
  EXPECT_EQ(state.slots[2].label, "Two");
  CGameClientDiscModel current;
  current.AddDisc(disc2, "New label");
  current.AddDisc(CreateDisc());
  current.AddDisc(disc1);
  CGameClientDiscModel restored;
  ASSERT_TRUE(current.ResolveState(state, restored));
  EXPECT_EQ(restored.Size(), 3U);
  EXPECT_EQ(restored.GetPathByIndex(0), disc1);
  EXPECT_TRUE(restored.IsRemovedSlotByIndex(1));
  EXPECT_EQ(restored.GetSelectedDiscIndex(), 2U);
  EXPECT_EQ(restored.GetSelectedDiscPath(), disc2);
  EXPECT_EQ(restored.GetSelectedDiscLabel(), "Two");
  EXPECT_TRUE(restored.IsEjected());
}

TEST_F(TestGameClientDiscModel, SavestateNeverSubstitutesReusedSlotOrAmbiguousFilename)
{
  CGameClientDiscModel saved;
  saved.AddDisc("/old/disc2.chd", "Two");
  const auto state = saved.GetState();
  CGameClientDiscModel current;
  current.AddDisc("/new/disc1.chd", "Two");
  CGameClientDiscModel output = current;
  EXPECT_FALSE(current.ResolveState(state, output));
  EXPECT_TRUE(output == current);

  current.AddDisc("/a/disc2.chd", "Two");
  current.AddDisc("/b/disc2.chd", "Other label");
  EXPECT_FALSE(current.ResolveState(state, output));
  EXPECT_EQ(output.Size(), 1U);
}

TEST_F(TestGameClientDiscModel, SavestateNoDiscAndTrayAreIndependent)
{
  CGameClientDiscModel saved;
  const std::string path = CreateDisc();
  saved.AddDisc(path);
  saved.SetSelectedNoDisc();
  for (const bool ejected : {false, true})
  {
    saved.SetEjected(ejected);
    CGameClientDiscModel restored;
    ASSERT_TRUE(saved.ResolveState(saved.GetState(), restored));
    EXPECT_TRUE(restored.IsSelectedNoDisc());
    EXPECT_EQ(restored.IsEjected(), ejected);
    EXPECT_EQ(saved.GetState().slots[0].fileName, CGameClientDiscModel::DeriveBasename(path));
  }
}

TEST_F(TestGameClientDiscModel, SavestateRejectsInvalidSlotsAndSelection)
{
  CGameClientDiscModel model;
  model.AddDisc(CreateDisc());
  model.AddRemovedSlot();
  auto state = model.GetState();
  CGameClientDiscModel restored;
  state.selectedSlot = 1;
  EXPECT_FALSE(model.ResolveState(state, restored));
  state.selectedSlot = -2;
  EXPECT_FALSE(model.ResolveState(state, restored));
  state.selectedSlot = 20;
  EXPECT_FALSE(model.ResolveState(state, restored));
  state.selectedSlot = -1;
  state.slots[0].type = DiscSlotType::Unknown;
  EXPECT_FALSE(model.ResolveState(state, restored));
  state.slots[0].type = DiscSlotType::Disc;
  state.slots[0].fileName = "/roms/disc1.chd";
  EXPECT_FALSE(model.ResolveState(state, restored));
}

TEST_F(TestGameClientDiscModel, SnapshotEqualityIncludesAllMachineDiscState)
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

TEST_F(TestGameClientDiscModel, RemovedDiscStillResolvesItsHistoricalSlot)
{
  const std::string path = CreateDisc();
  CGameClientDiscModel model;
  model.AddDisc(CreateDisc());
  model.AddRemovedSlot();
  model.AddDisc(path, "Second disc");
  ASSERT_TRUE(model.SetSelectedDiscByIndex(2));
  model.SetEjected(true);
  const auto saved = model.GetState();

  ASSERT_TRUE(model.MarkRemovedByIndex(2));
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.SetSelectedDiscByIndex(2));
  EXPECT_FALSE(model.GetDiscIndexByPath(path).has_value());
  EXPECT_TRUE(model.GetPathByIndex(2).empty());
  EXPECT_TRUE(model.GetLabelByIndex(2).empty());
  EXPECT_EQ(model.GetState().slots[2].type, DiscSlotType::Removed);
  EXPECT_TRUE(model.GetState().slots[2].fileName.empty());

  CGameClientDiscModel restored;
  ASSERT_TRUE(model.ResolveState(saved, restored));
  EXPECT_EQ(restored.Size(), 3U);
  EXPECT_TRUE(restored.IsRemovedSlotByIndex(1));
  EXPECT_EQ(restored.GetSelectedDiscIndex(), 2U);
  EXPECT_EQ(restored.GetSelectedDiscPath(), path);
  EXPECT_EQ(restored.GetSelectedDiscLabel(), "Second disc");
  EXPECT_TRUE(restored.IsEjected());
  EXPECT_TRUE(model.IsRemovedSlotByIndex(2));
}

TEST_F(TestGameClientDiscModel, ErasingEverySlotKeepsHistoricalMedia)
{
  const std::string firstPath = CreateDisc();
  const std::string secondPath = CreateDisc();
  CGameClientDiscModel model;
  model.AddDisc(firstPath);
  model.AddDisc(secondPath);
  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
  const auto saved = model.GetState();

  ASSERT_TRUE(model.EraseDiscByIndex(0));
  ASSERT_TRUE(model.EraseDiscByIndex(0));
  EXPECT_TRUE(model.Empty());
  EXPECT_TRUE(model.GetState().slots.empty());
  EXPECT_TRUE(model.IsSelectedNoDisc());

  CGameClientDiscModel restored;
  ASSERT_TRUE(model.ResolveState(saved, restored));
  EXPECT_EQ(restored.Size(), 2U);
  EXPECT_EQ(restored.GetPathByIndex(0), firstPath);
  EXPECT_EQ(restored.GetSelectedDiscPath(), secondPath);
  EXPECT_EQ(restored.GetSelectedDiscIndex(), 1U);
}

TEST_F(TestGameClientDiscModel, ReplacingRemovedSlotKeepsPreviousMediaIdentity)
{
  const std::string oldPath = CreateDisc();
  const std::string newPath = CreateDisc();
  CGameClientDiscModel model;
  model.AddDisc(oldPath, "Old disc");
  const auto saved = model.GetState();
  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  model.SetDiscs({{GameClientDiscEntry::DiscSlotType::Disc, newPath,
                   CGameClientDiscModel::DeriveBasename(newPath), "New disc"}});
  EXPECT_EQ(model.Size(), 1U);
  EXPECT_EQ(model.GetPathByIndex(0), newPath);
  EXPECT_FALSE(model.GetDiscIndexByPath(oldPath).has_value());

  CGameClientDiscModel restored;
  ASSERT_TRUE(model.ResolveState(saved, restored));
  EXPECT_EQ(restored.GetSelectedDiscPath(), oldPath);
  EXPECT_EQ(restored.GetSelectedDiscLabel(), "Old disc");
  EXPECT_EQ(model.GetPathByIndex(0), newPath);
}

TEST_F(TestGameClientDiscModel, ReplacingTheListWithEmptyKeepsHistoricalMedia)
{
  const std::string path = CreateDisc();
  CGameClientDiscModel model;
  model.SetDiscs({{GameClientDiscEntry::DiscSlotType::Disc,
                   path,
                   CGameClientDiscModel::DeriveBasename(path),
                   {}}});
  const auto saved = model.GetState();
  model.SetDiscs({});
  EXPECT_TRUE(model.Empty());

  CGameClientDiscModel restored;
  ASSERT_TRUE(model.ResolveState(saved, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), path);
}

TEST_F(TestGameClientDiscModel, ResolvingEmptyStateKeepsHistoricalMedia)
{
  const std::string path = CreateDisc();
  CGameClientDiscModel model;
  model.AddDisc(path);
  const auto saved = model.GetState();

  CGameClientDiscModel empty;
  ASSERT_TRUE(model.ResolveState(GameClientDiscState{}, empty));
  EXPECT_TRUE(empty.Empty());
  EXPECT_TRUE(empty.IsSelectedNoDisc());
  CGameClientDiscModel restored;
  ASSERT_TRUE(empty.ResolveState(saved, restored));
  EXPECT_EQ(restored.GetSelectedDiscPath(), path);
}

TEST_F(TestGameClientDiscModel, DeletedCurrentDiscCannotResolveAndLeavesOutputUnchanged)
{
  const std::string path = CreateDisc();
  CGameClientDiscModel model;
  model.AddDisc(path);
  const auto saved = model.GetState();
  ASSERT_TRUE(XFILE::CFile::Delete(path));

  CGameClientDiscModel output;
  output.AddDisc(CreateDisc());
  output.SetEjected(true);
  const CGameClientDiscModel previous = output;
  EXPECT_FALSE(model.ResolveState(saved, output));
  EXPECT_TRUE(output == previous);
}

TEST_F(TestGameClientDiscModel, RemovedMissingCandidateDoesNotDisambiguateSavedFilename)
{
  const std::string firstPath = CreateDisc();
  const std::string secondPath = CreateDiscWithSameName(firstPath);
  CGameClientDiscModel model;
  model.AddDisc(firstPath);
  const auto saved = model.GetState();
  model.AddDisc(secondPath);
  ASSERT_TRUE(model.MarkRemovedByIndex(1));
  ASSERT_TRUE(XFILE::CFile::Delete(secondPath));

  CGameClientDiscModel output;
  output.AddDisc(CreateDisc());
  const CGameClientDiscModel previous = output;
  EXPECT_FALSE(model.ResolveState(saved, output));
  EXPECT_TRUE(output == previous);
}

TEST_F(TestGameClientDiscModel, HistoricalMediaDoesNotChangeActiveModelEquality)
{
  CGameClientDiscModel current;
  current.AddDisc(CreateDisc());
  CGameClientDiscModel withHistory = current;
  withHistory.AddDisc(CreateDisc());
  const auto saved = withHistory.GetState();
  ASSERT_TRUE(withHistory.EraseDiscByIndex(1));
  EXPECT_TRUE(current == withHistory);

  CGameClientDiscModel restored;
  ASSERT_TRUE(withHistory.ResolveState(saved, restored));
  EXPECT_EQ(restored.Size(), 2U);
}

TEST_F(TestGameClientDiscModel, ClearForgetsPreviousSessionMedia)
{
  CGameClientDiscModel model;
  model.AddDisc(CreateDisc());
  const auto saved = model.GetState();
  model.Clear();

  CGameClientDiscModel restored;
  EXPECT_FALSE(model.ResolveState(saved, restored));
  EXPECT_TRUE(model.Empty());
}

TEST_F(TestGameClientDiscModel, IndexedReplacementReusesSlotAndPreservesOtherSelection)
{
  const std::string oldPath = CreateDisc();
  const std::string newPath = CreateDisc();
  CGameClientDiscModel model;
  model.AddDisc(CreateDisc());
  model.AddDisc(oldPath, "Old disc");
  const auto saved = model.GetState();
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  ASSERT_TRUE(model.SetDiscByIndex(1, newPath, "Replacement"));
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_EQ(model.GetSelectedDiscIndex(), 0U);
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_EQ(model.GetPathByIndex(1), newPath);
  EXPECT_EQ(model.GetLabelByIndex(1), "Replacement");
  EXPECT_EQ(model.GetDiscByIndex(1)->basename, CGameClientDiscModel::DeriveBasename(newPath));

  CGameClientDiscModel restored;
  ASSERT_TRUE(model.ResolveState(saved, restored));
  EXPECT_EQ(restored.GetPathByIndex(1), oldPath);
}

TEST_F(TestGameClientDiscModel, IndexedReplacementPreservesNoDiscAndSelectedSlot)
{
  CGameClientDiscModel model;
  model.AddRemovedSlot();
  model.SetEjected(true);
  ASSERT_TRUE(model.SetDiscByIndex(0, CreateDisc()));
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_TRUE(model.IsEjected());

  ASSERT_TRUE(model.SetSelectedDiscByIndex(0));
  const std::string path = CreateDisc();
  ASSERT_TRUE(model.SetDiscByIndex(0, path));
  EXPECT_EQ(model.Size(), 1U);
  EXPECT_EQ(model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(model.GetSelectedDiscPath(), path);
  EXPECT_TRUE(model.IsEjected());
}

TEST_F(TestGameClientDiscModel, InvalidIndexedReplacementLeavesModelAndKnownPathsUnchanged)
{
  CGameClientDiscModel model;
  model.AddDisc(CreateDisc());
  const CGameClientDiscModel previous = model;
  const std::string newPath = CreateDisc();

  EXPECT_FALSE(model.SetDiscByIndex(model.Size(), newPath));
  EXPECT_FALSE(model.SetDiscByIndex(0, ""));
  EXPECT_TRUE(model == previous);
  EXPECT_EQ(model.GetKnownDiscPaths(), previous.GetKnownDiscPaths());
}

TEST_F(TestGameClientDiscModel, RememberingMediaDoesNotAddSlotsOrDuplicatePaths)
{
  const std::string firstPath = CreateDisc();
  const std::string secondPath = CreateDisc();
  CGameClientDiscModel model;
  model.RememberDiscPath(firstPath);
  model.RememberDiscPath("");
  model.RememberDiscPath(firstPath);
  model.RememberDiscPath(model.GetKnownDiscPaths().front());
  model.RememberDiscs(model);

  CGameClientDiscModel other;
  other.AddDisc(firstPath);
  other.AddDisc(secondPath);
  const auto saved = other.GetState();
  ASSERT_TRUE(other.EraseDiscByIndex(1));
  model.RememberDiscs(other);
  EXPECT_EQ(model.GetKnownDiscPaths(), (std::vector<std::string>{firstPath, secondPath}));
  EXPECT_TRUE(model.Empty());
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.SetSelectedDiscByIndex(0));
  EXPECT_TRUE(model.GetState().slots.empty());

  CGameClientDiscModel restored;
  ASSERT_TRUE(model.ResolveState(saved, restored));
  EXPECT_EQ(restored.Size(), 2U);
  model.Clear();
  EXPECT_TRUE(model.GetKnownDiscPaths().empty());
}
