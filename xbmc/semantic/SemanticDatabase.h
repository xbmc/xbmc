/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SemanticTypes.h"
#include "dbwrappers/Database.h"
#include "media/MediaType.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Database for semantic search indexing
 *
 * Manages content chunks from subtitles, transcriptions, and metadata
 * with full-text search capabilities via SQLite FTS5.
 */
class CSemanticDatabase : public CDatabase
{
public:
  CSemanticDatabase();
  ~CSemanticDatabase() override;

  bool Open() override;

  /*!
   * @brief Insert a semantic chunk into the database
   * @param chunk The chunk data to insert
   * @return The chunk ID if successful, -1 otherwise
   */
  int InsertChunk(const SemanticChunk& chunk);

  /*!
   * @brief Get a semantic chunk by ID
   * @param chunkId The chunk ID to retrieve
   * @param chunk Output parameter for the chunk data
   * @return true if successful, false otherwise
   */
  bool GetChunk(int chunkId, SemanticChunk& chunk);

  /*!
   * @brief Get all chunks for a specific media item
   * @param mediaId The media item ID
   * @param mediaType The media type (movie, episode, musicvideo)
   * @param chunks Output vector of chunks
   * @return true if successful, false otherwise
   */
  bool GetChunksForMedia(int mediaId, const MediaType& mediaType, std::vector<SemanticChunk>& chunks);

  /*!
   * @brief Delete all chunks for a specific media item
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @return true if successful, false otherwise
   */
  bool DeleteChunksForMedia(int mediaId, const MediaType& mediaType);

  /*!
   * @brief Delete chunks for a specific media item and source type
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @param sourceType The source type to delete (subtitle, transcription, metadata)
   * @return true if successful, false otherwise
   */
  bool DeleteChunksForMediaBySourceType(int mediaId,
                                        const MediaType& mediaType,
                                        SourceType sourceType);

  /*!
   * @brief Update or insert index state for a media item
   * @param state The index state to update
   * @return true if successful, false otherwise
   */
  bool UpdateIndexState(const SemanticIndexState& state);

  /*!
   * @brief Get index state for a specific media item
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @param state Output parameter for the state data
   * @return true if successful, false otherwise
   */
  bool GetIndexState(int mediaId, const MediaType& mediaType, SemanticIndexState& state);

  /*!
   * @brief Get all media items that need indexing (pending status)
   * @param maxResults Maximum number of results to return
   * @param states Output vector of index states
   * @return true if successful, false otherwise
   */
  bool GetPendingIndexStates(int maxResults, std::vector<SemanticIndexState>& states);

  /*!
   * @brief Perform full-text search on indexed content
   * @param searchQuery The search query
   * @param chunks Output vector of matching chunks
   * @param limit Maximum number of results
   * @return true if successful, false otherwise
   */
  bool SearchChunks(const std::string& searchQuery, std::vector<SemanticChunk>& chunks, int limit = 50);

  /*!
   * @brief Update provider configuration
   * @param providerId Provider identifier (e.g., "whisper", "deepgram")
   * @param displayName Human-readable provider name
   * @param isEnabled Whether the provider is enabled
   * @param apiKeySet Whether an API key is configured
   * @return true if successful, false otherwise
   */
  bool UpdateProvider(const std::string& providerId,
                      const std::string& displayName,
                      bool isEnabled,
                      bool apiKeySet);

  /*!
   * @brief Get provider information
   * @param providerId Provider identifier
   * @param displayName Output parameter for display name
   * @param isEnabled Output parameter for enabled status
   * @param totalMinutesUsed Output parameter for usage tracking
   * @return true if successful, false otherwise
   */
  bool GetProvider(const std::string& providerId,
                   std::string& displayName,
                   bool& isEnabled,
                   float& totalMinutesUsed);

  /*!
   * @brief Update provider usage statistics
   * @param providerId Provider identifier
   * @param minutesUsed Minutes to add to total usage
   * @return true if successful, false otherwise
   */
  bool UpdateProviderUsage(const std::string& providerId, float minutesUsed);

  // ========== Enhanced FTS5 Search Operations ==========

  /*!
   * @brief Perform full-text search with advanced options and BM25 ranking
   * @param query The search query (FTS5 MATCH syntax)
   * @param options Search options (filters, limits, etc.)
   * @return Vector of search results with scores and snippets
   */
  std::vector<SearchResult> SearchChunks(const std::string& query, const SearchOptions& options);

  /*!
   * @brief Get a highlighted snippet for a specific chunk
   * @param query The search query to highlight
   * @param chunkId The chunk ID
   * @param snippetLength Maximum number of tokens in snippet
   * @return Highlighted snippet with <b></b> tags
   */
  std::string GetSnippet(const std::string& query, int64_t chunkId, int snippetLength = 50);

  /*!
   * @brief Get surrounding chunks by timestamp (context window)
   * @param mediaId The media item ID
   * @param mediaType The media type
   * @param timestampMs Center timestamp in milliseconds
   * @param windowMs Window size in milliseconds (default 60s)
   * @return Vector of chunks within the time window
   */
  std::vector<SemanticChunk> GetContext(int mediaId,
                                         const std::string& mediaType,
                                         int64_t timestampMs,
                                         int64_t windowMs = 60000);

  // ========== Batch Operations ==========

  /*!
   * @brief Insert multiple chunks in a single transaction
   * @param chunks Vector of chunks to insert
   * @return true if all chunks inserted successfully, false otherwise
   */
  bool InsertChunks(const std::vector<SemanticChunk>& chunks);

  /*!
   * @brief Remove chunks for media items that no longer exist
   * @return Number of chunks deleted
   */
  int CleanupOrphanedChunks();

  // ========== Statistics ==========

  /*!
   * @brief Get statistics about the semantic index
   * @return IndexStats structure with counts and metrics
   */
  IndexStats GetStats();

  // ========== Transaction Support ==========

  /*!
   * @brief Begin a database transaction
   * @return true if successful, false otherwise
   */
  bool BeginTransaction();

  /*!
   * @brief Commit the current transaction
   * @return true if successful, false otherwise
   */
  bool CommitTransaction();

  /*!
   * @brief Rollback the current transaction
   * @return true if successful, false otherwise
   */
  bool RollbackTransaction();

  // ========== Search History Operations ==========

  /*!
   * @brief Execute a raw SQL query
   * @param sql The SQL query to execute
   * @return true if successful, false otherwise
   */
  bool ExecuteSQLQuery(const std::string& sql);

  /*!
   * @brief Execute a query and return the result dataset
   * @param sql The SQL query to execute
   * @return Pointer to dataset, or nullptr on error
   */
  std::unique_ptr<dbiplus::Dataset> Query(const std::string& sql);

  /*!
   * @brief Get the number of rows affected by the last operation
   * @return Number of changes
   */
  int GetChanges() const;

protected:
  // Database schema management
  void CreateTables() override;
  void CreateAnalytics() override;
  void UpdateTables(int version) override;
  int GetMinSchemaVersion() const override { return 1; }
  int GetSchemaVersion() const override;
  const char* GetBaseDBName() const override { return "SemanticIndex"; }

private:
  /*!
   * @brief Create FTS5 triggers for automatic index updates
   */
  void CreateFTSTriggers();

  /*!
   * @brief Rebuild the FTS5 index
   */
  void RebuildFTSIndex();

  /*!
   * @brief Helper to populate chunk from dataset
   */
  SemanticChunk GetChunkFromDataset();

  /*!
   * @brief Helper to populate index state from dataset
   */
  SemanticIndexState GetIndexStateFromDataset();
};

} // namespace SEMANTIC
} // namespace KODI
