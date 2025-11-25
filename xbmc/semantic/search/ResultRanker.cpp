/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ResultRanker.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

using namespace KODI::SEMANTIC;

CResultRanker::CResultRanker() = default;

CResultRanker::CResultRanker(const RankingConfig& config) : m_config(config)
{
}

void CResultRanker::SetConfig(const RankingConfig& config)
{
  m_config = config;
}

std::vector<RankedItem> CResultRanker::Combine(
    const std::vector<std::pair<int64_t, float>>& list1,
    const std::vector<std::pair<int64_t, float>>& list2)
{
  switch (m_config.algorithm)
  {
    case RankingAlgorithm::RRF:
      return CombineRRF(list1, list2);
    case RankingAlgorithm::Linear:
      return CombineLinear(list1, list2);
    case RankingAlgorithm::Borda:
      return CombineBorda(list1, list2);
    case RankingAlgorithm::CombMNZ:
      return CombineCombMNZ(list1, list2);
    default:
      return CombineRRF(list1, list2); // Fallback to RRF
  }
}

std::vector<RankedItem> CResultRanker::CombineMultiple(
    const std::vector<std::vector<std::pair<int64_t, float>>>& lists,
    const std::vector<float>& weights)
{
  if (lists.empty())
    return {};

  if (lists.size() != weights.size())
    throw std::invalid_argument("Number of lists must match number of weights");

  // For two lists, use standard Combine with weight override
  if (lists.size() == 2)
  {
    RankingConfig tempConfig = m_config;
    tempConfig.weight1 = weights[0];
    tempConfig.weight2 = weights[1];
    CResultRanker tempRanker(tempConfig);
    return tempRanker.Combine(lists[0], lists[1]);
  }

  // For multiple lists, implement multi-way fusion
  std::unordered_map<int64_t, RankedItem> items;

  if (m_config.algorithm == RankingAlgorithm::RRF)
  {
    // Multi-way RRF
    for (size_t listIdx = 0; listIdx < lists.size(); ++listIdx)
    {
      const auto& list = lists[listIdx];
      float weight = weights[listIdx];

      for (size_t rank = 0; rank < list.size(); ++rank)
      {
        int64_t id = list[rank].first;
        float score = list[rank].second;
        float rrfScore = weight / (m_config.rrfK + rank + 1);

        RankedItem& item = items[id];
        item.id = id;
        item.combinedScore += rrfScore;

        // Store first two scores/ranks for compatibility
        if (listIdx == 0)
        {
          item.score1 = score;
          item.rank1 = static_cast<int>(rank);
        }
        else if (listIdx == 1)
        {
          item.score2 = score;
          item.rank2 = static_cast<int>(rank);
        }
      }
    }
  }
  else if (m_config.algorithm == RankingAlgorithm::Linear)
  {
    // Multi-way linear combination with normalization
    std::vector<std::vector<std::pair<int64_t, float>>> normalizedLists;
    normalizedLists.reserve(lists.size());

    for (const auto& list : lists)
      normalizedLists.push_back(NormalizeScores(list));

    for (size_t listIdx = 0; listIdx < normalizedLists.size(); ++listIdx)
    {
      const auto& list = normalizedLists[listIdx];
      float weight = weights[listIdx];

      for (size_t rank = 0; rank < list.size(); ++rank)
      {
        int64_t id = list[rank].first;
        float score = list[rank].second;

        RankedItem& item = items[id];
        item.id = id;
        item.combinedScore += weight * score;

        if (listIdx == 0)
        {
          item.score1 = score;
          item.rank1 = static_cast<int>(rank);
        }
        else if (listIdx == 1)
        {
          item.score2 = score;
          item.rank2 = static_cast<int>(rank);
        }
      }
    }
  }
  else
  {
    throw std::invalid_argument("CombineMultiple only supports RRF and Linear algorithms");
  }

  // Convert to vector and sort
  std::vector<RankedItem> result;
  result.reserve(items.size());
  for (auto& [id, item] : items)
    result.push_back(std::move(item));

  std::sort(result.begin(), result.end(),
            [](const RankedItem& a, const RankedItem& b) {
              return a.combinedScore > b.combinedScore;
            });

  // Apply topK limit
  if (m_config.topK > 0 && result.size() > static_cast<size_t>(m_config.topK))
    result.resize(m_config.topK);

  return result;
}

std::vector<std::pair<int64_t, float>> CResultRanker::NormalizeScores(
    const std::vector<std::pair<int64_t, float>>& scores)
{
  if (scores.empty())
    return {};

  // Find min and max scores
  float minScore = std::numeric_limits<float>::max();
  float maxScore = std::numeric_limits<float>::lowest();

  for (const auto& [id, score] : scores)
  {
    minScore = std::min(minScore, score);
    maxScore = std::max(maxScore, score);
  }

  // Calculate range (avoid division by zero)
  float range = maxScore - minScore;
  if (range < 1e-6f)
    range = 1.0f; // All scores are the same, normalize to 1.0

  // Normalize to [0, 1] range
  std::vector<std::pair<int64_t, float>> normalized;
  normalized.reserve(scores.size());

  for (const auto& [id, score] : scores)
  {
    float normScore = (score - minScore) / range;
    normalized.emplace_back(id, normScore);
  }

  return normalized;
}

std::vector<RankedItem> CResultRanker::CombineRRF(
    const std::vector<std::pair<int64_t, float>>& list1,
    const std::vector<std::pair<int64_t, float>>& list2)
{
  // Reciprocal Rank Fusion (RRF) algorithm
  // Formula: score = sum(weight / (k + rank))
  // Where k is a constant (typically 60) and rank is 0-indexed position
  //
  // RRF is robust to different score scales and distributions.
  // It focuses on rank rather than raw scores, making it ideal for
  // combining heterogeneous sources (e.g., semantic + keyword search).

  std::unordered_map<int64_t, RankedItem> items;

  // Process first list
  for (size_t i = 0; i < list1.size(); ++i)
  {
    int64_t id = list1[i].first;
    float rrfScore = m_config.weight1 / (m_config.rrfK + i + 1);

    RankedItem& item = items[id];
    item.id = id;
    item.score1 = list1[i].second;
    item.rank1 = static_cast<int>(i);
    item.combinedScore += rrfScore;
  }

  // Process second list
  for (size_t i = 0; i < list2.size(); ++i)
  {
    int64_t id = list2[i].first;
    float rrfScore = m_config.weight2 / (m_config.rrfK + i + 1);

    RankedItem& item = items[id];
    item.id = id;
    item.score2 = list2[i].second;
    item.rank2 = static_cast<int>(i);
    item.combinedScore += rrfScore;
  }

  // Convert to vector and sort by combined score (descending)
  std::vector<RankedItem> result;
  result.reserve(items.size());
  for (auto& [id, item] : items)
    result.push_back(std::move(item));

  std::sort(result.begin(), result.end(),
            [](const RankedItem& a, const RankedItem& b) {
              return a.combinedScore > b.combinedScore;
            });

  // Apply topK limit if specified
  if (m_config.topK > 0 && result.size() > static_cast<size_t>(m_config.topK))
    result.resize(m_config.topK);

  return result;
}

std::vector<RankedItem> CResultRanker::CombineLinear(
    const std::vector<std::pair<int64_t, float>>& list1,
    const std::vector<std::pair<int64_t, float>>& list2)
{
  // Linear weighted combination of normalized scores
  // Formula: score = weight1 * norm_score1 + weight2 * norm_score2
  //
  // Requires score normalization to ensure fair combination.
  // Best when scores from both sources are meaningful and comparable.

  // Normalize scores to [0, 1] range first
  auto norm1 = NormalizeScores(list1);
  auto norm2 = NormalizeScores(list2);

  std::unordered_map<int64_t, RankedItem> items;

  // Process first normalized list
  for (size_t i = 0; i < norm1.size(); ++i)
  {
    int64_t id = norm1[i].first;
    float normScore = norm1[i].second;

    RankedItem& item = items[id];
    item.id = id;
    item.score1 = normScore;
    item.rank1 = static_cast<int>(i);
    item.combinedScore += m_config.weight1 * normScore;
  }

  // Process second normalized list
  for (size_t i = 0; i < norm2.size(); ++i)
  {
    int64_t id = norm2[i].first;
    float normScore = norm2[i].second;

    RankedItem& item = items[id];
    item.id = id;
    item.score2 = normScore;
    item.rank2 = static_cast<int>(i);
    item.combinedScore += m_config.weight2 * normScore;
  }

  // Convert to vector and sort
  std::vector<RankedItem> result;
  result.reserve(items.size());
  for (auto& [id, item] : items)
    result.push_back(std::move(item));

  std::sort(result.begin(), result.end(),
            [](const RankedItem& a, const RankedItem& b) {
              return a.combinedScore > b.combinedScore;
            });

  // Apply topK limit
  if (m_config.topK > 0 && result.size() > static_cast<size_t>(m_config.topK))
    result.resize(m_config.topK);

  return result;
}

std::vector<RankedItem> CResultRanker::CombineBorda(
    const std::vector<std::pair<int64_t, float>>& list1,
    const std::vector<std::pair<int64_t, float>>& list2)
{
  // Borda count voting method
  // Each item gets points based on its position: (list_size - rank)
  // Items appearing in multiple lists accumulate points
  //
  // Simple rank-based method, democratic voting approach.
  // Good when all rankings are equally trustworthy.

  std::unordered_map<int64_t, RankedItem> items;

  size_t size1 = list1.size();
  size_t size2 = list2.size();

  // Process first list - assign Borda points
  for (size_t i = 0; i < list1.size(); ++i)
  {
    int64_t id = list1[i].first;
    float bordaPoints = m_config.weight1 * (size1 - i);

    RankedItem& item = items[id];
    item.id = id;
    item.score1 = list1[i].second;
    item.rank1 = static_cast<int>(i);
    item.combinedScore += bordaPoints;
  }

  // Process second list - assign Borda points
  for (size_t i = 0; i < list2.size(); ++i)
  {
    int64_t id = list2[i].first;
    float bordaPoints = m_config.weight2 * (size2 - i);

    RankedItem& item = items[id];
    item.id = id;
    item.score2 = list2[i].second;
    item.rank2 = static_cast<int>(i);
    item.combinedScore += bordaPoints;
  }

  // Convert to vector and sort
  std::vector<RankedItem> result;
  result.reserve(items.size());
  for (auto& [id, item] : items)
    result.push_back(std::move(item));

  std::sort(result.begin(), result.end(),
            [](const RankedItem& a, const RankedItem& b) {
              return a.combinedScore > b.combinedScore;
            });

  // Apply topK limit
  if (m_config.topK > 0 && result.size() > static_cast<size_t>(m_config.topK))
    result.resize(m_config.topK);

  return result;
}

std::vector<RankedItem> CResultRanker::CombineCombMNZ(
    const std::vector<std::pair<int64_t, float>>& list1,
    const std::vector<std::pair<int64_t, float>>& list2)
{
  // CombMNZ (Combination via Multiple Non-Zero)
  // Formula: score = (sum of normalized scores) * (count of non-zero scores)
  //
  // Favors items that appear in multiple sources while still considering
  // their individual scores. Items in both lists get a multiplier boost.

  // Normalize scores first
  auto norm1 = NormalizeScores(list1);
  auto norm2 = NormalizeScores(list2);

  std::unordered_map<int64_t, RankedItem> items;

  // Collect scores from first list
  for (size_t i = 0; i < norm1.size(); ++i)
  {
    int64_t id = norm1[i].first;
    float score = norm1[i].second;

    RankedItem& item = items[id];
    item.id = id;
    item.score1 = score;
    item.rank1 = static_cast<int>(i);
  }

  // Collect scores from second list
  for (size_t i = 0; i < norm2.size(); ++i)
  {
    int64_t id = norm2[i].first;
    float score = norm2[i].second;

    RankedItem& item = items[id];
    item.id = id;
    item.score2 = score;
    item.rank2 = static_cast<int>(i);
  }

  // Calculate CombMNZ score for each item
  for (auto& [id, item] : items)
  {
    float sum = item.score1 + item.score2;
    int nonZero = (item.score1 > 0 ? 1 : 0) + (item.score2 > 0 ? 1 : 0);
    item.combinedScore = sum * nonZero;
  }

  // Convert to vector and sort
  std::vector<RankedItem> result;
  result.reserve(items.size());
  for (auto& [id, item] : items)
    result.push_back(std::move(item));

  std::sort(result.begin(), result.end(),
            [](const RankedItem& a, const RankedItem& b) {
              return a.combinedScore > b.combinedScore;
            });

  // Apply topK limit
  if (m_config.topK > 0 && result.size() > static_cast<size_t>(m_config.topK))
    result.resize(m_config.topK);

  return result;
}
