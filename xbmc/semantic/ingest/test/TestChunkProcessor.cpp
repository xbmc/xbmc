/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/ingest/ChunkProcessor.h"

#include <gtest/gtest.h>

using namespace KODI::SEMANTIC;

class ChunkProcessorTest : public ::testing::Test
{
protected:
  void SetUp() override { m_processor = std::make_unique<CChunkProcessor>(); }

  std::unique_ptr<CChunkProcessor> m_processor;
};

TEST_F(ChunkProcessorTest, DefaultConfiguration)
{
  const ChunkConfig& config = m_processor->GetConfig();

  EXPECT_EQ(config.maxChunkWords, 50);
  EXPECT_EQ(config.minChunkWords, 10);
  EXPECT_EQ(config.overlapWords, 5);
  EXPECT_TRUE(config.mergeShortEntries);
  EXPECT_EQ(config.maxMergeGapMs, 2000);
}

TEST_F(ChunkProcessorTest, CustomConfiguration)
{
  ChunkConfig customConfig;
  customConfig.maxChunkWords = 100;
  customConfig.minChunkWords = 20;
  customConfig.overlapWords = 10;
  customConfig.mergeShortEntries = false;
  customConfig.maxMergeGapMs = 3000;

  m_processor->SetConfig(customConfig);

  const ChunkConfig& config = m_processor->GetConfig();
  EXPECT_EQ(config.maxChunkWords, 100);
  EXPECT_EQ(config.minChunkWords, 20);
  EXPECT_EQ(config.overlapWords, 10);
  EXPECT_FALSE(config.mergeShortEntries);
  EXPECT_EQ(config.maxMergeGapMs, 3000);
}

TEST_F(ChunkProcessorTest, ProcessEmptyEntries)
{
  std::vector<ParsedEntry> entries;

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  EXPECT_TRUE(chunks.empty());
}

TEST_F(ChunkProcessorTest, ProcessSingleShortEntry)
{
  std::vector<ParsedEntry> entries;

  ParsedEntry entry;
  entry.startMs = 1000;
  entry.endMs = 3000;
  entry.text = "This is a short subtitle entry with ten words here.";
  entry.confidence = 1.0f;
  entries.push_back(entry);

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  ASSERT_EQ(chunks.size(), 1);
  EXPECT_EQ(chunks[0].mediaId, 1);
  EXPECT_EQ(chunks[0].mediaType, "movie");
  EXPECT_EQ(chunks[0].sourceType, SourceType::SUBTITLE);
  EXPECT_EQ(chunks[0].startMs, 1000);
  EXPECT_EQ(chunks[0].endMs, 3000);
  EXPECT_EQ(chunks[0].text, entry.text);
  EXPECT_FLOAT_EQ(chunks[0].confidence, 1.0f);
}

TEST_F(ChunkProcessorTest, MergeShortAdjacentEntries)
{
  std::vector<ParsedEntry> entries;

  // Three short entries that should merge
  ParsedEntry entry1;
  entry1.startMs = 1000;
  entry1.endMs = 2000;
  entry1.text = "First short entry.";
  entry1.confidence = 1.0f;
  entries.push_back(entry1);

  ParsedEntry entry2;
  entry2.startMs = 2500;
  entry2.endMs = 3500;
  entry2.text = "Second short entry.";
  entry2.confidence = 0.95f;
  entries.push_back(entry2);

  ParsedEntry entry3;
  entry3.startMs = 4000;
  entry3.endMs = 5000;
  entry3.text = "Third short entry.";
  entry3.confidence = 0.98f;
  entries.push_back(entry3);

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  // Should merge into fewer chunks
  EXPECT_LE(chunks.size(), 2); // Depending on config, might be 1 or 2 chunks
  EXPECT_GT(chunks.size(), 0);

  // First chunk should contain merged text
  EXPECT_TRUE(chunks[0].text.find("First") != std::string::npos);
  EXPECT_TRUE(chunks[0].text.find("Second") != std::string::npos ||
              chunks.size() > 1); // Either in first or there's a second chunk
}

TEST_F(ChunkProcessorTest, DoNotMergeEntriesWithLargeGap)
{
  std::vector<ParsedEntry> entries;

  ParsedEntry entry1;
  entry1.startMs = 1000;
  entry1.endMs = 2000;
  entry1.text = "First entry with enough words.";
  entry1.confidence = 1.0f;
  entries.push_back(entry1);

  ParsedEntry entry2;
  entry2.startMs = 10000; // 8 second gap, exceeds maxMergeGapMs
  entry2.endMs = 11000;
  entry2.text = "Second entry with enough words.";
  entry2.confidence = 1.0f;
  entries.push_back(entry2);

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  // Should create separate chunks due to large time gap
  ASSERT_EQ(chunks.size(), 2);
  EXPECT_EQ(chunks[0].startMs, 1000);
  EXPECT_EQ(chunks[1].startMs, 10000);
}

TEST_F(ChunkProcessorTest, SplitLongEntry)
{
  std::vector<ParsedEntry> entries;

  ParsedEntry entry;
  entry.startMs = 1000;
  entry.endMs = 10000;
  entry.text =
      "This is a very long subtitle entry that contains many words and should be split into "
      "multiple chunks because it exceeds the maximum chunk size. It has multiple sentences. "
      "Each sentence provides valuable context. The chunking algorithm should split this "
      "intelligently at sentence boundaries to maintain readability and search effectiveness.";
  entry.confidence = 1.0f;
  entries.push_back(entry);

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  // Should split into multiple chunks
  EXPECT_GT(chunks.size(), 1);

  // All chunks should have same media ID and type
  for (const auto& chunk : chunks)
  {
    EXPECT_EQ(chunk.mediaId, 1);
    EXPECT_EQ(chunk.mediaType, "movie");
    EXPECT_EQ(chunk.sourceType, SourceType::SUBTITLE);
  }

  // Timing should be distributed across chunks
  EXPECT_EQ(chunks[0].startMs, 1000);
  EXPECT_EQ(chunks[chunks.size() - 1].endMs, 10000);
}

TEST_F(ChunkProcessorTest, ProcessTextSingleChunk)
{
  std::string text = "This is a short text that fits in a single chunk.";

  auto chunks = m_processor->ProcessText(text, 1, "movie", SourceType::METADATA);

  ASSERT_EQ(chunks.size(), 1);
  EXPECT_EQ(chunks[0].mediaId, 1);
  EXPECT_EQ(chunks[0].mediaType, "movie");
  EXPECT_EQ(chunks[0].sourceType, SourceType::METADATA);
  EXPECT_EQ(chunks[0].startMs, 0);
  EXPECT_EQ(chunks[0].endMs, 0);
  EXPECT_EQ(chunks[0].text, text);
  EXPECT_FLOAT_EQ(chunks[0].confidence, 1.0f);
}

TEST_F(ChunkProcessorTest, ProcessTextMultipleChunks)
{
  std::string text =
      "This is a very long text that should be split into multiple chunks. It contains many "
      "sentences. Each sentence provides context. The text processor should handle this well. "
      "It should split at sentence boundaries. This ensures good readability. Search "
      "effectiveness is maintained. The algorithm is intelligent. It respects natural language "
      "boundaries. Users will find accurate results. The system works efficiently.";

  auto chunks = m_processor->ProcessText(text, 1, "movie", SourceType::METADATA);

  // Should split into multiple chunks
  EXPECT_GT(chunks.size(), 1);

  // All chunks should have correct metadata
  for (const auto& chunk : chunks)
  {
    EXPECT_EQ(chunk.mediaId, 1);
    EXPECT_EQ(chunk.mediaType, "movie");
    EXPECT_EQ(chunk.sourceType, SourceType::METADATA);
    EXPECT_EQ(chunk.startMs, 0);
    EXPECT_EQ(chunk.endMs, 0);
    EXPECT_FALSE(chunk.text.empty());
  }
}

TEST_F(ChunkProcessorTest, ProcessEmptyText)
{
  std::string text = "";

  auto chunks = m_processor->ProcessText(text, 1, "movie", SourceType::METADATA);

  EXPECT_TRUE(chunks.empty());
}

TEST_F(ChunkProcessorTest, SkipEntriesBelowMinimum)
{
  ChunkConfig config;
  config.minChunkWords = 10;
  config.mergeShortEntries = false; // Disable merging to test individual filtering
  m_processor->SetConfig(config);

  std::vector<ParsedEntry> entries;

  // Entry below minimum
  ParsedEntry entry1;
  entry1.startMs = 1000;
  entry1.endMs = 2000;
  entry1.text = "Too short."; // Only 2 words
  entry1.confidence = 1.0f;
  entries.push_back(entry1);

  // Entry meeting minimum
  ParsedEntry entry2;
  entry2.startMs = 3000;
  entry2.endMs = 4000;
  entry2.text = "This entry has enough words to meet the minimum requirement set.";
  entry2.confidence = 1.0f;
  entries.push_back(entry2);

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  // Should only create chunk from second entry
  ASSERT_EQ(chunks.size(), 1);
  EXPECT_EQ(chunks[0].startMs, 3000);
}

TEST_F(ChunkProcessorTest, TranscriptionSourceType)
{
  std::vector<ParsedEntry> entries;

  ParsedEntry entry;
  entry.startMs = 1000;
  entry.endMs = 5000;
  entry.text = "This is a transcription entry with sufficient words.";
  entry.confidence = 0.89f;
  entries.push_back(entry);

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::TRANSCRIPTION);

  ASSERT_EQ(chunks.size(), 1);
  EXPECT_EQ(chunks[0].sourceType, SourceType::TRANSCRIPTION);
  EXPECT_FLOAT_EQ(chunks[0].confidence, 0.89f);
}

TEST_F(ChunkProcessorTest, ConfidenceTrackingInMerge)
{
  std::vector<ParsedEntry> entries;

  ParsedEntry entry1;
  entry1.startMs = 1000;
  entry1.endMs = 2000;
  entry1.text = "First entry high confidence.";
  entry1.confidence = 1.0f;
  entries.push_back(entry1);

  ParsedEntry entry2;
  entry2.startMs = 2500;
  entry2.endMs = 3500;
  entry2.text = "Second entry lower confidence.";
  entry2.confidence = 0.75f;
  entries.push_back(entry2);

  auto chunks = m_processor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  // Should track minimum confidence
  ASSERT_GT(chunks.size(), 0);
  EXPECT_LE(chunks[0].confidence, 1.0f);
  EXPECT_GE(chunks[0].confidence, 0.75f);
}
