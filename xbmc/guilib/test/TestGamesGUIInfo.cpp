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
#include "games/AchievementRuntime.h"
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
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.GameClientName"),
            RETROPLAYER_GAME_CLIENT_NAME);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.GameClientPlatforms"),
            RETROPLAYER_GAME_CLIENT_PLATFORMS);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.RichPresence"), RETROPLAYER_RICH_PRESENCE);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsLoggedIn"),
            RETROPLAYER_ACHIEVEMENTS_LOGGED_IN);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsIndicatorTitle"),
            RETROPLAYER_ACHIEVEMENTS_INDICATOR_TITLE);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsIndicatorProgress"),
            RETROPLAYER_ACHIEVEMENTS_INDICATOR_PROGRESS);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsIndicatorPercent"),
            RETROPLAYER_ACHIEVEMENTS_INDICATOR_PERCENT);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsIndicatorBadge"),
            RETROPLAYER_ACHIEVEMENTS_INDICATOR_BADGE);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsChallengeTitle"),
            RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsChallengeBadge"),
            RETROPLAYER_ACHIEVEMENTS_CHALLENGE_BADGE);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsProgress"),
            RETROPLAYER_ACHIEVEMENTS_PROGRESS);
}

namespace
{
AchievementState MakeAchievementState()
{
  AchievementInfo earned;
  earned.id = 1;
  earned.title = "Fated Hour";
  earned.earned = true;

  AchievementInfo locked;
  locked.id = 2;
  locked.title = "The Fall of Guardia";

  AchievementState state;
  state.gameTitle = "Chrono Trigger";
  state.totalAchievements = 2;
  state.unlockedAchievements = 1;
  state.achievements = {earned, locked};
  state.loaded = true;

  return state;
}
} // namespace

TEST_F(TestGamesGUIInfo, GetsAchievementProgressFromAchievementState)
{
  CAchievementRuntime achievementRuntime;
  achievementRuntime.SetState(MakeAchievementState());

  CGamesGUIInfo gamesGUIInfo{achievementRuntime};
  std::string value;

  EXPECT_TRUE(gamesGUIInfo.GetLabel(value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_PROGRESS),
                                    nullptr));
  EXPECT_EQ(value, "1 / 2");
}

TEST_F(TestGamesGUIInfo, AchievementProgressIsEmptyWithoutAchievements)
{
  CAchievementRuntime achievementRuntime;

  CGamesGUIInfo gamesGUIInfo{achievementRuntime};
  std::string value{"stale"};

  EXPECT_TRUE(gamesGUIInfo.GetLabel(value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_PROGRESS),
                                    nullptr));
  EXPECT_TRUE(value.empty());
}

TEST_F(TestGamesGUIInfo, MarkEarnedOnlyCountsTheFirstUnlock)
{
  CAchievementRuntime achievementRuntime;
  achievementRuntime.SetState(MakeAchievementState());

  const CDateTime firstUnlock{2026, 8, 5, 19, 20, 0};
  const CDateTime laterUnlock{2026, 8, 6, 8, 15, 0};

  bool newlyEarned = false;
  AchievementState state = achievementRuntime.MarkEarned(2, firstUnlock, newlyEarned);
  EXPECT_TRUE(newlyEarned);
  EXPECT_EQ(state.unlockedAchievements, 2U);
  EXPECT_EQ(state.achievements[1].unlockedDate, firstUnlock);

  // The achievement runtime re-reports achievements earned in an earlier
  // session, which must not inflate the count or replace the unlock date
  state = achievementRuntime.MarkEarned(2, laterUnlock, newlyEarned);
  EXPECT_FALSE(newlyEarned);
  EXPECT_EQ(state.unlockedAchievements, 2U);
  EXPECT_EQ(state.achievements[1].unlockedDate, firstUnlock);

  // An unknown ID must not change anything
  state = achievementRuntime.MarkEarned(99, CDateTime{2026, 8, 7, 12, 0, 0}, newlyEarned);
  EXPECT_FALSE(newlyEarned);
  EXPECT_EQ(state.unlockedAchievements, 2U);
}

TEST_F(TestGamesGUIInfo, GetsRichPresenceFromAchievementState)
{
  CAchievementRuntime achievementRuntime;
  achievementRuntime.SetRichPresence("Fighting Lavos");

  CGamesGUIInfo gamesGUIInfo{achievementRuntime};
  std::string value;

  EXPECT_TRUE(
      gamesGUIInfo.GetLabel(value, nullptr, 0, CGUIInfo(RETROPLAYER_RICH_PRESENCE), nullptr));
  EXPECT_EQ(value, "Fighting Lavos");
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

TEST_F(TestGamesGUIInfo, ShowsTheAchievementTheRuntimeIndicated)
{
  CAchievementRuntime achievementRuntime;
  achievementRuntime.SetState(MakeAchievementState());

  ProgressIndicator indicator;
  indicator.id = 3;
  indicator.title = "Collect 180 rings";
  indicator.badgeUrl = "https://example.invalid/badge.png";
  indicator.measuredProgress = "130/180";
  indicator.measuredPercent = 72.0f;
  achievementRuntime.SetProgressIndicator(indicator, true);

  CGamesGUIInfo gamesGUIInfo{achievementRuntime};

  std::string value;
  EXPECT_TRUE(gamesGUIInfo.GetLabel(
      value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_TITLE), nullptr));
  EXPECT_EQ(value, "Collect 180 rings");

  EXPECT_TRUE(gamesGUIInfo.GetLabel(
      value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_PROGRESS), nullptr));
  EXPECT_EQ(value, "130/180");

  EXPECT_TRUE(gamesGUIInfo.GetLabel(
      value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_BADGE), nullptr));
  EXPECT_EQ(value, "https://example.invalid/badge.png");

  int percent = 0;
  EXPECT_TRUE(gamesGUIInfo.GetInt(percent, nullptr, 0,
                                  CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_PERCENT)));
  EXPECT_EQ(percent, 72);
}

TEST_F(TestGamesGUIInfo, ShowsNothingOnceTheIndicatorIsCleared)
{
  CAchievementRuntime achievementRuntime;
  achievementRuntime.SetState(MakeAchievementState());

  ProgressIndicator indicator;
  indicator.id = 3;
  indicator.title = "Collect 180 rings";
  indicator.measuredProgress = "130/180";
  indicator.measuredPercent = 72.0f;
  achievementRuntime.SetProgressIndicator(indicator, true);
  achievementRuntime.SetProgressIndicator({}, false);

  CGamesGUIInfo gamesGUIInfo{achievementRuntime};

  std::string value;
  EXPECT_TRUE(gamesGUIInfo.GetLabel(
      value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_TITLE), nullptr));
  EXPECT_TRUE(value.empty());

  int percent = -1;
  EXPECT_TRUE(gamesGUIInfo.GetInt(percent, nullptr, 0,
                                  CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_PERCENT)));
  EXPECT_EQ(percent, 0);
}

TEST_F(TestGamesGUIInfo, ShowsTheClosestOfTwoAchievementsCountingAtOnce)
{
  CAchievementRuntime achievementRuntime;
  achievementRuntime.SetState(MakeAchievementState());

  ProgressIndicator behind;
  behind.id = 4;
  behind.title = "Trip Pop Pro";
  behind.measuredProgress = "1/25";
  behind.measuredPercent = 4.0f;

  ProgressIndicator ahead;
  ahead.id = 5;
  ahead.title = "Orange Ace";
  ahead.measuredProgress = "18/20";
  ahead.measuredPercent = 90.0f;

  // Announced separately, as the runtime does, and in the order that would
  // leave the wrong one showing if the latest simply replaced the last
  achievementRuntime.SetProgressIndicator(ahead, true);
  achievementRuntime.SetProgressIndicator(behind, true);

  CGamesGUIInfo gamesGUIInfo{achievementRuntime};

  std::string value;
  EXPECT_TRUE(gamesGUIInfo.GetLabel(
      value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_TITLE), nullptr));
  EXPECT_EQ(value, "Orange Ace");

  // The one that finished stops counting without taking the other with it
  achievementRuntime.SetProgressIndicator(ahead, false);
  EXPECT_TRUE(gamesGUIInfo.GetLabel(
      value, nullptr, 0, CGUIInfo(RETROPLAYER_ACHIEVEMENTS_INDICATOR_TITLE), nullptr));
  EXPECT_EQ(value, "Trip Pop Pro");
}
