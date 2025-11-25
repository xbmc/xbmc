/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/search/VectorSearcher.h"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <algorithm>
#include <cmath>
#include <random>

class VectorSearcherTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Create in-memory SQLite database
    sqlite3_open(":memory:", &m_db);

    m_searcher = std::make_unique<KODI::SEMANTIC::CVectorSearcher>();
    m_initialized = m_searcher->InitializeExtension(m_db);

    if (m_initialized)
      m_searcher->CreateVectorTable();
  }

  void TearDown() override
  {
    m_searcher.reset();
    if (m_db)
      sqlite3_close(m_db);
  }

  std::array<float, 384> RandomEmbedding()
  {
    std::array<float, 384> emb;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 1.0f);

    float norm = 0.0f;
    for (auto& v : emb)
    {
      v = dist(gen);
      norm += v * v;
    }
    norm = std::sqrt(norm);
    for (auto& v : emb)
      v /= norm;

    return emb;
  }

  std::array<float, 384> ZeroEmbedding()
  {
    std::array<float, 384> emb;
    emb.fill(0.0f);
    return emb;
  }

  std::array<float, 384> OnesEmbedding()
  {
    std::array<float, 384> emb;
    emb.fill(1.0f);
    // Normalize
    float norm = std::sqrt(384.0f);
    for (auto& v : emb)
      v /= norm;
    return emb;
  }

  sqlite3* m_db = nullptr;
  std::unique_ptr<KODI::SEMANTIC::CVectorSearcher> m_searcher;
  bool m_initialized = false;
};

TEST_F(VectorSearcherTest, InsertAndSearch)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  auto emb1 = RandomEmbedding();
  ASSERT_TRUE(m_searcher->InsertVector(1, emb1));

  auto results = m_searcher->SearchSimilar(emb1, 10);

  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].chunkId, 1);
  EXPECT_NEAR(results[0].distance, 0.0f, 0.01f); // Same vector = 0 distance
}

TEST_F(VectorSearcherTest, BatchInsert)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  std::vector<std::pair<int64_t, std::array<float, 384>>> batch;
  for (int i = 0; i < 100; ++i)
  {
    batch.emplace_back(i + 1, RandomEmbedding());
  }

  ASSERT_TRUE(m_searcher->InsertVectorBatch(batch));
  EXPECT_EQ(m_searcher->GetVectorCount(), 100);
}

TEST_F(VectorSearcherTest, TopKSearch)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert 50 vectors
  for (int i = 0; i < 50; ++i)
  {
    m_searcher->InsertVector(i + 1, RandomEmbedding());
  }

  auto query = RandomEmbedding();
  auto results = m_searcher->SearchSimilar(query, 10);

  EXPECT_LE(results.size(), 10);

  // Results should be sorted by distance (ascending)
  for (size_t i = 1; i < results.size(); ++i)
  {
    EXPECT_LE(results[i - 1].distance, results[i].distance);
  }
}

TEST_F(VectorSearcherTest, FilteredSearch)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert vectors
  for (int i = 1; i <= 20; ++i)
  {
    m_searcher->InsertVector(i, RandomEmbedding());
  }

  auto query = RandomEmbedding();
  std::vector<int64_t> candidates = {5, 10, 15};

  auto results = m_searcher->SearchSimilarFiltered(query, candidates, 10);

  // Results should only contain candidates
  for (const auto& r : results)
  {
    bool found =
        std::find(candidates.begin(), candidates.end(), r.chunkId) != candidates.end();
    EXPECT_TRUE(found);
  }

  // Should not exceed candidate count
  EXPECT_LE(results.size(), candidates.size());
}

TEST_F(VectorSearcherTest, DeleteVector)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  auto emb = RandomEmbedding();
  ASSERT_TRUE(m_searcher->InsertVector(1, emb));
  EXPECT_EQ(m_searcher->GetVectorCount(), 1);

  ASSERT_TRUE(m_searcher->DeleteVector(1));
  EXPECT_EQ(m_searcher->GetVectorCount(), 0);
}

TEST_F(VectorSearcherTest, BatchDelete)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert 10 vectors
  for (int i = 1; i <= 10; ++i)
  {
    m_searcher->InsertVector(i, RandomEmbedding());
  }
  EXPECT_EQ(m_searcher->GetVectorCount(), 10);

  // Delete half of them
  std::vector<int64_t> toDelete = {2, 4, 6, 8, 10};
  ASSERT_TRUE(m_searcher->DeleteVectorBatch(toDelete));
  EXPECT_EQ(m_searcher->GetVectorCount(), 5);
}

TEST_F(VectorSearcherTest, ClearAllVectors)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert vectors
  for (int i = 1; i <= 20; ++i)
  {
    m_searcher->InsertVector(i, RandomEmbedding());
  }
  EXPECT_EQ(m_searcher->GetVectorCount(), 20);

  ASSERT_TRUE(m_searcher->ClearAllVectors());
  EXPECT_EQ(m_searcher->GetVectorCount(), 0);
}

TEST_F(VectorSearcherTest, UpdateVector)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  auto emb1 = RandomEmbedding();
  auto emb2 = RandomEmbedding();

  // Insert first embedding
  ASSERT_TRUE(m_searcher->InsertVector(1, emb1));

  // Update with new embedding (insert with same ID)
  ASSERT_TRUE(m_searcher->InsertVector(1, emb2));

  // Should still have only one vector
  EXPECT_EQ(m_searcher->GetVectorCount(), 1);

  // Search with second embedding should find it
  auto results = m_searcher->SearchSimilar(emb2, 1);
  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].chunkId, 1);
}

TEST_F(VectorSearcherTest, EmptySearch)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Search in empty database
  auto query = RandomEmbedding();
  auto results = m_searcher->SearchSimilar(query, 10);

  EXPECT_TRUE(results.empty());
}

TEST_F(VectorSearcherTest, LargeBatchInsert)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Test with a larger batch to verify transaction handling
  std::vector<std::pair<int64_t, std::array<float, 384>>> batch;
  for (int i = 0; i < 1000; ++i)
  {
    batch.emplace_back(i + 1, RandomEmbedding());
  }

  ASSERT_TRUE(m_searcher->InsertVectorBatch(batch));
  EXPECT_EQ(m_searcher->GetVectorCount(), 1000);
}

TEST_F(VectorSearcherTest, FilteredSearchEmptyCandidates)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert some vectors
  for (int i = 1; i <= 10; ++i)
  {
    m_searcher->InsertVector(i, RandomEmbedding());
  }

  auto query = RandomEmbedding();
  std::vector<int64_t> emptyCandidates;

  auto results = m_searcher->SearchSimilarFiltered(query, emptyCandidates, 10);

  EXPECT_TRUE(results.empty());
}

TEST_F(VectorSearcherTest, FilteredSearchNonexistentCandidates)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert vectors 1-10
  for (int i = 1; i <= 10; ++i)
  {
    m_searcher->InsertVector(i, RandomEmbedding());
  }

  auto query = RandomEmbedding();
  std::vector<int64_t> nonexistentCandidates = {100, 200, 300};

  auto results = m_searcher->SearchSimilarFiltered(query, nonexistentCandidates, 10);

  EXPECT_TRUE(results.empty());
}

TEST_F(VectorSearcherTest, TopKLargerThanResults)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert only 5 vectors
  for (int i = 1; i <= 5; ++i)
  {
    m_searcher->InsertVector(i, RandomEmbedding());
  }

  auto query = RandomEmbedding();
  auto results = m_searcher->SearchSimilar(query, 100); // Request more than available

  EXPECT_EQ(results.size(), 5); // Should return all available
}

TEST_F(VectorSearcherTest, DistanceMetric)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Create two identical embeddings
  auto emb1 = OnesEmbedding();
  auto emb2 = OnesEmbedding();

  m_searcher->InsertVector(1, emb1);
  auto results = m_searcher->SearchSimilar(emb2, 1);

  ASSERT_EQ(results.size(), 1);
  // Identical normalized vectors should have distance close to 0
  EXPECT_NEAR(results[0].distance, 0.0f, 0.01f);
}

TEST_F(VectorSearcherTest, GetStats)
{
  if (!m_initialized)
    GTEST_SKIP() << "VectorSearcher not initialized - sqlite-vec extension not available";

  // Insert some vectors
  for (int i = 1; i <= 10; ++i)
  {
    m_searcher->InsertVector(i, RandomEmbedding());
  }

  auto stats = m_searcher->GetStats();

  EXPECT_EQ(stats.totalVectors, 10);
  // Note: vectorsWithMedia may be 0 if semantic_chunks table doesn't exist
}
