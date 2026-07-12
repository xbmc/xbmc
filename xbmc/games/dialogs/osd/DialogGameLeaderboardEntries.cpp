/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameLeaderboardEntries.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/CurlFile.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/ViewState.h"

#include <cmath>
#include <ctime>

using namespace KODI::GAME;

namespace
{
constexpr auto RA_BASE_URL = "https://retroachievements.org/dorequest.php";
constexpr auto RA_USER_AGENT = "Kodi/22.0 RetroPlayer";
constexpr int RA_PAGE_SIZE = 10;
constexpr int RA_MAX_ENTRIES = 100;
} // namespace

CDialogGameLeaderboardEntries::CDialogGameLeaderboardEntries()
  : CGUIDialog(WINDOW_DIALOG_GAME_LEADERBOARD_ENTRIES, "DialogGameControllers.xml")
{
}

void CDialogGameLeaderboardEntries::OnWindowLoaded()
{
  // Initialize ancestor
  CGUIDialog::OnWindowLoaded();

  // Initialize dialog
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(3));
}

void CDialogGameLeaderboardEntries::OnWindowUnload()
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

void CDialogGameLeaderboardEntries::OnInitWindow()
{
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(3));
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  m_leaderboardId = CServiceBroker::GetGameServices().GameSettings().GetSelectedLeaderboardId();

  const auto state = CServiceBroker::GetGameServices().GameSettings().GetLeaderboardState();
  for (const auto& lb : state.leaderboards)
  {
    if (lb.id == m_leaderboardId)
    {
      m_leaderboardTitle = lb.title;
      m_leaderboardFormat = lb.format;
      break;
    }
  }

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_username = settings->GetString("gamesachievements.username");
  m_token = settings->GetString("gamesachievements.token");

  SetProperty("LeaderboardTitle", m_leaderboardTitle);
  SetProperty("HeaderLabel", m_leaderboardTitle);

  // Initialize fixed player row (shown immediately)
  SetProperty("PlayerRowUsername", m_username);
  SetProperty("PlayerRowNotRanked", "true");
  SetProperty("PlayerRowRank", "-");
  SetProperty("PlayerRowScore", "");
  SetProperty("PlayerRowAvatarUrl", "");

  // Reset state for fresh open
  m_stopFetch = false;
  m_playerFoundInEntries = false;
  m_totalEntries = 0;
  {
    std::lock_guard<std::mutex> lock(m_fetchMutex);
    m_fetchedEntries.clear();
  }

  // Show player placeholder immediately — user sees their row before any fetch completes
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  RefreshList();

  // Ensure list has at least one item so control 3 can accept focus on open
  if (m_items.Size() == 0)
  {
    m_items.Add(std::make_shared<CFileItem>(""));
    m_viewControl.SetItems(m_items);
  }

  CGUIDialog::OnInitWindow();

  // Start background paged fetch
  m_fetchThread = std::thread(&CDialogGameLeaderboardEntries::FetchEntriesAsync, this);
}

bool CDialogGameLeaderboardEntries::OnAction(const CAction& action)
{
  if (m_viewControl.HasControl(GetFocusedControlID()))
  {
    CGUIControl* control = GetControl(GetFocusedControlID());
    if (control && control->OnAction(action))
      return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CDialogGameLeaderboardEntries::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_NOTIFY_ALL:
      if (message.GetParam1() == GUI_MSG_REFRESH_LIST)
      {
        RefreshList();
        return true;
      }
      break;
    default:
      break;
  }
  return CGUIDialog::OnMessage(message);
}

void CDialogGameLeaderboardEntries::RefreshList()
{
  std::lock_guard<std::mutex> lock(m_fetchMutex);
  m_items.Clear();

  for (const auto& item : m_fetchedEntries)
    m_items.Add(item);

  // Always show player row at bottom if not found in fetched entries
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  m_viewControl.SetItems(m_items);
}

void CDialogGameLeaderboardEntries::FetchEntriesAsync()
{
  int offset = 0;

  while (!m_stopFetch)
  {
    const std::string url =
        StringUtils::Format("{}?r=lbinfo&i={}&u={}&t={}&c={}&o={}", RA_BASE_URL, m_leaderboardId,
                            CURL::Encode(m_username), CURL::Encode(m_token), RA_PAGE_SIZE, offset);

    XFILE::CCurlFile curl;
    curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
    curl.SetTimeout(10);

    std::string response;
    if (!curl.Get(url, response))
      break;

    CVariant data;
    if (!CJSONVariantParser::Parse(response, data) || !data["Success"].asBoolean())
      break;

    const CVariant& lbData = data["LeaderboardData"];

    // Update header with total count on first page
    if (offset == 0)
    {
      m_totalEntries = static_cast<unsigned int>(lbData["TotalEntries"].asUnsignedInteger());
      SetProperty("HeaderLabel",
                  StringUtils::Format("{} — {} players", m_leaderboardTitle, m_totalEntries));
    }

    const CVariant& entries = lbData["Entries"];
    if (entries.begin_array() == entries.end_array())
      break; // No more entries

    bool anyAdded = false;
    for (auto it = entries.begin_array(); it != entries.end_array() && !m_stopFetch; ++it)
    {
      const CVariant& entry = *it;
      const unsigned int rank = static_cast<unsigned int>(entry["Rank"].asUnsignedInteger());
      const std::string entryUsername = entry["User"].asString();
      const unsigned int score = static_cast<unsigned int>(entry["Score"].asUnsignedInteger());
      const bool isPlayer = StringUtils::EqualsNoCase(entryUsername, m_username);

      if (isPlayer)
      {
        m_playerFoundInEntries = true;
        SetProperty("PlayerRowRank", std::to_string(rank));
        SetProperty("PlayerRowScore", FormatScore(m_leaderboardFormat, score));
        SetProperty("PlayerRowNotRanked", "");
        SetProperty("PlayerRowAvatarUrl", entry["AvatarUrl"].asString());
        anyAdded = true;
        continue;
      }

      auto item = std::make_shared<CFileItem>(entryUsername);
      item->SetLabel(entryUsername);
      item->SetLabel2(FormatScore(m_leaderboardFormat, score));
      item->SetProperty("Rank", std::to_string(rank));
      item->SetProperty("IsPlayer", isPlayer ? "true" : "");
      item->SetArt("thumb", entry["AvatarUrl"].asString());

      const std::time_t dateTs =
          static_cast<std::time_t>(entry["DateSubmitted"].asUnsignedInteger());
      if (dateTs > 0)
      {
        struct tm* tmInfo = std::localtime(&dateTs);
        char dateBuf[32];
        std::strftime(dateBuf, sizeof(dateBuf), "%b %d, %Y", tmInfo);
        item->SetProperty("DateSubmitted", std::string(dateBuf));
      }

      {
        std::lock_guard<std::mutex> lock(m_fetchMutex);
        m_fetchedEntries.push_back(item);
      }
      anyAdded = true;
    }

    if (!anyAdded || m_stopFetch)
      break;

    // Push update to UI after each page
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID());

    offset += RA_PAGE_SIZE;

    if (offset >= static_cast<int>(m_totalEntries) || offset >= RA_MAX_ENTRIES)
      break;
  }

  // Cache final state back to leaderboard settings
  if (!m_stopFetch)
  {
    auto lbState = CServiceBroker::GetGameServices().GameSettings().GetLeaderboardState();
    for (auto& lb : lbState.leaderboards)
    {
      if (lb.id == m_leaderboardId)
      {
        lb.totalEntries = m_totalEntries;
        std::lock_guard<std::mutex> lock(m_fetchMutex);
        if (!m_fetchedEntries.empty())
        {
          lb.topUsername = m_fetchedEntries[0]->GetLabel();
          lb.topScore = m_fetchedEntries[0]->GetLabel2();
        }
        for (const auto& item : m_fetchedEntries)
        {
          if (!item->GetProperty("IsPlayer").asString().empty() &&
              item->GetProperty("NotRanked").asString().empty())
          {
            try
            {
              lb.playerRank =
                  static_cast<unsigned int>(std::stoul(item->GetProperty("Rank").asString()));
            }
            catch (...)
            {
            }
            lb.playerScore = item->GetLabel2();
          }
        }
        break;
      }
    }
    CServiceBroker::GetGameServices().GameSettings().SetLeaderboardState(lbState);
  }
}

std::string CDialogGameLeaderboardEntries::FormatScore(const std::string& format,
                                                       unsigned int score) const
{
  if (format == "TIME" || format == "FRAMES")
  {
    const double totalSecs = static_cast<double>(score) / 60.0;
    const unsigned int minutes = static_cast<unsigned int>(totalSecs) / 60;
    const unsigned int seconds = static_cast<unsigned int>(totalSecs) % 60;
    const unsigned int centiseconds =
        static_cast<unsigned int>((totalSecs - std::floor(totalSecs)) * 100);
    return StringUtils::Format("{}:{:02d}.{:02d}", minutes, seconds, centiseconds);
  }
  else if (format == "TIMESECS")
    return StringUtils::Format("{}:{:02d}", score / 60, score % 60);
  else if (format == "FIXED1")
    return StringUtils::Format("{:.1f}", score / 10.0);
  else if (format == "FIXED2")
    return StringUtils::Format("{:.2f}", score / 100.0);
  else if (format == "FIXED3")
    return StringUtils::Format("{:.3f}", score / 1000.0);
  return std::to_string(score);
}
