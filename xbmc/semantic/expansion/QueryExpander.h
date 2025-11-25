/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "semantic/embedding/EmbeddingEngine.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

// Forward declarations
class CSynonymDatabase;
class CSemanticDatabase;

/*!
 * @brief Expansion strategy flags
 */
enum class ExpansionStrategy
{
  None = 0,
  Synonym = 1 << 0,         //!< WordNet-style synonym expansion
  Embedding = 1 << 1,       //!< Embedding-based semantic expansion
  Stemming = 1 << 2,        //!< Stemming/lemmatization
  SpellCorrection = 1 << 3, //!< Spell correction
  All = Synonym | Embedding | Stemming | SpellCorrection
};

// Enable bitwise operations
inline ExpansionStrategy operator|(ExpansionStrategy a, ExpansionStrategy b)
{
  return static_cast<ExpansionStrategy>(static_cast<int>(a) | static_cast<int>(b));
}

inline ExpansionStrategy operator&(ExpansionStrategy a, ExpansionStrategy b)
{
  return static_cast<ExpansionStrategy>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool HasFlag(ExpansionStrategy value, ExpansionStrategy flag)
{
  return (static_cast<int>(value) & static_cast<int>(flag)) != 0;
}

/*!
 * @brief Expanded query term with weight/boost
 */
struct ExpandedTerm
{
  std::string term;            //!< The expanded term
  float weight{1.0f};          //!< Term weight/boost (0-1)
  std::string source;          //!< Expansion source (original, synonym, embedding, etc.)
  bool isOriginal{false};      //!< True if this is an original query term
};

/*!
 * @brief Query expansion configuration
 */
struct ExpansionConfig
{
  ExpansionStrategy strategies{ExpansionStrategy::All}; //!< Enabled strategies
  int maxTermsPerWord{3};                               //!< Max expansion terms per word
  int maxTotalTerms{10};                                //!< Max total expansion terms
  float synonymWeight{0.7f};                            //!< Weight for synonym expansions (0-1)
  float embeddingWeight{0.6f};                          //!< Weight for embedding expansions (0-1)
  float originalWeight{1.0f};                           //!< Weight for original terms (0-1)
  float similarityThreshold{0.7f};                      //!< Min similarity for embedding expansions
  int embeddingTopK{5};                                 //!< Top-K neighbors for embedding expansion
  bool preserveOriginalOrder{true};                     //!< Keep original terms first
  bool deduplicateTerms{true};                          //!< Remove duplicate terms
};

/*!
 * @brief Result of query expansion
 */
struct ExpansionResult
{
  std::string originalQuery;                  //!< Original query string
  std::string expandedQuery;                  //!< Expanded query string (space-separated)
  std::vector<ExpandedTerm> terms;            //!< All expanded terms with weights
  std::unordered_map<std::string, std::vector<std::string>> expansionMap; //!< word -> expansions mapping
  int totalExpansions{0};                     //!< Total number of expansions added
  bool cached{false};                         //!< True if result came from cache
};

/*!
 * @brief Query expansion engine for improved search recall
 *
 * This class implements multiple query expansion strategies to improve search recall
 * by adding synonyms, semantically similar terms, and query reformulations.
 *
 * Expansion Strategies:
 * 1. **Synonym Expansion**: WordNet-style synonyms from database
 *    - "car" -> "automobile", "vehicle"
 *    - Weighted by synonym confidence
 *
 * 2. **Embedding-Based Expansion**: Semantically similar terms via embeddings
 *    - Uses word embeddings to find nearest neighbors
 *    - Filtered by similarity threshold
 *    - Top-K nearest terms
 *
 * 3. **Stemming/Lemmatization**: Morphological variations
 *    - "running" -> "run"
 *    - Uses Porter stemming algorithm
 *
 * 4. **Spell Correction**: Fix common typos (future)
 *    - Edit distance based correction
 *    - Dictionary-based validation
 *
 * Features:
 * - Multiple configurable expansion strategies
 * - Term weight/boost control
 * - Expansion result caching
 * - Deduplication of expanded terms
 * - Max term limits for performance
 * - Expansion effectiveness metrics
 *
 * Example usage:
 * \code
 * CQueryExpander expander;
 * expander.Initialize(database, embeddingEngine, synonymDb);
 *
 * ExpansionConfig config;
 * config.strategies = ExpansionStrategy::Synonym | ExpansionStrategy::Embedding;
 * config.maxTotalTerms = 10;
 *
 * auto result = expander.ExpandQuery("car chase scene", config);
 * // result.expandedQuery = "car automobile vehicle chase pursuit scene sequence"
 * \endcode
 */
class CQueryExpander
{
public:
  CQueryExpander();
  ~CQueryExpander();

  /*!
   * @brief Initialize the query expander
   * @param database Pointer to semantic database (must remain valid)
   * @param embeddingEngine Pointer to embedding engine (can be null if not using embeddings)
   * @param synonymDb Pointer to synonym database (can be null if not using synonyms)
   * @return true if initialization succeeded, false otherwise
   */
  bool Initialize(CSemanticDatabase* database,
                  CEmbeddingEngine* embeddingEngine = nullptr,
                  CSynonymDatabase* synonymDb = nullptr);

  /*!
   * @brief Check if the expander is initialized
   * @return true if initialized and ready
   */
  bool IsInitialized() const;

  /*!
   * @brief Expand a query with configured strategies
   * @param query The original query string
   * @param config Expansion configuration (uses default if not specified)
   * @return Expansion result with expanded terms and weights
   */
  ExpansionResult ExpandQuery(const std::string& query, const ExpansionConfig& config = {});

  /*!
   * @brief Expand a single word with all enabled strategies
   * @param word The word to expand
   * @param config Expansion configuration
   * @return Vector of expanded terms with weights
   */
  std::vector<ExpandedTerm> ExpandWord(const std::string& word, const ExpansionConfig& config = {});

  /*!
   * @brief Get the default expansion configuration
   * @return Default configuration with recommended settings
   */
  static ExpansionConfig GetDefaultConfig();

  /*!
   * @brief Enable or disable expansion result caching
   * @param enabled True to enable caching, false to disable
   */
  void SetCacheEnabled(bool enabled);

  /*!
   * @brief Clear the expansion cache
   */
  void ClearCache();

  /*!
   * @brief Get cache hit rate for monitoring
   * @return Cache hit rate (0.0 - 1.0)
   */
  float GetCacheHitRate() const;

  /*!
   * @brief Get expansion statistics
   * @param totalQueries Output: total queries expanded
   * @param cacheHits Output: number of cache hits
   * @param avgExpansions Output: average expansions per query
   */
  void GetStats(int& totalQueries, int& cacheHits, float& avgExpansions) const;

private:
  CSemanticDatabase* m_database{nullptr};
  CEmbeddingEngine* m_embeddingEngine{nullptr};
  CSynonymDatabase* m_synonymDb{nullptr};

  bool m_cacheEnabled{true};
  mutable std::unordered_map<std::string, ExpansionResult> m_cache;
  static constexpr size_t MAX_CACHE_SIZE = 100;

  // Statistics
  mutable int m_totalQueries{0};
  mutable int m_cacheHits{0};
  mutable int m_totalExpansions{0};

  // Word-level embedding caches
  struct WordEmbedding
  {
    std::string word;
    Embedding embedding;
  };
  mutable std::vector<WordEmbedding> m_wordEmbeddingCache;
  static constexpr size_t MAX_WORD_EMBEDDING_CACHE_SIZE = 1000;

  // Cache for word expansion results
  mutable std::unordered_map<std::string, std::vector<ExpandedTerm>> m_wordExpansionCache;
  static constexpr size_t MAX_WORD_EXPANSION_CACHE_SIZE = 500;

  // Expansion strategy implementations
  std::vector<ExpandedTerm> ExpandSynonym(const std::string& word, const ExpansionConfig& config);
  std::vector<ExpandedTerm> ExpandEmbedding(const std::string& word, const ExpansionConfig& config);
  std::vector<ExpandedTerm> ExpandStemming(const std::string& word, const ExpansionConfig& config);

  // Helper methods
  std::vector<std::string> TokenizeQuery(const std::string& query);
  std::string NormalizeWord(const std::string& word);
  std::string StemWord(const std::string& word);
  void DeduplicateTerms(std::vector<ExpandedTerm>& terms);
  void LimitTerms(std::vector<ExpandedTerm>& terms, int maxTerms);
  std::string BuildExpandedQuery(const std::vector<ExpandedTerm>& terms);

  // Cache helpers
  bool GetFromCache(const std::string& query, ExpansionResult& result) const;
  void AddToCache(const std::string& query, const ExpansionResult& result);

  // Word embedding cache helpers
  bool GetWordEmbedding(const std::string& word, Embedding& embedding) const;
  void CacheWordEmbedding(const std::string& word, const Embedding& embedding);
  bool GetCachedWordExpansion(const std::string& word, std::vector<ExpandedTerm>& expansions) const;
  void CacheWordExpansion(const std::string& word, const std::vector<ExpandedTerm>& expansions);
};

} // namespace SEMANTIC
} // namespace KODI
