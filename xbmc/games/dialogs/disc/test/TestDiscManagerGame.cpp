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
  game.NotifyDiscSelection();

  game.Deinitialize();

  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestDiscManagerGame, FailedAutomaticInsertLeavesTrayOpen)
{
  CDiscManagerGame game;
  game.Initialize(m_client);
  game.NotifyDiscSelection();
  m_core.failInsert = true;

  game.Deinitialize();

  EXPECT_TRUE(m_core.ejected);
}
