/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SearchHistory.h"

#include "ServiceBroker.h"
#include "profiles/ProfileManager.h"
#include "semantic/SemanticDatabase.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <chrono>

using namespace KODI::SEMANTIC;

CSearchHistory::CSearchHistory() = default;

CSearchHistory::~CSearchHistory() = default;

bool CSearchHistory::Initialize(CSemanticDatabase* database)
{
  if (!database)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Invalid database pointer");
    return false;
  }

  m_database = database;
  CLog::Log(LOGINFO, "CSearchHistory: Successfully initialized");
  return true;
}

int CSearchHistory::GetCurrentProfileId() const
{
  auto profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();
  if (profileManager)
  {
    return profileManager->GetCurrentProfileId();
  }
  return 0; // Default to master profile
}

bool CSearchHistory::AddSearch(const std::string& queryText,
                                int resultCount,
                                const std::vector<int64_t>& clickedResultIds)
{
  if (!m_database || queryText.empty())
    return false;

  if (m_privacyMode)
  {
    CLog::Log(LOGDEBUG, "CSearchHistory: Privacy mode enabled, not recording search");
    return true; // Return true to avoid errors, but don't record
  }

  CSingleLock lock(m_criticalSection);

  int profileId = GetCurrentProfileId();
  int64_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  // Convert clicked result IDs to JSON array
  CVariant clickedArray(CVariant::VariantTypeArray);
  for (int64_t id : clickedResultIds)
  {
    clickedArray.push_back(id);
  }
  std::string clickedJson = CJSONVariantWriter::Write(clickedArray, false);

  try
  {
    std::string sql = StringUtils::Format(
        "INSERT INTO semantic_search_history "
        "(profile_id, query_text, result_count, timestamp, clicked_result_ids) "
        "VALUES (%d, '%s', %d, %lld, '%s')",
        profileId, StringUtils::Replace(queryText, "'", "''").c_str(), resultCount,
        static_cast<long long>(timestamp), clickedJson.c_str());

    m_database->ExecuteSQLQuery(sql);

    CLog::Log(LOGDEBUG, "CSearchHistory: Added search '{}' with {} results", queryText,
              resultCount);

    // Cleanup old entries if needed
    CleanupOldEntries(m_maxHistorySize);

    return true;
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Failed to add search: {}", ex.what());
    return false;
  }
}

std::vector<SearchHistoryEntry> CSearchHistory::GetRecentSearches(int limit, int profileId)
{
  std::vector<SearchHistoryEntry> entries;

  if (!m_database)
    return entries;

  CSingleLock lock(m_criticalSection);

  if (profileId == -1)
    profileId = GetCurrentProfileId();

  try
  {
    std::string sql = StringUtils::Format(
        "SELECT id, profile_id, query_text, result_count, timestamp, clicked_result_ids "
        "FROM semantic_search_history "
        "WHERE profile_id = %d "
        "ORDER BY timestamp DESC "
        "LIMIT %d",
        profileId, limit);

    auto results = m_database->Query(sql);

    if (!results)
      return entries;

    while (!results->eof())
    {
      SearchHistoryEntry entry;
      entry.id = results->fv("id").get_asInt64();
      entry.profileId = results->fv("profile_id").get_asInt();
      entry.queryText = results->fv("query_text").get_asString();
      entry.resultCount = results->fv("result_count").get_asInt();
      entry.timestamp = results->fv("timestamp").get_asInt64();

      // Parse clicked result IDs from JSON
      std::string clickedJson = results->fv("clicked_result_ids").get_asString();
      if (!clickedJson.empty())
      {
        CVariant clickedArray;
        if (CJSONVariantParser::Parse(clickedJson, clickedArray))
        {
          if (clickedArray.isArray())
          {
            for (size_t i = 0; i < clickedArray.size(); i++)
            {
              entry.clickedResultIds.push_back(clickedArray[i].asInteger());
            }
          }
        }
      }

      entries.push_back(entry);
      results->next();
    }

    CLog::Log(LOGDEBUG, "CSearchHistory: Retrieved {} recent searches", entries.size());
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Failed to get recent searches: {}", ex.what());
  }

  return entries;
}

std::vector<SearchHistoryEntry> CSearchHistory::GetSearchesByPrefix(const std::string& prefix,
                                                                      int limit,
                                                                      int profileId)
{
  std::vector<SearchHistoryEntry> entries;

  if (!m_database || prefix.empty())
    return entries;

  CSingleLock lock(m_criticalSection);

  if (profileId == -1)
    profileId = GetCurrentProfileId();

  try
  {
    std::string sql = StringUtils::Format(
        "SELECT id, profile_id, query_text, result_count, timestamp, clicked_result_ids "
        "FROM semantic_search_history "
        "WHERE profile_id = %d AND query_text LIKE '%s%%' "
        "ORDER BY timestamp DESC "
        "LIMIT %d",
        profileId, StringUtils::Replace(prefix, "'", "''").c_str(), limit);

    auto results = m_database->Query(sql);

    if (!results)
      return entries;

    while (!results->eof())
    {
      SearchHistoryEntry entry;
      entry.id = results->fv("id").get_asInt64();
      entry.profileId = results->fv("profile_id").get_asInt();
      entry.queryText = results->fv("query_text").get_asString();
      entry.resultCount = results->fv("result_count").get_asInt();
      entry.timestamp = results->fv("timestamp").get_asInt64();

      // Parse clicked result IDs from JSON
      std::string clickedJson = results->fv("clicked_result_ids").get_asString();
      if (!clickedJson.empty())
      {
        CVariant clickedArray;
        if (CJSONVariantParser::Parse(clickedJson, clickedArray))
        {
          if (clickedArray.isArray())
          {
            for (size_t i = 0; i < clickedArray.size(); i++)
            {
              entry.clickedResultIds.push_back(clickedArray[i].asInteger());
            }
          }
        }
      }

      entries.push_back(entry);
      results->next();
    }

    CLog::Log(LOGDEBUG, "CSearchHistory: Found {} searches matching prefix '{}'", entries.size(),
              prefix);
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Failed to get searches by prefix: {}", ex.what());
  }

  return entries;
}

int CSearchHistory::GetSearchFrequency(const std::string& queryText, int profileId)
{
  if (!m_database || queryText.empty())
    return 0;

  CSingleLock lock(m_criticalSection);

  if (profileId == -1)
    profileId = GetCurrentProfileId();

  try
  {
    std::string sql = StringUtils::Format(
        "SELECT COUNT(*) as frequency "
        "FROM semantic_search_history "
        "WHERE profile_id = %d AND query_text = '%s'",
        profileId, StringUtils::Replace(queryText, "'", "''").c_str());

    auto results = m_database->Query(sql);

    if (!results->eof())
    {
      return results->fv("frequency").get_asInt();
    }
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Failed to get search frequency: {}", ex.what());
  }

  return 0;
}

SearchHistoryEntry CSearchHistory::GetMostRecentSearch(const std::string& queryText, int profileId)
{
  SearchHistoryEntry entry;

  if (!m_database || queryText.empty())
    return entry;

  if (profileId == -1)
    profileId = GetCurrentProfileId();

  try
  {
    std::string sql = StringUtils::Format(
        "SELECT id, profile_id, query_text, result_count, timestamp, clicked_result_ids "
        "FROM semantic_search_history "
        "WHERE profile_id = %d AND query_text = '%s' "
        "ORDER BY timestamp DESC "
        "LIMIT 1",
        profileId, StringUtils::Replace(queryText, "'", "''").c_str());

    auto results = m_database->Query(sql);

    if (!results->eof())
    {
      entry.id = results->fv("id").get_asInt64();
      entry.profileId = results->fv("profile_id").get_asInt();
      entry.queryText = results->fv("query_text").get_asString();
      entry.resultCount = results->fv("result_count").get_asInt();
      entry.timestamp = results->fv("timestamp").get_asInt64();

      // Parse clicked result IDs from JSON
      std::string clickedJson = results->fv("clicked_result_ids").get_asString();
      if (!clickedJson.empty())
      {
        CVariant clickedArray;
        if (CJSONVariantParser::Parse(clickedJson, clickedArray))
        {
          if (clickedArray.isArray())
          {
            for (size_t i = 0; i < clickedArray.size(); i++)
            {
              entry.clickedResultIds.push_back(clickedArray[i].asInteger());
            }
          }
        }
      }
    }
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Failed to get most recent search: {}", ex.what());
  }

  return entry;
}

bool CSearchHistory::UpdateClickedResult(const std::string& queryText, int64_t clickedResultId)
{
  if (!m_database || queryText.empty())
    return false;

  if (m_privacyMode)
  {
    return true; // Don't record in privacy mode
  }

  CSingleLock lock(m_criticalSection);

  int profileId = GetCurrentProfileId();

  // Get the most recent search entry
  auto entry = GetMostRecentSearch(queryText, profileId);
  if (entry.id < 0)
  {
    CLog::Log(LOGWARNING, "CSearchHistory: No history entry found for '{}'", queryText);
    return false;
  }

  // Add the clicked result ID if not already present
  if (std::find(entry.clickedResultIds.begin(), entry.clickedResultIds.end(), clickedResultId) ==
      entry.clickedResultIds.end())
  {
    entry.clickedResultIds.push_back(clickedResultId);

    // Convert to JSON
    CVariant clickedArray(CVariant::VariantTypeArray);
    for (int64_t id : entry.clickedResultIds)
    {
      clickedArray.push_back(id);
    }
    std::string clickedJson = CJSONVariantWriter::Write(clickedArray, false);

    try
    {
      std::string sql = StringUtils::Format("UPDATE semantic_search_history "
                                             "SET clicked_result_ids = '%s' "
                                             "WHERE id = %lld",
                                             clickedJson.c_str(), static_cast<long long>(entry.id));

      m_database->ExecuteSQLQuery(sql);
      CLog::Log(LOGDEBUG, "CSearchHistory: Updated clicked results for '{}'", queryText);
      return true;
    }
    catch (const std::exception& ex)
    {
      CLog::Log(LOGERROR, "CSearchHistory: Failed to update clicked result: {}", ex.what());
      return false;
    }
  }

  return true;
}

bool CSearchHistory::ClearHistory(int profileId)
{
  if (!m_database)
    return false;

  CSingleLock lock(m_criticalSection);

  try
  {
    std::string sql;
    if (profileId == -1)
    {
      // Clear current profile
      profileId = GetCurrentProfileId();
      sql = StringUtils::Format("DELETE FROM semantic_search_history WHERE profile_id = %d",
                                 profileId);
    }
    else if (profileId == 0)
    {
      // Clear all profiles
      sql = "DELETE FROM semantic_search_history";
    }
    else
    {
      // Clear specific profile
      sql = StringUtils::Format("DELETE FROM semantic_search_history WHERE profile_id = %d",
                                 profileId);
    }

    m_database->ExecuteSQLQuery(sql);
    CLog::Log(LOGINFO, "CSearchHistory: Cleared history for profile {}", profileId);
    return true;
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Failed to clear history: {}", ex.what());
    return false;
  }
}

int CSearchHistory::CleanupOldEntries(int limit)
{
  if (!m_database || limit <= 0)
    return 0;

  CSingleLock lock(m_criticalSection);

  int profileId = GetCurrentProfileId();

  try
  {
    // Delete entries beyond the limit, keeping the most recent ones
    std::string sql = StringUtils::Format(
        "DELETE FROM semantic_search_history "
        "WHERE profile_id = %d AND id NOT IN ("
        "  SELECT id FROM semantic_search_history "
        "  WHERE profile_id = %d "
        "  ORDER BY timestamp DESC "
        "  LIMIT %d"
        ")",
        profileId, profileId, limit);

    m_database->ExecuteSQLQuery(sql);
    int deletedCount = m_database->GetChanges();

    if (deletedCount > 0)
    {
      CLog::Log(LOGDEBUG, "CSearchHistory: Cleaned up {} old entries", deletedCount);
    }

    return deletedCount;
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchHistory: Failed to cleanup old entries: {}", ex.what());
    return 0;
  }
}

void CSearchHistory::SetPrivacyMode(bool enabled)
{
  CSingleLock lock(m_criticalSection);
  m_privacyMode = enabled;
  CLog::Log(LOGINFO, "CSearchHistory: Privacy mode {}", enabled ? "enabled" : "disabled");
}
