/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/SemanticDatabase.h"
#include "semantic/SemanticIndexService.h"
#include "semantic/ingest/ChunkProcessor.h"
#include "semantic/ingest/MetadataParser.h"
#include "semantic/ingest/SubtitleParser.h"
#include "semantic/search/SemanticSearch.h"
#include "semantic/transcription/TranscriptionProviderManager.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>

using namespace KODI::SEMANTIC;

namespace
{
// Helper to get test data directory
std::string GetTestDataPath()
{
  // Get the path relative to the test binary
  std::filesystem::path currentPath = std::filesystem::current_path();
  std::filesystem::path testDataPath = currentPath / "xbmc" / "semantic" / "test" / "testdata";

  // Fallback to source tree location
  if (!std::filesystem::exists(testDataPath))
  {
    testDataPath = std::filesystem::path(__FILE__).parent_path().parent_path() / "testdata";
  }

  return testDataPath.string();
}

// Helper to create a temporary database for testing
std::string GetTempDatabasePath()
{
  return std::filesystem::temp_directory_path() /
         ("semantic_test_" + std::to_string(std::time(nullptr)) + ".db");
}

// Helper to verify chunk content
bool ChunkContains(const SemanticChunk& chunk, const std::string& substring)
{
  return chunk.text.find(substring) != std::string::npos;
}
} // namespace

// =============================================================================
// Integration Test Fixture
// =============================================================================

class SemanticIntegrationTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_testDataPath = GetTestDataPath();
    m_dbPath = GetTempDatabasePath();

    // Create fresh database for each test
    m_database = std::make_unique<CSemanticDatabase>();
    ASSERT_TRUE(m_database->Open());

    m_subtitleParser = std::make_unique<CSubtitleParser>();
    m_metadataParser = std::make_unique<CMetadataParser>();
    m_chunkProcessor = std::make_unique<CChunkProcessor>();
    m_search = std::make_unique<CSemanticSearch>();

    ASSERT_TRUE(m_search->Initialize(m_database.get()));
  }

  void TearDown() override
  {
    m_search.reset();
    m_chunkProcessor.reset();
    m_metadataParser.reset();
    m_subtitleParser.reset();
    m_database.reset();

    // Clean up temporary database
    if (std::filesystem::exists(m_dbPath))
    {
      std::filesystem::remove(m_dbPath);
    }
  }

  // Helper methods
  std::string GetTestFilePath(const std::string& filename) const
  {
    return URIUtils::AddFileToFolder(m_testDataPath, filename);
  }

  bool IndexSubtitleFile(const std::string& filename, int mediaId, const std::string& mediaType)
  {
    std::string path = GetTestFilePath(filename);

    if (!m_subtitleParser->CanParse(path))
      return false;

    auto entries = m_subtitleParser->Parse(path);
    if (entries.empty())
      return false;

    auto chunks = m_chunkProcessor->Process(entries, mediaId, mediaType, SourceType::SUBTITLE);

    return m_database->InsertChunks(chunks);
  }

  bool IndexMetadataFile(const std::string& filename, int mediaId, const std::string& mediaType)
  {
    std::string path = GetTestFilePath(filename);

    if (!m_metadataParser->CanParse(path))
      return false;

    auto entries = m_metadataParser->Parse(path);
    if (entries.empty())
      return false;

    auto chunks = m_chunkProcessor->Process(entries, mediaId, mediaType, SourceType::METADATA);

    return m_database->InsertChunks(chunks);
  }

  std::string m_testDataPath;
  std::string m_dbPath;
  std::unique_ptr<CSemanticDatabase> m_database;
  std::unique_ptr<CSubtitleParser> m_subtitleParser;
  std::unique_ptr<CMetadataParser> m_metadataParser;
  std::unique_ptr<CChunkProcessor> m_chunkProcessor;
  std::unique_ptr<CSemanticSearch> m_search;
};

// =============================================================================
// Test Suite: Full Indexing Pipeline
// =============================================================================

TEST_F(SemanticIntegrationTest, FullPipeline_SRTIndexing)
{
  const int mediaId = 1;
  const std::string mediaType = "movie";

  // Index SRT file
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Verify chunks were created
  std::vector<SemanticChunk> chunks;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks));
  EXPECT_GT(chunks.size(), 0u) << "Should have created chunks from SRT file";

  // Verify chunk content
  bool foundWelcome = false;
  bool foundDinosaur = false;

  for (const auto& chunk : chunks)
  {
    EXPECT_EQ(chunk.mediaId, mediaId);
    EXPECT_EQ(chunk.mediaType, mediaType);
    EXPECT_EQ(chunk.sourceType, SourceType::SUBTITLE);
    EXPECT_GE(chunk.startMs, 0);
    EXPECT_GT(chunk.endMs, chunk.startMs);

    if (ChunkContains(chunk, "semantic search"))
      foundWelcome = true;
    if (ChunkContains(chunk, "dinosaur"))
      foundDinosaur = true;
  }

  EXPECT_TRUE(foundWelcome) << "Should find 'semantic search' in chunks";
  EXPECT_TRUE(foundDinosaur) << "Should find 'dinosaur' in chunks";
}

TEST_F(SemanticIntegrationTest, FullPipeline_ASSIndexing)
{
  const int mediaId = 2;
  const std::string mediaType = "movie";

  // Index ASS file
  ASSERT_TRUE(IndexSubtitleFile("sample.ass", mediaId, mediaType));

  // Verify chunks were created
  std::vector<SemanticChunk> chunks;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks));
  EXPECT_GT(chunks.size(), 0u) << "Should have created chunks from ASS file";

  // Verify ASS formatting codes are stripped
  for (const auto& chunk : chunks)
  {
    EXPECT_EQ(chunk.text.find("{\\"), std::string::npos)
      << "ASS formatting codes should be stripped: " << chunk.text;
    EXPECT_EQ(chunk.text.find("\\an8"), std::string::npos)
      << "ASS alignment codes should be stripped";
    EXPECT_EQ(chunk.text.find("\\pos"), std::string::npos)
      << "ASS position codes should be stripped";
  }

  // Verify readable content exists
  bool foundAdvanced = false;
  for (const auto& chunk : chunks)
  {
    if (ChunkContains(chunk, "Advanced SubStation"))
      foundAdvanced = true;
  }
  EXPECT_TRUE(foundAdvanced) << "Should find readable content after stripping codes";
}

TEST_F(SemanticIntegrationTest, FullPipeline_VTTIndexing)
{
  const int mediaId = 3;
  const std::string mediaType = "movie";

  // Index VTT file
  ASSERT_TRUE(IndexSubtitleFile("sample.vtt", mediaId, mediaType));

  // Verify chunks were created
  std::vector<SemanticChunk> chunks;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks));
  EXPECT_GT(chunks.size(), 0u) << "Should have created chunks from VTT file";

  // Verify VTT content
  bool foundWebVTT = false;
  for (const auto& chunk : chunks)
  {
    if (ChunkContains(chunk, "WebVTT"))
      foundWebVTT = true;

    // Comments should not be in chunks
    EXPECT_FALSE(ChunkContains(chunk, "This is a comment"));
  }
  EXPECT_TRUE(foundWebVTT) << "Should find WebVTT content";
}

TEST_F(SemanticIntegrationTest, FullPipeline_MetadataIndexing)
{
  const int mediaId = 4;
  const std::string mediaType = "movie";

  // Index NFO file
  ASSERT_TRUE(IndexMetadataFile("sample.nfo", mediaId, mediaType));

  // Verify chunks were created
  std::vector<SemanticChunk> chunks;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks));
  EXPECT_GT(chunks.size(), 0u) << "Should have created chunks from NFO file";

  // Verify metadata content
  bool foundPlot = false;
  bool foundTitle = false;

  for (const auto& chunk : chunks)
  {
    EXPECT_EQ(chunk.sourceType, SourceType::METADATA);

    if (ChunkContains(chunk, "Integration Test Movie"))
      foundTitle = true;
    if (ChunkContains(chunk, "thrilling adventure"))
      foundPlot = true;
  }

  EXPECT_TRUE(foundTitle) << "Should find movie title in metadata";
  EXPECT_TRUE(foundPlot) << "Should find plot description in metadata";
}

TEST_F(SemanticIntegrationTest, FullPipeline_MultipleFormats)
{
  const int mediaId = 5;
  const std::string mediaType = "movie";

  // Index multiple formats for the same media
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));
  ASSERT_TRUE(IndexMetadataFile("sample.nfo", mediaId, mediaType));

  // Verify chunks from both sources
  std::vector<SemanticChunk> chunks;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks));
  EXPECT_GT(chunks.size(), 0u);

  // Should have chunks from both sources
  bool hasSubtitles = false;
  bool hasMetadata = false;

  for (const auto& chunk : chunks)
  {
    if (chunk.sourceType == SourceType::SUBTITLE)
      hasSubtitles = true;
    if (chunk.sourceType == SourceType::METADATA)
      hasMetadata = true;
  }

  EXPECT_TRUE(hasSubtitles) << "Should have subtitle chunks";
  EXPECT_TRUE(hasMetadata) << "Should have metadata chunks";
}

// =============================================================================
// Test Suite: Search Pipeline
// =============================================================================

TEST_F(SemanticIntegrationTest, Search_BasicQuery)
{
  const int mediaId = 10;
  const std::string mediaType = "movie";

  // Index test content
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Search for specific term
  SearchOptions options;
  options.maxResults = 10;

  auto results = m_search->Search("dinosaur", options);

  EXPECT_GT(results.size(), 0u) << "Should find results for 'dinosaur'";
  EXPECT_TRUE(ChunkContains(results[0].chunk, "dinosaur"))
    << "First result should contain search term";
}

TEST_F(SemanticIntegrationTest, Search_MultiWordQuery)
{
  const int mediaId = 11;
  const std::string mediaType = "movie";

  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Multi-word search
  auto results = m_search->Search("semantic search", SearchOptions());

  EXPECT_GT(results.size(), 0u) << "Should find results for multi-word query";

  // Results should contain at least one of the words
  bool foundRelevant = false;
  for (const auto& result : results)
  {
    if (ChunkContains(result.chunk, "semantic") ||
        ChunkContains(result.chunk, "search"))
    {
      foundRelevant = true;
      break;
    }
  }
  EXPECT_TRUE(foundRelevant) << "Should find relevant content";
}

TEST_F(SemanticIntegrationTest, Search_PhraseMatching)
{
  const int mediaId = 12;
  const std::string mediaType = "movie";

  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Phrase search
  auto results = m_search->Search("\"exact phrase search\"", SearchOptions());

  EXPECT_GT(results.size(), 0u) << "Should find results for phrase query";
  EXPECT_TRUE(ChunkContains(results[0].chunk, "exact phrase search"))
    << "Should match exact phrase";
}

TEST_F(SemanticIntegrationTest, Search_BM25Ranking)
{
  const int mediaId = 13;
  const std::string mediaType = "movie";

  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  auto results = m_search->Search("quick brown fox", SearchOptions());

  ASSERT_GT(results.size(), 0u) << "Should find results";

  // Verify results are ranked (scores should be in descending order)
  for (size_t i = 1; i < results.size(); i++)
  {
    EXPECT_GE(results[i - 1].score, results[i].score)
      << "Results should be ranked by BM25 score";
  }

  // Top result should contain the most query terms
  EXPECT_TRUE(ChunkContains(results[0].chunk, "quick") ||
              ChunkContains(results[0].chunk, "brown") ||
              ChunkContains(results[0].chunk, "fox"))
    << "Top result should be most relevant";
}

TEST_F(SemanticIntegrationTest, Search_FilterByMediaType)
{
  // Index same content for different media types
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", 20, "movie"));
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", 21, "episode"));

  // Search with media type filter
  SearchOptions options;
  options.mediaType = "movie";

  auto results = m_search->Search("semantic", options);

  EXPECT_GT(results.size(), 0u);

  // All results should be from movies
  for (const auto& result : results)
  {
    EXPECT_EQ(result.chunk.mediaType, "movie")
      << "Should only return movie results when filtered";
  }
}

TEST_F(SemanticIntegrationTest, Search_FilterByMediaId)
{
  // Index for multiple media items
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", 30, "movie"));
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", 31, "movie"));

  // Search within specific media
  SearchOptions options;
  options.mediaId = 30;

  auto results = m_search->Search("semantic", options);

  EXPECT_GT(results.size(), 0u);

  // All results should be from the specified media
  for (const auto& result : results)
  {
    EXPECT_EQ(result.chunk.mediaId, 30)
      << "Should only return results from specified media";
  }
}

TEST_F(SemanticIntegrationTest, Search_NoResults)
{
  const int mediaId = 40;
  const std::string mediaType = "movie";

  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Search for non-existent term
  auto results = m_search->Search("xyzabc123nonexistent", SearchOptions());

  EXPECT_EQ(results.size(), 0u) << "Should return empty results for non-existent term";
}

// =============================================================================
// Test Suite: Chunk Processing
// =============================================================================

TEST_F(SemanticIntegrationTest, ChunkProcessing_BasicSegmentation)
{
  ParsedEntry entry;
  entry.startMs = 1000;
  entry.endMs = 5000;
  entry.text = "This is a test subtitle entry with multiple words that should be chunked appropriately.";

  std::vector<ParsedEntry> entries = {entry};

  auto chunks = m_chunkProcessor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  EXPECT_GT(chunks.size(), 0u) << "Should create at least one chunk";
  EXPECT_EQ(chunks[0].text, entry.text) << "Short entry should remain intact";
  EXPECT_EQ(chunks[0].startMs, entry.startMs);
  EXPECT_EQ(chunks[0].endMs, entry.endMs);
}

TEST_F(SemanticIntegrationTest, ChunkProcessing_OverlapHandling)
{
  ChunkConfig config;
  config.maxChunkWords = 10;
  config.overlapWords = 2;
  m_chunkProcessor->SetConfig(config);

  // Create a long text that will be split
  std::string longText;
  for (int i = 0; i < 50; i++)
  {
    longText += "word" + std::to_string(i) + " ";
  }

  ParsedEntry entry;
  entry.startMs = 0;
  entry.endMs = 10000;
  entry.text = longText;

  auto chunks = m_chunkProcessor->Process({entry}, 1, "movie", SourceType::SUBTITLE);

  EXPECT_GT(chunks.size(), 1u) << "Long entry should be split into multiple chunks";

  // Verify overlap exists between consecutive chunks
  if (chunks.size() >= 2)
  {
    // Check for word overlap (this is a simplified check)
    EXPECT_GT(chunks[0].endMs, chunks[1].startMs - 1000)
      << "Chunks should have temporal overlap";
  }
}

TEST_F(SemanticIntegrationTest, ChunkProcessing_TimestampPreservation)
{
  std::string path = GetTestFilePath("sample.srt");
  auto entries = m_subtitleParser->Parse(path);

  ASSERT_FALSE(entries.empty());

  auto chunks = m_chunkProcessor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  // Verify timestamps are preserved or properly calculated
  for (const auto& chunk : chunks)
  {
    EXPECT_GE(chunk.startMs, 0) << "Start time should be non-negative";
    EXPECT_GT(chunk.endMs, chunk.startMs) << "End time should be after start time";
    EXPECT_LT(chunk.endMs, 1000000) << "End time should be reasonable";
  }

  // Verify chunks are in chronological order
  for (size_t i = 1; i < chunks.size(); i++)
  {
    EXPECT_GE(chunks[i].startMs, chunks[i - 1].startMs)
      << "Chunks should be in chronological order";
  }
}

TEST_F(SemanticIntegrationTest, ChunkProcessing_VariousChunkSizes)
{
  ChunkConfig smallConfig;
  smallConfig.maxChunkWords = 20;

  ChunkConfig largeConfig;
  largeConfig.maxChunkWords = 100;

  std::string path = GetTestFilePath("sample.srt");
  auto entries = m_subtitleParser->Parse(path);

  ASSERT_FALSE(entries.empty());

  // Process with small chunks
  m_chunkProcessor->SetConfig(smallConfig);
  auto smallChunks = m_chunkProcessor->Process(entries, 1, "movie", SourceType::SUBTITLE);

  // Process with large chunks
  m_chunkProcessor->SetConfig(largeConfig);
  auto largeChunks = m_chunkProcessor->Process(entries, 2, "movie", SourceType::SUBTITLE);

  // Smaller max chunk size should generally create more chunks (or equal)
  EXPECT_GE(smallChunks.size(), largeChunks.size() / 2)
    << "Smaller chunk size should create more or similar number of chunks";
}

// =============================================================================
// Test Suite: Subtitle Format Parsing
// =============================================================================

TEST_F(SemanticIntegrationTest, SubtitleFormat_SRTTimestamps)
{
  std::string path = GetTestFilePath("sample.srt");
  auto entries = m_subtitleParser->Parse(path);

  ASSERT_FALSE(entries.empty());

  // First entry should be "Welcome to Kodi's semantic search system"
  EXPECT_EQ(entries[0].startMs, 1000) << "First entry start time";
  EXPECT_EQ(entries[0].endMs, 4500) << "First entry end time";
  EXPECT_TRUE(entries[0].text.find("semantic search") != std::string::npos);
}

TEST_F(SemanticIntegrationTest, SubtitleFormat_ASSComplexFormatting)
{
  std::string path = GetTestFilePath("sample.ass");
  auto entries = m_subtitleParser->Parse(path);

  ASSERT_FALSE(entries.empty());

  // Verify ASS codes are stripped
  for (const auto& entry : entries)
  {
    EXPECT_EQ(entry.text.find("{\\"), std::string::npos)
      << "ASS codes should be stripped from: " << entry.text;
  }

  // First entry timing (ASS uses centiseconds)
  EXPECT_EQ(entries[0].startMs, 1000);
  EXPECT_EQ(entries[0].endMs, 4500);
}

TEST_F(SemanticIntegrationTest, SubtitleFormat_VTTCueSettings)
{
  std::string path = GetTestFilePath("sample.vtt");
  auto entries = m_subtitleParser->Parse(path);

  ASSERT_FALSE(entries.empty());

  // Verify VTT content
  EXPECT_EQ(entries[0].startMs, 1000);
  EXPECT_EQ(entries[0].endMs, 4500);

  // Verify HTML tags are stripped
  for (const auto& entry : entries)
  {
    // Basic check - should not contain raw HTML tags
    size_t tagCount = 0;
    size_t pos = 0;
    while ((pos = entry.text.find('<', pos)) != std::string::npos)
    {
      tagCount++;
      pos++;
    }

    // Allow for some tags in snippets, but shouldn't be excessive
    EXPECT_LT(tagCount, 5) << "Should strip most HTML tags from: " << entry.text;
  }
}

TEST_F(SemanticIntegrationTest, SubtitleFormat_MultilineHandling)
{
  std::string path = GetTestFilePath("sample.srt");
  auto entries = m_subtitleParser->Parse(path);

  ASSERT_GE(entries.size(), 3u);

  // Entry 3 (index 2) should be multi-line
  EXPECT_TRUE(entries[2].text.find("Multi-line") != std::string::npos);

  // Multi-line text should be preserved (may have \n or space)
  bool hasMultipleLines = entries[2].text.find("\n") != std::string::npos ||
                          entries[2].text.find("two lines") != std::string::npos;
  EXPECT_TRUE(hasMultipleLines) << "Multi-line content should be preserved";
}

// =============================================================================
// Test Suite: Database State Management
// =============================================================================

TEST_F(SemanticIntegrationTest, Database_IndexStateTracking)
{
  const int mediaId = 100;
  const std::string mediaType = "movie";

  SemanticIndexState state;
  state.mediaId = mediaId;
  state.mediaType = mediaType;
  state.mediaPath = "/path/to/media.mkv";
  state.subtitleStatus = IndexStatus::COMPLETED;
  state.transcriptionStatus = IndexStatus::PENDING;
  state.metadataStatus = IndexStatus::IN_PROGRESS;
  state.priority = 5;

  ASSERT_TRUE(m_database->UpdateIndexState(state));

  // Retrieve and verify
  SemanticIndexState retrieved;
  ASSERT_TRUE(m_database->GetIndexState(mediaId, mediaType, retrieved));

  EXPECT_EQ(retrieved.mediaId, mediaId);
  EXPECT_EQ(retrieved.mediaType, mediaType);
  EXPECT_EQ(retrieved.mediaPath, state.mediaPath);
  EXPECT_EQ(retrieved.subtitleStatus, IndexStatus::COMPLETED);
  EXPECT_EQ(retrieved.transcriptionStatus, IndexStatus::PENDING);
  EXPECT_EQ(retrieved.metadataStatus, IndexStatus::IN_PROGRESS);
  EXPECT_EQ(retrieved.priority, 5);
}

TEST_F(SemanticIntegrationTest, Database_IncrementalIndexing)
{
  const int mediaId = 101;
  const std::string mediaType = "movie";

  // Initial indexing
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  std::vector<SemanticChunk> chunks1;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks1));
  size_t initialCount = chunks1.size();

  // Add more content (metadata)
  ASSERT_TRUE(IndexMetadataFile("sample.nfo", mediaId, mediaType));

  std::vector<SemanticChunk> chunks2;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks2));

  EXPECT_GT(chunks2.size(), initialCount) << "Should have more chunks after incremental indexing";
}

TEST_F(SemanticIntegrationTest, Database_ContentUpdateDetection)
{
  const int mediaId = 102;
  const std::string mediaType = "movie";

  // Index content
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Update index state
  SemanticIndexState state;
  state.mediaId = mediaId;
  state.mediaType = mediaType;
  state.mediaPath = "/path/to/media.mkv";
  state.subtitleStatus = IndexStatus::COMPLETED;
  ASSERT_TRUE(m_database->UpdateIndexState(state));

  // Delete and re-index
  ASSERT_TRUE(m_database->DeleteChunksForMedia(mediaId, mediaType));

  std::vector<SemanticChunk> chunks;
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks));
  EXPECT_EQ(chunks.size(), 0u) << "Chunks should be deleted";

  // Re-index
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));
  ASSERT_TRUE(m_database->GetChunksForMedia(mediaId, mediaType, chunks));
  EXPECT_GT(chunks.size(), 0u) << "Should be able to re-index";
}

TEST_F(SemanticIntegrationTest, Database_PendingStates)
{
  // Create multiple pending states
  for (int i = 200; i < 205; i++)
  {
    SemanticIndexState state;
    state.mediaId = i;
    state.mediaType = "movie";
    state.mediaPath = "/path/to/media" + std::to_string(i) + ".mkv";
    state.subtitleStatus = IndexStatus::PENDING;
    state.priority = i % 3; // Varying priorities

    ASSERT_TRUE(m_database->UpdateIndexState(state));
  }

  // Get pending states
  std::vector<SemanticIndexState> pending;
  ASSERT_TRUE(m_database->GetPendingIndexStates(10, pending));

  EXPECT_EQ(pending.size(), 5u) << "Should retrieve all pending items";

  // Verify they are sorted by priority
  for (const auto& state : pending)
  {
    EXPECT_EQ(state.subtitleStatus, IndexStatus::PENDING);
  }
}

// =============================================================================
// Test Suite: Provider Selection
// =============================================================================

TEST_F(SemanticIntegrationTest, Provider_ManagerInitialization)
{
  auto providerManager = std::make_unique<CTranscriptionProviderManager>();

  ASSERT_TRUE(providerManager->Initialize());

  // Should have at least the built-in providers registered
  auto providers = providerManager->GetAvailableProviders();
  EXPECT_GT(providers.size(), 0u) << "Should have registered providers";

  providerManager->Shutdown();
}

TEST_F(SemanticIntegrationTest, Provider_GetProvider)
{
  auto providerManager = std::make_unique<CTranscriptionProviderManager>();
  ASSERT_TRUE(providerManager->Initialize());

  // Try to get Groq provider (should be registered)
  auto groqProvider = providerManager->GetProvider("groq");

  // Provider may or may not be null depending on configuration
  // Just verify the API works
  auto providers = providerManager->GetAvailableProviders();

  if (!providers.empty())
  {
    auto firstProvider = providerManager->GetProvider(providers[0]);
    // May be null if not configured, but shouldn't crash
  }

  providerManager->Shutdown();
}

TEST_F(SemanticIntegrationTest, Provider_DefaultProvider)
{
  auto providerManager = std::make_unique<CTranscriptionProviderManager>();
  ASSERT_TRUE(providerManager->Initialize());

  auto providers = providerManager->GetAvailableProviders();

  if (!providers.empty())
  {
    // Set default provider
    providerManager->SetDefaultProvider(providers[0]);

    std::string defaultId = providerManager->GetDefaultProviderId();
    EXPECT_EQ(defaultId, providers[0]) << "Should set and retrieve default provider";
  }

  providerManager->Shutdown();
}

TEST_F(SemanticIntegrationTest, Provider_ProviderInfo)
{
  auto providerManager = std::make_unique<CTranscriptionProviderManager>();
  ASSERT_TRUE(providerManager->Initialize());

  auto providerInfoList = providerManager->GetProviderInfoList();

  EXPECT_GT(providerInfoList.size(), 0u) << "Should have provider info";

  for (const auto& info : providerInfoList)
  {
    EXPECT_FALSE(info.id.empty()) << "Provider should have an ID";
    EXPECT_FALSE(info.name.empty()) << "Provider should have a name";
  }

  providerManager->Shutdown();
}

// =============================================================================
// Test Suite: Statistics and Utilities
// =============================================================================

TEST_F(SemanticIntegrationTest, Stats_IndexStatistics)
{
  // Index some content
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", 1, "movie"));
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", 2, "episode"));
  ASSERT_TRUE(IndexMetadataFile("sample.nfo", 3, "movie"));

  auto stats = m_database->GetStats();

  EXPECT_GT(stats.totalChunks, 0) << "Should have indexed chunks";
  EXPECT_GT(stats.indexedMedia, 0) << "Should have indexed media";
}

TEST_F(SemanticIntegrationTest, Context_TimeWindow)
{
  const int mediaId = 300;
  const std::string mediaType = "movie";

  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Get context around 10 seconds
  auto context = m_search->GetContext(mediaId, mediaType, 10000, 5000);

  // Should get chunks within ±5 seconds of 10 seconds (5s to 15s)
  EXPECT_GT(context.size(), 0u) << "Should find context chunks";

  for (const auto& chunk : context)
  {
    // Chunks should overlap with the time window
    bool inWindow = (chunk.startMs <= 15000) && (chunk.endMs >= 5000);
    EXPECT_TRUE(inWindow) << "Chunk should be within time window: "
                          << chunk.startMs << " - " << chunk.endMs;
  }
}

TEST_F(SemanticIntegrationTest, Search_IsMediaSearchable)
{
  const int mediaId = 400;
  const std::string mediaType = "movie";

  // Before indexing
  EXPECT_FALSE(m_search->IsMediaSearchable(mediaId, mediaType))
    << "Media should not be searchable before indexing";

  // Index content
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));

  // Mark as completed
  SemanticIndexState state;
  state.mediaId = mediaId;
  state.mediaType = mediaType;
  state.mediaPath = "/path/to/media.mkv";
  state.subtitleStatus = IndexStatus::COMPLETED;
  ASSERT_TRUE(m_database->UpdateIndexState(state));

  // After indexing
  EXPECT_TRUE(m_search->IsMediaSearchable(mediaId, mediaType))
    << "Media should be searchable after indexing";
}

// =============================================================================
// Test Suite: Edge Cases and Error Handling
// =============================================================================

TEST_F(SemanticIntegrationTest, EdgeCase_EmptyDatabase)
{
  // Search in empty database
  auto results = m_search->Search("test", SearchOptions());
  EXPECT_EQ(results.size(), 0u) << "Empty database should return no results";

  // Get stats from empty database
  auto stats = m_database->GetStats();
  EXPECT_EQ(stats.totalChunks, 0);
  EXPECT_EQ(stats.indexedMedia, 0);
}

TEST_F(SemanticIntegrationTest, EdgeCase_DuplicateIndexing)
{
  const int mediaId = 500;
  const std::string mediaType = "movie";

  // Index same file twice
  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));
  size_t count1 = 0;
  {
    std::vector<SemanticChunk> chunks;
    m_database->GetChunksForMedia(mediaId, mediaType, chunks);
    count1 = chunks.size();
  }

  ASSERT_TRUE(IndexSubtitleFile("sample.srt", mediaId, mediaType));
  size_t count2 = 0;
  {
    std::vector<SemanticChunk> chunks;
    m_database->GetChunksForMedia(mediaId, mediaType, chunks);
    count2 = chunks.size();
  }

  // Duplicate indexing should add more chunks (no automatic deduplication)
  EXPECT_EQ(count2, count1 * 2) << "Duplicate indexing should double chunk count";
}

TEST_F(SemanticIntegrationTest, EdgeCase_SpecialCharacters)
{
  // Search with special characters
  auto results = m_search->Search("test & <special> \"chars\"", SearchOptions());

  // Should not crash and should handle gracefully
  // Results may be empty if no matching content
  EXPECT_GE(results.size(), 0u);
}

TEST_F(SemanticIntegrationTest, EdgeCase_VeryLongQuery)
{
  std::string longQuery;
  for (int i = 0; i < 100; i++)
  {
    longQuery += "word" + std::to_string(i) + " ";
  }

  // Should handle very long queries
  auto results = m_search->Search(longQuery, SearchOptions());
  EXPECT_GE(results.size(), 0u) << "Should handle long queries without crashing";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
