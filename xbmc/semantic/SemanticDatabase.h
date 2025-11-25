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
