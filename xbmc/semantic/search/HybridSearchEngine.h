/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "semantic/SemanticTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

// Forward declarations
class CSemanticDatabase;
class CEmbeddingEngine;
class CSemanticSearch;
class CVectorSearcher;

/*!
 * @brief Search mode for hybrid search engine
 */
enum class SearchMode
{
  Hybrid,       //!< Combined FTS5 + Vector (default, best relevance)
  KeywordOnly,  //!< FTS5 only (faster, exact matches)
  SemanticOnly  //!< Vector only (conceptual matches)
};

/*!
 * @brief Options for hybrid search queries
 */
struct HybridSearchOptions
{
  SearchMode mode{SearchMode::Hybrid};  //!< Search mode
  float keywordWeight{0.4f};            //!< FTS5 weight in hybrid mode
  float vectorWeight{0.6f};             //!< Vector weight in hybrid mode
  int keywordTopK{100};                 //!< Number of FTS5 candidates
  int vectorTopK{100};                  //!< Number of vector candidates
  int maxResults{20};                   //!< Final result limit

  // Filtering options (from SearchOptions)
  std::string mediaType;      //!< Filter by media type (empty = all)
  int mediaId{-1};            //!< Filter by specific media ID (-1 = all)
  float minConfidence{0.0f};  //!< Minimum confidence threshold
};

/*!
 * @brief Hybrid search result with score breakdown
 */
struct HybridSearchResult
{
  int64_t chunkId{-1};  //!< Chunk ID from database
  SemanticChunk chunk;  //!< Full chunk data

  // Score breakdown
  float combinedScore{0.0f};  //!< Combined RRF score
  float keywordScore{0.0f};   //!< BM25 normalized score (0-1)
  float vectorScore{0.0f};    //!< Cosine similarity score (0-1)

  // Display fields
  std::string snippet;             //!< Text snippet for display
  std::string formattedTimestamp;  //!< Formatted timestamp (e.g., "1:23:45")
};

/*!
 * @brief Hybrid search engine combining FTS5 keyword and vector similarity search
 *
 * This class implements a hybrid search strategy that combines traditional
 * keyword search (BM25 via FTS5) with semantic vector search for optimal
 * relevance. Results are merged using Reciprocal Rank Fusion (RRF).
 *
 * Features:
 * - Hybrid search mode for best relevance
 * - Keyword-only mode for exact matches
 * - Semantic-only mode for conceptual similarity
 * - Configurable score weighting
 * - Result enrichment with snippets and timestamps
 * - Context retrieval around results
 *
 * Example usage:
 * \code
 * CHybridSearchEngine engine;
 * engine.Initialize(database, embeddingEngine);
 *
 * HybridSearchOptions options;
 * options.mode = SearchMode::Hybrid;
 * options.maxResults = 20;
 *
 * auto results = engine.Search("detective solving mystery", options);
 * \endcode
 */
class CHybridSearchEngine
{
public:
  CHybridSearchEngine();
  ~CHybridSearchEngine();

  /*!
   * @brief Initialize the hybrid search engine
   * @param database Pointer to semantic database (must remain valid)
   * @param embeddingEngine Pointer to embedding engine (must remain valid)
   * @param vectorSearcher Pointer to vector searcher (must remain valid)
   * @return true if initialization succeeded, false otherwise
   */
  bool Initialize(CSemanticDatabase* database,
                  CEmbeddingEngine* embeddingEngine,
                  CVectorSearcher* vectorSearcher);

  /*!
   * @brief Check if the engine is initialized
   * @return true if initialized and ready to search
   */
  bool IsInitialized() const;

  /*!
   * @brief Main search interface
   * @param query User search query
   * @param options Search options (mode, weights, filters)
   * @return Vector of hybrid search results sorted by relevance
   */
  std::vector<HybridSearchResult> Search(const std::string& query,
                                         const HybridSearchOptions& options = {});

  /*!
   * @brief Find chunks similar to a given chunk
   * @param chunkId ID of the source chunk
   * @param topK Maximum number of results to return (default: 20)
   * @return Vector of similar chunks sorted by similarity
   *
   * Uses vector similarity only (no keyword search).
   * Useful for "more like this" features.
   */
  std::vector<HybridSearchResult> FindSimilar(int64_t chunkId, int topK = 20);

  /*!
   * @brief Get temporal context around a search result
   * @param result The search result to get context for
   * @param windowMs Time window in milliseconds (default: 30 seconds)
   * @return Vector of chunks within the time window
   *
   * Returns chunks from the same media item within the specified
   * time window centered on the result's timestamp.
   */
  std::vector<SemanticChunk> GetResultContext(const HybridSearchResult& result,
                                              int64_t windowMs = 30000);

private:
  CSemanticDatabase* m_database{nullptr};
  CEmbeddingEngine* m_embeddingEngine{nullptr};
  CVectorSearcher* m_vectorSearcher{nullptr};

  std::unique_ptr<CSemanticSearch> m_keywordSearch;

  // Search implementation methods
  std::vector<HybridSearchResult> SearchKeywordOnly(const std::string& query,
                                                    const HybridSearchOptions& options);

  std::vector<HybridSearchResult> SearchSemanticOnly(const std::string& query,
                                                     const HybridSearchOptions& options);

  std::vector<HybridSearchResult> SearchHybrid(const std::string& query,
                                               const HybridSearchOptions& options);

  // Result combination using Reciprocal Rank Fusion
  std::vector<HybridSearchResult> CombineResultsRRF(
      const std::vector<SearchResult>& keywordResults,
      const std::vector<CVectorSearcher::VectorResult>& vectorResults,
      const HybridSearchOptions& options);

  // Score normalization helpers
  float NormalizeBM25Score(float score, float maxScore);
  float CosineDistanceToSimilarity(float distance);

  // Result enrichment
  HybridSearchResult EnrichResult(int64_t chunkId, float keywordScore, float vectorScore);
  std::string FormatTimestamp(int64_t ms);
};

} // namespace SEMANTIC
} // namespace KODI
