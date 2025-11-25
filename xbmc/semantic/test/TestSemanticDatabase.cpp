/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/SemanticDatabase.h"
#include "semantic/SemanticTypes.h"

#include <gtest/gtest.h>

using namespace KODI::SEMANTIC;

class SemanticDatabaseTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_database = std::make_unique<CSemanticDatabase>();
    // Use in-memory SQLite for testing
    ASSERT_TRUE(m_database->Open());
  }

  void TearDown() override { m_database->Close(); }

  std::unique_ptr<CSemanticDatabase> m_database;
};

TEST_F(SemanticDatabaseTest, InsertAndRetrieveChunk)
{
  SemanticChunk chunk;
  chunk.mediaId = 123;
  chunk.mediaType = "movie";
  chunk.sourceType = SourceType::SUBTITLE;
  chunk.startMs = 1000;
  chunk.endMs = 5000;
  chunk.text = "Hello, world!";
  chunk.confidence = 1.0f;

  int chunkId = m_database->InsertChunk(chunk);
  EXPECT_GT(chunkId, 0);

  SemanticChunk retrieved;
  ASSERT_TRUE(m_database->GetChunk(chunkId, retrieved));

  EXPECT_EQ(retrieved.mediaId, chunk.mediaId);
  EXPECT_EQ(retrieved.text, chunk.text);
  EXPECT_EQ(retrieved.startMs, chunk.startMs);
  EXPECT_EQ(retrieved.endMs, chunk.endMs);
  EXPECT_EQ(retrieved.sourceType, chunk.sourceType);
}

TEST_F(SemanticDatabaseTest, FTS5Search)
{
  // Insert test chunks
  for (int i = 0; i < 5; ++i)
  {
    SemanticChunk chunk;
    chunk.mediaId = 100 + i;
    chunk.mediaType = "movie";
    chunk.sourceType = SourceType::SUBTITLE;
    chunk.text = "The quick brown fox jumps over the lazy dog " + std::to_string(i);
    m_database->InsertChunk(chunk);
  }

  SearchOptions options;
  options.maxResults = 10;

  auto results = m_database->SearchChunks("quick fox", options);
  EXPECT_EQ(results.size(), 5);

  // Verify results have scores
  for (const auto& result : results)
  {
    EXPECT_GT(result.score, 0.0f);
    EXPECT_NE(result.chunk.text.find("quick"), std::string::npos);
  }
}

TEST_F(SemanticDatabaseTest, IndexStateTracking)
{
  SemanticIndexState state;
  state.mediaId = 456;
  state.mediaType = "episode";
  state.subtitleStatus = IndexStatus::IN_PROGRESS;
  state.subtitleProgress = 0.5f;

  ASSERT_TRUE(m_database->UpdateIndexState(state));

  SemanticIndexState retrieved;
  ASSERT_TRUE(m_database->GetIndexState(456, "episode", retrieved));

  EXPECT_EQ(retrieved.subtitleStatus, IndexStatus::IN_PROGRESS);
  EXPECT_FLOAT_EQ(retrieved.subtitleProgress, 0.5f);
}

TEST_F(SemanticDatabaseTest, BatchInsert)
{
  std::vector<SemanticChunk> chunks;
  for (int i = 0; i < 100; ++i)
  {
    SemanticChunk chunk;
    chunk.mediaId = 789;
    chunk.mediaType = "movie";
    chunk.sourceType = SourceType::SUBTITLE;
    chunk.text = "Test chunk " + std::to_string(i);
    chunks.push_back(chunk);
  }

  ASSERT_TRUE(m_database->InsertChunks(chunks));

  auto stats = m_database->GetStats();
  EXPECT_GE(stats.totalChunks, 100);
}

TEST_F(SemanticDatabaseTest, DeleteChunksForMedia)
{
  // Insert chunks for multiple media items
  SemanticChunk chunk1;
  chunk1.mediaId = 1;
  chunk1.mediaType = "movie";
  chunk1.text = "Movie 1 content";
  m_database->InsertChunk(chunk1);

  SemanticChunk chunk2;
  chunk2.mediaId = 2;
  chunk2.mediaType = "movie";
  chunk2.text = "Movie 2 content";
  m_database->InsertChunk(chunk2);

  // Delete chunks for media ID 1
  ASSERT_TRUE(m_database->DeleteChunksForMedia(1, "movie"));

  // Verify deletion
  std::vector<SemanticChunk> remainingChunks;
  ASSERT_TRUE(m_database->GetChunksForMedia(1, "movie", remainingChunks));
  EXPECT_EQ(remainingChunks.size(), 0);

  // Media ID 2 should still have chunks
  ASSERT_TRUE(m_database->GetChunksForMedia(2, "movie", remainingChunks));
  EXPECT_EQ(remainingChunks.size(), 1);
}

TEST_F(SemanticDatabaseTest, GetContext)
{
  // Insert chunks with different timestamps
  for (int i = 0; i < 10; ++i)
  {
    SemanticChunk chunk;
    chunk.mediaId = 1;
    chunk.mediaType = "movie";
    chunk.sourceType = SourceType::SUBTITLE;
    chunk.startMs = i * 10000; // Every 10 seconds
    chunk.endMs = chunk.startMs + 5000;
    chunk.text = "Chunk " + std::to_string(i);
    m_database->InsertChunk(chunk);
  }

  // Get context around 30 seconds (30000ms) with 20s window
  auto context = m_database->GetContext(1, "movie", 30000, 20000);

  // Should get chunks from 20s to 40s (indices 2, 3, 4)
  EXPECT_GE(context.size(), 3);
  EXPECT_LE(context.size(), 5);
}

TEST_F(SemanticDatabaseTest, SearchWithFilters)
{
  // Insert chunks for different media types
  SemanticChunk movieChunk;
  movieChunk.mediaId = 1;
  movieChunk.mediaType = "movie";
  movieChunk.sourceType = SourceType::SUBTITLE;
  movieChunk.text = "Batman fights crime";
  m_database->InsertChunk(movieChunk);

  SemanticChunk episodeChunk;
  episodeChunk.mediaId = 2;
  episodeChunk.mediaType = "episode";
  episodeChunk.sourceType = SourceType::SUBTITLE;
  episodeChunk.text = "Batman appears in the show";
  m_database->InsertChunk(episodeChunk);

  // Search only in movies
  SearchOptions options;
  options.mediaType = "movie";

  auto results = m_database->SearchChunks("Batman", options);
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].chunk.mediaType, "movie");
}

TEST_F(SemanticDatabaseTest, ProviderManagement)
{
  ASSERT_TRUE(m_database->UpdateProvider("groq", "Groq Whisper", true, true));

  std::string displayName;
  bool isEnabled;
  float totalMinutesUsed;

  ASSERT_TRUE(m_database->GetProvider("groq", displayName, isEnabled, totalMinutesUsed));
  EXPECT_EQ(displayName, "Groq Whisper");
  EXPECT_TRUE(isEnabled);
  EXPECT_FLOAT_EQ(totalMinutesUsed, 0.0f);

  // Update usage
  ASSERT_TRUE(m_database->UpdateProviderUsage("groq", 5.5f));

  ASSERT_TRUE(m_database->GetProvider("groq", displayName, isEnabled, totalMinutesUsed));
  EXPECT_FLOAT_EQ(totalMinutesUsed, 5.5f);
}

TEST_F(SemanticDatabaseTest, TransactionSupport)
{
  ASSERT_TRUE(m_database->BeginTransaction());

  SemanticChunk chunk;
  chunk.mediaId = 1;
  chunk.mediaType = "movie";
  chunk.text = "Transaction test";

  int chunkId = m_database->InsertChunk(chunk);
  EXPECT_GT(chunkId, 0);

  ASSERT_TRUE(m_database->CommitTransaction());

  // Verify chunk was committed
  SemanticChunk retrieved;
  ASSERT_TRUE(m_database->GetChunk(chunkId, retrieved));
  EXPECT_EQ(retrieved.text, "Transaction test");
}

TEST_F(SemanticDatabaseTest, TransactionRollback)
{
  ASSERT_TRUE(m_database->BeginTransaction());

  SemanticChunk chunk;
  chunk.mediaId = 1;
  chunk.mediaType = "movie";
  chunk.text = "Rollback test";

  int chunkId = m_database->InsertChunk(chunk);
  EXPECT_GT(chunkId, 0);

  ASSERT_TRUE(m_database->RollbackTransaction());

  // Verify chunk was not committed
  SemanticChunk retrieved;
  EXPECT_FALSE(m_database->GetChunk(chunkId, retrieved));
}

TEST_F(SemanticDatabaseTest, GetPendingIndexStates)
{
  // Create some pending index states
  for (int i = 0; i < 5; ++i)
  {
    SemanticIndexState state;
    state.mediaId = 100 + i;
    state.mediaType = "movie";
    state.subtitleStatus = IndexStatus::PENDING;
    m_database->UpdateIndexState(state);
  }

  std::vector<SemanticIndexState> pending;
  ASSERT_TRUE(m_database->GetPendingIndexStates(3, pending));

  EXPECT_EQ(pending.size(), 3);
  for (const auto& state : pending)
  {
    EXPECT_EQ(state.subtitleStatus, IndexStatus::PENDING);
  }
}

TEST_F(SemanticDatabaseTest, GetStats)
{
  // Insert some test data
  for (int i = 0; i < 10; ++i)
  {
    SemanticChunk chunk;
    chunk.mediaId = i;
    chunk.mediaType = "movie";
    chunk.text = "Test chunk";
    m_database->InsertChunk(chunk);
  }

  auto stats = m_database->GetStats();
  EXPECT_GE(stats.totalChunks, 10);
  EXPECT_GE(stats.totalMediaItems, 1);
}

TEST_F(SemanticDatabaseTest, GetSnippet)
{
  SemanticChunk chunk;
  chunk.mediaId = 1;
  chunk.mediaType = "movie";
  chunk.text = "The quick brown fox jumps over the lazy dog";

  int chunkId = m_database->InsertChunk(chunk);
  ASSERT_GT(chunkId, 0);

  std::string snippet = m_database->GetSnippet("quick fox", chunkId, 50);
  EXPECT_FALSE(snippet.empty());
  EXPECT_NE(snippet.find("quick"), std::string::npos);
  EXPECT_NE(snippet.find("fox"), std::string::npos);
}
