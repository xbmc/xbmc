/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameLeaderboards.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/CurlFile.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/ViewState.h"

#include <cmath>

using namespace KODI::GAME;

CDialogGameLeaderboards::CDialogGameLeaderboards()
  : CGUIDialog(WINDOW_DIALOG_GAME_LEADERBOARDS, "DialogGameControllers.xml")
{
}

void CDialogGameLeaderboards::OnWindowLoaded()
{
  // Initialize ancestor
  CGUIDialog::OnWindowLoaded();

  // Initialize dialog
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(3));
}

void CDialogGameLeaderboards::OnWindowUnload()
{
  // Stop background thread if still running
  m_stopFetch = true;
  if (m_fetchThread.joinable())
    m_fetchThread.join();

  // Reset dialog
  m_viewControl.Reset();

  // Unload ancestor
  CGUIDialog::OnWindowUnload();
}

void CDialogGameLeaderboards::OnInitWindow()
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

  if (!settings.GetLeaderboardsLoaded() || settings.GetLeaderboardState().leaderboards.empty())
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "RetroAchievements",
                                          "This game has no leaderboards", 6000, false, 500);
    Close();
    return;
  }

  PopulateList();
  CGUIDialog::OnInitWindow();
  // Restore focus to last selected board
  if (m_lastSelectedItem >= 0)
    m_viewControl.SetSelectedItem(m_lastSelectedItem);
  // Start background thread to fetch top entries
  m_stopFetch = false;
  m_fetchThread = std::thread(&CDialogGameLeaderboards::FetchTopEntriesAsync, this);
}

bool CDialogGameLeaderboards::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_REFRESH_LIST)
      {
        PopulateList();
        return true;
      }
      break;
    }
    case GUI_MSG_CLICKED:
    {
      const int actionId = message.GetParam1();
      if (actionId == ACTION_SELECT_ITEM || actionId == ACTION_MOUSE_LEFT_CLICK)
      {
        const int selectedItem = m_viewControl.GetSelectedItem();
        if (selectedItem >= 0 && selectedItem < m_items.Size())
        {
          const unsigned int lbId = static_cast<unsigned int>(
              m_items[selectedItem]->GetProperty("LeaderboardId").asInteger());
          CServiceBroker::GetGameServices().GameSettings().SetSelectedLeaderboardId(lbId);
          m_lastSelectedItem = selectedItem;
          CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(
              WINDOW_DIALOG_GAME_LEADERBOARD_ENTRIES);
          return true;
        }
      }
      break;
    }
    default:
      break;
  }
  return CGUIDialog::OnMessage(message);
}

void CDialogGameLeaderboards::FetchTopEntriesAsync()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string username = settings->GetString("gamesachievements.username");
  const std::string token = settings->GetString("gamesachievements.token");

  auto lbState = CServiceBroker::GetGameServices().GameSettings().GetLeaderboardState();

  for (auto& lb : lbState.leaderboards)
  {
    if (m_stopFetch)
      break;

    const std::string url = StringUtils::Format(
        "https://retroachievements.org/dorequest.php?r=lbinfo&i={}&u={}&t={}&c=1&o=0", lb.id,
        CURL::Encode(username), CURL::Encode(token));

    XFILE::CCurlFile curl;
    curl.SetRequestHeader("User-Agent", "Kodi/22.0 RetroPlayer");
    curl.SetTimeout(10);

    std::string response;
    if (!curl.Get(url, response))
      continue;

    CVariant data;
    if (!CJSONVariantParser::Parse(response, data) || !data["Success"].asBoolean())
      continue;

    const CVariant& lbData = data["LeaderboardData"];
    lb.totalEntries = static_cast<unsigned int>(lbData["TotalEntries"].asUnsignedInteger());

    const CVariant& entries = lbData["Entries"];
    if (entries.begin_array() != entries.end_array())
    {
      const CVariant& top = *entries.begin_array();
      lb.topUsername = top["User"].asString();
      // Format score
      const unsigned int score = static_cast<unsigned int>(top["Score"].asUnsignedInteger());
      if (lb.format == "TIME" || lb.format == "FRAMES")
      {
        const double totalSecs = static_cast<double>(score) / 60.0;
        const unsigned int mins = static_cast<unsigned int>(totalSecs) / 60;
        const unsigned int secs = static_cast<unsigned int>(totalSecs) % 60;
        const unsigned int cs =
            static_cast<unsigned int>((totalSecs - std::floor(totalSecs)) * 100);
        lb.topScore = StringUtils::Format("{}:{:02d}.{:02d}", mins, secs, cs);
      }
      else if (lb.format == "TIMESECS")
      {
        lb.topScore = StringUtils::Format("{}:{:02d}", score / 60, score % 60);
      }
      else if (lb.format == "FIXED1")
        lb.topScore = StringUtils::Format("{:.1f}", score / 10.0);
      else if (lb.format == "FIXED2")
        lb.topScore = StringUtils::Format("{:.2f}", score / 100.0);
      else if (lb.format == "FIXED3")
        lb.topScore = StringUtils::Format("{:.3f}", score / 1000.0);
      else
        lb.topScore = std::to_string(score);
    }

    // Player's own rank (present in UserEntry when ranked)
    const CVariant& userEntry = lbData["UserEntry"];
    if (userEntry.isObject() && !userEntry["Rank"].isNull())
      lb.playerRank = static_cast<unsigned int>(userEntry["Rank"].asUnsignedInteger());

    // Save updated state and trigger UI refresh
    CServiceBroker::GetGameServices().GameSettings().SetLeaderboardState(lbState);

    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());
  }
}

void CDialogGameLeaderboards::PopulateList()
{
  m_items.Clear();

  const auto state = CServiceBroker::GetGameServices().GameSettings().GetLeaderboardState();

  for (const auto& lb : state.leaderboards)
  {
    auto item = std::make_shared<CFileItem>(lb.title);
    item->SetLabel(lb.title);
    item->SetLabel2(lb.description);
    item->SetProperty("LeaderboardId", static_cast<int>(lb.id));
    std::string formatLabel;
    if (lb.format == "TIME" || lb.format == "TIMESECS" || lb.format == "FRAMES")
      formatLabel = lb.lowerIsBetter ? "↓ Best Time" : "↑ Best Time";
    else if (lb.format == "SCORE")
      formatLabel = lb.lowerIsBetter ? "↓ Low Score" : "↑ High Score";
    else if (lb.format == "FIXED1" || lb.format == "FIXED2" || lb.format == "FIXED3")
      formatLabel = lb.lowerIsBetter ? "↓ Low Score" : "↑ High Score";
    else
      formatLabel = lb.lowerIsBetter ? "↓ " + lb.format : "↑ " + lb.format;
    item->SetProperty("Format", formatLabel);
    item->SetProperty("TotalEntries", static_cast<int>(lb.totalEntries));
    item->SetProperty("PlayerRank",
                      lb.playerRank > 0 ? StringUtils::Format("Rank #{}", lb.playerRank) : "");
    item->SetProperty("TopUsername", lb.topUsername);
    item->SetProperty("TopScore", lb.topScore);
    item->SetProperty("TotalEntries", lb.totalEntries > 0
                                          ? StringUtils::Format("{} entries", lb.totalEntries)
                                          : "");
    item->SetProperty("LowerIsBetter", lb.lowerIsBetter ? "true" : "");
    m_items.Add(item);
  }

  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  m_viewControl.SetItems(m_items);
}
