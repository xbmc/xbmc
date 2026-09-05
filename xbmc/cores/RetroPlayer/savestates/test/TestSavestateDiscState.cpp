/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/savestates/SavestateFlatBuffer.h"
#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscState.h"
#include "savestate_generated.h"
#include "test/TestUtils.h"

#include <memory>
#include <string>
#include <vector>

#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;
using namespace RETRO;

namespace
{
std::vector<uint8_t> Serialize(CSavestateFlatBuffer& savestate)
{
  savestate.Finalize();

  const uint8_t* data = nullptr;
  size_t size = 0;
  EXPECT_TRUE(savestate.Serialize(data, size));
  return {data, data + size};
}

GameClientDiscState MakeDiscState()
{
  GameClientDiscState state;
  state.slots = {
      {DiscSlotType::Disc, "/games/Multi Disc/Disc 1.chd", "Disc One"},
      {DiscSlotType::Removed, {}, "Removed Disc"},
      {DiscSlotType::Unknown, {}, "smb://server/share/Unknown Slot.chd"},
      {DiscSlotType::Disc, "C:\\games\\Multi Disc\\Disc 2.chd", "Disc Two"},
  };
  state.selectedSlot = 3;
  state.trayEjected = true;
  return state;
}
} // namespace

TEST(TestSavestateDiscState, RoundTripsTopologySelectionAndTrayState)
{
  CSavestateFlatBuffer writer;
  writer.SetDiscState(MakeDiscState());

  CSavestateFlatBuffer reader;
  const auto bytes = Serialize(writer);
  EXPECT_EQ(SAVESTATE::GetSavestate(bytes.data())->version(), 6);
  ASSERT_TRUE(reader.Deserialize(bytes));

  const std::optional<GameClientDiscState> state = reader.GetDiscState();
  ASSERT_TRUE(state.has_value());
  ASSERT_EQ(state->slots.size(), 4U);
  EXPECT_EQ(state->slots[0], (DiscSlot{DiscSlotType::Disc, "Disc 1.chd", "Disc One"}));
  EXPECT_EQ(state->slots[1], (DiscSlot{DiscSlotType::Removed, {}, "Removed Disc"}));
  EXPECT_EQ(state->slots[2], (DiscSlot{DiscSlotType::Unknown, {}, "Unknown Slot.chd"}));
  EXPECT_EQ(state->slots[3], (DiscSlot{DiscSlotType::Disc, "Disc 2.chd", "Disc Two"}));
  EXPECT_EQ(state->selectedSlot, 3);
  EXPECT_TRUE(state->trayEjected);
}

TEST(TestSavestateDiscState, RoundTripsNoDiscWithClosedTray)
{
  GameClientDiscState expected = MakeDiscState();
  expected.selectedSlot = -1;
  expected.trayEjected = false;

  CSavestateFlatBuffer writer;
  writer.SetDiscState(expected);

  CSavestateFlatBuffer reader;
  ASSERT_TRUE(reader.Deserialize(Serialize(writer)));
  ASSERT_TRUE(reader.GetDiscState().has_value());
  EXPECT_EQ(*reader.GetDiscState(),
            GameClientDiscState({{DiscSlotType::Disc, "Disc 1.chd", "Disc One"},
                                 {DiscSlotType::Removed, {}, "Removed Disc"},
                                 {DiscSlotType::Unknown, {}, "Unknown Slot.chd"},
                                 {DiscSlotType::Disc, "Disc 2.chd", "Disc Two"}},
                                -1, false));
}

TEST(TestSavestateDiscState, OldSavestateHasNoDiscState)
{
  for (uint8_t version = 1; version <= 5; ++version)
  {
    flatbuffers::FlatBufferBuilder builder;
    SAVESTATE::SavestateBuilder savestateBuilder(builder);
    savestateBuilder.add_version(version);
    const auto root = savestateBuilder.Finish();
    SAVESTATE::FinishSavestateBuffer(builder, root);

    CSavestateFlatBuffer reader;
    ASSERT_TRUE(reader.Deserialize(std::vector<uint8_t>(
        builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize())));
    EXPECT_EQ(reader.GetDiscState(), std::nullopt);
  }
}

TEST(TestSavestateDiscState, SerializedDiscStateContainsNoDirectoryPaths)
{
  CSavestateFlatBuffer writer;
  writer.SetDiscState(MakeDiscState());
  const std::vector<uint8_t> bytes = Serialize(writer);
  const std::string serialized(reinterpret_cast<const char*>(bytes.data()), bytes.size());

  EXPECT_EQ(serialized.find("/games/Multi Disc/"), std::string::npos);
  EXPECT_EQ(serialized.find("C:\\games\\Multi Disc\\"), std::string::npos);
  EXPECT_EQ(serialized.find("smb://server/share/"), std::string::npos);
  EXPECT_NE(serialized.find("Disc 1.chd"), std::string::npos);
  EXPECT_NE(serialized.find("Disc 2.chd"), std::string::npos);
}

TEST(TestSavestateDiscState, PortableStoragePreservesDisplayLabelsContainingSlashes)
{
  GameClientDiscState expected;
  expected.slots = {{DiscSlotType::Disc, "/games/disc.chd", "Disc 1/2"}};

  CSavestateFlatBuffer writer;
  writer.SetDiscState(expected);

  CSavestateFlatBuffer reader;
  ASSERT_TRUE(reader.Deserialize(Serialize(writer)));
  ASSERT_TRUE(reader.GetDiscState().has_value());
  EXPECT_EQ(reader.GetDiscState()->slots[0].label, "Disc 1/2");
}

TEST(TestSavestateDiscState, RenamePreservesDiscState)
{
  std::unique_ptr<XFILE::CFile> tempFile(XBMC_CREATETEMPFILE(".sav"));
  ASSERT_NE(tempFile, nullptr);
  const std::string path = XBMC_TEMPFILEPATH(tempFile.get());
  tempFile->Close();

  CSavestateFlatBuffer writer;
  writer.SetLabel("Before");
  writer.SetDiscState(MakeDiscState());
  ASSERT_NE(writer.GetMemoryBuffer(1), nullptr);
  writer.Finalize();

  CSavestateDatabase database;
  ASSERT_TRUE(database.AddSavestate(path, {}, writer));
  const std::unique_ptr<ISavestate> renamed = database.RenameSavestate(path, "After");
  ASSERT_NE(renamed, nullptr);
  EXPECT_EQ(renamed->Label(), "After");
  ASSERT_TRUE(renamed->GetDiscState().has_value());

  GameClientDiscState expected = MakeDiscState();
  expected.slots[0].fileName = "Disc 1.chd";
  expected.slots[2].label = "Unknown Slot.chd";
  expected.slots[3].fileName = "Disc 2.chd";
  EXPECT_EQ(*renamed->GetDiscState(), expected);

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tempFile.release()));
}
