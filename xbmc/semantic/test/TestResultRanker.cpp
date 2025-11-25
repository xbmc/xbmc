/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/search/ResultRanker.h"

#include <gtest/gtest.h>

class ResultRankerTest : public ::testing::Test
{
protected:
  KODI::SEMANTIC::CResultRanker m_ranker;
};

TEST_F(ResultRankerTest, RRFCombination)
{
  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.8}, {3, 0.7}};
  std::vector<std::pair<int64_t, float>> list2 = {{2, 0.95}, {3, 0.85}, {4, 0.75}};

  auto results = m_ranker.Combine(list1, list2);

  // Item 2 should rank highest (appears in both lists with good scores)
  ASSERT_FALSE(results.empty());
  EXPECT_EQ(results[0].id, 2);

  // Item 3 also appears in both lists, should rank second
  EXPECT_EQ(results[1].id, 3);
}

TEST_F(ResultRankerTest, LinearCombination)
{
  KODI::SEMANTIC::RankingConfig config;
  config.algorithm = KODI::SEMANTIC::RankingAlgorithm::Linear;
  config.weight1 = 0.3f;
  config.weight2 = 0.7f;

  m_ranker.SetConfig(config);

  std::vector<std::pair<int64_t, float>> list1 = {{1, 1.0}, {2, 0.5}};
  std::vector<std::pair<int64_t, float>> list2 = {{1, 0.5}, {2, 1.0}};

  auto results = m_ranker.Combine(list1, list2);

  // With weight2 > weight1, item 2 should rank higher
  ASSERT_GE(results.size(), 2);
  EXPECT_EQ(results[0].id, 2);
  EXPECT_EQ(results[1].id, 1);
}

TEST_F(ResultRankerTest, ScoreNormalization)
{
  std::vector<std::pair<int64_t, float>> scores = {{1, 10.0}, {2, 50.0}, {3, 100.0}};

  auto normalized = KODI::SEMANTIC::CResultRanker::NormalizeScores(scores);

  ASSERT_EQ(normalized.size(), 3);
  EXPECT_NEAR(normalized[0].second, 0.0f, 0.01f); // Min -> 0
  EXPECT_NEAR(normalized[2].second, 1.0f, 0.01f); // Max -> 1
  EXPECT_NEAR(normalized[1].second, 0.44f, 0.01f); // Middle value
}

TEST_F(ResultRankerTest, EmptyLists)
{
  std::vector<std::pair<int64_t, float>> empty;
  std::vector<std::pair<int64_t, float>> list = {{1, 0.9}};

  auto results = m_ranker.Combine(empty, list);

  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].id, 1);
}

TEST_F(ResultRankerTest, BothListsEmpty)
{
  std::vector<std::pair<int64_t, float>> empty1;
  std::vector<std::pair<int64_t, float>> empty2;

  auto results = m_ranker.Combine(empty1, empty2);

  EXPECT_TRUE(results.empty());
}

TEST_F(ResultRankerTest, DisjointLists)
{
  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.8}};
  std::vector<std::pair<int64_t, float>> list2 = {{3, 0.95}, {4, 0.85}};

  auto results = m_ranker.Combine(list1, list2);

  // All 4 items should be in results
  EXPECT_EQ(results.size(), 4);

  // Items appearing in both lists (none in this case) would normally rank higher
  // With RRF, items with better ranks should appear first
}

TEST_F(ResultRankerTest, BordaCombination)
{
  KODI::SEMANTIC::RankingConfig config;
  config.algorithm = KODI::SEMANTIC::RankingAlgorithm::Borda;

  m_ranker.SetConfig(config);

  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.8}, {3, 0.7}};
  std::vector<std::pair<int64_t, float>> list2 = {{2, 0.95}, {3, 0.85}, {1, 0.75}};

  auto results = m_ranker.Combine(list1, list2);

  ASSERT_FALSE(results.empty());
  // All items appear in both lists, so ranking depends on positions
}

TEST_F(ResultRankerTest, CombMNZCombination)
{
  KODI::SEMANTIC::RankingConfig config;
  config.algorithm = KODI::SEMANTIC::RankingAlgorithm::CombMNZ;

  m_ranker.SetConfig(config);

  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.5}};
  std::vector<std::pair<int64_t, float>> list2 = {{1, 0.8}, {3, 0.7}};

  auto results = m_ranker.Combine(list1, list2);

  // Item 1 appears in both lists, should be boosted by CombMNZ
  ASSERT_FALSE(results.empty());
  EXPECT_EQ(results[0].id, 1);
}

TEST_F(ResultRankerTest, TopKLimit)
{
  KODI::SEMANTIC::RankingConfig config;
  config.topK = 3;

  m_ranker.SetConfig(config);

  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.8}, {3, 0.7}, {4, 0.6}};
  std::vector<std::pair<int64_t, float>> list2 = {{5, 0.95}, {6, 0.85}, {7, 0.75}, {8, 0.65}};

  auto results = m_ranker.Combine(list1, list2);

  // Should only return top 3 results
  EXPECT_EQ(results.size(), 3);
}

TEST_F(ResultRankerTest, RRFConstantEffect)
{
  KODI::SEMANTIC::RankingConfig config1;
  config1.algorithm = KODI::SEMANTIC::RankingAlgorithm::RRF;
  config1.rrfK = 10.0f;

  KODI::SEMANTIC::RankingConfig config2;
  config2.algorithm = KODI::SEMANTIC::RankingAlgorithm::RRF;
  config2.rrfK = 100.0f;

  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.8}};
  std::vector<std::pair<int64_t, float>> list2 = {{1, 0.95}, {3, 0.85}};

  KODI::SEMANTIC::CResultRanker ranker1(config1);
  KODI::SEMANTIC::CResultRanker ranker2(config2);

  auto results1 = ranker1.Combine(list1, list2);
  auto results2 = ranker2.Combine(list1, list2);

  // Different K values may produce different relative scores
  // but item 1 should still rank first in both (appears in both lists)
  ASSERT_FALSE(results1.empty());
  ASSERT_FALSE(results2.empty());
  EXPECT_EQ(results1[0].id, 1);
  EXPECT_EQ(results2[0].id, 1);
}

TEST_F(ResultRankerTest, WeightBalancing)
{
  KODI::SEMANTIC::RankingConfig config;
  config.algorithm = KODI::SEMANTIC::RankingAlgorithm::Linear;
  config.weight1 = 0.8f;
  config.weight2 = 0.2f;

  m_ranker.SetConfig(config);

  std::vector<std::pair<int64_t, float>> list1 = {{1, 1.0}, {2, 0.1}};
  std::vector<std::pair<int64_t, float>> list2 = {{1, 0.1}, {2, 1.0}};

  auto results = m_ranker.Combine(list1, list2);

  // With weight1 much higher, item 1 (which scores high in list1) should rank first
  ASSERT_GE(results.size(), 2);
  EXPECT_EQ(results[0].id, 1);
}

TEST_F(ResultRankerTest, NormalizationIdenticalScores)
{
  // Edge case: all scores are the same
  std::vector<std::pair<int64_t, float>> scores = {{1, 5.0}, {2, 5.0}, {3, 5.0}};

  auto normalized = KODI::SEMANTIC::CResultRanker::NormalizeScores(scores);

  // When all scores are identical, normalized values should all be equal
  ASSERT_EQ(normalized.size(), 3);
  for (const auto& item : normalized)
  {
    // Implementation may set all to 0.0, 0.5, or 1.0 - just check they're equal
    EXPECT_FLOAT_EQ(normalized[0].second, item.second);
  }
}

TEST_F(ResultRankerTest, MetadataPreservation)
{
  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.8}};
  std::vector<std::pair<int64_t, float>> list2 = {{1, 0.7}, {3, 0.6}};

  auto results = m_ranker.Combine(list1, list2);

  // Check that metadata is preserved
  for (const auto& item : results)
  {
    if (item.id == 1)
    {
      // Item 1 appears in both lists
      EXPECT_NE(item.rank1, -1);
      EXPECT_NE(item.rank2, -1);
      EXPECT_GT(item.score1, 0.0f);
      EXPECT_GT(item.score2, 0.0f);
    }
    else if (item.id == 2)
    {
      // Item 2 only in list1
      EXPECT_NE(item.rank1, -1);
      EXPECT_EQ(item.rank2, -1);
    }
    else if (item.id == 3)
    {
      // Item 3 only in list2
      EXPECT_EQ(item.rank1, -1);
      EXPECT_NE(item.rank2, -1);
    }
  }
}

TEST_F(ResultRankerTest, GetSetConfig)
{
  KODI::SEMANTIC::RankingConfig config;
  config.algorithm = KODI::SEMANTIC::RankingAlgorithm::Borda;
  config.weight1 = 0.6f;
  config.weight2 = 0.4f;
  config.topK = 10;

  m_ranker.SetConfig(config);

  auto retrievedConfig = m_ranker.GetConfig();

  EXPECT_EQ(retrievedConfig.algorithm, KODI::SEMANTIC::RankingAlgorithm::Borda);
  EXPECT_FLOAT_EQ(retrievedConfig.weight1, 0.6f);
  EXPECT_FLOAT_EQ(retrievedConfig.weight2, 0.4f);
  EXPECT_EQ(retrievedConfig.topK, 10);
}

TEST_F(ResultRankerTest, SingleList)
{
  std::vector<std::pair<int64_t, float>> list1 = {{1, 0.9}, {2, 0.8}, {3, 0.7}};
  std::vector<std::pair<int64_t, float>> empty;

  auto results = m_ranker.Combine(list1, empty);

  // Should return all items from list1 in order
  ASSERT_EQ(results.size(), 3);
  EXPECT_EQ(results[0].id, 1);
  EXPECT_EQ(results[1].id, 2);
  EXPECT_EQ(results[2].id, 3);
}
