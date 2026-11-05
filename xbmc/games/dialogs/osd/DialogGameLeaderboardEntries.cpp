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
#include "utils/ArtTypes.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "view/ViewState.h"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <optional>

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

//! Asked without the player, RetroAchievements starts at the top of the board.
//! Asked with them, it starts at their own entry on a board they have entered.
constexpr const char* LBINFO_TOP_BODY = "r=lbinfo&i={}&c={}&o=0";
constexpr const char* LBINFO_PLAYER_BODY = "r=lbinfo&i={}&u={}&t={}&c=1&o=0";

//! Where RetroAchievements serves player avatars from
constexpr const char* USER_PIC_URL = "https://i.retroachievements.org/UserPic/{}.png";

constexpr const char* PROPERTY_TITLE = "Leaderboards.Title";
constexpr const char* PROPERTY_DESCRIPTION = "Leaderboards.Description";
constexpr const char* PROPERTY_STATUS = "Leaderboards.Status";
constexpr const char* PROPERTY_PLAYER_BEST = "Leaderboards.PlayerBest";

constexpr const char* PROPERTY_ITEM_RANK = "Rank";
constexpr const char* PROPERTY_ITEM_RANK_LABEL = "RankLabel";
constexpr const char* PROPERTY_ITEM_SCORE = "Score";
constexpr const char* PROPERTY_ITEM_MEDAL = "Medal";
constexpr const char* PROPERTY_ITEM_DATE = "Date";
constexpr const char* PROPERTY_ITEM_DATE_RELATIVE = "DateRelative";
constexpr const char* PROPERTY_ITEM_IS_PLAYER = "IsPlayer";

/*!
 * \brief Asks RetroAchievements for a page of a leaderboard
 *
 * \return The page's rows, or nothing if RetroAchievements didn't answer
 */
std::optional<CVariant> RequestEntries(const std::string& body)
{
  XFILE::CCurlFile curl;
  curl.SetTimeout(REQUEST_TIMEOUT_SECS);

  std::string response;
  if (!curl.Post(DOREQUEST_URL, body, response))
    return {};

  CVariant data;
  if (!CJSONVariantParser::Parse(response, data) || !data["Success"].asBoolean())
    return {};

  return data["LeaderboardData"]["Entries"];
}

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
    const std::optional<CVariant> top =
        RequestEntries(StringUtils::Format(LBINFO_TOP_BODY, m_id, ENTRY_COUNT));
    if (!top)
      return false;

    for (auto it = top->begin_array(); it != top->end_array(); ++it)
      m_entries.push_back(ReadEntry(*it));

    // Seeing where you stand is the point of looking, so the player's own entry
    // is added when they placed below the top of the board
    if (std::none_of(m_entries.begin(), m_entries.end(),
                     [](const LeaderboardEntry& entry) { return entry.isPlayer; }))
    {
      const std::optional<CVariant> own = RequestEntries(StringUtils::Format(
          LBINFO_PLAYER_BODY, m_id, CURL::Encode(m_username), CURL::Encode(m_token)));

      // Kept without it, the player would read as not having entered
      if (!own)
        return false;

      if (own->begin_array() != own->end_array())
      {
        LeaderboardEntry entry = ReadEntry(*own->begin_array());
        if (entry.isPlayer)
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
  LeaderboardEntry ReadEntry(const CVariant& row) const
  {
    LeaderboardEntry entry;
    entry.rank = static_cast<unsigned int>(row["Rank"].asUnsignedInteger());
    entry.username = row["User"].asString();
    entry.score = FormatLeaderboardScore(static_cast<int>(row["Score"].asInteger()), m_format);
    // Sent as a unix timestamp. Shown raw it is a meaningless ten digit
    // number, so it is turned into whatever date format the player has set.
    entry.submitted = static_cast<std::time_t>(row["DateSubmitted"].asInteger());
    entry.isPlayer = StringUtils::EqualsNoCase(entry.username, m_username);
    return entry;
  }

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

  // The shared shell defaults to this list; Process() focuses it once it is ready.
  if (m_defaultControl == CONTROL_ENTRY_LIST)
    m_defaultControl = 0;

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
    // Forced: Open() makes the dialog active before OnInitWindow() runs, so a
    // plain Close() would animate out and flash
    Close(true);
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

  // "Loading standings..."
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

void CDialogGameLeaderboardEntries::Process(unsigned int currentTime,
                                            CDirtyRegionList& dirtyregions)
{
  CGUIDialog::Process(currentTime, dirtyregions);

  const auto* list = GetControl(CONTROL_ENTRY_LIST);
  const auto* focused = GetFocusedControl();
  if ((!focused || !focused->CanFocus()) && list && list->CanFocus())
    m_viewControl.SetFocused();
}

bool CDialogGameLeaderboardEntries::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_NOTIFY_ALL && message.GetParam1() == GUI_MSG_REFRESH_LIST)
  {
    // A status set while closed would hide the list when the dialog next opens,
    // and a list hidden then is never given its rows
    if (!IsActive())
      return true;

    PopulateList();

    // Only when something dropped the page. A refresh that follows a fetch of
    // our own must not ask again, or a failed one would be retried for as long
    // as the dialog stays open.
    if (message.GetParam2() == REFRESH_STANDINGS_INVALIDATED)
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
        item->SetArt(KODI::ART::TYPE::ICON,
                     StringUtils::Format(USER_PIC_URL, CURL::Encode(entry.username)));

      item->SetProperty(PROPERTY_ITEM_RANK, static_cast<int>(entry.rank));
      item->SetProperty(PROPERTY_ITEM_RANK_LABEL, StringUtils::Format("{}", entry.rank));
      item->SetProperty(PROPERTY_ITEM_SCORE, entry.score);

      // "gold" / "silver" / "bronze" for the top three, so a skin can mark
      // them however suits it rather than being handed a drawn medal
      item->SetProperty(PROPERTY_ITEM_MEDAL, RankMedal(entry.rank));

      // Both forms: the age is what gets read at a glance, the date is there
      // for anyone who wants to know exactly when
      if (entry.submitted > 0)
      {
        CDateTime when;
        when.SetFromUTCDateTime(entry.submitted);
        item->SetProperty(PROPERTY_ITEM_DATE, when.GetAsLocalizedDate());
        item->SetProperty(PROPERTY_ITEM_DATE_RELATIVE, FormatRelativeDate(entry.submitted, now));
      }

      // So a skin can pick the player's own row out of the table
      item->SetProperty(PROPERTY_ITEM_IS_PLAYER, entry.isPlayer ? "true" : "");

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
    SetProperty(
        PROPERTY_PLAYER_BEST,
        StringUtils::Format("#{}  ·  {}", StringUtils::FormatNumber(player->rank), player->score));
  }
  else if (loaded)
  {
    // "Not submitted"
    SetProperty(PROPERTY_PLAYER_BEST, strings.Get(35376));
  }
  else
  {
    SetProperty(PROPERTY_PLAYER_BEST, "");
  }

  if (!entries.empty())
    SetStatus("");
  else if (loaded)
    // "Nobody has submitted an entry to this leaderboard yet"
    SetStatus(strings.Get(35369));
  else
    // "The standings could not be loaded"
    SetStatus(strings.Get(35343));
}
