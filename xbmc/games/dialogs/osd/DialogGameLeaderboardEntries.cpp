/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameLeaderboardEntries.h"

#include "FileItem.h"
#include "LeaderboardUtils.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "XBDateTime.h"
#include "filesystem/CurlFile.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/ViewState.h"

#include <algorithm>
#include <ctime>
#include <mutex>

using namespace KODI::GAME;

namespace
{
constexpr int CONTROL_ENTRY_LIST = 3;

constexpr int REQUEST_TIMEOUT_SECS = 10;

//! How many rows to ask for. The site shows the top of the table plus the
//! player's own row, and more than this cannot be read from a sofa anyway.
constexpr unsigned int ENTRY_COUNT = 50;

//! Sent as a POST body, never a query string: CURL::GetRedacted() hides a
//! URL's password but not its parameters, so a token in one reaches the log
constexpr const char* DOREQUEST_URL = "https://retroachievements.org/dorequest.php";

constexpr const char* LBINFO_BODY = "r=lbinfo&i={}&u={}&t={}&c={}&o=0";

//! Where RetroAchievements serves player avatars from
constexpr const char* USER_PIC_URL = "https://i.retroachievements.org/UserPic/{}.png";

constexpr const char* PROPERTY_TITLE = "Leaderboards.Title";
constexpr const char* PROPERTY_DESCRIPTION = "Leaderboards.Description";
constexpr const char* PROPERTY_STATUS = "Leaderboards.Status";
constexpr const char* PROPERTY_PLAYER_BEST = "Leaderboards.PlayerBest";

/*!
 * \brief Fetches one leaderboard's standings, off the GUI thread
 */
class CLeaderboardEntriesJob : public CJob
{
public:
  CLeaderboardEntriesJob(unsigned int leaderboardId,
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

  const char* GetType() const override { return "leaderboard-entries"; }

  bool DoWork() override
  {
    const std::string body = StringUtils::Format(LBINFO_BODY, m_id, CURL::Encode(m_username),
                                                 CURL::Encode(m_token), ENTRY_COUNT);

    XFILE::CCurlFile curl;
    curl.SetTimeout(REQUEST_TIMEOUT_SECS);

    std::string response;
    if (!curl.Post(DOREQUEST_URL, body, response))
      return false;

    CVariant data;
    if (!CJSONVariantParser::Parse(response, data) || !data["Success"].asBoolean())
      return false;

    const CVariant& leaderboard = data["LeaderboardData"];
    const CVariant& entries = leaderboard["Entries"];

    for (auto it = entries.begin_array(); it != entries.end_array(); ++it)
    {
      const CVariant& row = *it;

      LeaderboardEntry entry;
      entry.rank = static_cast<unsigned int>(row["Rank"].asUnsignedInteger());
      entry.username = row["User"].asString();
      entry.score = FormatLeaderboardScore(static_cast<int>(row["Score"].asInteger()), m_format);
      // Sent as a unix timestamp. Shown raw it is a meaningless ten digit
      // number, so it is turned into whatever date format the player has set.
      entry.submitted = static_cast<std::time_t>(row["DateSubmitted"].asInteger());
      entry.isPlayer = StringUtils::EqualsNoCase(entry.username, m_username);

      m_entries.push_back(std::move(entry));
    }

    // The player's own standing comes back separately, and is only in the rows
    // above if they placed inside the page that was asked for. Seeing where you
    // stand is the point of looking, so it is appended when it is not.
    const CVariant& playerEntry = leaderboard["UserEntry"];
    if (playerEntry.isObject() && !playerEntry["Rank"].isNull())
    {
      const auto rank = static_cast<unsigned int>(playerEntry["Rank"].asUnsignedInteger());

      const bool alreadyListed =
          std::any_of(m_entries.begin(), m_entries.end(),
                      [rank](const LeaderboardEntry& e) { return e.rank == rank && e.isPlayer; });

      if (!alreadyListed)
      {
        LeaderboardEntry entry;
        entry.rank = rank;
        entry.username = m_username;
        entry.score =
            FormatLeaderboardScore(static_cast<int>(playerEntry["Score"].asInteger()), m_format);
        entry.submitted = static_cast<std::time_t>(playerEntry["DateSubmitted"].asInteger());
        entry.isPlayer = true;

        m_entries.push_back(std::move(entry));
      }
    }

    return true;
  }

  unsigned int GetLeaderboardId() const { return m_id; }
  unsigned int GetAccountGeneration() const { return m_accountGeneration; }
  const std::string& GetUsername() const { return m_username; }
  const std::vector<LeaderboardEntry>& GetEntries() const { return m_entries; }

private:
  const unsigned int m_id;
  const std::string m_format;
  const std::string m_username;
  const std::string m_token;
  const unsigned int m_accountGeneration;

  std::vector<LeaderboardEntry> m_entries;
};
} // namespace

CDialogGameLeaderboardEntries::CDialogGameLeaderboardEntries()
  : CGUIDialog(WINDOW_DIALOG_GAME_LEADERBOARD_ENTRIES, "DialogGameControllers.xml"),
    CJobQueue(false, 1)
{
}

CDialogGameLeaderboardEntries::~CDialogGameLeaderboardEntries() = default;

void CDialogGameLeaderboardEntries::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_ENTRY_LIST));
}

void CDialogGameLeaderboardEntries::OnWindowUnload()
{
  m_viewControl.Reset();

  CGUIDialog::OnWindowUnload();
}

void CDialogGameLeaderboardEntries::OnInitWindow()
{
  auto& runtime = CServiceBroker::GetGameServices().AchievementRuntime();

  m_leaderboardId = runtime.GetSelectedLeaderboard();

  const LeaderboardState state = runtime.GetLeaderboardState();

  const LeaderboardInfo* leaderboard = nullptr;
  for (const LeaderboardInfo& candidate : state.leaderboards)
  {
    if (candidate.id == m_leaderboardId)
    {
      leaderboard = &candidate;
      break;
    }
  }

  if (leaderboard == nullptr)
  {
    CLog::Log(LOGERROR, "CDialogGameLeaderboardEntries: leaderboard {} is not in this game",
              m_leaderboardId);
    Close();
    return;
  }

  SetProperty(PROPERTY_TITLE, leaderboard->title);
  SetProperty(PROPERTY_DESCRIPTION, leaderboard->description);

  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  PopulateList();

  CGUIDialog::OnInitWindow();

  FetchEntries();
}

void CDialogGameLeaderboardEntries::FetchEntries()
{
  auto& runtime = CServiceBroker::GetGameServices().AchievementRuntime();

  const LeaderboardState state = runtime.GetLeaderboardState();

  const auto board = std::find_if(state.leaderboards.begin(), state.leaderboards.end(),
                                  [this](const LeaderboardInfo& candidate)
                                  { return candidate.id == m_leaderboardId; });
  if (board == state.leaderboards.end())
    return;

  // Already looked at this session, so there is nothing to wait for. An empty
  // list is a real answer, a board nobody has entered, so the flag is what
  // says whether the fetch happened.
  if (board->entriesLoaded)
    return;

  const auto& gameSettings = CServiceBroker::GetGameServices().GameSettings();

  // Read either side of the credentials so the two describe one account. Taken
  // afterwards, a sign-in between them would pair the last account's name with
  // the new one's generation and the rows would be accepted as the new one's.
  const unsigned int generation = runtime.GetAccountGeneration();

  const std::string username = gameSettings.GetRAUsername();
  const std::string token = gameSettings.GetRAToken();
  if (username.empty() || token.empty())
    return;

  if (generation != runtime.GetAccountGeneration())
    return;

  // Nor if a previous session kept them and they are still fresh
  std::vector<LeaderboardEntry> remembered;
  if (LoadLeaderboardEntries(m_leaderboardId, username, remembered))
  {
    runtime.SetLeaderboardEntries(m_leaderboardId, generation, remembered);
    PopulateList();
    return;
  }

  // "Loading standings…"
  SetStatus(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35342));

  AddJob(new CLeaderboardEntriesJob(m_leaderboardId, board->format, username, token, generation));
}

void CDialogGameLeaderboardEntries::OnDeinitWindow(int nextWindowID)
{
  CancelJobs();

  {
    std::unique_lock lock(m_section);
    m_items.Clear();
  }

  m_viewControl.Clear();

  SetProperty(PROPERTY_TITLE, "");
  SetProperty(PROPERTY_DESCRIPTION, "");
  SetProperty(PROPERTY_STATUS, "");
  SetProperty(PROPERTY_PLAYER_BEST, "");

  m_leaderboardId = 0;

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CDialogGameLeaderboardEntries::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (StringUtils::EqualsNoCase(job->GetType(), "leaderboard-entries"))
  {
    const auto* entriesJob = static_cast<CLeaderboardEntriesJob*>(job);

    if (success)
    {
      CServiceBroker::GetGameServices().AchievementRuntime().SetLeaderboardEntries(
          entriesJob->GetLeaderboardId(), entriesJob->GetAccountGeneration(),
          entriesJob->GetEntries());

      SaveLeaderboardEntries(entriesJob->GetLeaderboardId(), entriesJob->GetUsername(),
                             entriesJob->GetEntries());
    }
    else
    {
      CLog::Log(LOGERROR, "CDialogGameLeaderboardEntries: could not fetch standings for {}",
                entriesJob->GetLeaderboardId());
    }

    // Rebuilding a list control is only safe on the GUI thread
    CGUIMessage refresh(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(refresh, GetID());
  }

  CJobQueue::OnJobComplete(jobID, success, job);
}

bool CDialogGameLeaderboardEntries::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_NOTIFY_ALL && message.GetParam1() == GUI_MSG_REFRESH_LIST)
  {
    PopulateList();

    // A submission drops the page it invalidated, so a refresh has to go and
    // ask for it rather than draw the empty list that is left
    FetchEntries();
    return true;
  }

  return CGUIDialog::OnMessage(message);
}

void CDialogGameLeaderboardEntries::SetStatus(const std::string& status)
{
  SetProperty(PROPERTY_STATUS, status);
}

void CDialogGameLeaderboardEntries::PopulateList()
{
  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  const LeaderboardState state =
      CServiceBroker::GetGameServices().AchievementRuntime().GetLeaderboardState();

  std::vector<LeaderboardEntry> entries;
  bool loaded = false;
  for (const LeaderboardInfo& leaderboard : state.leaderboards)
  {
    if (leaderboard.id == m_leaderboardId)
    {
      entries = leaderboard.entries;
      loaded = leaderboard.entriesLoaded;
      break;
    }
  }

  const std::time_t now = std::time(nullptr);

  {
    std::unique_lock lock(m_section);

    m_items.Clear();

    for (const LeaderboardEntry& entry : entries)
    {
      auto item = std::make_shared<CFileItem>(entry.username);
      item->SetLabel(entry.username);
      item->SetLabel2(entry.score);

      // A face against every name, the way the site shows them
      if (!entry.username.empty())
        item->SetArt("icon", StringUtils::Format(USER_PIC_URL, CURL::Encode(entry.username)));

      item->SetProperty("Rank", static_cast<int>(entry.rank));
      item->SetProperty("RankLabel", StringUtils::Format("{}", entry.rank));
      item->SetProperty("Score", entry.score);

      // "gold" / "silver" / "bronze" for the top three, so a skin can mark
      // them however suits it rather than being handed a drawn medal
      item->SetProperty("Medal", RankMedal(entry.rank));

      // Both forms: the age is what gets read at a glance, the date is there
      // for anyone who wants to know exactly when
      if (entry.submitted > 0)
      {
        CDateTime when;
        when.SetFromUTCDateTime(entry.submitted);
        item->SetProperty("Date", when.GetAsLocalizedDate());
        item->SetProperty("DateRelative", FormatRelativeDate(entry.submitted, now));
      }

      // So a skin can pick the player's own row out of the table
      item->SetProperty("IsPlayer", entry.isPlayer ? "true" : "");

      m_items.Add(std::move(item));
    }
  }

  m_viewControl.SetItems(m_items);

  // Where the player stands, said once at the top rather than left to be found
  // by reading down the table
  const auto player = std::find_if(entries.begin(), entries.end(),
                                   [](const LeaderboardEntry& e) { return e.isPlayer; });
  if (player != entries.end())
  {
    // "Your best"
    SetProperty(PROPERTY_PLAYER_BEST,
                StringUtils::Format("{}  ·  {}  ·  {}", strings.Get(35350),
                                    StringUtils::Format(strings.Get(35340), player->rank),
                                    player->score));
  }
  else if (!entries.empty())
  {
    // "You have not submitted an entry to this leaderboard yet"
    SetProperty(PROPERTY_PLAYER_BEST, strings.Get(35351));
  }
  else
  {
    SetProperty(PROPERTY_PLAYER_BEST, "");
  }

  if (!entries.empty())
    SetStatus("");
  else if (loaded)
    // "Nobody has submitted an entry to this leaderboard yet"
    SetStatus(strings.Get(35368));
  else
    // "The standings could not be loaded"
    SetStatus(strings.Get(35343));
}
