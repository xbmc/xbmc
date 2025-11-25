/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SearchSuggestions.h"

#include "SearchHistory.h"
#include "semantic/SemanticDatabase.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <set>

using namespace KODI::SEMANTIC;

CSearchSuggestions::CSearchSuggestions() = default;

CSearchSuggestions::~CSearchSuggestions() = default;

bool CSearchSuggestions::Initialize(CSemanticDatabase* database, CSearchHistory* history)
{
  if (!database || !history)
  {
    CLog::Log(LOGERROR, "CSearchSuggestions: Invalid parameters");
    return false;
  }

  m_database = database;
  m_history = history;

  CLog::Log(LOGINFO, "CSearchSuggestions: Successfully initialized");
  return true;
}

std::vector<SearchSuggestion> CSearchSuggestions::GetSuggestions(const std::string& partialQuery,
                                                                  int maxSuggestions)
{
  if (partialQuery.empty() || maxSuggestions <= 0)
    return {};

  CSingleLock lock(m_criticalSection);

  // Collect suggestions from multiple sources
  std::vector<std::vector<SearchSuggestion>> allSuggestions;

  // 1. Get history-based suggestions (highest priority)
  auto historySuggestions = GetHistorySuggestions(partialQuery, maxSuggestions);
  if (!historySuggestions.empty())
  {
    allSuggestions.push_back(historySuggestions);
  }

  // 2. Get fuzzy match suggestions if enabled
  if (m_fuzzyMatchingEnabled)
  {
    auto fuzzySuggestions = GetFuzzySuggestions(partialQuery, maxSuggestions);
    if (!fuzzySuggestions.empty())
    {
      allSuggestions.push_back(fuzzySuggestions);
    }
  }

  // 3. Get category suggestions if query is very short
  if (partialQuery.length() <= 3)
  {
    auto categorySuggestions = GetCategorySuggestions(5);
    if (!categorySuggestions.empty())
    {
      allSuggestions.push_back(categorySuggestions);
    }
  }

  // Merge and rank all suggestions
  return MergeAndRankSuggestions(allSuggestions, maxSuggestions);
}

std::vector<SearchSuggestion> CSearchSuggestions::GetHistorySuggestions(
    const std::string& partialQuery,
    int maxSuggestions)
{
  std::vector<SearchSuggestion> suggestions;

  if (!m_history)
    return suggestions;

  // Get prefix matches from history
  auto historyEntries = m_history->GetSearchesByPrefix(partialQuery, maxSuggestions * 2);

  // Convert to suggestions and calculate scores
  int64_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  for (const auto& entry : historyEntries)
  {
    SearchSuggestion suggestion;
    suggestion.text = entry.queryText;
    suggestion.type = SuggestionType::HISTORY;
    suggestion.lastUsed = entry.timestamp;

    // Get frequency
    suggestion.frequency = m_history->GetSearchFrequency(entry.queryText);

    // Calculate score based on:
    // - Exact prefix match bonus
    // - Frequency (normalized to 0-1)
    // - Recency (using recency bias)
    float prefixBonus = StringUtils::StartsWith(entry.queryText, partialQuery) ? 1.0f : 0.5f;
    float frequencyScore = std::min(1.0f, suggestion.frequency / 10.0f); // Normalize
    float recencyScore = CalculateRecencyScore(entry.timestamp);

    suggestion.score = (prefixBonus * 0.4f) + (frequencyScore * 0.3f) +
                       (recencyScore * m_recencyBias);

    suggestions.push_back(suggestion);
  }

  // Sort by score (descending)
  std::sort(suggestions.begin(), suggestions.end(),
            [](const SearchSuggestion& a, const SearchSuggestion& b) {
              return a.score > b.score;
            });

  // Limit results
  if (suggestions.size() > static_cast<size_t>(maxSuggestions))
  {
    suggestions.resize(maxSuggestions);
  }

  CLog::Log(LOGDEBUG, "CSearchSuggestions: Found {} history suggestions for '{}'",
            suggestions.size(), partialQuery);

  return suggestions;
}

std::vector<SearchSuggestion> CSearchSuggestions::GetFuzzySuggestions(
    const std::string& partialQuery,
    int maxSuggestions)
{
  std::vector<SearchSuggestion> suggestions;

  if (!m_history || partialQuery.length() < 3)
    return suggestions;

  // Get all recent searches to check for fuzzy matches
  auto allHistory = m_history->GetRecentSearches(100);

  for (const auto& entry : allHistory)
  {
    // Skip exact prefix matches (already handled by history suggestions)
    if (StringUtils::StartsWith(entry.queryText, partialQuery))
      continue;

    // Calculate similarity
    float similarity = CalculateSimilarity(partialQuery, entry.queryText);

    // Only include if similarity is above threshold
    if (similarity >= m_fuzzyThreshold)
    {
      SearchSuggestion suggestion;
      suggestion.text = entry.queryText;
      suggestion.type = SuggestionType::FUZZY;
      suggestion.lastUsed = entry.timestamp;
      suggestion.frequency = m_history->GetSearchFrequency(entry.queryText);

      // Score based on similarity and frequency
      float frequencyScore = std::min(1.0f, suggestion.frequency / 10.0f);
      float recencyScore = CalculateRecencyScore(entry.timestamp);

      suggestion.score = (similarity * 0.5f) + (frequencyScore * 0.3f) +
                         (recencyScore * m_recencyBias);

      suggestions.push_back(suggestion);
    }
  }

  // Sort by score (descending)
  std::sort(suggestions.begin(), suggestions.end(),
            [](const SearchSuggestion& a, const SearchSuggestion& b) {
              return a.score > b.score;
            });

  // Limit results
  if (suggestions.size() > static_cast<size_t>(maxSuggestions))
  {
    suggestions.resize(maxSuggestions);
  }

  CLog::Log(LOGDEBUG, "CSearchSuggestions: Found {} fuzzy suggestions for '{}'",
            suggestions.size(), partialQuery);

  return suggestions;
}

std::vector<SearchSuggestion> CSearchSuggestions::GetCategorySuggestions(int maxSuggestions)
{
  std::vector<SearchSuggestion> suggestions;

  if (!m_database)
    return suggestions;

  try
  {
    // Get popular media types and genres from indexed content
    std::string sql =
        "SELECT media_type, COUNT(*) as count "
        "FROM semantic_chunks "
        "GROUP BY media_type "
        "ORDER BY count DESC "
        "LIMIT " +
        std::to_string(maxSuggestions);

    auto results = m_database->Query(sql);

    if (!results)
      return suggestions;

    while (!results->eof())
    {
      std::string mediaType = results->fv("media_type").get_asString();
      int count = results->fv("count").get_asInt();

      SearchSuggestion suggestion;
      suggestion.type = SuggestionType::CATEGORY;
      suggestion.score = std::min(1.0f, count / 1000.0f); // Normalize

      // Format media type as category suggestion
      if (mediaType == "movie")
      {
        suggestion.text = "movies";
        suggestion.category = "Media Type";
      }
      else if (mediaType == "episode")
      {
        suggestion.text = "tv shows";
        suggestion.category = "Media Type";
      }
      else if (mediaType == "musicvideo")
      {
        suggestion.text = "music videos";
        suggestion.category = "Media Type";
      }

      if (!suggestion.text.empty())
      {
        suggestions.push_back(suggestion);
      }

      results->next();
    }

    CLog::Log(LOGDEBUG, "CSearchSuggestions: Generated {} category suggestions",
              suggestions.size());
  }
  catch (const std::exception& ex)
  {
    CLog::Log(LOGERROR, "CSearchSuggestions: Failed to get category suggestions: {}", ex.what());
  }

  return suggestions;
}

int CSearchSuggestions::LevenshteinDistance(const std::string& s1, const std::string& s2) const
{
  const size_t len1 = s1.size();
  const size_t len2 = s2.size();

  std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

  d[0][0] = 0;
  for (size_t i = 1; i <= len1; ++i)
    d[i][0] = i;
  for (size_t i = 1; i <= len2; ++i)
    d[0][i] = i;

  for (size_t i = 1; i <= len1; ++i)
  {
    for (size_t j = 1; j <= len2; ++j)
    {
      int cost = (StringUtils::ToLower(s1[i - 1]) == StringUtils::ToLower(s2[j - 1])) ? 0 : 1;

      d[i][j] = std::min({d[i - 1][j] + 1,      // deletion
                          d[i][j - 1] + 1,      // insertion
                          d[i - 1][j - 1] + cost}); // substitution
    }
  }

  return d[len1][len2];
}

float CSearchSuggestions::CalculateSimilarity(const std::string& s1, const std::string& s2) const
{
  if (s1.empty() || s2.empty())
    return 0.0f;

  // Convert to lowercase for comparison
  std::string lower1 = StringUtils::ToLower(s1);
  std::string lower2 = StringUtils::ToLower(s2);

  // Calculate Levenshtein distance
  int distance = LevenshteinDistance(lower1, lower2);

  // Normalize to 0-1 range (1 = identical, 0 = completely different)
  int maxLen = std::max(lower1.length(), lower2.length());
  if (maxLen == 0)
    return 1.0f;

  return 1.0f - (static_cast<float>(distance) / static_cast<float>(maxLen));
}

float CSearchSuggestions::CalculateRecencyScore(int64_t timestamp) const
{
  int64_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  int64_t age = currentTime - timestamp;

  // Age in days
  float ageDays = age / 86400.0f;

  // Exponential decay: score = e^(-age/30)
  // After 30 days, score is ~0.37
  // After 90 days, score is ~0.05
  return std::exp(-ageDays / 30.0f);
}

std::vector<SearchSuggestion> CSearchSuggestions::MergeAndRankSuggestions(
    const std::vector<std::vector<SearchSuggestion>>& suggestions,
    int maxResults)
{
  // Use a map to deduplicate by text (keep highest score)
  std::map<std::string, SearchSuggestion> uniqueSuggestions;

  for (const auto& suggestionList : suggestions)
  {
    for (const auto& suggestion : suggestionList)
    {
      auto it = uniqueSuggestions.find(suggestion.text);
      if (it == uniqueSuggestions.end())
      {
        uniqueSuggestions[suggestion.text] = suggestion;
      }
      else
      {
        // Keep the one with higher score
        if (suggestion.score > it->second.score)
        {
          it->second = suggestion;
        }
      }
    }
  }

  // Convert map to vector
  std::vector<SearchSuggestion> merged;
  merged.reserve(uniqueSuggestions.size());
  for (const auto& pair : uniqueSuggestions)
  {
    merged.push_back(pair.second);
  }

  // Sort by score (descending)
  std::sort(merged.begin(), merged.end(),
            [](const SearchSuggestion& a, const SearchSuggestion& b) {
              return a.score > b.score;
            });

  // Limit results
  if (merged.size() > static_cast<size_t>(maxResults))
  {
    merged.resize(maxResults);
  }

  return merged;
}

void CSearchSuggestions::SetFuzzyThreshold(float threshold)
{
  CSingleLock lock(m_criticalSection);
  m_fuzzyThreshold = std::max(0.0f, std::min(1.0f, threshold));
  CLog::Log(LOGDEBUG, "CSearchSuggestions: Fuzzy threshold set to {}", m_fuzzyThreshold);
}

void CSearchSuggestions::SetFuzzyMatchingEnabled(bool enabled)
{
  CSingleLock lock(m_criticalSection);
  m_fuzzyMatchingEnabled = enabled;
  CLog::Log(LOGDEBUG, "CSearchSuggestions: Fuzzy matching {}", enabled ? "enabled" : "disabled");
}

void CSearchSuggestions::SetRecencyBias(float bias)
{
  CSingleLock lock(m_criticalSection);
  m_recencyBias = std::max(0.0f, std::min(1.0f, bias));
  CLog::Log(LOGDEBUG, "CSearchSuggestions: Recency bias set to {}", m_recencyBias);
}
