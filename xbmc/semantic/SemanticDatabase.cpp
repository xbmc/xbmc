/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SemanticDatabase.h"

#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace KODI::SEMANTIC;
using namespace dbiplus;

CSemanticDatabase::CSemanticDatabase() = default;

CSemanticDatabase::~CSemanticDatabase() = default;

bool CSemanticDatabase::Open()
{
  return CDatabase::Open(
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseVideo);
}

void CSemanticDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "SemanticDatabase: Creating semantic_chunks table");
  m_pDS->exec(
      "CREATE TABLE semantic_chunks ("
      "  chunk_id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  media_id INTEGER NOT NULL,"
      "  media_type TEXT NOT NULL CHECK(media_type IN ('movie', 'episode', 'musicvideo')),"
      "  source_type TEXT NOT NULL CHECK(source_type IN ('subtitle', 'transcription', 'metadata')),"
      "  source_path TEXT,"
      "  start_ms INTEGER,"
      "  end_ms INTEGER,"
      "  text TEXT NOT NULL,"
      "  language TEXT,"
      "  confidence REAL DEFAULT 1.0,"
      "  created_at TEXT DEFAULT (datetime('now')),"
      "  UNIQUE(media_id, media_type, source_type, start_ms)"
      ")");

  CLog::Log(LOGINFO, "SemanticDatabase: Creating semantic_fts virtual table");
  m_pDS->exec(
      "CREATE VIRTUAL TABLE semantic_fts USING fts5("
      "  text,"
      "  content='semantic_chunks',"
      "  content_rowid='chunk_id',"
      "  tokenize='porter unicode61 remove_diacritics 1'"
      ")");

  CLog::Log(LOGINFO, "SemanticDatabase: Creating semantic_index_state table");
  m_pDS->exec(
      "CREATE TABLE semantic_index_state ("
      "  state_id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  media_id INTEGER NOT NULL,"
      "  media_type TEXT NOT NULL,"
      "  media_path TEXT NOT NULL,"
      "  subtitle_status TEXT DEFAULT 'pending',"
      "  transcription_status TEXT DEFAULT 'pending',"
      "  transcription_provider TEXT,"
      "  transcription_progress REAL DEFAULT 0.0,"
      "  metadata_status TEXT DEFAULT 'pending',"
      "  priority INTEGER DEFAULT 0,"
      "  chunk_count INTEGER DEFAULT 0,"
      "  created_at TEXT DEFAULT (datetime('now')),"
      "  updated_at TEXT DEFAULT (datetime('now')),"
      "  UNIQUE(media_id, media_type)"
      ")");

  CLog::Log(LOGINFO, "SemanticDatabase: Creating semantic_providers table");
  m_pDS->exec(
      "CREATE TABLE semantic_providers ("
      "  provider_id TEXT PRIMARY KEY,"
      "  display_name TEXT NOT NULL,"
      "  is_enabled INTEGER DEFAULT 1,"
      "  api_key_set INTEGER DEFAULT 0,"
      "  total_minutes_used REAL DEFAULT 0.0,"
      "  last_used_at TEXT"
      ")");

  CreateFTSTriggers();
}

void CSemanticDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "SemanticDatabase: Creating indexes");

  // Index for chunk queries by media
  m_pDS->exec(
      "CREATE INDEX ix_semantic_chunks_media ON semantic_chunks (media_id, media_type)");

  // Index for chunk queries by source
  m_pDS->exec(
      "CREATE INDEX ix_semantic_chunks_source ON semantic_chunks (source_type, media_type)");

  // Index for time range queries
  m_pDS->exec("CREATE INDEX ix_semantic_chunks_time ON semantic_chunks (start_ms, end_ms)");

  // Index for index state queries
  m_pDS->exec(
      "CREATE INDEX ix_semantic_index_state_media ON semantic_index_state (media_id, media_type)");

  // Index for pending indexing queries
  m_pDS->exec(
      "CREATE INDEX ix_semantic_index_state_status ON semantic_index_state "
      "(subtitle_status, transcription_status, metadata_status, priority)");

  CLog::Log(LOGINFO, "SemanticDatabase: Creating triggers");

  // Trigger to update chunk count when chunks are inserted
  m_pDS->exec(
      "CREATE TRIGGER semantic_chunks_insert AFTER INSERT ON semantic_chunks "
      "BEGIN "
      "  UPDATE semantic_index_state "
      "  SET chunk_count = chunk_count + 1, updated_at = datetime('now') "
      "  WHERE media_id = NEW.media_id AND media_type = NEW.media_type; "
      "END");

  // Trigger to update chunk count when chunks are deleted
  m_pDS->exec(
      "CREATE TRIGGER semantic_chunks_delete AFTER DELETE ON semantic_chunks "
      "BEGIN "
      "  UPDATE semantic_index_state "
      "  SET chunk_count = chunk_count - 1, updated_at = datetime('now') "
      "  WHERE media_id = OLD.media_id AND media_type = OLD.media_type; "
      "END");
}

void CSemanticDatabase::CreateFTSTriggers()
{
  CLog::Log(LOGINFO, "SemanticDatabase: Creating FTS triggers");

  // Trigger to keep FTS index in sync when chunks are inserted
  m_pDS->exec(
      "CREATE TRIGGER semantic_fts_insert AFTER INSERT ON semantic_chunks "
      "BEGIN "
      "  INSERT INTO semantic_fts(rowid, text) VALUES (NEW.chunk_id, NEW.text); "
      "END");

  // Trigger to keep FTS index in sync when chunks are updated
  m_pDS->exec(
      "CREATE TRIGGER semantic_fts_update AFTER UPDATE ON semantic_chunks "
      "BEGIN "
      "  UPDATE semantic_fts SET text = NEW.text WHERE rowid = NEW.chunk_id; "
      "END");

  // Trigger to keep FTS index in sync when chunks are deleted
  m_pDS->exec(
      "CREATE TRIGGER semantic_fts_delete AFTER DELETE ON semantic_chunks "
      "BEGIN "
      "  DELETE FROM semantic_fts WHERE rowid = OLD.chunk_id; "
      "END");
}

void CSemanticDatabase::UpdateTables(int version)
{
  // Future schema updates will be handled here
  CLog::Log(LOGINFO, "SemanticDatabase: UpdateTables from version {}", version);
}

int CSemanticDatabase::GetSchemaVersion() const
{
  return 1; // Initial schema version
}

int CSemanticDatabase::InsertChunk(const SemanticChunk& chunk)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return -1;

    std::string sql = PrepareSQL(
        "INSERT INTO semantic_chunks "
        "(media_id, media_type, source_type, source_path, start_ms, end_ms, text, language, "
        "confidence) "
        "VALUES (%i, '%s', '%s', '%s', %i, %i, '%s', '%s', %f)",
        chunk.mediaId, chunk.mediaType.c_str(), SourceTypeToString(chunk.sourceType),
        chunk.sourcePath.c_str(), chunk.startMs, chunk.endMs, chunk.text.c_str(),
        chunk.language.c_str(), chunk.confidence);

    if (ExecuteQuery(sql))
    {
      int chunkId = static_cast<int>(m_pDB->lastinsertid());
      CLog::Log(LOGDEBUG, "SemanticDatabase: Inserted chunk {} for media {} ({})", chunkId,
                chunk.mediaId, chunk.mediaType);
      return chunkId;
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to insert chunk for media {} ({})", chunk.mediaId,
               chunk.mediaType);
  }
  return -1;
}

bool CSemanticDatabase::GetChunk(int chunkId, SemanticChunk& chunk)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    std::string sql = PrepareSQL("SELECT * FROM semantic_chunks WHERE chunk_id = %i", chunkId);

    if (!m_pDS->query(sql))
      return false;

    if (m_pDS->eof())
    {
      m_pDS->close();
      return false;
    }

    chunk = GetChunkFromDataset();
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to get chunk {}", chunkId);
  }
  return false;
}

bool CSemanticDatabase::GetChunksForMedia(int mediaId,
                                          const MediaType& mediaType,
                                          std::vector<SemanticChunk>& chunks)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    chunks.clear();

    std::string sql = PrepareSQL(
        "SELECT * FROM semantic_chunks WHERE media_id = %i AND media_type = '%s' ORDER BY start_ms",
        mediaId, mediaType.c_str());

    if (!m_pDS->query(sql))
      return false;

    while (!m_pDS->eof())
    {
      chunks.push_back(GetChunkFromDataset());
      m_pDS->next();
    }
    m_pDS->close();

    CLog::Log(LOGDEBUG, "SemanticDatabase: Retrieved {} chunks for media {} ({})", chunks.size(),
              mediaId, mediaType);
    return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to get chunks for media {} ({})", mediaId, mediaType);
  }
  return false;
}

bool CSemanticDatabase::DeleteChunksForMedia(int mediaId, const MediaType& mediaType)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    std::string sql =
        PrepareSQL("DELETE FROM semantic_chunks WHERE media_id = %i AND media_type = '%s'",
                   mediaId, mediaType.c_str());

    if (ExecuteQuery(sql))
    {
      CLog::Log(LOGDEBUG, "SemanticDatabase: Deleted chunks for media {} ({})", mediaId,
                mediaType);
      return true;
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to delete chunks for media {} ({})", mediaId, mediaType);
  }
  return false;
}

bool CSemanticDatabase::UpdateIndexState(const SemanticIndexState& state)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    // Check if state already exists
    std::string checkSql = PrepareSQL(
        "SELECT state_id FROM semantic_index_state WHERE media_id = %i AND media_type = '%s'",
        state.mediaId, state.mediaType.c_str());

    int existingStateId = GetSingleValueInt(checkSql);

    std::string sql;
    if (existingStateId > 0)
    {
      // Update existing state
      sql = PrepareSQL(
          "UPDATE semantic_index_state SET "
          "media_path = '%s', subtitle_status = '%s', transcription_status = '%s', "
          "transcription_provider = '%s', transcription_progress = %f, metadata_status = '%s', "
          "priority = %i, updated_at = datetime('now') "
          "WHERE state_id = %i",
          state.mediaPath.c_str(), IndexStatusToString(state.subtitleStatus),
          IndexStatusToString(state.transcriptionStatus), state.transcriptionProvider.c_str(),
          state.transcriptionProgress, IndexStatusToString(state.metadataStatus), state.priority,
          existingStateId);
    }
    else
    {
      // Insert new state
      sql = PrepareSQL(
          "INSERT INTO semantic_index_state "
          "(media_id, media_type, media_path, subtitle_status, transcription_status, "
          "transcription_provider, transcription_progress, metadata_status, priority) "
          "VALUES (%i, '%s', '%s', '%s', '%s', '%s', %f, '%s', %i)",
          state.mediaId, state.mediaType.c_str(), state.mediaPath.c_str(),
          IndexStatusToString(state.subtitleStatus),
          IndexStatusToString(state.transcriptionStatus), state.transcriptionProvider.c_str(),
          state.transcriptionProgress, IndexStatusToString(state.metadataStatus), state.priority);
    }

    if (ExecuteQuery(sql))
    {
      CLog::Log(LOGDEBUG, "SemanticDatabase: Updated index state for media {} ({})", state.mediaId,
                state.mediaType);
      return true;
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to update index state for media {} ({})", state.mediaId,
               state.mediaType);
  }
  return false;
}

bool CSemanticDatabase::GetIndexState(int mediaId,
                                      const MediaType& mediaType,
                                      SemanticIndexState& state)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    std::string sql = PrepareSQL(
        "SELECT * FROM semantic_index_state WHERE media_id = %i AND media_type = '%s'", mediaId,
        mediaType.c_str());

    if (!m_pDS->query(sql))
      return false;

    if (m_pDS->eof())
    {
      m_pDS->close();
      return false;
    }

    state = GetIndexStateFromDataset();
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to get index state for media {} ({})", mediaId, mediaType);
  }
  return false;
}

bool CSemanticDatabase::GetPendingIndexStates(int maxResults,
                                               std::vector<SemanticIndexState>& states)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    states.clear();

    std::string sql = PrepareSQL(
        "SELECT * FROM semantic_index_state "
        "WHERE subtitle_status = 'pending' OR transcription_status = 'pending' OR "
        "metadata_status = 'pending' "
        "ORDER BY priority DESC, created_at ASC "
        "LIMIT %i",
        maxResults);

    if (!m_pDS->query(sql))
      return false;

    while (!m_pDS->eof())
    {
      states.push_back(GetIndexStateFromDataset());
      m_pDS->next();
    }
    m_pDS->close();

    CLog::Log(LOGDEBUG, "SemanticDatabase: Retrieved {} pending index states", states.size());
    return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to get pending index states");
  }
  return false;
}

bool CSemanticDatabase::SearchChunks(const std::string& searchQuery,
                                     std::vector<SemanticChunk>& chunks,
                                     int limit)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    chunks.clear();

    // Use FTS5 for full-text search
    std::string sql = PrepareSQL(
        "SELECT c.* FROM semantic_chunks c "
        "INNER JOIN semantic_fts f ON c.chunk_id = f.rowid "
        "WHERE semantic_fts MATCH '%s' "
        "ORDER BY rank "
        "LIMIT %i",
        searchQuery.c_str(), limit);

    if (!m_pDS->query(sql))
      return false;

    while (!m_pDS->eof())
    {
      chunks.push_back(GetChunkFromDataset());
      m_pDS->next();
    }
    m_pDS->close();

    CLog::Log(LOGDEBUG, "SemanticDatabase: Found {} chunks matching '{}'", chunks.size(),
              searchQuery);
    return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to search chunks for query '{}'", searchQuery);
  }
  return false;
}

bool CSemanticDatabase::UpdateProvider(const std::string& providerId,
                                       const std::string& displayName,
                                       bool isEnabled,
                                       bool apiKeySet)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    std::string sql = PrepareSQL(
        "INSERT OR REPLACE INTO semantic_providers "
        "(provider_id, display_name, is_enabled, api_key_set) "
        "VALUES ('%s', '%s', %i, %i)",
        providerId.c_str(), displayName.c_str(), isEnabled ? 1 : 0, apiKeySet ? 1 : 0);

    if (ExecuteQuery(sql))
    {
      CLog::Log(LOGDEBUG, "SemanticDatabase: Updated provider {}", providerId);
      return true;
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to update provider {}", providerId);
  }
  return false;
}

bool CSemanticDatabase::GetProvider(const std::string& providerId,
                                    std::string& displayName,
                                    bool& isEnabled,
                                    float& totalMinutesUsed)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    std::string sql = PrepareSQL(
        "SELECT display_name, is_enabled, total_minutes_used FROM semantic_providers WHERE "
        "provider_id = '%s'",
        providerId.c_str());

    if (!m_pDS->query(sql))
      return false;

    if (m_pDS->eof())
    {
      m_pDS->close();
      return false;
    }

    displayName = m_pDS->fv("display_name").get_asString();
    isEnabled = m_pDS->fv("is_enabled").get_asInt() != 0;
    totalMinutesUsed = m_pDS->fv("total_minutes_used").get_asFloat();

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to get provider {}", providerId);
  }
  return false;
}

bool CSemanticDatabase::UpdateProviderUsage(const std::string& providerId, float minutesUsed)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    std::string sql = PrepareSQL(
        "UPDATE semantic_providers SET "
        "total_minutes_used = total_minutes_used + %f, "
        "last_used_at = datetime('now') "
        "WHERE provider_id = '%s'",
        minutesUsed, providerId.c_str());

    if (ExecuteQuery(sql))
    {
      CLog::Log(LOGDEBUG, "SemanticDatabase: Updated usage for provider {} (+{} minutes)",
                providerId, minutesUsed);
      return true;
    }
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to update provider usage for {}", providerId);
  }
  return false;
}

void CSemanticDatabase::RebuildFTSIndex()
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return;

    CLog::Log(LOGINFO, "SemanticDatabase: Rebuilding FTS index");
    m_pDS->exec("INSERT INTO semantic_fts(semantic_fts) VALUES('rebuild')");
    CLog::Log(LOGINFO, "SemanticDatabase: FTS index rebuild complete");
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to rebuild FTS index");
  }
}

SemanticChunk CSemanticDatabase::GetChunkFromDataset()
{
  SemanticChunk chunk;
  chunk.chunkId = m_pDS->fv("chunk_id").get_asInt();
  chunk.mediaId = m_pDS->fv("media_id").get_asInt();
  chunk.mediaType = m_pDS->fv("media_type").get_asString();
  chunk.sourceType = StringToSourceType(m_pDS->fv("source_type").get_asString());
  chunk.sourcePath = m_pDS->fv("source_path").get_asString();
  chunk.startMs = m_pDS->fv("start_ms").get_asInt();
  chunk.endMs = m_pDS->fv("end_ms").get_asInt();
  chunk.text = m_pDS->fv("text").get_asString();
  chunk.language = m_pDS->fv("language").get_asString();
  chunk.confidence = m_pDS->fv("confidence").get_asFloat();
  chunk.createdAt = m_pDS->fv("created_at").get_asString();
  return chunk;
}

SemanticIndexState CSemanticDatabase::GetIndexStateFromDataset()
{
  SemanticIndexState state;
  state.stateId = m_pDS->fv("state_id").get_asInt();
  state.mediaId = m_pDS->fv("media_id").get_asInt();
  state.mediaType = m_pDS->fv("media_type").get_asString();
  state.mediaPath = m_pDS->fv("media_path").get_asString();
  state.subtitleStatus = StringToIndexStatus(m_pDS->fv("subtitle_status").get_asString());
  state.transcriptionStatus =
      StringToIndexStatus(m_pDS->fv("transcription_status").get_asString());
  state.transcriptionProvider = m_pDS->fv("transcription_provider").get_asString();
  state.transcriptionProgress = m_pDS->fv("transcription_progress").get_asFloat();
  state.metadataStatus = StringToIndexStatus(m_pDS->fv("metadata_status").get_asString());
  state.priority = m_pDS->fv("priority").get_asInt();
  state.chunkCount = m_pDS->fv("chunk_count").get_asInt();
  state.createdAt = m_pDS->fv("created_at").get_asString();
  state.updatedAt = m_pDS->fv("updated_at").get_asString();
  return state;
}
