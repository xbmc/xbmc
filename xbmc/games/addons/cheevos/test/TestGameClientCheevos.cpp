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
