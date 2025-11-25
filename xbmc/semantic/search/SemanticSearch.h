/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "semantic/SemanticTypes.h"

#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

// Forward declaration
class CSemanticDatabase;

/*!
 * @brief High-level semantic search API
 *
 * Wraps SemanticDatabase FTS5 functionality with user-friendly query
 * processing, normalization, and search history management.
 */
class CSemanticSearch
{
public:
  CSemanticSearch();
  ~CSemanticSearch();

  /*!
   * @brief Initialize with database connection
   * @param database Pointer to the semantic database (must remain valid)
   * @return true if successful, false otherwise
   */
  bool Initialize(CSemanticDatabase* database);

  /*!
   * @brief Check if the search interface is initialized
   * @return true if initialized, false otherwise
   */
  bool IsInitialized() const { return m_database != nullptr; }

  /*!
   * @brief Primary search interface
   * @param query User search query (will be normalized and converted to FTS5)
   * @param options Search options (filters, limits, etc.)
   * @return Vector of search results sorted by relevance
   */
  std::vector<SearchResult> Search(const std::string& query,
                                   const SearchOptions& options = {});

  /*!
   * @brief Get context around a timestamp
   * @param mediaId The media item ID
   * @param mediaType The media type (movie, episode, musicvideo)
   * @param timestampMs Center timestamp in milliseconds
   * @param windowMs Window size in milliseconds (default 60s)
   * @return Vector of chunks within the time window
   */
  std::vector<SemanticChunk> GetContext(int mediaId,
                                        const std::string& mediaType,
                                        int64_t timestampMs,
                                        int64_t windowMs = 60000);

  /*!
   * @brief Get all chunks for a media item
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return Vector of all chunks for the media item
   */
  std::vector<SemanticChunk> GetMediaChunks(int mediaId, const std::string& mediaType);

  /*!
   * @brief Search within a specific media item
   * @param query Search query
   * @param mediaId The media item ID to search within
   * @param mediaType The media type
   * @return Vector of search results limited to the specified media
   */
  std::vector<SearchResult> SearchInMedia(const std::string& query,
                                          int mediaId,
                                          const std::string& mediaType);

  /*!
   * @brief Get search suggestions based on history
   * @param prefix Search prefix to match
   * @param maxSuggestions Maximum number of suggestions to return
   * @return Vector of suggestion strings
   *
   * @note Search history tracking is planned for future implementation
   */
  std::vector<std::string> GetSuggestions(const std::string& prefix, int maxSuggestions = 10);

  /*!
   * @brief Record a search for history/suggestions
   * @param query The search query
   * @param resultCount Number of results returned
   *
   * @note Search history tracking is planned for future implementation
   */
  void RecordSearch(const std::string& query, int resultCount);

  /*!
   * @brief Check if specific media is indexed and searchable
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return true if media has completed indexing, false otherwise
   */
  bool IsMediaSearchable(int mediaId, const std::string& mediaType);

  /*!
   * @brief Get overall search statistics
   * @return IndexStats structure with counts and metrics
   */
  IndexStats GetSearchStats();

private:
  CSemanticDatabase* m_database{nullptr};

  /*!
   * @brief Normalize user query for search
   * @param query Raw user query
   * @return Normalized query (lowercased, trimmed, deduplicated spaces)
   */
  std::string NormalizeQuery(const std::string& query);

  /*!
   * @brief Build FTS5 query from normalized user query
   * @param normalizedQuery Normalized query string
   * @return FTS5 MATCH query with wildcards and proper escaping
   */
  std::string BuildFTS5Query(const std::string& normalizedQuery);

  /*!
   * @brief Escape FTS5 special characters
   * @param term Term to escape
   * @return Escaped term safe for FTS5 queries
   */
  std::string EscapeFTS5SpecialChars(const std::string& term);
};

} // namespace SEMANTIC
} // namespace KODI
