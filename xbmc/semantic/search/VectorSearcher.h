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

  // ===== Batch Operations =====

  /*!
   * \brief Insert multiple embeddings efficiently using a transaction
   * \param vectors Vector of (chunkId, embedding) pairs to insert
   * \return true if all insertions succeeded, false if any failed
   *
   * Uses a database transaction for better performance when inserting
   * many vectors. If any insertion fails, all changes are rolled back.
   */
  bool InsertVectorBatch(
      const std::vector<std::pair<int64_t, std::array<float, 384>>>& vectors);

  /*!
   * \brief Delete multiple vectors efficiently using a transaction
   * \param chunkIds Vector of chunk IDs to delete
   * \return true if all deletions succeeded, false if any failed
   */
  bool DeleteVectorBatch(const std::vector<int64_t>& chunkIds);

  // ===== Filtered Search =====

  /*!
   * \brief Search for similar vectors among specific candidates
   * \param queryVector The query embedding vector
   * \param candidateChunkIds Vector of chunk IDs to search within
   * \param topK Number of results to return (default: 50)
   * \return Vector of results sorted by distance (closest first)
   *
   * This is useful for hybrid search where you want to combine
   * keyword search results (candidateChunkIds) with vector similarity.
   */
  std::vector<VectorResult> SearchSimilarFiltered(
      const std::array<float, 384>& queryVector,
      const std::vector<int64_t>& candidateChunkIds,
      int topK = 50);

  /*!
   * \brief Search for similar vectors filtered by media type
   * \param queryVector The query embedding vector
   * \param mediaType Media type to filter by (e.g., "movie", "episode")
   * \param topK Number of results to return (default: 50)
   * \return Vector of results sorted by distance (closest first)
   *
   * Performs a join with semantic_chunks to filter by media type.
   */
  std::vector<VectorResult> SearchSimilarByMediaType(
      const std::array<float, 384>& queryVector,
      const std::string& mediaType,
      int topK = 50);

  // ===== Similar Chunks =====

  /*!
   * \brief Find chunks similar to an existing chunk
   * \param chunkId ID of the source chunk
   * \param topK Number of results to return (default: 20)
   * \param maxDistance Maximum distance threshold (default: 1.0)
   * \return Vector of results sorted by distance (closest first)
   *
   * Useful for "more like this" features. The source chunk itself
   * is excluded from results.
   */
  std::vector<VectorResult> FindSimilar(int64_t chunkId,
                                        int topK = 20,
                                        float maxDistance = 1.0f);

  // ===== Statistics =====

  /*!
   * \brief Statistics about the vector index
   */
  struct VectorStats
  {
    int64_t totalVectors;        //!< Total number of vectors stored
    int64_t vectorsWithMedia;    //!< Vectors linked to valid chunks
    float avgDistance;           //!< Average distance in recent searches
  };

  /*!
   * \brief Get statistics about the vector index
   * \return VectorStats structure with current statistics
   */
  VectorStats GetStats();

  // ===== Maintenance Operations =====

  /*!
   * \brief Rebuild the vector index
   * \return true if rebuild succeeded, false otherwise
   *
   * Recreates the vector table and index structure. This can be useful
   * for maintenance or after database corruption.
   */
  bool RebuildIndex();

  /*!
   * \brief Validate vector/chunk integrity
   * \return Count of orphaned vectors (vectors without corresponding chunks)
   *
   * Checks for vectors that reference non-existent chunks.
   */
  int ValidateIntegrity();

  /*!
   * \brief Remove vectors for chunks that no longer exist
   * \return Count of orphaned vectors removed
   *
   * Performs cleanup by deleting vectors whose chunk_id doesn't
   * exist in the semantic_chunks table.
   */
  int CleanupOrphanedVectors();

private:
  /*!
   * \brief Get the embedding vector for a chunk
   * \param chunkId The ID of the chunk
   * \param embedding Output parameter to store the embedding
   * \return true if the embedding was found, false otherwise
   */
  bool GetVector(int64_t chunkId, std::array<float, 384>& embedding);

  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace SEMANTIC
} // namespace KODI
