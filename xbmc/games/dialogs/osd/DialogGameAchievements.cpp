/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameAchievements.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "view/ViewState.h"

using namespace KODI::GAME;

CDialogGameAchievements::CDialogGameAchievements()
  : CGUIDialog(WINDOW_DIALOG_GAME_ACHIEVEMENTS, "DialogGameControllers.xml")
{
}

void CDialogGameAchievements::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(3));
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
}

void CDialogGameAchievements::OnWindowUnload()
{
  m_viewControl.Reset();
  CGUIDialog::OnWindowUnload();
}

void CDialogGameAchievements::OnInitWindow()
{
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  const auto& settings = CServiceBroker::GetGameServices().GameSettings();

  if (!settings.GetAchievementsLoggedIn())
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, "RetroAchievements",
                                          "Please log in to RetroAchievements in Settings", 6000,
                                          false, 500);
    Close();
    return;
  }

  if (!settings.GetAchievementsLoaded() || settings.GetAchievementTotal() == 0)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "RetroAchievements",
                                          "This game doesn't support RetroAchievements", 6000,
                                          false, 500);
    Close();
    return;
  }

  PopulateList();
  CGUIDialog::OnInitWindow();
}

bool CDialogGameAchievements::OnMessage(CGUIMessage& message)
{
  return CGUIDialog::OnMessage(message);
}

void CDialogGameAchievements::PopulateList()
{
  m_items.Clear();

  const auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();

  for (const auto& achievement : state.achievements)
  {
    auto item = std::make_shared<CFileItem>(achievement.title);
    item->SetLabel(achievement.title);
    item->SetLabel2(achievement.description);
    // Use locked badge for unearned, full colour for earned
    const std::string iconUrl = achievement.earned || achievement.lockedBadgeUrl.empty()
                                    ? achievement.badgeUrl
                                    : achievement.lockedBadgeUrl;
    item->SetArt("icon", iconUrl);
    item->SetProperty("Points", achievement.points);
    item->SetProperty("Earned", achievement.earned ? "true" : "");
    item->SetProperty("UnlockedDate", achievement.unlockedDate);
    std::string rarityVal;
    if (!achievement.rarity.empty())
    {
      try
      {
        double r = std::stod(achievement.rarity);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f%% unlock rate", r);
        rarityVal = buf;
      }
      catch (...)
      {
      }
    }
    std::string rarityCategory;
    if (!achievement.rarity.empty())
    {
      try
      {
        double r = std::stod(achievement.rarity);
        if (r > 50.0)
          rarityCategory = "\u2605 Common";
        else if (r > 10.0)
          rarityCategory = "\u2605\u2605 Uncommon";
        else if (r > 2.0)
          rarityCategory = "\u2605\u2605\u2605 Rare";
        else
          rarityCategory = "\u2605\u2605\u2605\u2605 Ultra Rare";
      }
      catch (...)
      {
      }
    }
    item->SetProperty("Rarity", rarityVal);
    item->SetProperty("RarityCategory", rarityCategory);
    m_items.Add(item);
  }

  // Sort: earned first, then unearned
  CFileItemList earned, unearned;
  for (int i = 0; i < m_items.Size(); ++i)
  {
    const auto item = m_items[i];
    if (!item->GetProperty("Earned").asString().empty())
      earned.Add(item);
    else
      unearned.Add(item);
  }
  m_items.Clear();
  for (int i = 0; i < earned.Size(); ++i)
    m_items.Add(earned[i]);
  for (int i = 0; i < unearned.Size(); ++i)
    m_items.Add(unearned[i]);

  // Calculate weighted progress
  unsigned int totalPoints = 0, earnedPoints = 0;
  for (int i = 0; i < m_items.Size(); ++i)
  {
    const unsigned int pts =
        static_cast<unsigned int>(m_items[i]->GetProperty("Points").asInteger());
    totalPoints += pts;
    if (!m_items[i]->GetProperty("Earned").asString().empty())
      earnedPoints += pts;
  }
  if (totalPoints > 0)
    SetProperty(
        "WeightedProgress",
        StringUtils::Format("{}% Complete", static_cast<int>(earnedPoints * 100.0 / totalPoints)));
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  m_viewControl.SetItems(m_items);
}
