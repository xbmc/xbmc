/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/guiinfo/GamesGUIInfo.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"

#include <gtest/gtest.h>

using namespace KODI::GAME;
using namespace KODI::GUILIB::GUIINFO;

class TestGamesGUIInfo : public testing::Test
{
protected:
  void SetUp() override
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_showExtensionsOriginal = settings->GetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS);
    settings->SetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, false);

    CServiceBroker::GetFileExtensionProvider().RegisterGameExtensions({".rom"});
  }

  void TearDown() override
  {
    CServiceBroker::GetFileExtensionProvider().UnregisterGameExtensions({".rom"});

    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, m_showExtensionsOriginal);
  }

  bool m_showExtensionsOriginal{false};
};

TEST_F(TestGamesGUIInfo, TranslatesRetroPlayerLabels)
{
  CGUIInfoManager infoManager;

  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.Title"), RETROPLAYER_TITLE);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.Platform"), RETROPLAYER_PLATFORM);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.Genres"), RETROPLAYER_GENRES);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.Publisher"), RETROPLAYER_PUBLISHER);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.Developer"), RETROPLAYER_DEVELOPER);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.Overview"), RETROPLAYER_OVERVIEW);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.GameClient"), RETROPLAYER_GAME_CLIENT);
}

TEST_F(TestGamesGUIInfo, GetLabelRequiresCurrentGameInGUIInfoManager)
{
  CFileItem item{"/roms/test.rom", false};
  item.GetGameInfoTag()->SetTitle("Chrono Trigger");

  CGamesGUIInfo gamesGUIInfo;
  std::string value;

  EXPECT_FALSE(gamesGUIInfo.GetLabel(value, &item, 0, CGUIInfo(RETROPLAYER_TITLE), nullptr));
  EXPECT_TRUE(value.empty());
}

TEST_F(TestGamesGUIInfo, InitCurrentItemSetsTitleFromFilesystemPath)
{
  CFileItem item{"/roms/test.rom", false};
  item.GetGameInfoTag();

  CGamesGUIInfo gamesGUIInfo;

  EXPECT_TRUE(gamesGUIInfo.InitCurrentItem(&item));

  const CGameInfoTag* tag = item.GetGameInfoTag();
  ASSERT_NE(tag, nullptr);
  EXPECT_EQ(tag->GetTitle(), "test");
}

TEST_F(TestGamesGUIInfo, InitCurrentItemSetsTitleFromVfsHostnamePath)
{
  CFileItem item{"zip://test.rom/", false};
  item.GetGameInfoTag();

  CGamesGUIInfo gamesGUIInfo;

  EXPECT_TRUE(gamesGUIInfo.InitCurrentItem(&item));

  const CGameInfoTag* tag = item.GetGameInfoTag();
  ASSERT_NE(tag, nullptr);
  EXPECT_EQ(tag->GetTitle(), "test");
}
