/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

// Forward declaration
class CSemanticDatabase;

/*!
 * @brief Entry in search history
 */
struct SearchHistoryEntry
{
  int64_t id{-1};                        //!< History entry ID
  int profileId{-1};                     //!< Profile ID
  std::string queryText;                 //!< Search query text
  int resultCount{0};                    //!< Number of results returned
  int64_t timestamp{0};                  //!< Unix timestamp in seconds
  std::vector<int64_t> clickedResultIds; //!< IDs of results that were clicked
};

/*!
 * @brief Manages search history persistence and retrieval
 *
 * Features:
 * - Store recent searches with metadata
 * - Per-profile history tracking
 * - Configurable history size limit
 * - Privacy mode support
 * - Clear history functionality
 *
 * The search history is stored in the semantic_search_history table
 * and respects Kodi's profile system.
 *
 * Example usage:
 * \code
 * CSearchHistory history;
 * if (history.Initialize(database))
 * {
 *   // Add a search
 *   history.AddSearch("action movies", 42);
 *
 *   // Get recent searches
 *   auto recent = history.GetRecentSearches(10);
 *
 *   // Clear all history
 *   history.ClearHistory();
 * }
 * \endcode
 */
class CSearchHistory
{
public:
  CSearchHistory();
  ~CSearchHistory();

  /*!
   * @brief Initialize the search history manager
   * @param database Semantic database instance
   * @return true if initialization succeeded
   */
  bool Initialize(CSemanticDatabase* database);

  /*!
   * @brief Add a search query to the history
   * @param queryText The search query text
   * @param resultCount Number of results returned
   * @param clickedResultIds Optional vector of clicked result IDs
   * @return true if the search was added successfully
   */
  bool AddSearch(const std::string& queryText,
                 int resultCount,
                 const std::vector<int64_t>& clickedResultIds = {});

  /*!
   * @brief Get recent search queries
   * @param limit Maximum number of entries to return
   * @param profileId Profile ID (-1 = current profile)
   * @return Vector of recent search entries, newest first
   */
  std::vector<SearchHistoryEntry> GetRecentSearches(int limit = 100, int profileId = -1);

  /*!
   * @brief Get search history matching a prefix
   * @param prefix Query prefix to match
   * @param limit Maximum number of results
   * @param profileId Profile ID (-1 = current profile)
   * @return Vector of matching history entries
   */
  std::vector<SearchHistoryEntry> GetSearchesByPrefix(const std::string& prefix,
                                                       int limit = 10,
                                                       int profileId = -1);

  /*!
   * @brief Get search frequency (number of times searched)
   * @param queryText The search query
   * @param profileId Profile ID (-1 = current profile)
   * @return Number of times this query has been searched
   */
  int GetSearchFrequency(const std::string& queryText, int profileId = -1);

  /*!
   * @brief Update clicked results for a search entry
   * @param queryText The search query
   * @param clickedResultId The result ID that was clicked
   * @return true if updated successfully
   */
  bool UpdateClickedResult(const std::string& queryText, int64_t clickedResultId);

  /*!
   * @brief Clear all search history
   * @param profileId Profile ID (-1 = current profile, 0 = all profiles)
   * @return true if history was cleared successfully
   */
  bool ClearHistory(int profileId = -1);

  /*!
   * @brief Remove old entries beyond the configured limit
   * @param limit Maximum number of entries to keep per profile
   * @return Number of entries removed
   */
  int CleanupOldEntries(int limit = 100);

  /*!
   * @brief Set privacy mode (don't record history)
   * @param enabled true to enable privacy mode
   */
  void SetPrivacyMode(bool enabled);

  /*!
   * @brief Check if privacy mode is enabled
   * @return true if privacy mode is enabled
   */
  bool IsPrivacyModeEnabled() const { return m_privacyMode; }

  /*!
   * @brief Get the current profile ID
   * @return Current profile ID
   */
  int GetCurrentProfileId() const;

private:
  /*!
   * @brief Get the most recent search entry for a query
   * @param queryText The search query
   * @param profileId Profile ID
   * @return Search history entry, or empty entry if not found
   */
  SearchHistoryEntry GetMostRecentSearch(const std::string& queryText, int profileId);

  CSemanticDatabase* m_database{nullptr};
  mutable CCriticalSection m_criticalSection;
  bool m_privacyMode{false};
  int m_maxHistorySize{100};
};

} // namespace SEMANTIC
} // namespace KODI
