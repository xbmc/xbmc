/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VectorSearcher.h"

#include "utils/log.h"

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

} // namespace SEMANTIC
} // namespace KODI
