/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameLeaderboards.h"

#include "FileItem.h"
#include "LeaderboardUtils.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/CurlFile.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "jobs/Job.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/ViewState.h"

#include <algorithm>
#include <cmath>
#include <mutex>

using namespace KODI::GAME;

namespace
{
constexpr const char* PROPERTY_STATUS = "Leaderboards.Status";

constexpr int CONTROL_LEADERBOARD_LIST = 3;

//! How long to wait on RetroAchievements before giving up on one leaderboard
constexpr int REQUEST_TIMEOUT_SECS = 10;

//! Sent as a POST body, never a query string: CURL::GetRedacted() hides a
//! URL's password but not its parameters, so a token in one reaches the log
constexpr const char* DOREQUEST_URL = "https://retroachievements.org/dorequest.php";

constexpr const char* PATCH_BODY = "r=patch&u={}&t={}&g={}";

constexpr const char* LBINFO_BODY = "r=lbinfo&i={}&u={}&t={}&c=1&o=0";

/*!
 * \brief Fetches which leaderboards a game has, off the GUI thread
 *
 * Kodi's own patch fetch cannot be relied on for this: it derives the game from
 * a hash it generates itself, and that fails outright for anything the add-on
 * opened on its behalf - a ROM inside an archive or a plugin's cache. The
 * add-on hashes it instead and hands the resolved id over when the game loads,
 * so that is what this asks with.
 */
class CLeaderboardListJob : public CJob
{
public:
  CLeaderboardListJob(unsigned int gameId, std::string username, std::string token)
    : m_gameId(gameId),
      m_username(std::move(username)),
      m_token(std::move(token))
  {
  }

  const char* GetType() const override { return "leaderboard-list"; }

  bool DoWork() override
  {
    const std::string body =
        StringUtils::Format(PATCH_BODY, CURL::Encode(m_username), CURL::Encode(m_token), m_gameId);

    XFILE::CCurlFile curl;
    curl.SetTimeout(REQUEST_TIMEOUT_SECS);

    std::string response;
    if (!curl.Post(DOREQUEST_URL, body, response))
      return false;

    CVariant data;
    if (!CJSONVariantParser::Parse(response, data) || !data["Success"].asBoolean())
      return false;

    const CVariant& patch = data["PatchData"];
    m_gameTitle = patch["Title"].asString();

    const CVariant& leaderboards = patch["Leaderboards"];
    if (!leaderboards.isArray())
      return true;

    for (auto it = leaderboards.begin_array(); it != leaderboards.end_array(); ++it)
    {
      const CVariant& leaderboard = *it;

      // Retired or unfinished ones, which the site does not show either
      if (leaderboard["Hidden"].asBoolean())
        continue;

      LeaderboardInfo info;
      info.id = static_cast<unsigned int>(leaderboard["ID"].asUnsignedInteger());
      info.title = leaderboard["Title"].asString();
      info.description = leaderboard["Description"].asString();
      info.format = leaderboard["Format"].asString();
      info.lowerIsBetter = leaderboard["LowerIsBetter"].asBoolean();

      m_leaderboards.push_back(std::move(info));
    }

    return true;
  }

  const std::string& GetGameTitle() const { return m_gameTitle; }
  const std::vector<LeaderboardInfo>& GetLeaderboards() const { return m_leaderboards; }

private:
  const unsigned int m_gameId;
  const std::string m_username;
  const std::string m_token;

  std::string m_gameTitle;
  std::vector<LeaderboardInfo> m_leaderboards;
};

/*!
 * \brief Fetches the standings of one leaderboard, off the GUI thread
 *
 * One job per leaderboard rather than one for all of them, so that a game with
 * thirty of them fills its list in as the answers come rather than after the
 * last one, and so that closing the dialog abandons what is left.
 */
class CLeaderboardStandingsJob : public CJob
{
public:
  CLeaderboardStandingsJob(unsigned int leaderboardId,
                           std::string format,
                           std::string username,
                           std::string token,
                           unsigned int accountGeneration)
    : m_id(leaderboardId),
      m_format(std::move(format)),
      m_username(std::move(username)),
      m_token(std::move(token)),
      m_accountGeneration(accountGeneration)
  {
  }

  const char* GetType() const override { return "leaderboard-standings"; }

  bool DoWork() override
  {
    const std::string body =
        StringUtils::Format(LBINFO_BODY, m_id, CURL::Encode(m_username), CURL::Encode(m_token));

    XFILE::CCurlFile curl;
    curl.SetTimeout(REQUEST_TIMEOUT_SECS);

    std::string response;
    if (!curl.Post(DOREQUEST_URL, body, response))
      return false;

    CVariant data;
    if (!CJSONVariantParser::Parse(response, data) || !data["Success"].asBoolean())
      return false;

    const CVariant& leaderboard = data["LeaderboardData"];
    m_totalEntries = static_cast<unsigned int>(leaderboard["TotalEntries"].asUnsignedInteger());

    const CVariant& entries = leaderboard["Entries"];
    if (entries.begin_array() != entries.end_array())
    {
      const CVariant& top = *entries.begin_array();
      m_topUsername = top["User"].asString();
      m_topScore = FormatLeaderboardScore(static_cast<int>(top["Score"].asInteger()), m_format);
    }

    // Only present once the player has submitted to this leaderboard
    const CVariant& playerEntry = leaderboard["UserEntry"];
    if (playerEntry.isObject() && !playerEntry["Rank"].isNull())
    {
      m_playerRank = static_cast<unsigned int>(playerEntry["Rank"].asUnsignedInteger());
      m_playerScore =
          FormatLeaderboardScore(static_cast<int>(playerEntry["Score"].asInteger()), m_format);
    }

    return true;
  }

  unsigned int GetLeaderboardId() const { return m_id; }
  unsigned int GetAccountGeneration() const { return m_accountGeneration; }
  unsigned int GetTotalEntries() const { return m_totalEntries; }
  unsigned int GetPlayerRank() const { return m_playerRank; }
  const std::string& GetPlayerScore() const { return m_playerScore; }
  const std::string& GetTopUsername() const { return m_topUsername; }
  const std::string& GetTopScore() const { return m_topScore; }

private:
  const unsigned int m_id;
  const std::string m_format;
  const std::string m_username;
  const std::string m_token;
  const unsigned int m_accountGeneration;

  unsigned int m_totalEntries{0};
  unsigned int m_playerRank{0};
  std::string m_playerScore;
  std::string m_topUsername;
  std::string m_topScore;
};
} // namespace

CDialogGameLeaderboards::CDialogGameLeaderboards()
  : CGUIDialog(WINDOW_DIALOG_GAME_LEADERBOARDS, "DialogGameControllers.xml"),
    CJobQueue(false, 1)
{
}

CDialogGameLeaderboards::~CDialogGameLeaderboards() = default;

void CDialogGameLeaderboards::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LEADERBOARD_LIST));
}

void CDialogGameLeaderboards::OnWindowUnload()
{
  m_viewControl.Reset();

  CGUIDialog::OnWindowUnload();
}

void CDialogGameLeaderboards::OnInitWindow()
{
  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
  const auto& gameSettings = CServiceBroker::GetGameServices().GameSettings();

  if (!gameSettings.GetAchievementsLoggedIn())
  {
    // "Leaderboards", "Sign in to RetroAchievements to see leaderboards"
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, strings.Get(35331),
                                          strings.Get(35333));
    Close();
    return;
  }

  auto& runtime = CServiceBroker::GetGameServices().AchievementRuntime();

  const LeaderboardState state = runtime.GetLeaderboardState();
  const unsigned int gameId = runtime.GetState().gameId;

  // Nothing to show and nothing to ask with: no game, or one RetroAchievements
  // does not know
  if (state.leaderboards.empty() && gameId == 0)
  {
    // "Leaderboards", "This game has no leaderboards"
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strings.Get(35331),
                                          strings.Get(35334));
    Close();
    return;
  }

  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  PopulateList();

  CGUIDialog::OnInitWindow();

  if (m_lastSelected >= 0)
    m_viewControl.SetSelectedItem(m_lastSelected);

  if (!state.loaded)
  {
    // "Loading standings…" while the list itself is fetched
    SetProperty(PROPERTY_STATUS, strings.Get(35342));
    FetchList(gameId);
  }
  else if (!state.leaderboards.empty())
  {
    FetchStandings();
  }
}

void CDialogGameLeaderboards::OnDeinitWindow(int nextWindowID)
{
  // Whatever is still in flight is for a screen that is going away
  CancelJobs();

  {
    std::unique_lock lock(m_section);
    m_items.Clear();
    m_requested.clear();
  }

  m_lastFetched = -1;

  m_viewControl.Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CDialogGameLeaderboards::FetchList(unsigned int gameId)
{
  const auto& gameSettings = CServiceBroker::GetGameServices().GameSettings();

  const std::string username = gameSettings.GetRAUsername();
  const std::string token = gameSettings.GetRAToken();
  if (username.empty() || token.empty())
    return;

  AddJob(new CLeaderboardListJob(gameId, username, token));
}

void CDialogGameLeaderboards::FetchStandings()
{
  const auto& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  auto& runtime = CServiceBroker::GetGameServices().AchievementRuntime();

  // Read either side of the credentials so the two describe one account. Taken
  // afterwards, a sign-in between them would send the last account's token
  // under the new one's generation and the answer would be accepted.
  const unsigned int generation = runtime.GetAccountGeneration();

  const std::string username = gameSettings.GetRAUsername();
  const std::string token = gameSettings.GetRAToken();
  if (username.empty() || token.empty())
    return;

  if (generation != runtime.GetAccountGeneration())
    return;

  const LeaderboardState state = runtime.GetLeaderboardState();

  // Only what is on screen, and only once. Standings are a request each, and a
  // game like Super Mario Bros. has thirty-six leaderboards - asking for them
  // all the moment the dialog opens is a burst of requests for rows most
  // players will never scroll to.
  unsigned int queued = 0;

  for (const LeaderboardInfo& leaderboard : state.leaderboards)
  {
    if (queued >= STANDINGS_PREFETCH)
      break;

    if (leaderboard.standingsLoaded)
      continue;

    {
      // Asked and inserted under one hold, so a leaderboard cannot be
      // requested twice
      std::unique_lock lock(m_section);
      if (!m_requested.insert(leaderboard.id).second)
        continue;
    }

    AddJob(new CLeaderboardStandingsJob(leaderboard.id, leaderboard.format, username, token,
                                        generation));
    ++queued;
  }
}

void CDialogGameLeaderboards::FetchStandingsFor(unsigned int leaderboardId)
{
  if (leaderboardId == 0)
    return;

  {
    std::unique_lock lock(m_section);
    if (m_requested.count(leaderboardId) > 0)
      return;
  }

  const auto& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  auto& runtime = CServiceBroker::GetGameServices().AchievementRuntime();

  // Read either side of the credentials, as in FetchStandings()
  const unsigned int generation = runtime.GetAccountGeneration();

  const std::string username = gameSettings.GetRAUsername();
  const std::string token = gameSettings.GetRAToken();
  if (username.empty() || token.empty())
    return;

  if (generation != runtime.GetAccountGeneration())
    return;

  const LeaderboardState state = runtime.GetLeaderboardState();

  for (const LeaderboardInfo& leaderboard : state.leaderboards)
  {
    if (leaderboard.id != leaderboardId || leaderboard.standingsLoaded)
      continue;

    {
      std::unique_lock lock(m_section);
      if (!m_requested.insert(leaderboardId).second)
        return;
    }

    AddJob(new CLeaderboardStandingsJob(leaderboard.id, leaderboard.format, username, token,
                                        generation));
    return;
  }
}

void CDialogGameLeaderboards::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (StringUtils::EqualsNoCase(job->GetType(), "leaderboard-list"))
  {
    const auto* listJob = static_cast<CLeaderboardListJob*>(job);

    auto& runtime = CServiceBroker::GetGameServices().AchievementRuntime();

    LeaderboardState fetched;
    fetched.gameTitle = listJob->GetGameTitle();
    fetched.leaderboards = listJob->GetLeaderboards();
    fetched.loaded = success;
    runtime.SetLeaderboardState(fetched);

    CLog::Log(LOGINFO, "CDialogGameLeaderboards: {} leaderboard(s) for this game",
              fetched.leaderboards.size());

    CGUIMessage refresh(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(refresh, GetID());

    // Now that there is a list, fill in where everyone stands on each
    if (success && !fetched.leaderboards.empty())
      FetchStandings();
  }
  else if (success && StringUtils::EqualsNoCase(job->GetType(), "leaderboard-standings"))
  {
    const auto* standings = static_cast<CLeaderboardStandingsJob*>(job);

    LeaderboardSummary summary;
    summary.totalEntries = standings->GetTotalEntries();
    summary.playerRank = standings->GetPlayerRank();
    summary.playerScore = standings->GetPlayerScore();
    summary.topUsername = standings->GetTopUsername();
    summary.topScore = standings->GetTopScore();

    CServiceBroker::GetGameServices().AchievementRuntime().SetLeaderboardSummary(
        standings->GetLeaderboardId(), standings->GetAccountGeneration(), summary);

    // Rebuilding a list control is only safe on the GUI thread, so the
    // rebuild is asked for rather than done here
    CGUIMessage refresh(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(refresh, GetID());
  }

  CJobQueue::OnJobComplete(jobID, success, job);
}

void CDialogGameLeaderboards::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // Fetch the standings of whatever the player has moved onto. Done here
  // rather than from a message because the list reports its selection by
  // position, and nothing is sent when a held direction scrolls through rows.
  const int selected = m_viewControl.GetSelectedItem();
  if (selected != m_lastFetched)
  {
    m_lastFetched = selected;

    std::unique_lock lock(m_section);
    if (selected >= 0 && selected < m_items.Size())
    {
      const auto leaderboardId =
          static_cast<unsigned int>(m_items[selected]->GetProperty("LeaderboardId").asInteger());
      lock.unlock();

      FetchStandingsFor(leaderboardId);
    }
  }

  CGUIDialog::Process(currentTime, dirtyregions);
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
      const int action = message.GetParam1();
      if (action == ACTION_SELECT_ITEM || action == ACTION_MOUSE_LEFT_CLICK)
      {
        const int selected = m_viewControl.GetSelectedItem();

        std::unique_lock lock(m_section);
        if (selected >= 0 && selected < m_items.Size())
        {
          const auto leaderboardId = static_cast<unsigned int>(
              m_items[selected]->GetProperty("LeaderboardId").asInteger());
          lock.unlock();

          m_lastSelected = selected;

          // The entries dialog is opened by the skin, which cannot pass
          // anything, so which leaderboard is meant goes through the runtime
          CServiceBroker::GetGameServices().AchievementRuntime().SetSelectedLeaderboard(
              leaderboardId);

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

std::string CDialogGameLeaderboards::DescribeFormat(const std::string& format, bool lowerIsBetter)
{
  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  if (IsTimeFormat(format))
  {
    // "Fastest time" / "Longest time"
    return strings.Get(lowerIsBetter ? 35335 : 35336);
  }

  // "Lowest score" / "Highest score"
  return strings.Get(lowerIsBetter ? 35337 : 35338);
}

void CDialogGameLeaderboards::PopulateList()
{
  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  const LeaderboardState state =
      CServiceBroker::GetGameServices().AchievementRuntime().GetLeaderboardState();

  std::unique_lock lock(m_section);

  m_items.Clear();

  for (const LeaderboardInfo& leaderboard : state.leaderboards)
  {
    auto item = std::make_shared<CFileItem>(leaderboard.title);
    item->SetLabel(leaderboard.title);
    item->SetLabel2(leaderboard.description);

    item->SetProperty("LeaderboardId", static_cast<int>(leaderboard.id));
    item->SetProperty("Format", DescribeFormat(leaderboard.format, leaderboard.lowerIsBetter));
    item->SetProperty("TopUsername", leaderboard.topUsername);
    item->SetProperty("TopScore", leaderboard.topScore);

    // Left empty rather than shown as zero: a leaderboard whose standings have
    // not arrived yet, and one nobody has entered, should not read the same
    if (leaderboard.totalEntries > 0)
    {
      // "{0:d} entries"
      item->SetProperty("TotalEntries",
                        StringUtils::Format(strings.Get(35339), leaderboard.totalEntries));
    }

    if (leaderboard.playerRank > 0)
    {
      // "Your rank: {0:d}"
      item->SetProperty("PlayerRank",
                        StringUtils::Format(strings.Get(35340), leaderboard.playerRank));
      item->SetProperty("PlayerScore", leaderboard.playerScore);
    }
    else if (leaderboard.standingsLoaded)
    {
      // "Not ranked"
      item->SetProperty("PlayerRank", strings.Get(35341));
    }

    m_items.Add(std::move(item));
  }

  lock.unlock();

  const bool empty = m_items.IsEmpty();

  m_viewControl.SetItems(m_items);

  // A game with none and a fetch that failed both arrive with an empty list
  std::string status;
  if (!state.loaded)
    status = strings.Get(35369); // "The leaderboards could not be loaded"
  else if (empty)
    status = strings.Get(35334); // "This game has no leaderboards"

  SetProperty(PROPERTY_STATUS, status);
}
