/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/dialogs/disc/DiscManagerGame.h"
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
struct DiscManagerCore
{
  std::vector<std::string> slots{"/roms/disc1.chd"};
  unsigned int selected{0};
  bool ejected{true};
  bool failInsert{false};
  bool failSelection{false};
  bool failReplace{false};
  bool failRemove{false};
};

DiscManagerCore& Core(const AddonInstance_Game* instance)
{
  return *static_cast<DiscManagerCore*>(instance->toAddon->addonInstance);
}
} // namespace

class TestDiscManagerGame : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml =
        R"(<addon id="game.test.discmanager" name="Disc manager test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so">
        <supports_disc_control>true</supports_disc_control>
        <extensions>chd</extensions>
      </extension>
      <extension point="xbmc.addon.metadata"><platform>all</platform></extension>
    </addon>)";
    ASSERT_TRUE(xml.Parse(addonXml));
    const auto info =
        ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    m_client = std::make_shared<CGameClient>(info);

    auto* callbacks = m_client->GetInstanceInterface()->toAddon;
    callbacks->addonInstance = &m_core;
    callbacks->GetEjectState = [](const AddonInstance_Game* game) { return Core(game).ejected; };
    callbacks->SetEjectState = [](const AddonInstance_Game* game, bool ejected)
    {
      if (Core(game).failInsert && !ejected)
        return GAME_ERROR_FAILED;
      Core(game).ejected = ejected;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->GetImageCount = [](const AddonInstance_Game* game)
    { return static_cast<unsigned int>(Core(game).slots.size()); };
    callbacks->GetImageIndex = [](const AddonInstance_Game* game) { return Core(game).selected; };
    callbacks->SetImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      if (Core(game).failSelection)
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
      if (Core(game).failReplace || index >= Core(game).slots.size())
        return GAME_ERROR_FAILED;
      Core(game).slots[index] = path ? path : "";
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->RemoveImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      if (Core(game).failRemove || index >= Core(game).slots.size())
        return GAME_ERROR_FAILED;
      Core(game).slots[index].clear();
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->GetImagePath = [](const AddonInstance_Game* game, unsigned int index) -> char*
    { return index < Core(game).slots.size() ? strdup(Core(game).slots[index].c_str()) : nullptr; };
    callbacks->GetImageLabel = [](const AddonInstance_Game*, unsigned int) -> char*
    { return nullptr; };
    callbacks->FreeString = [](const AddonInstance_Game*, char* value) { free(value); };
  }

  DiscManagerCore m_core;
  std::shared_ptr<CGameClient> m_client;
};

TEST_F(TestDiscManagerGame, ExplicitSameDiscSelectionClosesTrayOnDeinitialize)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(0));
  game.NotifyDiscChange();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, FailedAutomaticInsertLeavesTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  game.NotifyDiscChange();
  m_core.failInsert = true;

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, RestoredEmptyPlaylistKeepsTrayOpenOnDeinitialize)
{
  m_core.slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
  CDiscManagerGame game;
  game.Initialize(m_client);

  CGameClientDiscModel restored;
  restored.SetEjected(true);
  m_client->Discs().SetDiscModel(restored);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs().Empty());
  EXPECT_TRUE(m_client->Discs().GetDiscs().IsSelectedNoDisc());
}

TEST_F(TestDiscManagerGame, RestoredSelectionKeepsTrayOpenOnDeinitialize)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  CDiscManagerGame game;
  game.Initialize(m_client);

  CGameClientDiscModel restored = m_client->Discs().GetDiscs();
  ASSERT_TRUE(restored.SetSelectedDiscByIndex(1));
  m_client->Discs().SetDiscModel(restored);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
  EXPECT_EQ(m_core.selected, 1U);
}

TEST_F(TestDiscManagerGame, RestoreSupersedesEarlierExplicitSelection)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(1));
  game.NotifyDiscChange();

  CGameClientDiscModel restored = m_client->Discs().GetDiscs();
  ASSERT_TRUE(restored.SetSelectedDiscByIndex(0));
  m_client->Discs().SetDiscModel(restored);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
  EXPECT_EQ(m_core.selected, 0U);
}

TEST_F(TestDiscManagerGame, RestoringSameStateSupersedesEarlierExplicitSelection)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(0));
  game.NotifyDiscChange();

  const CGameClientDiscModel restored = m_client->Discs().GetDiscs();
  m_client->Discs().SetDiscModel(restored);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
  EXPECT_EQ(m_core.selected, 0U);
}

TEST_F(TestDiscManagerGame, SessionResetSupersedesEarlierExplicitSelection)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(0));
  game.NotifyDiscChange();
  m_client->Discs().Deinitialize();
  m_client->Discs().RefreshDiscState();

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, SelectionAfterRestoreClosesTrayOnDeinitialize)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  const CGameClientDiscModel restored = m_client->Discs().GetDiscs();
  m_client->Discs().SetDiscModel(restored);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(0));
  game.NotifyDiscChange();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, DiscOperationsDoNotInvalidatePendingSelection)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(0));
  game.NotifyDiscChange();
  ASSERT_TRUE(m_client->Discs().SetEjected(false));
  ASSERT_TRUE(m_client->Discs().SetEjected(true));
  m_client->Discs().RefreshDiscState();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, ExplicitNoDiscSelectionClosesTrayOnDeinitialize)
{
  m_core.selected = 1;
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().InsertDisc(""));
  game.NotifyDiscChange();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs().IsSelectedNoDisc());
}

TEST_F(TestDiscManagerGame, SuccessfulAdditionClosesTrayOnDeinitialize)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc2.chd"));
  game.NotifyDiscChange();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
  EXPECT_EQ(m_client->Discs().GetDiscs().GetPathByIndex(1), "/roms/disc2.chd");
}

TEST_F(TestDiscManagerGame, SuccessfulRemovalClosesTrayOnDeinitialize)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().RemoveDiscByIndex(0));
  game.NotifyDiscChange();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs().IsSelectedNoDisc());
}

TEST_F(TestDiscManagerGame, FailedNoDiscSelectionAfterRemovalKeepsTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  m_core.failSelection = true;
  ASSERT_FALSE(m_client->Discs().RemoveDiscByIndex(0));

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, FailedSelectionKeepsTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  m_core.failSelection = true;
  ASSERT_FALSE(m_client->Discs().InsertDisc(""));

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
  EXPECT_EQ(m_core.selected, 0U);
}

TEST_F(TestDiscManagerGame, FailedAdditionKeepsTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  m_core.failReplace = true;
  ASSERT_FALSE(m_client->Discs().AddDisc("/roms/disc2.chd"));

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, FailedRemovalKeepsTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  m_core.failRemove = true;
  ASSERT_FALSE(m_client->Discs().RemoveDiscByIndex(0));

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, DuplicateAdditionKeepsTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc1.chd"));

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, NoManagerEditKeepsTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, ExplicitInsertThenEjectSupersedesPendingSelection)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(1));
  game.NotifyDiscChange();
  ASSERT_TRUE(m_client->Discs().SetEjected(false));
  game.NotifyTrayChange();
  ASSERT_TRUE(m_client->Discs().SetEjected(true));
  game.NotifyTrayChange();

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
  EXPECT_EQ(m_core.selected, 1U);
}

TEST_F(TestDiscManagerGame, SelectionAfterExplicitEjectClosesTrayOnDeinitialize)
{
  m_core.ejected = false;
  CDiscManagerGame game;
  game.Initialize(m_client);
  ASSERT_TRUE(m_client->Discs().SetEjected(true));
  game.NotifyTrayChange();
  ASSERT_TRUE(m_client->Discs().InsertDiscByIndex(0));
  game.NotifyDiscChange();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
}
