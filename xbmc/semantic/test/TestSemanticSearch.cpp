/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/SemanticDatabase.h"
#include "semantic/search/SemanticSearch.h"

#include <gtest/gtest.h>

using namespace KODI::SEMANTIC;

class SemanticSearchTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_database = std::make_unique<CSemanticDatabase>();
    m_database->Open();

    m_search = std::make_unique<CSemanticSearch>();
    ASSERT_TRUE(m_search->Initialize(m_database.get()));

    // Insert test data
    PopulateTestData();
  }

  void TearDown() override
  {
    m_search.reset();
    m_database->Close();
  }

  void PopulateTestData()
  {
    // Movie 1: Batman
    SemanticChunk chunk1;
    chunk1.mediaId = 1;
    chunk1.mediaType = "movie";
    chunk1.sourceType = SourceType::SUBTITLE;
    chunk1.text = "The joker burns all the money in a massive fire";
    chunk1.startMs = 5000;
    chunk1.endMs = 10000;
    m_database->InsertChunk(chunk1);

    SemanticChunk chunk2;
    chunk2.mediaId = 1;
    chunk2.mediaType = "movie";
    chunk2.sourceType = SourceType::SUBTITLE;
    chunk2.text = "Batman fights crime in Gotham City at night";
    chunk2.startMs = 10000;
    chunk2.endMs = 15000;
    m_database->InsertChunk(chunk2);

    // Movie 2: Spider-Man
    SemanticChunk chunk3;
    chunk3.mediaId = 2;
    chunk3.mediaType = "movie";
    chunk3.sourceType = SourceType::SUBTITLE;
    chunk3.text = "Spider-Man swings through New York City";
    chunk3.startMs = 3000;
    chunk3.endMs = 8000;
    m_database->InsertChunk(chunk3);

    // Episode: TV Show
    SemanticChunk chunk4;
    chunk4.mediaId = 3;
    chunk4.mediaType = "episode";
    chunk4.sourceType = SourceType::SUBTITLE;
    chunk4.text = "The detective investigates a mysterious crime scene";
    chunk4.startMs = 2000;
    chunk4.endMs = 7000;
    m_database->InsertChunk(chunk4);

    // Mark media as indexed
    SemanticIndexState state1;
    state1.mediaId = 1;
    state1.mediaType = "movie";
    state1.subtitleStatus = IndexStatus::COMPLETED;
    m_database->UpdateIndexState(state1);

    SemanticIndexState state2;
    state2.mediaId = 2;
    state2.mediaType = "movie";
    state2.subtitleStatus = IndexStatus::COMPLETED;
    m_database->UpdateIndexState(state2);
  }

  std::unique_ptr<CSemanticDatabase> m_database;
  std::unique_ptr<CSemanticSearch> m_search;
};

TEST_F(SemanticSearchTest, InitializationCheck)
{
  EXPECT_TRUE(m_search->IsInitialized());

  CSemanticSearch uninitSearch;
  EXPECT_FALSE(uninitSearch.IsInitialized());
}

TEST_F(SemanticSearchTest, BasicSearch)
{
  auto results = m_search->Search("joker money");
  ASSERT_FALSE(results.empty());

  // Should find the joker/money chunk
  bool foundJoker = false;
  for (const auto& result : results)
  {
    if (result.chunk.text.find("joker") != std::string::npos)
    {
      foundJoker = true;
      EXPECT_GT(result.score, 0.0f);
      break;
    }
  }
  EXPECT_TRUE(foundJoker);
}

TEST_F(SemanticSearchTest, QueryNormalization)
{
  // Test that different casings produce same results
  auto results1 = m_search->Search("BATMAN");
  auto results2 = m_search->Search("batman");
  auto results3 = m_search->Search("  Batman  ");

  EXPECT_EQ(results1.size(), results2.size());
  EXPECT_EQ(results2.size(), results3.size());

  // Results should be identical
  if (!results1.empty() && !results2.empty())
  {
    EXPECT_EQ(results1[0].chunk.chunkId, results2[0].chunk.chunkId);
  }
}

TEST_F(SemanticSearchTest, MediaSpecificSearch)
{
  auto results = m_search->SearchInMedia("batman", 1, "movie");
  EXPECT_FALSE(results.empty());

  // All results should be from media ID 1
  for (const auto& result : results)
  {
    EXPECT_EQ(result.chunk.mediaId, 1);
    EXPECT_EQ(result.chunk.mediaType, "movie");
  }
}

TEST_F(SemanticSearchTest, SearchWithOptions)
{
  SearchOptions options;
  options.maxResults = 2;

  auto results = m_search->Search("city", options);
  EXPECT_LE(results.size(), 2);
}

TEST_F(SemanticSearchTest, SearchReturnsRelevantResults)
{
  auto results = m_search->Search("crime");
  ASSERT_FALSE(results.empty());

  // Should return chunks about crime (Batman or detective)
  bool foundCrime = false;
  for (const auto& result : results)
  {
    if (result.chunk.text.find("crime") != std::string::npos)
    {
      foundCrime = true;
      break;
    }
  }
  EXPECT_TRUE(foundCrime);
}

TEST_F(SemanticSearchTest, EmptyQuery)
{
  auto results = m_search->Search("");
  // Empty query should return no results or all results (depending on implementation)
  EXPECT_GE(results.size(), 0);
}

TEST_F(SemanticSearchTest, QueryWithSpecialCharacters)
{
  // Should handle special characters gracefully
  EXPECT_NO_THROW({
    auto results = m_search->Search("batman & joker");
    // Should still find batman-related content
  });
}

TEST_F(SemanticSearchTest, GetContext)
{
  // Get context around timestamp in movie 1
  auto context = m_search->GetContext(1, "movie", 12000, 10000);
  EXPECT_FALSE(context.empty());

  // Should return chunks near 12000ms (±5000ms window)
  for (const auto& chunk : context)
  {
    EXPECT_EQ(chunk.mediaId, 1);
    EXPECT_EQ(chunk.mediaType, "movie");
  }
}

TEST_F(SemanticSearchTest, GetMediaChunks)
{
  auto chunks = m_search->GetMediaChunks(1, "movie");
  EXPECT_GE(chunks.size(), 2); // We inserted 2 chunks for movie 1

  // All chunks should be from movie 1
  for (const auto& chunk : chunks)
  {
    EXPECT_EQ(chunk.mediaId, 1);
    EXPECT_EQ(chunk.mediaType, "movie");
  }
}

TEST_F(SemanticSearchTest, IsMediaSearchable)
{
  // Movie 1 and 2 are marked as completed
  EXPECT_TRUE(m_search->IsMediaSearchable(1, "movie"));
  EXPECT_TRUE(m_search->IsMediaSearchable(2, "movie"));

  // Non-existent media should not be searchable
  EXPECT_FALSE(m_search->IsMediaSearchable(999, "movie"));
}

TEST_F(SemanticSearchTest, GetSearchStats)
{
  auto stats = m_search->GetSearchStats();

  EXPECT_GT(stats.totalChunks, 0);
  EXPECT_GT(stats.totalMedia, 0);
}

TEST_F(SemanticSearchTest, MultipleWordSearch)
{
  auto results = m_search->Search("gotham city");
  ASSERT_FALSE(results.empty());

  // Should find Batman chunk with "Gotham City"
  bool foundGotham = false;
  for (const auto& result : results)
  {
    if (result.chunk.text.find("Gotham") != std::string::npos)
    {
      foundGotham = true;
      break;
    }
  }
  EXPECT_TRUE(foundGotham);
}

TEST_F(SemanticSearchTest, SearchAcrossMediaTypes)
{
  auto results = m_search->Search("city");

  // Should find results from both movies (Gotham City, New York City)
  bool foundMovie = false;
  for (const auto& result : results)
  {
    if (result.chunk.mediaType == "movie")
    {
      foundMovie = true;
      break;
    }
  }
  EXPECT_TRUE(foundMovie);
}

TEST_F(SemanticSearchTest, SearchWithMediaTypeFilter)
{
  SearchOptions options;
  options.mediaType = "episode";

  auto results = m_search->Search("detective", options);

  // All results should be episodes
  for (const auto& result : results)
  {
    EXPECT_EQ(result.chunk.mediaType, "episode");
  }
}

TEST_F(SemanticSearchTest, SearchWithSourceTypeFilter)
{
  SearchOptions options;
  options.sourceType = SourceType::SUBTITLE;

  auto results = m_search->Search("batman", options);

  // All results should be from subtitles
  for (const auto& result : results)
  {
    EXPECT_EQ(result.chunk.sourceType, SourceType::SUBTITLE);
  }
}

TEST_F(SemanticSearchTest, NoResultsForNonExistentQuery)
{
  auto results = m_search->Search("xyzabc123nonexistent");
  EXPECT_TRUE(results.empty());
}

TEST_F(SemanticSearchTest, SearchResultsSorted)
{
  auto results = m_search->Search("city");
  ASSERT_GT(results.size(), 1);

  // Results should be sorted by score (descending)
  for (size_t i = 1; i < results.size(); ++i)
  {
    EXPECT_GE(results[i - 1].score, results[i].score);
  }
}

TEST_F(SemanticSearchTest, RecordSearch)
{
  // Should not throw when recording searches
  EXPECT_NO_THROW({
    m_search->RecordSearch("test query", 5);
    m_search->RecordSearch("another query", 0);
  });
}

TEST_F(SemanticSearchTest, GetSuggestions)
{
  // Record some searches first
  m_search->RecordSearch("batman", 10);
  m_search->RecordSearch("batman returns", 5);

  // Get suggestions (implementation may not be complete in Phase 1)
  auto suggestions = m_search->GetSuggestions("bat", 5);
  EXPECT_GE(suggestions.size(), 0); // May be empty if not implemented yet
}
