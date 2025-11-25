/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

// Forward declarations
class CSearchHistory;
class CSemanticDatabase;

/*!
 * @brief Type of suggestion
 */
enum class SuggestionType
{
  HISTORY,   //!< From search history
  CATEGORY,  //!< Category-based suggestion
  FUZZY      //!< Fuzzy match suggestion
};

/*!
 * @brief Search suggestion with metadata
 */
struct SearchSuggestion
{
  std::string text;              //!< Suggestion text
  SuggestionType type;           //!< Type of suggestion
  float score{0.0f};             //!< Relevance score (higher = more relevant)
  int frequency{0};              //!< How many times this has been searched
  int64_t lastUsed{0};           //!< Last usage timestamp
  std::string category;          //!< Category hint (e.g., "Movies", "TV Shows")

  /*!
   * @brief Get display label for the suggestion
   * @return Formatted label with category if available
   */
  std::string GetDisplayLabel() const
  {
    if (!category.empty())
      return text + " [" + category + "]";
    return text;
  }
};

/*!
 * @brief Generates autocomplete suggestions for semantic search
 *
 * Features:
 * - Prefix matching on search history
 * - Fuzzy matching for typos (Levenshtein distance)
 * - Frequency-weighted ranking
 * - Category suggestions based on indexed content
 * - Configurable maximum suggestions
 *
 * Suggestion ranking considers:
 * - Exact prefix match (highest priority)
 * - Search frequency
 * - Recency (recent searches ranked higher)
 * - Fuzzy match distance
 *
 * Example usage:
 * \code
 * CSearchSuggestions suggestions;
 * if (suggestions.Initialize(database, history))
 * {
 *   // Get suggestions for partial query
 *   auto results = suggestions.GetSuggestions("act", 10);
 *   for (const auto& suggestion : results)
 *   {
 *     std::cout << suggestion.GetDisplayLabel() << std::endl;
 *   }
 * }
 * \endcode
 */
class CSearchSuggestions
{
public:
  CSearchSuggestions();
  ~CSearchSuggestions();

  /*!
   * @brief Initialize the suggestions engine
   * @param database Semantic database instance
   * @param history Search history instance
   * @return true if initialization succeeded
   */
  bool Initialize(CSemanticDatabase* database, CSearchHistory* history);

  /*!
   * @brief Get autocomplete suggestions for a partial query
   * @param partialQuery Partial search query
   * @param maxSuggestions Maximum number of suggestions (default 10)
   * @return Vector of suggestions sorted by relevance
   */
  std::vector<SearchSuggestion> GetSuggestions(const std::string& partialQuery,
                                                int maxSuggestions = 10);

  /*!
   * @brief Get category-based suggestions
   * @param maxSuggestions Maximum number of suggestions
   * @return Vector of category suggestions
   */
  std::vector<SearchSuggestion> GetCategorySuggestions(int maxSuggestions = 5);

  /*!
   * @brief Set fuzzy matching threshold (0.0 - 1.0)
   * @param threshold Similarity threshold (higher = stricter matching)
   */
  void SetFuzzyThreshold(float threshold);

  /*!
   * @brief Enable or disable fuzzy matching
   * @param enabled true to enable fuzzy matching
   */
  void SetFuzzyMatchingEnabled(bool enabled);

  /*!
   * @brief Set recency bias factor
   * @param bias Bias factor (0.0 = no bias, 1.0 = strong recency bias)
   */
  void SetRecencyBias(float bias);

private:
  /*!
   * @brief Get suggestions from search history
   * @param partialQuery Partial search query
   * @param maxSuggestions Maximum suggestions
   * @return Vector of history-based suggestions
   */
  std::vector<SearchSuggestion> GetHistorySuggestions(const std::string& partialQuery,
                                                       int maxSuggestions);

  /*!
   * @brief Get fuzzy match suggestions
   * @param partialQuery Partial search query
   * @param maxSuggestions Maximum suggestions
   * @return Vector of fuzzy match suggestions
   */
  std::vector<SearchSuggestion> GetFuzzySuggestions(const std::string& partialQuery,
                                                     int maxSuggestions);

  /*!
   * @brief Calculate Levenshtein distance between two strings
   * @param s1 First string
   * @param s2 Second string
   * @return Edit distance
   */
  int LevenshteinDistance(const std::string& s1, const std::string& s2) const;

  /*!
   * @brief Calculate normalized similarity score (0.0 - 1.0)
   * @param s1 First string
   * @param s2 Second string
   * @return Similarity score (1.0 = identical, 0.0 = completely different)
   */
  float CalculateSimilarity(const std::string& s1, const std::string& s2) const;

  /*!
   * @brief Calculate recency score for a timestamp
   * @param timestamp Unix timestamp
   * @return Recency score (0.0 - 1.0)
   */
  float CalculateRecencyScore(int64_t timestamp) const;

  /*!
   * @brief Merge and rank suggestions from multiple sources
   * @param suggestions Vector of suggestion vectors to merge
   * @param maxResults Maximum number of results
   * @return Merged and ranked suggestions
   */
  std::vector<SearchSuggestion> MergeAndRankSuggestions(
      const std::vector<std::vector<SearchSuggestion>>& suggestions,
      int maxResults);

  CSemanticDatabase* m_database{nullptr};
  CSearchHistory* m_history{nullptr};
  mutable CCriticalSection m_criticalSection;

  // Configuration
  bool m_fuzzyMatchingEnabled{true};
  float m_fuzzyThreshold{0.7f};    // Minimum similarity for fuzzy matches
  float m_recencyBias{0.3f};       // Weight for recency in ranking
};

} // namespace SEMANTIC
} // namespace KODI
