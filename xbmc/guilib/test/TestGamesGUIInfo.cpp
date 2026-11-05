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
#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
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
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.HasCheats"), RETROPLAYER_HAS_CHEATS);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.SupportsCheats"), RETROPLAYER_SUPPORTS_CHEATS);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsProgress"),
            RETROPLAYER_ACHIEVEMENTS_PROGRESS);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsHardcore"),
            RETROPLAYER_ACHIEVEMENTS_HARDCORE);
}

TEST_F(TestGamesGUIInfo, NamesAnAttemptByItsPlace)
{
  CGUIInfoManager infoManager;

  // Without a place, the first
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsChallengeTitle"),
            RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsChallengeBadge"),
            RETROPLAYER_ACHIEVEMENTS_CHALLENGE_BADGE);

  const int secondTitle = infoManager.TranslateString("RetroPlayer.AchievementsChallengeTitle(2)");
  EXPECT_NE(secondTitle, 0);
  EXPECT_NE(secondTitle, RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE);
  EXPECT_NE(secondTitle, infoManager.TranslateString("RetroPlayer.AchievementsChallengeTitle(3)"));
  EXPECT_NE(secondTitle, infoManager.TranslateString("RetroPlayer.AchievementsChallengeBadge(2)"));

  // Places count from 1
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsChallengeTitle(0)"), 0);
  EXPECT_EQ(infoManager.TranslateString("RetroPlayer.AchievementsChallengeTitle(2nd)"), 0);
}

TEST_F(TestGamesGUIInfo, AnswersAnAttemptByItsPlace)
{
  // Game services, which hold the setting that turns the indicators off
  KODI::RETRO::CPlaybackTestEnvironment environment;
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const bool indicatorOriginal = settings->GetBool("gamesachievements.challengeindicator");
  settings->SetBool("gamesachievements.challengeindicator", true);

  CAchievementRuntime achievementRuntime;
  AchievementChallenge first;
  first.id = 1;
  first.title = "Shroom";
  first.badgeUrl = "shroom.png";
  achievementRuntime.SetChallenge(first, true);
  AchievementChallenge second;
  second.id = 2;
  second.title = "Fire Flower";
  second.badgeUrl = "flower.png";
  achievementRuntime.SetChallenge(second, true);

  CGamesGUIInfo gamesGUIInfo{achievementRuntime};
  const auto label = [&gamesGUIInfo](uint32_t info, uint32_t place)
  {
    std::string value{"stale"};
    EXPECT_TRUE(gamesGUIInfo.GetLabel(value, nullptr, 0, CGUIInfo(info, place), nullptr));
    return value;
  };

  // Without a place, the first
  EXPECT_EQ(label(RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE, 0), "Shroom");
  EXPECT_EQ(label(RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE, 1), "Shroom");
  EXPECT_EQ(label(RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE, 2), "Fire Flower");
  EXPECT_EQ(label(RETROPLAYER_ACHIEVEMENTS_CHALLENGE_BADGE, 2), "flower.png");
  EXPECT_EQ(label(RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE, 3), "");

  settings->SetBool("gamesachievements.challengeindicator", indicatorOriginal);
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

TEST_F(TestGamesGUIInfo, AnswersListItemLabelsFromTheGameTag)
{
  CFileItem item{"/roms/test.rom", false};
  CGameInfoTag* tag = item.GetGameInfoTag();
  tag->SetTitle("Chrono Trigger");
  tag->SetYear(1995);
  tag->SetGenres({"Role-playing", "Adventure"});
  tag->SetOverview("A band of adventurers travels through time.");

  CGUIInfoManager infoManager;

  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_TITLE), "Chrono Trigger");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_YEAR), "1995");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_GENRE), "Role-playing, Adventure");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_PLOT),
            "A band of adventurers travels through time.");
}

TEST_F(TestGamesGUIInfo, AnswersListItemLabelsForAGamesMakers)
{
  CFileItem item{"/roms/test.rom", false};
  CGameInfoTag* tag = item.GetGameInfoTag();
  tag->SetPlatform("Super Nintendo");
  tag->SetDeveloper("Square");
  tag->SetPublisher("Square");

  CGUIInfoManager infoManager;

  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, infoManager.TranslateString("ListItem.Platform")),
            "Super Nintendo");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, infoManager.TranslateString("ListItem.Developer")),
            "Square");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, infoManager.TranslateString("ListItem.Publisher")),
            "Square");
}

TEST_F(TestGamesGUIInfo, AnswersListItemLabelsForAGamesWiderDetails)
{
  CFileItem item{"/roms/test.rom", false};
  CGameInfoTag* tag = item.GetGameInfoTag();
  tag->SetOriginalTitle("Seiken Densetsu 2");
  tag->SetRating(8.5f);
  tag->SetPlayers(3);
  tag->SetAgeRating("PEGI 7");
  tag->SetTrailer("https://example.com/trailer.mp4");
  tag->SetTags({"Co-op", "JRPG"});

  CGUIInfoManager infoManager;

  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_ORIGINALTITLE), "Seiken Densetsu 2");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_RATING), "8.5");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, infoManager.TranslateString("ListItem.Players")),
            "3");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, infoManager.TranslateString("ListItem.AgeRating")),
            "PEGI 7");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_TRAILER),
            "https://example.com/trailer.mp4");
  EXPECT_EQ(infoManager.GetItemLabel(&item, 0, LISTITEM_TAG), "Co-op, JRPG");
}

TEST_F(TestGamesGUIInfo, JoinsGenresWithTheSkinsSeparator)
{
  CFileItem item{"/roms/test.rom", false};
  item.GetGameInfoTag()->SetGenres({"Role-playing", "Adventure"});

  CGamesGUIInfo gamesGUIInfo;
  std::string value;

  EXPECT_TRUE(gamesGUIInfo.GetLabel(value, &item, 0, CGUIInfo(LISTITEM_GENRE, " / ", 0), nullptr));
  EXPECT_EQ(value, "Role-playing / Adventure");
}

TEST_F(TestGamesGUIInfo, LeavesItemsWithoutAGameTagAlone)
{
  CFileItem item{"/videos/test.mkv", false};

  CGamesGUIInfo gamesGUIInfo;
  std::string value;

  EXPECT_FALSE(gamesGUIInfo.GetLabel(value, &item, 0, CGUIInfo(LISTITEM_TITLE), nullptr));
  EXPECT_FALSE(gamesGUIInfo.GetLabel(value, &item, 0, CGUIInfo(LISTITEM_PLATFORM), nullptr));
  EXPECT_FALSE(item.HasGameInfoTag());
}
