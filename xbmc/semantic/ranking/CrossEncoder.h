/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Configuration for cross-encoder re-ranking
 */
struct CrossEncoderConfig
{
  bool enabled{false};                    //!< Enable cross-encoder re-ranking
  int topN{20};                           //!< Number of top results to re-rank
  float scoreWeight{1.0f};                //!< Weight for cross-encoder score (0-1, 1 = replace)
  std::string modelPath;                  //!< Path to ONNX model file
  bool lazyLoad{true};                    //!< Load model on first use
  int idleTimeoutSec{300};                //!< Unload after idle seconds (0 = never)
  int batchSize{8};                       //!< Batch size for inference
  bool useGPU{false};                     //!< Use GPU if available (future)
};

/*!
 * @brief Query-passage pair for cross-encoder scoring
 */
struct QueryPassagePair
{
  std::string query;                      //!< Query text
  std::string passage;                    //!< Passage/document text
  int64_t id{-1};                         //!< Original item ID
  float originalScore{0.0f};              //!< Original ranking score
};

/*!
 * @brief Result after cross-encoder re-ranking
 */
struct CrossEncoderResult
{
  int64_t id{-1};                         //!< Item ID
  float crossEncoderScore{0.0f};          //!< Cross-encoder relevance score (0-1)
  float originalScore{0.0f};              //!< Original ranking score
  float finalScore{0.0f};                 //!< Combined final score
};

/*!
 * @brief Cross-encoder re-ranking engine using ONNX Runtime
 *
 * The cross-encoder provides more accurate relevance scoring by jointly
 * encoding query-passage pairs. Unlike bi-encoders which encode query and
 * passage separately, cross-encoders can capture fine-grained interactions
 * between query and passage tokens.
 *
 * Key characteristics:
 * - Higher accuracy: 2-5% improvement over bi-encoder alone
 * - Slower inference: Only applied to top-N candidates (typically 20-50)
 * - Memory efficient: Lazy loading and automatic unloading on idle
 * - Graceful degradation: Falls back to original scores if model unavailable
 *
 * Architecture:
 * - Model: ms-marco-MiniLM-L-6-v2 (or similar cross-encoder)
 * - Input: Concatenated [CLS] query [SEP] passage [SEP] tokens
 * - Output: Single relevance score (0-1 range after normalization)
 * - Inference: ONNX Runtime with CPU/GPU acceleration
 *
 * Usage example:
 * @code
 * CrossEncoderConfig config;
 * config.enabled = true;
 * config.topN = 20;
 * config.modelPath = "special://home/semantic/models/cross-encoder.onnx";
 *
 * CCrossEncoder encoder(config);
 * if (encoder.Initialize())
 * {
 *   std::vector<QueryPassagePair> pairs;
 *   // ... populate pairs ...
 *   auto results = encoder.ReRank(pairs);
 *   // Use results for final ranking
 * }
 * @endcode
 *
 * Thread-safety: Not thread-safe. Create separate instances per thread.
 */
class CCrossEncoder
{
public:
  /*!
   * @brief Constructor with default configuration
   */
  CCrossEncoder();

  /*!
   * @brief Constructor with custom configuration
   * @param config Cross-encoder configuration
   */
  explicit CCrossEncoder(const CrossEncoderConfig& config);

  /*!
   * @brief Destructor
   */
  ~CCrossEncoder();

  // Prevent copying (use move semantics if needed)
  CCrossEncoder(const CCrossEncoder&) = delete;
  CCrossEncoder& operator=(const CCrossEncoder&) = delete;
  CCrossEncoder(CCrossEncoder&&) = default;
  CCrossEncoder& operator=(CCrossEncoder&&) = default;

  /*!
   * @brief Initialize the cross-encoder engine
   *
   * Loads tokenizer and optionally the model (based on lazyLoad setting).
   * Must be called before any re-ranking operations.
   *
   * @return true if initialization succeeded, false otherwise
   */
  bool Initialize();

  /*!
   * @brief Initialize with custom paths
   *
   * @param modelPath Path to ONNX model file
   * @param vocabPath Path to tokenizer vocabulary file
   * @return true if initialization succeeded, false otherwise
   */
  bool Initialize(const std::string& modelPath, const std::string& vocabPath);

  /*!
   * @brief Check if the engine is properly initialized
   * @return true if ready to perform re-ranking
   */
  bool IsInitialized() const;

  /*!
   * @brief Check if the model is currently loaded in memory
   * @return true if model is loaded, false otherwise
   */
  bool IsModelLoaded() const;

  /*!
   * @brief Manually load the model (if lazy loading is enabled)
   * @return true if load succeeded, false otherwise
   */
  bool LoadModel();

  /*!
   * @brief Manually unload the model to free memory
   */
  void UnloadModel();

  /*!
   * @brief Update configuration
   * @param config New configuration
   */
  void SetConfig(const CrossEncoderConfig& config);

  /*!
   * @brief Get current configuration
   * @return Current configuration
   */
  const CrossEncoderConfig& GetConfig() const;

  /*!
   * @brief Score a single query-passage pair
   *
   * @param query Query text
   * @param passage Passage/document text
   * @return Relevance score in range [0, 1] where 1 = highly relevant
   * @throws std::runtime_error if engine is not initialized
   */
  float Score(const std::string& query, const std::string& passage);

  /*!
   * @brief Score multiple query-passage pairs in a batch
   *
   * Batch processing is more efficient than individual calls.
   * Results are returned in the same order as input pairs.
   *
   * @param pairs Vector of query-passage pairs to score
   * @return Vector of relevance scores [0, 1]
   * @throws std::runtime_error if engine is not initialized
   */
  std::vector<float> ScoreBatch(const std::vector<std::pair<std::string, std::string>>& pairs);

  /*!
   * @brief Re-rank results using cross-encoder
   *
   * Takes top-N results from initial ranking and re-scores them using
   * the cross-encoder model. Combines original and cross-encoder scores
   * based on scoreWeight configuration.
   *
   * @param pairs Query-passage pairs with original scores
   * @return Re-ranked results sorted by final score (descending)
   *
   * Formula: finalScore = (1 - scoreWeight) * originalScore + scoreWeight * crossEncoderScore
   * If scoreWeight = 1.0, only cross-encoder score is used
   * If scoreWeight = 0.5, both scores are weighted equally
   */
  std::vector<CrossEncoderResult> ReRank(const std::vector<QueryPassagePair>& pairs);

  /*!
   * @brief Re-rank with custom score blending
   *
   * @param pairs Query-passage pairs with original scores
   * @param scoreWeight Custom weight for cross-encoder score (0-1)
   * @return Re-ranked results sorted by final score (descending)
   */
  std::vector<CrossEncoderResult> ReRankWithWeight(const std::vector<QueryPassagePair>& pairs,
                                                    float scoreWeight);

  /*!
   * @brief Get statistics about cross-encoder usage
   */
  struct Stats
  {
    int64_t totalInferences{0};           //!< Total number of inferences performed
    int64_t totalPairs{0};                //!< Total pairs scored
    double avgInferenceTimeMs{0.0};       //!< Average inference time in milliseconds
    double avgScoreImpact{0.0};           //!< Average score change from re-ranking
    int64_t modelLoadCount{0};            //!< Number of times model was loaded
  };

  /*!
   * @brief Get usage statistics
   * @return Current statistics
   */
  Stats GetStats() const;

  /*!
   * @brief Reset statistics counters
   */
  void ResetStats();

private:
  /*!
   * @brief Normalize cross-encoder logits to [0, 1] probability range
   *
   * Applies sigmoid function to convert raw logits to probabilities.
   *
   * @param logit Raw output from cross-encoder model
   * @return Normalized score in [0, 1] range
   */
  static float NormalizeScore(float logit);

  /*!
   * @brief Validate and clamp configuration values
   */
  void ValidateConfig();

  /*!
   * @brief Private implementation (PIMPL pattern)
   *
   * Hides ONNX Runtime implementation details from the header.
   */
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace SEMANTIC
} // namespace KODI
