/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LeaderboardUtils.h"

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "games/AchievementRuntime.h"
#include "profiles/ProfileManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <cctype>
#include <cstdint>

using namespace KODI::GAME;

namespace
{
//! RetroAchievements measures time trials in frames at 60Hz, whatever the
//! console actually ran at
constexpr unsigned int FRAMES_PER_SECOND = 60;

//! Hours, minutes and seconds out of a count of seconds
std::string FormatSeconds(uint32_t seconds)
{
  uint32_t minutes = seconds / 60;
  seconds -= minutes * 60;

  if (minutes < 60)
    return StringUtils::Format("{}:{:02}", minutes, seconds);

  const uint32_t hours = minutes / 60;
  minutes -= hours * 60;

  return StringUtils::Format("{}h{:02}:{:02}", hours, minutes, seconds);
}

std::string FormatCentiseconds(uint32_t centiseconds)
{
  const uint32_t seconds = centiseconds / 100;
  return StringUtils::Format("{}.{:02}", FormatSeconds(seconds), centiseconds - seconds * 100);
}

std::string FormatMinutes(uint32_t minutes)
{
  const uint32_t hours = minutes / 60;
  return StringUtils::Format("{}h{:02}", hours, minutes - hours * 60);
}

//! A scaled value writes its own trailing zeroes, but zero itself stays "0"
std::string FormatScaled(int32_t value, const char* zeroes)
{
  if (value == 0)
    return "0";

  return StringUtils::Format("{}{}", value, zeroes);
}

std::string FormatFixed(int32_t value, int32_t factor, size_t decimals)
{
  // Widened before negating, so the most negative value does not overflow
  const int64_t magnitude = value < 0 ? -static_cast<int64_t>(value) : value;

  // The sign is written separately: below the factor the whole part is zero,
  // which cannot carry it. rcheevos prints "0.50" for -50 in FIXED2; this is
  // the one place we knowingly differ from it.
  return StringUtils::Format("{}{}.{:0{}}", value < 0 ? "-" : "", magnitude / factor,
                             magnitude % factor, decimals);
}

//! Thousands separators through the leading run of digits, the way
//! rc_format_insert_commas does, leaving any unit or fraction after it alone
std::string InsertCommas(const std::string& text)
{
  size_t start = (!text.empty() && text.front() == '-') ? 1 : 0;

  size_t end = start;
  while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end])) != 0)
    ++end;

  if (end - start < 4)
    return text;

  std::string digits = text.substr(start, end - start);
  std::string grouped;
  for (size_t i = 0; i < digits.size(); ++i)
  {
    if (i > 0 && (digits.size() - i) % 3 == 0)
      grouped += ',';
    grouped += digits[i];
  }

  return text.substr(0, start) + grouped + text.substr(end);
}

constexpr std::time_t SECONDS_PER_DAY = 24 * 60 * 60;

//! Rough, and deliberately so: these only decide which sentence to use
constexpr std::time_t DAYS_PER_MONTH = 30;
constexpr std::time_t DAYS_PER_YEAR = 365;
} // namespace

bool KODI::GAME::IsTimeFormat(const std::string& format)
{
  return format == "TIME" || format == "FRAMES" || format == "MILLISECS" || format == "TIMESECS" ||
         format == "SECS" || format == "SECS_AS_MINS" || format == "MINUTES";
}

std::string KODI::GAME::FormatLeaderboardScore(int score, const std::string& format)
{
  const auto unsignedScore = static_cast<uint32_t>(score);

  // A score is a plain count of points, written to six places and never
  // grouped. Every other format reads as a quantity and is grouped below.
  if (format == "SCORE" || format == "POINTS" || format == "OTHER")
    return StringUtils::Format("{:06}", score);

  std::string formatted;

  if (format == "TIME" || format == "FRAMES")
    formatted = FormatCentiseconds(unsignedScore * 10 / 6);
  else if (format == "MILLISECS")
    formatted = FormatCentiseconds(unsignedScore);
  else if (format == "TIMESECS" || format == "SECS")
    formatted = FormatSeconds(unsignedScore);
  else if (format == "SECS_AS_MINS")
    formatted = FormatMinutes(unsignedScore / FRAMES_PER_SECOND);
  else if (format == "MINUTES")
    formatted = FormatMinutes(unsignedScore);
  else if (format == "TENS")
    formatted = FormatScaled(score, "0");
  else if (format == "HUNDREDS")
    formatted = FormatScaled(score, "00");
  else if (format == "THOUSANDS")
    formatted = FormatScaled(score, "000");
  else if (format == "UNSIGNED")
    formatted = std::to_string(unsignedScore);
  else if (format == "FIXED1")
    formatted = FormatFixed(score, 10, 1);
  else if (format == "FIXED2")
    formatted = FormatFixed(score, 100, 2);
  else if (format == "FIXED3")
    formatted = FormatFixed(score, 1000, 3);
  else if (format.size() == 6 && format.compare(0, 5, "FLOAT") == 0 && format[5] >= '1' &&
           format[5] <= '6')
    formatted = StringUtils::Format("{:.{}f}", static_cast<double>(score), format[5] - '0');
  else
    formatted = std::to_string(score);

  return InsertCommas(formatted);
}

std::string KODI::GAME::FormatRelativeDate(std::time_t submitted, std::time_t now)
{
  if (submitted <= 0)
    return {};

  const auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  // A submission timestamped in the future is a clock disagreement rather than
  // anything meaningful, so it is treated as just now
  const std::time_t elapsed = (now > submitted) ? now - submitted : 0;
  const std::time_t days = elapsed / SECONDS_PER_DAY;

  if (days <= 0)
    return strings.Get(33006); // "Today"
  if (days == 1)
    return strings.Get(35346); // "Yesterday"
  if (days < DAYS_PER_MONTH)
    return StringUtils::Format(strings.Get(35347), static_cast<int>(days));

  if (days < DAYS_PER_YEAR)
  {
    const auto months = std::max(1, static_cast<int>(days / DAYS_PER_MONTH));
    if (months == 1)
      return strings.Get(35352); // "Last month"
    return StringUtils::Format(strings.Get(35348), months);
  }

  const auto years = std::max(1, static_cast<int>(days / DAYS_PER_YEAR));
  if (years == 1)
    return strings.Get(35353); // "Last year"
  return StringUtils::Format(strings.Get(35349), years);
}

std::string KODI::GAME::RankMedal(unsigned int rank)
{
  switch (rank)
  {
    case 1:
      return "gold";
    case 2:
      return "silver";
    case 3:
      return "bronze";
    default:
      return {};
  }
}

namespace
{
constexpr const char* CACHE_FILE = "gameleaderboards.xml";
constexpr const char* ROOT_ELEMENT = "gameleaderboards";
constexpr const char* BOARD_ELEMENT = "leaderboard";
constexpr const char* ENTRY_ELEMENT = "entry";

//! Standings older than this are refetched. A table that has stood for months
//! will not have moved overnight, but a week is long enough that somebody may
//! have beaten it.
constexpr std::time_t CACHE_MAX_AGE = 7 * 24 * 60 * 60;

//! Enough leaderboards to cover a session's browsing without the file growing
//! without bound
constexpr size_t MAX_CACHED_BOARDS = 200;

std::string CachePath()
{
  const auto settings = CServiceBroker::GetSettingsComponent();
  if (!settings)
    return {};

  const auto profileManager = settings->GetProfileManager();
  if (!profileManager)
    return {};

  return profileManager->GetUserDataItem(CACHE_FILE);
}
} // namespace

void KODI::GAME::SaveLeaderboardEntries(unsigned int leaderboardId,
                                        const std::string& account,
                                        const std::vector<LeaderboardEntry>& entries)
{
  const std::string path = CachePath();
  if (path.empty() || entries.empty())
    return;

  CXBMCTinyXML2 doc;

  tinyxml2::XMLElement* root = nullptr;
  if (XFILE::CFile::Exists(path) && doc.LoadFile(path) && doc.RootElement() != nullptr &&
      std::string(doc.RootElement()->Value()) == ROOT_ELEMENT)
  {
    root = doc.RootElement();

    // Replace rather than accumulate: the same leaderboard looked at twice
    // should leave one record, not two
    for (auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;)
    {
      auto* next = board->NextSiblingElement(BOARD_ELEMENT);
      if (board->UnsignedAttribute("id") == leaderboardId)
        root->DeleteChild(board);
      board = next;
    }
  }
  else
  {
    doc.Clear();
    root = doc.NewElement(ROOT_ELEMENT);
    doc.InsertEndChild(root);
  }

  if (root == nullptr)
    return;

  // Oldest first, so trimming takes the least recently looked at
  size_t boards = 0;
  for (auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;
       board = board->NextSiblingElement(BOARD_ELEMENT))
    ++boards;

  while (boards >= MAX_CACHED_BOARDS)
  {
    auto* oldest = root->FirstChildElement(BOARD_ELEMENT);
    if (oldest == nullptr)
      break;
    root->DeleteChild(oldest);
    --boards;
  }

  auto* board = doc.NewElement(BOARD_ELEMENT);
  board->SetAttribute("id", leaderboardId);
  board->SetAttribute("account", account.c_str());
  board->SetAttribute("fetched", static_cast<int64_t>(std::time(nullptr)));

  for (const LeaderboardEntry& entry : entries)
  {
    auto* element = doc.NewElement(ENTRY_ELEMENT);
    element->SetAttribute("rank", entry.rank);
    element->SetAttribute("user", entry.username.c_str());
    element->SetAttribute("score", entry.score.c_str());
    element->SetAttribute("submitted", static_cast<int64_t>(entry.submitted));
    element->SetAttribute("player", entry.isPlayer);
    board->InsertEndChild(element);
  }

  root->InsertEndChild(board);

  if (!doc.SaveFile(path))
    CLog::Log(LOGERROR, "Leaderboards: unable to save {}", path);
}

void KODI::GAME::ForgetLeaderboardEntries(unsigned int leaderboardId)
{
  const std::string path = CachePath();
  if (path.empty() || !XFILE::CFile::Exists(path))
    return;

  CXBMCTinyXML2 doc;
  if (!doc.LoadFile(path))
    return;

  auto* root = doc.RootElement();
  if (root == nullptr || std::string(root->Value()) != ROOT_ELEMENT)
    return;

  bool removed = false;
  for (auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;)
  {
    auto* next = board->NextSiblingElement(BOARD_ELEMENT);
    if (board->UnsignedAttribute("id") == leaderboardId)
    {
      root->DeleteChild(board);
      removed = true;
    }
    board = next;
  }

  if (removed && !doc.SaveFile(path))
    CLog::Log(LOGERROR, "Leaderboards: unable to save {}", path);
}

void KODI::GAME::ClearLeaderboardEntries()
{
  const std::string path = CachePath();
  if (path.empty() || !XFILE::CFile::Exists(path))
    return;

  if (!XFILE::CFile::Delete(path))
    CLog::Log(LOGERROR, "Leaderboards: unable to delete {}", path);
  else
    CLog::Log(LOGINFO, "Leaderboards: kept standings cleared");
}

bool KODI::GAME::LoadLeaderboardEntries(unsigned int leaderboardId,
                                        const std::string& account,
                                        std::vector<LeaderboardEntry>& entries)
{
  const std::string path = CachePath();
  if (path.empty() || !XFILE::CFile::Exists(path))
    return false;

  CXBMCTinyXML2 doc;
  if (!doc.LoadFile(path))
    return false;

  const auto* root = doc.RootElement();
  if (root == nullptr || std::string(root->Value()) != ROOT_ELEMENT)
    return false;

  for (const auto* board = root->FirstChildElement(BOARD_ELEMENT); board != nullptr;
       board = board->NextSiblingElement(BOARD_ELEMENT))
  {
    if (board->UnsignedAttribute("id") != leaderboardId)
      continue;

    // The rows carry the account's own standing - which one is the player, and
    // the entry appended for them - so another account's copy is not ours
    const char* cached = board->Attribute("account");
    if (cached == nullptr || account != cached)
      return false;

    int64_t fetched = 0;
    board->QueryInt64Attribute("fetched", &fetched);
    if (fetched <= 0 || std::time(nullptr) - static_cast<std::time_t>(fetched) > CACHE_MAX_AGE)
      return false;

    for (const auto* element = board->FirstChildElement(ENTRY_ELEMENT); element != nullptr;
         element = element->NextSiblingElement(ENTRY_ELEMENT))
    {
      LeaderboardEntry entry;
      entry.rank = element->UnsignedAttribute("rank");

      const char* user = element->Attribute("user");
      entry.username = (user != nullptr) ? user : "";

      const char* score = element->Attribute("score");
      entry.score = (score != nullptr) ? score : "";

      int64_t submitted = 0;
      element->QueryInt64Attribute("submitted", &submitted);
      entry.submitted = static_cast<std::time_t>(submitted);

      entry.isPlayer = element->BoolAttribute("player");

      entries.push_back(std::move(entry));
    }

    return !entries.empty();
  }

  return false;
}
