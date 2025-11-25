/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VectorSearcher.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <sqlite3.h>
#include <sstream>

// Forward declaration of sqlite-vec initialization function
extern "C" {
#ifdef SQLITE_CORE
int sqlite3_vec_init(sqlite3* db, char** pzErrMsg, const void* pApi);
#endif
}

namespace KODI
{
namespace SEMANTIC
{

// Private implementation (PIMPL pattern)
struct CVectorSearcher::Impl
{
  sqlite3* db{nullptr};
  bool extensionInitialized{false};
};

CVectorSearcher::CVectorSearcher() : m_impl(std::make_unique<Impl>())
{
}

CVectorSearcher::~CVectorSearcher() = default;

bool CVectorSearcher::InitializeExtension(sqlite3* db)
{
  if (!db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Invalid database handle", __func__);
    return false;
  }

  m_impl->db = db;

#ifdef SQLITE_CORE
  // Enable extension loading
  int rc = sqlite3_enable_load_extension(db, 1);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to enable extension loading: {}",
              __func__, sqlite3_errmsg(db));
    return false;
  }

  // Initialize sqlite-vec extension
  char* errMsg = nullptr;
  rc = sqlite3_vec_init(db, &errMsg, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to initialize sqlite-vec: {}",
              __func__, errMsg ? errMsg : "unknown error");
    if (errMsg)
      sqlite3_free(errMsg);
    return false;
  }

  m_impl->extensionInitialized = true;
  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Successfully initialized sqlite-vec extension", __func__);
  return true;
#else
  CLog::Log(LOGERROR, "CVectorSearcher::{}: sqlite-vec not compiled with SQLITE_CORE", __func__);
  return false;
#endif
}

bool CVectorSearcher::CreateVectorTable()
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  if (!m_impl->extensionInitialized)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Extension not initialized", __func__);
    return false;
  }

  // Create virtual table using vec0
  // embedding FLOAT[384] specifies 384-dimensional float vectors
  // distance_metric=cosine uses cosine distance (0 = identical, 2 = opposite)
  const char* createTableSQL = R"(
    CREATE VIRTUAL TABLE IF NOT EXISTS semantic_vectors USING vec0(
      chunk_id INTEGER PRIMARY KEY,
      embedding FLOAT[384] distance_metric=cosine
    )
  )";

  char* errMsg = nullptr;
  int rc = sqlite3_exec(m_impl->db, createTableSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to create vector table: {}",
              __func__, errMsg ? errMsg : "unknown error");
    if (errMsg)
      sqlite3_free(errMsg);
    return false;
  }

  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Vector table created successfully", __func__);
  return true;
}

bool CVectorSearcher::InsertVector(int64_t chunkId, const std::array<float, 384>& embedding)
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  // Use INSERT OR REPLACE to update existing embeddings
  const char* insertSQL = R"(
    INSERT OR REPLACE INTO semantic_vectors (chunk_id, embedding)
    VALUES (?, vec_f32(?))
  )";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, insertSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare insert statement: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return false;
  }

  // Bind chunk ID
  rc = sqlite3_bind_int64(stmt, 1, chunkId);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind chunk_id: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return false;
  }

  // Bind embedding as blob (float array)
  rc = sqlite3_bind_blob(stmt, 2, embedding.data(),
                         embedding.size() * sizeof(float), SQLITE_TRANSIENT);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind embedding: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return false;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to insert vector: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return false;
  }

  return true;
}

bool CVectorSearcher::DeleteVector(int64_t chunkId)
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  const char* deleteSQL = "DELETE FROM semantic_vectors WHERE chunk_id = ?";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, deleteSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare delete statement: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return false;
  }

  rc = sqlite3_bind_int64(stmt, 1, chunkId);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind chunk_id: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return false;
  }

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to delete vector: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return false;
  }

  return true;
}

std::vector<CVectorSearcher::VectorResult> CVectorSearcher::SearchSimilar(
    const std::array<float, 384>& queryVector,
    int topK)
{
  std::vector<VectorResult> results;

  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return results;
  }

  if (topK <= 0)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Invalid topK value: {}", __func__, topK);
    return results;
  }

  // Vector similarity search using vec_f32() and MATCH
  // The WHERE clause uses MATCH to perform k-NN search
  // Results are automatically ordered by distance (ascending)
  const char* searchSQL = R"(
    SELECT chunk_id, distance
    FROM semantic_vectors
    WHERE embedding MATCH vec_f32(?)
      AND k = ?
    ORDER BY distance
  )";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, searchSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare search statement: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return results;
  }

  // Bind query vector as blob
  rc = sqlite3_bind_blob(stmt, 1, queryVector.data(),
                         queryVector.size() * sizeof(float), SQLITE_TRANSIENT);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind query vector: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return results;
  }

  // Bind k value (number of results)
  rc = sqlite3_bind_int(stmt, 2, topK);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind k value: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return results;
  }

  // Fetch results
  results.reserve(topK);
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
  {
    VectorResult result;
    result.chunkId = sqlite3_column_int64(stmt, 0);
    result.distance = static_cast<float>(sqlite3_column_double(stmt, 1));
    results.push_back(result);
  }

  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Search query failed: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    results.clear();
    return results;
  }

  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Found {} similar vectors", __func__, results.size());
  return results;
}

int64_t CVectorSearcher::GetVectorCount()
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return -1;
  }

  const char* countSQL = "SELECT COUNT(*) FROM semantic_vectors";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, countSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare count statement: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return -1;
  }

  int64_t count = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    count = sqlite3_column_int64(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return count;
}

bool CVectorSearcher::ClearAllVectors()
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  const char* clearSQL = "DELETE FROM semantic_vectors";

  char* errMsg = nullptr;
  int rc = sqlite3_exec(m_impl->db, clearSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to clear vectors: {}",
              __func__, errMsg ? errMsg : "unknown error");
    if (errMsg)
      sqlite3_free(errMsg);
    return false;
  }

  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: All vectors cleared successfully", __func__);
  return true;
}

// ===== Batch Operations =====

bool CVectorSearcher::InsertVectorBatch(
    const std::vector<std::pair<int64_t, std::array<float, 384>>>& vectors)
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  if (vectors.empty())
  {
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Empty vector batch, nothing to insert", __func__);
    return true;
  }

  // Start transaction for better performance
  char* errMsg = nullptr;
  int rc = sqlite3_exec(m_impl->db, "BEGIN TRANSACTION", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to begin transaction: {}",
              __func__, errMsg ? errMsg : "unknown error");
    if (errMsg)
      sqlite3_free(errMsg);
    return false;
  }

  // Insert each vector
  bool success = true;
  for (const auto& [chunkId, embedding] : vectors)
  {
    if (!InsertVector(chunkId, embedding))
    {
      CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to insert vector for chunk {}",
                __func__, chunkId);
      success = false;
      break;
    }
  }

  // Commit or rollback based on success
  if (success)
  {
    rc = sqlite3_exec(m_impl->db, "COMMIT", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
      CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to commit transaction: {}",
                __func__, errMsg ? errMsg : "unknown error");
      if (errMsg)
        sqlite3_free(errMsg);
      sqlite3_exec(m_impl->db, "ROLLBACK", nullptr, nullptr, nullptr);
      return false;
    }
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Successfully inserted {} vectors",
              __func__, vectors.size());
  }
  else
  {
    sqlite3_exec(m_impl->db, "ROLLBACK", nullptr, nullptr, nullptr);
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Rolled back transaction due to error", __func__);
  }

  return success;
}

bool CVectorSearcher::DeleteVectorBatch(const std::vector<int64_t>& chunkIds)
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  if (chunkIds.empty())
  {
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Empty chunk ID list, nothing to delete", __func__);
    return true;
  }

  // Start transaction for better performance
  char* errMsg = nullptr;
  int rc = sqlite3_exec(m_impl->db, "BEGIN TRANSACTION", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to begin transaction: {}",
              __func__, errMsg ? errMsg : "unknown error");
    if (errMsg)
      sqlite3_free(errMsg);
    return false;
  }

  // Delete each vector
  bool success = true;
  for (int64_t chunkId : chunkIds)
  {
    if (!DeleteVector(chunkId))
    {
      CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to delete vector for chunk {}",
                __func__, chunkId);
      success = false;
      break;
    }
  }

  // Commit or rollback based on success
  if (success)
  {
    rc = sqlite3_exec(m_impl->db, "COMMIT", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
      CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to commit transaction: {}",
                __func__, errMsg ? errMsg : "unknown error");
      if (errMsg)
        sqlite3_free(errMsg);
      sqlite3_exec(m_impl->db, "ROLLBACK", nullptr, nullptr, nullptr);
      return false;
    }
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Successfully deleted {} vectors",
              __func__, chunkIds.size());
  }
  else
  {
    sqlite3_exec(m_impl->db, "ROLLBACK", nullptr, nullptr, nullptr);
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Rolled back transaction due to error", __func__);
  }

  return success;
}

// ===== Filtered Search =====

std::vector<CVectorSearcher::VectorResult> CVectorSearcher::SearchSimilarFiltered(
    const std::array<float, 384>& queryVector,
    const std::vector<int64_t>& candidateChunkIds,
    int topK)
{
  std::vector<VectorResult> results;

  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return results;
  }

  if (candidateChunkIds.empty())
  {
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Empty candidate list", __func__);
    return results;
  }

  if (topK <= 0)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Invalid topK value: {}", __func__, topK);
    return results;
  }

  // Build IN clause for filtering
  std::vector<std::string> idStrings;
  idStrings.reserve(candidateChunkIds.size());
  for (int64_t id : candidateChunkIds)
  {
    idStrings.push_back(std::to_string(id));
  }
  std::string inClause = StringUtils::Join(idStrings, ",");

  // Query: Search among candidates only
  std::string searchSQL = StringUtils::Format(
      "SELECT chunk_id, distance "
      "FROM semantic_vectors "
      "WHERE embedding MATCH vec_f32(?) "
      "  AND chunk_id IN ({}) "
      "  AND k = ? "
      "ORDER BY distance",
      inClause);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, searchSQL.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare filtered search statement: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return results;
  }

  // Bind query vector as blob
  rc = sqlite3_bind_blob(stmt, 1, queryVector.data(),
                         queryVector.size() * sizeof(float), SQLITE_TRANSIENT);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind query vector: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return results;
  }

  // Bind k value
  rc = sqlite3_bind_int(stmt, 2, topK);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind k value: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return results;
  }

  // Fetch results
  results.reserve(topK);
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
  {
    VectorResult result;
    result.chunkId = sqlite3_column_int64(stmt, 0);
    result.distance = static_cast<float>(sqlite3_column_double(stmt, 1));
    results.push_back(result);
  }

  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Filtered search query failed: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    results.clear();
    return results;
  }

  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Found {} similar vectors among {} candidates",
            __func__, results.size(), candidateChunkIds.size());
  return results;
}

std::vector<CVectorSearcher::VectorResult> CVectorSearcher::SearchSimilarByMediaType(
    const std::array<float, 384>& queryVector,
    const std::string& mediaType,
    int topK)
{
  std::vector<VectorResult> results;

  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return results;
  }

  if (mediaType.empty())
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Empty media type", __func__);
    return results;
  }

  if (topK <= 0)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Invalid topK value: {}", __func__, topK);
    return results;
  }

  // Query: Search with media type filter via join
  // Note: sqlite-vec doesn't support JOINs in the same way, so we need a workaround
  // First get all chunk IDs for the media type, then search filtered
  const char* getChunksSQL = "SELECT chunk_id FROM semantic_chunks WHERE media_type = ?";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, getChunksSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare get chunks statement: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return results;
  }

  rc = sqlite3_bind_text(stmt, 1, mediaType.c_str(), -1, SQLITE_TRANSIENT);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind media type: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return results;
  }

  // Collect chunk IDs
  std::vector<int64_t> candidateChunkIds;
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
  {
    candidateChunkIds.push_back(sqlite3_column_int64(stmt, 0));
  }

  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to get chunks by media type: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return results;
  }

  if (candidateChunkIds.empty())
  {
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: No chunks found for media type '{}'",
              __func__, mediaType);
    return results;
  }

  // Now perform filtered search
  return SearchSimilarFiltered(queryVector, candidateChunkIds, topK);
}

// ===== Similar Chunks =====

std::vector<CVectorSearcher::VectorResult> CVectorSearcher::FindSimilar(
    int64_t chunkId,
    int topK,
    float maxDistance)
{
  std::vector<VectorResult> results;

  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return results;
  }

  // First get the embedding for this chunk
  std::array<float, 384> embedding;
  if (!GetVector(chunkId, embedding))
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to get embedding for chunk {}",
              __func__, chunkId);
    return results;
  }

  // Then search for similar, requesting one extra to account for the source chunk
  auto allResults = SearchSimilar(embedding, topK + 1);

  // Remove the source chunk and apply distance filter
  for (const auto& result : allResults)
  {
    if (result.chunkId != chunkId && result.distance <= maxDistance)
    {
      results.push_back(result);
      if (results.size() >= static_cast<size_t>(topK))
        break;
    }
  }

  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Found {} similar chunks to chunk {}",
            __func__, results.size(), chunkId);
  return results;
}

// ===== Statistics =====

CVectorSearcher::VectorStats CVectorSearcher::GetStats()
{
  VectorStats stats{};

  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return stats;
  }

  // Get total vectors
  stats.totalVectors = GetVectorCount();

  // Get vectors with valid chunks
  const char* validVectorsSQL = R"(
    SELECT COUNT(*)
    FROM semantic_vectors v
    INNER JOIN semantic_chunks c ON v.chunk_id = c.chunk_id
  )";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, validVectorsSQL, -1, &stmt, nullptr);
  if (rc == SQLITE_OK)
  {
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
      stats.vectorsWithMedia = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }

  // For avgDistance, we could track this in memory during searches
  // For now, return 0.0 as a placeholder (would need search history tracking)
  stats.avgDistance = 0.0f;

  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Stats - total: {}, with media: {}",
            __func__, stats.totalVectors, stats.vectorsWithMedia);
  return stats;
}

// ===== Maintenance Operations =====

bool CVectorSearcher::RebuildIndex()
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  CLog::Log(LOGINFO, "CVectorSearcher::{}: Rebuilding vector index...", __func__);

  // Drop the existing table
  const char* dropTableSQL = "DROP TABLE IF EXISTS semantic_vectors";
  char* errMsg = nullptr;
  int rc = sqlite3_exec(m_impl->db, dropTableSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to drop vector table: {}",
              __func__, errMsg ? errMsg : "unknown error");
    if (errMsg)
      sqlite3_free(errMsg);
    return false;
  }

  // Recreate the table
  if (!CreateVectorTable())
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to recreate vector table", __func__);
    return false;
  }

  CLog::Log(LOGINFO, "CVectorSearcher::{}: Vector index rebuilt successfully", __func__);
  return true;
}

int CVectorSearcher::ValidateIntegrity()
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return -1;
  }

  // Count orphaned vectors (vectors without corresponding chunks)
  const char* orphanedSQL = R"(
    SELECT COUNT(*)
    FROM semantic_vectors v
    LEFT JOIN semantic_chunks c ON v.chunk_id = c.chunk_id
    WHERE c.chunk_id IS NULL
  )";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, orphanedSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare validation query: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return -1;
  }

  int orphanedCount = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    orphanedCount = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);

  CLog::Log(LOGDEBUG, "CVectorSearcher::{}: Found {} orphaned vectors",
            __func__, orphanedCount);
  return orphanedCount;
}

int CVectorSearcher::CleanupOrphanedVectors()
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return -1;
  }

  // First, count orphaned vectors
  int orphanedCount = ValidateIntegrity();
  if (orphanedCount <= 0)
  {
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: No orphaned vectors to clean up", __func__);
    return 0;
  }

  // Delete orphaned vectors
  const char* cleanupSQL = R"(
    DELETE FROM semantic_vectors
    WHERE chunk_id NOT IN (SELECT chunk_id FROM semantic_chunks)
  )";

  char* errMsg = nullptr;
  int rc = sqlite3_exec(m_impl->db, cleanupSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to cleanup orphaned vectors: {}",
              __func__, errMsg ? errMsg : "unknown error");
    if (errMsg)
      sqlite3_free(errMsg);
    return -1;
  }

  int deletedCount = sqlite3_changes(m_impl->db);
  CLog::Log(LOGINFO, "CVectorSearcher::{}: Cleaned up {} orphaned vectors",
            __func__, deletedCount);
  return deletedCount;
}

// ===== Private Helper Methods =====

bool CVectorSearcher::GetVector(int64_t chunkId, std::array<float, 384>& embedding)
{
  if (!m_impl->db)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Database not initialized", __func__);
    return false;
  }

  const char* getVectorSQL = "SELECT embedding FROM semantic_vectors WHERE chunk_id = ?";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(m_impl->db, getVectorSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to prepare get vector statement: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    return false;
  }

  rc = sqlite3_bind_int64(stmt, 1, chunkId);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CVectorSearcher::{}: Failed to bind chunk_id: {}",
              __func__, sqlite3_errmsg(m_impl->db));
    sqlite3_finalize(stmt);
    return false;
  }

  bool found = false;
  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const void* blob = sqlite3_column_blob(stmt, 0);
    int blobSize = sqlite3_column_bytes(stmt, 0);

    if (blob && blobSize == static_cast<int>(embedding.size() * sizeof(float)))
    {
      std::memcpy(embedding.data(), blob, blobSize);
      found = true;
    }
    else
    {
      CLog::Log(LOGERROR, "CVectorSearcher::{}: Invalid embedding size for chunk {}",
                __func__, chunkId);
    }
  }

  sqlite3_finalize(stmt);

  if (!found)
  {
    CLog::Log(LOGDEBUG, "CVectorSearcher::{}: No embedding found for chunk {}",
              __func__, chunkId);
  }

  return found;
}

} // namespace SEMANTIC
} // namespace KODI
