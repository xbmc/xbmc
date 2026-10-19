/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/AchievementRuntime.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"

#include <memory>
#include <mutex>
#include <string>

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
class TestGUIComponent : public CGUIComponent
{
public:
  TestGUIComponent() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
  }

  ~TestGUIComponent() override { m_pWindowManager.reset(); }
};

class ToastQueue : public CGUIDialogKaiToast
{
public:
  static TOASTQUEUE TakeNotifications()
  {
    std::unique_lock lock(m_critical);
    TOASTQUEUE notifications;
    notifications.swap(m_notifications);
    return notifications;
  }
};
} // namespace

class TestGameClientCheevos : public testing::Test
{
protected:
  void SetUp() override
  {
    ToastQueue::TakeNotifications();
    CServiceBroker::RegisterGUI(&m_gui);
  }

  void TearDown() override
  {
    ToastQueue::TakeNotifications();
    TakeDialogRefreshes();
    CServiceBroker::UnregisterGUI();
  }

  int TakeDialogRefreshes()
  {
    const int messageIds[] = {GUI_MSG_NOTIFY_ALL, 0};
    return m_gui.GetWindowManager().RemoveThreadMessageByMessageIds(messageIds);
  }

  void LoadAchievement(bool earned)
  {
    AchievementInfo achievement;
    achievement.id = 540267;
    achievement.title = "100% Discount";
    achievement.earned = earned;
    if (earned)
      achievement.unlockedDate = m_originalDate;
    achievement.measuredPercent = 50.0f;
    achievement.measuredProgress = "5/10";

    AchievementState state;
    state.loaded = true;
    state.gameId = 586;
    state.totalAchievements = 1;
    state.unlockedAchievements = earned ? 1 : 0;
    state.achievements = {achievement};
    m_runtime.SetState(state);
  }

  void Trigger(bool encoreModeEnabled, unsigned int id = 540267)
  {
    game_rc_achievement_triggered event{};
    event.id = id;
    event.title = "100% Discount";
    event.badge_url = "badge.png";
    CGameClientCheevos::OnAchievementTriggered(event, m_runtime, encoreModeEnabled);
  }

  void ExpectUnlockNotification()
  {
    const auto notifications = ToastQueue::TakeNotifications();
    ASSERT_EQ(notifications.size(), 1U);
    EXPECT_EQ(notifications.front().description, "100% Discount");
    EXPECT_EQ(notifications.front().imagefile, "badge.png");
    EXPECT_TRUE(notifications.front().withSound);
  }

  void ExpectEarnedStateUnchanged()
  {
    const auto state = m_runtime.GetState();
    EXPECT_TRUE(state.loaded);
    EXPECT_EQ(state.gameId, 586U);
    EXPECT_EQ(state.totalAchievements, 1U);
    EXPECT_EQ(state.unlockedAchievements, 1U);
    ASSERT_EQ(state.achievements.size(), 1U);
    EXPECT_TRUE(state.achievements.front().earned);
    EXPECT_EQ(state.achievements.front().unlockedDate, m_originalDate);
    EXPECT_FLOAT_EQ(state.achievements.front().measuredPercent, 50.0f);
    EXPECT_EQ(state.achievements.front().measuredProgress, "5/10");
  }

  CAchievementRuntime m_runtime;
  TestGUIComponent m_gui;
  const CDateTime m_originalDate{2026, 8, 5, 19, 20, 0};
};

TEST_F(TestGameClientCheevos, FirstUnlockChangesStateAndNotifies)
{
  LoadAchievement(false);
  const auto before = CDateTime::GetCurrentDateTime();
  Trigger(false);

  const auto state = m_runtime.GetState();
  EXPECT_EQ(state.unlockedAchievements, 1U);
  ASSERT_EQ(state.achievements.size(), 1U);
  EXPECT_TRUE(state.achievements.front().earned);
  EXPECT_GE(state.achievements.front().unlockedDate, before);
  EXPECT_LE(state.achievements.front().unlockedDate, CDateTime::GetCurrentDateTime());
  EXPECT_FLOAT_EQ(state.achievements.front().measuredPercent, 0.0f);
  EXPECT_TRUE(state.achievements.front().measuredProgress.empty());
  ExpectUnlockNotification();
  EXPECT_EQ(TakeDialogRefreshes(), 1);

  Trigger(false);
  EXPECT_TRUE(ToastQueue::TakeNotifications().empty());
  EXPECT_EQ(TakeDialogRefreshes(), 0);
  EXPECT_EQ(m_runtime.GetState().achievements.front().unlockedDate,
            state.achievements.front().unlockedDate);
  EXPECT_EQ(m_runtime.GetUnlockedAchievements(), 1U);
}

TEST_F(TestGameClientCheevos, EarnedAchievementWithoutEncoreIsSuppressed)
{
  LoadAchievement(true);
  Trigger(false);

  EXPECT_TRUE(ToastQueue::TakeNotifications().empty());
  EXPECT_EQ(TakeDialogRefreshes(), 0);
  ExpectEarnedStateUnchanged();
}

TEST_F(TestGameClientCheevos, EarnedAchievementWithEncoreNotifiesWithoutChangingState)
{
  LoadAchievement(true);
  Trigger(true);

  ExpectUnlockNotification();
  EXPECT_EQ(TakeDialogRefreshes(), 0);
  ExpectEarnedStateUnchanged();
}

TEST_F(TestGameClientCheevos, UnknownAchievementWithEncoreIsSuppressed)
{
  LoadAchievement(true);
  Trigger(true, 999999);

  EXPECT_TRUE(ToastQueue::TakeNotifications().empty());
  EXPECT_EQ(TakeDialogRefreshes(), 0);
  ExpectEarnedStateUnchanged();
}

TEST_F(TestGameClientCheevos, EncoreEventAfterGameClosesIsSuppressed)
{
  LoadAchievement(true);
  m_runtime.Clear();
  Trigger(true);

  EXPECT_TRUE(ToastQueue::TakeNotifications().empty());
  EXPECT_EQ(TakeDialogRefreshes(), 0);
  EXPECT_FALSE(m_runtime.GetState().loaded);
  EXPECT_EQ(m_runtime.GetUnlockedAchievements(), 0U);
}

namespace
{
constexpr unsigned int GAME_SET = 586;
constexpr unsigned int BONUS_SET = 9001;

AchievementInfo Achievement(unsigned int id, bool earned)
{
  AchievementInfo achievement;
  achievement.id = id;
  achievement.title = "Achievement " + std::to_string(id);
  achievement.earned = earned;
  return achievement;
}
} // namespace

class TestAchievementSets : public TestGameClientCheevos
{
protected:
  // The game's own set needs one progression and one win achievement to beat;
  // the bonus set has a single achievement that can be missed
  void LoadSets()
  {
    AchievementState state;
    state.loaded = true;
    state.gameId = 586;
    state.gameTitle = "Super Mario Bros.";
    state.achievements = {Achievement(1, true), Achievement(2, false), Achievement(3, false)};
    state.totalAchievements = 3;
    state.unlockedAchievements = 1;
    m_runtime.SetState(state);

    m_runtime.SetAchievementSets(
        {{GAME_SET, "Super Mario Bros.", "", true}, {BONUS_SET, "Bonus", "", true}},
        {{1, GAME_SET, AchievementType::PROGRESSION},
         {2, GAME_SET, AchievementType::WIN},
         {3, BONUS_SET, AchievementType::MISSABLE}});
  }

  void TriggerWin()
  {
    game_rc_achievement_triggered event{};
    event.id = 2;
    event.title = "Achievement 2";
    CGameClientCheevos::OnAchievementTriggered(event, m_runtime, false);
  }
};

TEST_F(TestAchievementSets, EachAchievementKnowsItsSetAndType)
{
  LoadSets();

  const AchievementState state = m_runtime.GetState();
  ASSERT_EQ(state.sets.size(), 2U);
  ASSERT_EQ(state.achievements.size(), 3U);
  EXPECT_EQ(state.achievements[0].setId, GAME_SET);
  EXPECT_EQ(state.achievements[1].type, AchievementType::WIN);
  EXPECT_EQ(state.achievements[2].setId, BONUS_SET);
  EXPECT_EQ(state.achievements[2].type, AchievementType::MISSABLE);
}

TEST_F(TestAchievementSets, ATurnedOffSetLeavesTheTotals)
{
  LoadSets();
  EXPECT_EQ(m_runtime.GetTotalAchievements(), 3U);

  ASSERT_TRUE(m_runtime.SetAchievementSetEnabled(BONUS_SET, false));
  EXPECT_FALSE(m_runtime.GetState().IsSetEnabled(BONUS_SET));
  EXPECT_EQ(m_runtime.GetTotalAchievements(), 2U);
  EXPECT_EQ(m_runtime.GetUnlockedAchievements(), 1U);

  ASSERT_TRUE(m_runtime.SetAchievementSetEnabled(BONUS_SET, true));
  EXPECT_EQ(m_runtime.GetTotalAchievements(), 3U);

  EXPECT_FALSE(m_runtime.SetAchievementSetEnabled(1234, false));
}

TEST_F(TestAchievementSets, ATurnedOffSetTakesItsIndicatorsWithIt)
{
  LoadSets();
  m_runtime.SetChallenge({3, "Achievement 3", ""}, true);
  m_runtime.SetChallenge({2, "Achievement 2", ""}, true);

  m_runtime.SetAchievementSetEnabled(BONUS_SET, false);

  const auto challenges = m_runtime.GetChallenges();
  ASSERT_EQ(challenges.size(), 1U);
  EXPECT_EQ(challenges.front().id, 2U);
}

TEST_F(TestAchievementSets, RequestsGoToTheHandler)
{
  LoadSets();
  EXPECT_FALSE(m_runtime.RequestAchievementSetEnabled(BONUS_SET, false));

  unsigned int askedSet = 0;
  m_runtime.SetAchievementSetHandler(
      [&askedSet](unsigned int setId, bool enabled)
      {
        askedSet = setId;
        return !enabled;
      });
  EXPECT_TRUE(m_runtime.RequestAchievementSetEnabled(BONUS_SET, false));
  EXPECT_EQ(askedSet, BONUS_SET);
  EXPECT_FALSE(m_runtime.RequestAchievementSetEnabled(BONUS_SET, true));

  m_runtime.SetAchievementSetHandler(nullptr);
  EXPECT_FALSE(m_runtime.RequestAchievementSetEnabled(BONUS_SET, false));
}

TEST_F(TestAchievementSets, BeatenOnceProgressionAndAWinAreEarned)
{
  LoadSets();

  BeatenProgress progress = m_runtime.GetState().GetBeatenProgress(GAME_SET);
  EXPECT_TRUE(progress.CanBeBeaten());
  EXPECT_FALSE(progress.IsBeaten());
  EXPECT_FALSE(m_runtime.GetState().GetBeatenProgress(BONUS_SET).CanBeBeaten());

  TriggerWin();

  progress = m_runtime.GetState().GetBeatenProgress(GAME_SET);
  EXPECT_TRUE(progress.IsBeaten());

  // The unlock, then the set it beat
  const auto notifications = ToastQueue::TakeNotifications();
  ASSERT_EQ(notifications.size(), 2U);
  EXPECT_EQ(notifications.back().description, "Super Mario Bros.");
  EXPECT_FALSE(notifications.back().withSound);
}

TEST_F(TestAchievementSets, AHardcoreUnlockDoesNotBeatTheSetAgain)
{
  LoadSets();
  TriggerWin();
  ASSERT_EQ(ToastQueue::TakeNotifications().size(), 2U);

  game_rc_achievement_triggered event{};
  event.id = 2;
  event.title = "Achievement 2";
  event.hardcore = true;
  CGameClientCheevos::OnAchievementTriggered(event, m_runtime, false);

  // The unlock only
  EXPECT_EQ(ToastQueue::TakeNotifications().size(), 1U);
}
