/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
struct MutationCore
{
  std::vector<std::string> slots{"/roms/disc1.chd", "/roms/disc2.chd"};
  unsigned int selected{0};
  bool ejected{true};
  bool failSetImageIndex{false};
};

MutationCore& Core(const AddonInstance_Game* instance)
{
  return *static_cast<MutationCore*>(instance->toAddon->addonInstance);
}
} // namespace

class TestGameClientDiscMutations : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml =
        R"(<addon id="game.test.discmutations" name="Disc mutation test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so">
        <supports_disc_control>true</supports_disc_control>
        <extensions>chd|m3u</extensions>
      </extension>
      <extension point="xbmc.addon.metadata"><platform>all</platform></extension>
    </addon>)";
    ASSERT_TRUE(xml.Parse(addonXml));
    const auto info =
        ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    m_client = std::make_unique<CGameClient>(info);

    auto* callbacks = m_client->GetInstanceInterface()->toAddon;
    callbacks->addonInstance = &m_core;
    callbacks->GetEjectState = [](const AddonInstance_Game* game) { return Core(game).ejected; };
    callbacks->SetEjectState = [](const AddonInstance_Game* game, bool ejected)
    {
      Core(game).ejected = ejected;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->GetImageCount = [](const AddonInstance_Game* game)
    { return static_cast<unsigned int>(Core(game).slots.size()); };
    callbacks->GetImageIndex = [](const AddonInstance_Game* game) { return Core(game).selected; };
    callbacks->SetImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      if (Core(game).failSetImageIndex)
        return GAME_ERROR_FAILED;
      Core(game).selected = index;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->AddImageIndex = [](const AddonInstance_Game* game)
    {
      Core(game).slots.emplace_back();
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->ReplaceImageIndex =
        [](const AddonInstance_Game* game, unsigned int index, const char* path)
    {
      if (index >= Core(game).slots.size())
        return GAME_ERROR_FAILED;
      Core(game).slots[index] = path ? path : "";
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->RemoveImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      if (index >= Core(game).slots.size())
        return GAME_ERROR_FAILED;
      Core(game).slots[index].clear();
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->SetInitialImage = [](const AddonInstance_Game*, unsigned int, const char*)
    { return GAME_ERROR_NO_ERROR; };
    callbacks->GetImagePath = [](const AddonInstance_Game*, unsigned int) -> char*
    { return nullptr; };
    callbacks->GetImageLabel = [](const AddonInstance_Game*, unsigned int) -> char*
    { return nullptr; };
    callbacks->FreeString = [](const AddonInstance_Game*, char* value) { free(value); };

    m_playlist.reset(XBMC_CREATETEMPFILE(".m3u"));
    ASSERT_NE(m_playlist, nullptr);
    constexpr char PLAYLIST[] = "/roms/disc1.chd\n/roms/disc2.chd\n";
    ASSERT_EQ(m_playlist->Write(PLAYLIST, sizeof(PLAYLIST) - 1),
              static_cast<ssize_t>(sizeof(PLAYLIST) - 1));
    m_playlist->Close();
    m_client->Discs().Initialize(XBMC_TEMPFILEPATH(m_playlist.get()));
    m_client->Discs().RefreshDiscState();
  }

  void TearDown() override { EXPECT_TRUE(XBMC_DELETETEMPFILE(m_playlist.release())); }

  MutationCore m_core;
  std::unique_ptr<CGameClient> m_client;
  std::unique_ptr<XFILE::CFile> m_playlist;
};

TEST_F(TestGameClientDiscMutations, ReusedRemovedSlotKeepsIdentityWithoutCoreMetadata)
{
  ASSERT_TRUE(m_client->Discs().RemoveDiscByIndex(1));
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc3.chd"));

  const CGameClientDiscModel model = m_client->Discs().GetDiscs();
  ASSERT_EQ(model.Size(), 2U);
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_EQ(model.GetPathByIndex(1), "/roms/disc3.chd");
  EXPECT_EQ(model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc3.chd"}));
}

TEST_F(TestGameClientDiscMutations, FailedSlotReuseKeepsRemovedSlot)
{
  ASSERT_TRUE(m_client->Discs().RemoveDiscByIndex(1));
  m_client->GetInstanceInterface()->toAddon->ReplaceImageIndex =
      [](const AddonInstance_Game*, unsigned int, const char*) { return GAME_ERROR_FAILED; };

  EXPECT_FALSE(m_client->Discs().AddDisc("/roms/disc3.chd"));

  const CGameClientDiscModel model = m_client->Discs().GetDiscs();
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(model.GetPathByIndex(1).empty());
  EXPECT_EQ(model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", ""}));
}

TEST_F(TestGameClientDiscMutations, ReusedRemovedSlotPreservesNoDiscSelection)
{
  ASSERT_TRUE(m_client->Discs().InsertDisc(""));
  ASSERT_TRUE(m_client->Discs().RemoveDiscByIndex(1));
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc3.chd"));

  const CGameClientDiscModel model = m_client->Discs().GetDiscs();
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_EQ(model.GetPathByIndex(1), "/roms/disc3.chd");
  EXPECT_EQ(m_core.selected, 2U);
}

TEST_F(TestGameClientDiscMutations, CompactingRemovalKeepsIdentityWithoutCoreMetadata)
{
  m_client->GetInstanceInterface()->toAddon->RemoveImageIndex =
      [](const AddonInstance_Game* game, unsigned int index)
  {
    auto& core = Core(game);
    if (index >= core.slots.size())
      return GAME_ERROR_FAILED;
    core.slots.erase(core.slots.begin() + index);
    if (core.selected > index)
      --core.selected;
    return GAME_ERROR_NO_ERROR;
  };
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(1));

  ASSERT_TRUE(m_client->Discs().RemoveDiscByIndex(0));

  const CGameClientDiscModel model = m_client->Discs().GetDiscs();
  ASSERT_EQ(model.Size(), 1U);
  EXPECT_EQ(model.GetPathByIndex(0), "/roms/disc2.chd");
  EXPECT_EQ(model.GetSelectedDiscIndex(), 0U);
}

TEST_F(TestGameClientDiscMutations,
       FailedNoDiscSelectionAfterRemovalReportsFailureAndRefreshesModel)
{
  m_core.failSetImageIndex = true;

  EXPECT_FALSE(m_client->Discs().RemoveDiscByIndex(0));

  const CGameClientDiscModel model = m_client->Discs().GetDiscs();
  ASSERT_EQ(model.Size(), 2U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_TRUE(m_core.slots[0].empty());
}
