/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <utility>

namespace KODI
{
namespace SEMANTIC
{

/**
 * @brief Ranking algorithms for combining multiple result lists
 */
enum class RankingAlgorithm
{
  RRF,     // Reciprocal Rank Fusion (default) - rank-based, scale-independent
  Linear,  // Weighted linear combination - score-based, requires normalization
  Borda,   // Borda count - rank-based voting method
  CombMNZ  // CombMNZ method - score-based with non-zero multiplier
};

/**
 * @brief Configuration for result ranking
 */
struct RankingConfig
{
  RankingAlgorithm algorithm{RankingAlgorithm::RRF}; // Default to RRF
  float rrfK{60.0f};      // RRF constant (60 is typical, range: 10-100)
  float weight1{0.5f};    // Weight for first ranking (0.0-1.0)
  float weight2{0.5f};    // Weight for second ranking (0.0-1.0)
  int topK{-1};           // Result limit (-1 = return all results)
};

/**
 * @brief Ranked item with combined score and metadata
 */
struct RankedItem
{
  int64_t id{0};           // Item identifier
  float combinedScore{0.0f}; // Final combined score
  float score1{0.0f};      // Original score from first ranker
  float score2{0.0f};      // Original score from second ranker
  int rank1{-1};           // Position in first list (-1 = not present)
  int rank2{-1};           // Position in second list (-1 = not present)
};

/**
 * @brief Result ranking utility for combining multiple ranked lists
 *
 * CResultRanker provides flexible algorithms for merging results from
 * different sources (e.g., semantic + keyword search, multiple models).
 *
 * Supported algorithms:
 * - RRF (Reciprocal Rank Fusion): Rank-based fusion, robust to score differences
 * - Linear: Weighted score combination with normalization
 * - Borda: Rank-based voting system
 * - CombMNZ: Score combination with non-zero count multiplier
 *
 * Thread-safety: Not thread-safe. Create separate instances per thread.
 */
class CResultRanker
{
public:
  /**
   * @brief Construct ranker with default configuration (RRF algorithm)
   */
  CResultRanker();

  /**
   * @brief Construct ranker with custom configuration
   * @param config Ranking configuration
   */
  explicit CResultRanker(const RankingConfig& config);

  ~CResultRanker() = default;

  // Prevent copying (use move semantics if needed)
  CResultRanker(const CResultRanker&) = delete;
  CResultRanker& operator=(const CResultRanker&) = delete;
  CResultRanker(CResultRanker&&) = default;
  CResultRanker& operator=(CResultRanker&&) = default;

  /**
   * @brief Update ranking configuration
   * @param config New configuration
   */
  void SetConfig(const RankingConfig& config);

  /**
   * @brief Get current configuration
   * @return Current ranking configuration
   */
  const RankingConfig& GetConfig() const { return m_config; }

  /**
   * @brief Combine two ranked lists using configured algorithm
   *
   * @param list1 First ranked list (id, score) pairs in descending score order
   * @param list2 Second ranked list (id, score) pairs in descending score order
   * @return Combined and re-ranked results with metadata
   *
   * @note Lists should be pre-sorted by score (highest first)
   * @note Duplicate IDs are handled appropriately per algorithm
   */
  std::vector<RankedItem> Combine(const std::vector<std::pair<int64_t, float>>& list1,
                                   const std::vector<std::pair<int64_t, float>>& list2);

  /**
   * @brief Combine multiple ranked lists with custom weights
   *
   * @param lists Vector of ranked lists
   * @param weights Weight for each list (must match list count)
   * @return Combined and re-ranked results
   *
   * @note Currently supports RRF and Linear algorithms
   * @note For two lists, prefer Combine() method
   */
  std::vector<RankedItem> CombineMultiple(
      const std::vector<std::vector<std::pair<int64_t, float>>>& lists,
      const std::vector<float>& weights);

  /**
   * @brief Normalize scores to [0, 1] range using min-max normalization
   *
   * @param scores Input (id, score) pairs
   * @return Normalized (id, score) pairs
   *
   * @note Handles edge case where all scores are identical
   * @note Static utility method, can be used independently
   */
  static std::vector<std::pair<int64_t, float>> NormalizeScores(
      const std::vector<std::pair<int64_t, float>>& scores);

  /**
   * @brief Apply cross-encoder re-ranking to improve accuracy
   *
   * Takes already ranked results and re-scores the top-N using a cross-encoder
   * model for more accurate relevance scoring. Cross-encoders jointly encode
   * query-passage pairs, capturing fine-grained interactions that bi-encoders miss.
   *
   * @param rankedResults Already ranked results (sorted by score, descending)
   * @param query Original query text
   * @param passages Map from ID to passage text
   * @param topN Number of top results to re-rank (default: 20)
   * @param scoreWeight Weight for cross-encoder score (0-1, default: 0.7)
   * @return Re-ranked results with updated scores
   *
   * @note Requires cross-encoder to be enabled in settings
   * @note Falls back to original ranking if cross-encoder unavailable
   * @note Only re-ranks top-N results for efficiency (rest retain original order)
   *
   * Formula: finalScore = (1 - weight) * originalScore + weight * crossEncoderScore
   */
  std::vector<RankedItem> ApplyCrossEncoderReRanking(
      const std::vector<RankedItem>& rankedResults,
      const std::string& query,
      const std::unordered_map<int64_t, std::string>& passages,
      int topN = 20,
      float scoreWeight = 0.7f);

private:
  RankingConfig m_config;

  // Algorithm implementations
  std::vector<RankedItem> CombineRRF(const std::vector<std::pair<int64_t, float>>& list1,
                                      const std::vector<std::pair<int64_t, float>>& list2);

  std::vector<RankedItem> CombineLinear(const std::vector<std::pair<int64_t, float>>& list1,
                                         const std::vector<std::pair<int64_t, float>>& list2);

  std::vector<RankedItem> CombineBorda(const std::vector<std::pair<int64_t, float>>& list1,
                                        const std::vector<std::pair<int64_t, float>>& list2);

  std::vector<RankedItem> CombineCombMNZ(const std::vector<std::pair<int64_t, float>>& list1,
                                          const std::vector<std::pair<int64_t, float>>& list2);
};

} // namespace SEMANTIC
} // namespace KODI
