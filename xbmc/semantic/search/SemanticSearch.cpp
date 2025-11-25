/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SemanticSearch.h"

#include "semantic/SemanticDatabase.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace KODI::SEMANTIC;

CSemanticSearch::CSemanticSearch() = default;

CSemanticSearch::~CSemanticSearch() = default;

bool CSemanticSearch::Initialize(CSemanticDatabase* database)
{
  if (database == nullptr)
  {
    CLog::LogF(LOGERROR, "Cannot initialize with null database pointer");
    return false;
  }

  m_database = database;
  CLog::LogF(LOGDEBUG, "SemanticSearch initialized successfully");
  return true;
}

std::vector<SearchResult> CSemanticSearch::Search(const std::string& query,
                                                  const SearchOptions& options)
{
  std::vector<SearchResult> results;

  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Search called before initialization");
    return results;
  }

  if (query.empty())
  {
    CLog::LogF(LOGDEBUG, "Empty search query provided");
    return results;
  }

  try
  {
    // Normalize and convert to FTS5 query
    std::string normalized = NormalizeQuery(query);
    std::string fts5Query = BuildFTS5Query(normalized);

    CLog::LogF(LOGDEBUG, "Searching: '{}' -> FTS5: '{}'", query, fts5Query);

    // Execute search via database
    results = m_database->SearchChunks(fts5Query, options);

    CLog::LogF(LOGDEBUG, "Search returned {} results", results.size());

    // Record search in history (future implementation)
    RecordSearch(query, static_cast<int>(results.size()));
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Exception during search for query '{}'", query);
  }

  return results;
}

std::vector<SemanticChunk> CSemanticSearch::GetContext(int mediaId,
                                                       const std::string& mediaType,
                                                       int64_t timestampMs,
                                                       int64_t windowMs)
{
  std::vector<SemanticChunk> chunks;

  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "GetContext called before initialization");
    return chunks;
  }

  try
  {
    chunks = m_database->GetContext(mediaId, mediaType, timestampMs, windowMs);
    CLog::LogF(LOGDEBUG, "Retrieved {} chunks in context window for media {} ({})", chunks.size(),
               mediaId, mediaType);
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Exception getting context for media {} ({})", mediaId, mediaType);
  }

  return chunks;
}

std::vector<SemanticChunk> CSemanticSearch::GetMediaChunks(int mediaId,
                                                           const std::string& mediaType)
{
  std::vector<SemanticChunk> chunks;

  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "GetMediaChunks called before initialization");
    return chunks;
  }

  try
  {
    if (!m_database->GetChunksForMedia(mediaId, mediaType, chunks))
    {
      CLog::LogF(LOGWARNING, "Failed to retrieve chunks for media {} ({})", mediaId, mediaType);
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Retrieved {} chunks for media {} ({})", chunks.size(), mediaId,
                 mediaType);
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Exception getting chunks for media {} ({})", mediaId, mediaType);
  }

  return chunks;
}

std::vector<SearchResult> CSemanticSearch::SearchInMedia(const std::string& query,
                                                         int mediaId,
                                                         const std::string& mediaType)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "SearchInMedia called before initialization");
    return {};
  }

  // Create search options filtered to specific media
  SearchOptions options;
  options.mediaId = mediaId;
  options.mediaType = mediaType;

  CLog::LogF(LOGDEBUG, "Searching in media {} ({}) for '{}'", mediaId, mediaType, query);

  return Search(query, options);
}

std::vector<std::string> CSemanticSearch::GetSuggestions(const std::string& prefix,
                                                         int maxSuggestions)
{
  std::vector<std::string> suggestions;

  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "GetSuggestions called before initialization");
    return suggestions;
  }

  if (prefix.empty())
  {
    return suggestions;
  }

  // TODO: Implement search history table and query suggestions
  // This will be implemented when the semantic_search_history table is added
  CLog::LogF(LOGDEBUG, "Search suggestions requested for prefix '{}' (not yet implemented)",
             prefix);

  return suggestions;
}

void CSemanticSearch::RecordSearch(const std::string& query, int resultCount)
{
  if (!IsInitialized())
  {
    return;
  }

  // TODO: Implement search history recording
  // This will be implemented when the semantic_search_history table is added
  // INSERT INTO semantic_search_history (query, query_normalized, result_count)
  // VALUES (?, ?, ?)
  CLog::LogF(LOGDEBUG, "Search recorded: '{}' with {} results (history not yet implemented)",
             query, resultCount);
}

bool CSemanticSearch::IsMediaSearchable(int mediaId, const std::string& mediaType)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "IsMediaSearchable called before initialization");
    return false;
  }

  try
  {
    SemanticIndexState state;
    if (!m_database->GetIndexState(mediaId, mediaType, state))
    {
      CLog::LogF(LOGDEBUG, "No index state found for media {} ({})", mediaId, mediaType);
      return false;
    }

    // Media is searchable if any source has completed indexing
    bool searchable = (state.subtitleStatus == IndexStatus::COMPLETED ||
                       state.transcriptionStatus == IndexStatus::COMPLETED ||
                       state.metadataStatus == IndexStatus::COMPLETED);

    CLog::LogF(LOGDEBUG, "Media {} ({}) is {}searchable (chunks: {})", mediaId, mediaType,
               searchable ? "" : "not ", state.chunkCount);

    return searchable && (state.chunkCount > 0);
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Exception checking if media {} ({}) is searchable", mediaId, mediaType);
  }

  return false;
}

IndexStats CSemanticSearch::GetSearchStats()
{
  IndexStats stats;

  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "GetSearchStats called before initialization");
    return stats;
  }

  try
  {
    stats = m_database->GetStats();
    CLog::LogF(LOGDEBUG,
               "Search stats: {} total media, {} indexed, {} chunks, {} queued jobs",
               stats.totalMedia, stats.indexedMedia, stats.totalChunks, stats.queuedJobs);
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Exception getting search statistics");
  }

  return stats;
}

// ========== Query Processing Helpers ==========

std::string CSemanticSearch::NormalizeQuery(const std::string& query)
{
  std::string normalized = query;

  // Lowercase for case-insensitive search
  StringUtils::ToLower(normalized);

  // Trim leading/trailing whitespace
  StringUtils::Trim(normalized);

  // Remove excessive whitespace
  StringUtils::RemoveDuplicatedSpacesAndTabs(normalized);

  return normalized;
}

std::string CSemanticSearch::BuildFTS5Query(const std::string& normalizedQuery)
{
  if (normalizedQuery.empty())
  {
    return "";
  }

  // Split query into individual terms
  std::vector<std::string> terms = StringUtils::Split(normalizedQuery, ' ');

  std::string fts5Query;
  for (const auto& term : terms)
  {
    if (term.empty())
      continue;

    // Escape FTS5 special characters
    std::string escapedTerm = EscapeFTS5SpecialChars(term);

    // Add space separator between terms
    if (!fts5Query.empty())
      fts5Query += " ";

    // Add wildcard suffix for partial matching
    // This allows "car" to match "car", "cars", "cartoon", etc.
    fts5Query += escapedTerm + "*";
  }

  return fts5Query;
}

std::string CSemanticSearch::EscapeFTS5SpecialChars(const std::string& term)
{
  // FTS5 special characters that need escaping: " - + * ( ) : ^
  // We escape them with quotes or by removing them
  std::string escaped;
  escaped.reserve(term.size());

  for (char c : term)
  {
    // Skip FTS5 operator characters to prevent query syntax errors
    // These would otherwise be interpreted as FTS5 operators
    if (c == '"' || c == '-' || c == '+' || c == '*' || c == '(' || c == ')' || c == ':' ||
        c == '^')
    {
      // Skip these characters rather than escaping
      // This prevents malformed FTS5 queries
      continue;
    }
    escaped += c;
  }

  return escaped;
}
