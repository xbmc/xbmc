/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct sqlite3;

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief Vector similarity search wrapper for sqlite-vec extension
 *
 * This class provides a high-level interface to the sqlite-vec extension
 * for storing and querying 384-dimensional embeddings using vector similarity search.
 *
 * Features:
 * - Store embeddings associated with text chunks
 * - Search for similar vectors using cosine similarity
 * - Manage vector lifecycle (insert, delete)
 *
 * The sqlite-vec extension must be initialized on the database connection
 * before using this class.
 */
class CVectorSearcher
{
public:
  CVectorSearcher();
  ~CVectorSearcher();

  /*!
   * \brief Initialize the sqlite-vec extension on a database connection
   * \param db SQLite database connection handle
   * \return true if initialization succeeded, false otherwise
   *
   * This must be called after opening the database connection and before
   * creating vector tables or performing vector operations.
   */
  bool InitializeExtension(sqlite3* db);

  /*!
   * \brief Create the vector table if it doesn't exist
   * \return true if table was created or already exists, false on error
   *
   * Creates a virtual table using vec0 with 384-dimensional float vectors
   * and cosine distance metric.
   */
  bool CreateVectorTable();

  /*!
   * \brief Insert or update an embedding for a chunk
   * \param chunkId The ID of the text chunk (from semantic_chunks table)
   * \param embedding The 384-dimensional embedding vector
   * \return true if insertion succeeded, false otherwise
   *
   * If an embedding already exists for this chunk ID, it will be replaced.
   */
  bool InsertVector(int64_t chunkId, const std::array<float, 384>& embedding);

  /*!
   * \brief Delete an embedding for a chunk
   * \param chunkId The ID of the text chunk
   * \return true if deletion succeeded, false otherwise
   */
  bool DeleteVector(int64_t chunkId);

  /*!
   * \brief Search result containing chunk ID and distance
   */
  struct VectorResult
  {
    int64_t chunkId;  //!< ID of the matching chunk
    float distance;   //!< Cosine distance (0 = identical, 2 = opposite)
  };

  /*!
   * \brief Search for similar vectors (k-nearest neighbors)
   * \param queryVector The query embedding vector
   * \param topK Number of results to return (default: 50)
   * \return Vector of results sorted by distance (closest first)
   *
   * Performs a cosine similarity search to find the topK most similar
   * vectors in the database.
   */
  std::vector<VectorResult> SearchSimilar(const std::array<float, 384>& queryVector,
                                          int topK = 50);

  /*!
   * \brief Get the number of vectors stored
   * \return Count of vectors in the table, or -1 on error
   */
  int64_t GetVectorCount();

  /*!
   * \brief Clear all vectors from the table
   * \return true if successful, false otherwise
   */
  bool ClearAllVectors();

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace SEMANTIC
} // namespace KODI
