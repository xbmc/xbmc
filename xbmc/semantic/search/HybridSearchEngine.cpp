/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HybridSearchEngine.h"

#include "SemanticSearch.h"
#include "VectorSearcher.h"
#include "semantic/SemanticDatabase.h"
#include "semantic/embedding/EmbeddingEngine.h"
#include "semantic/perf/PerformanceMonitor.h"
#include "semantic/perf/QueryCache.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <future>
#include <sstream>
#include <unordered_map>

using namespace KODI::SEMANTIC;

CHybridSearchEngine::CHybridSearchEngine() = default;

CHybridSearchEngine::~CHybridSearchEngine() = default;

bool CHybridSearchEngine::Initialize(CSemanticDatabase* database,
                                     CEmbeddingEngine* embeddingEngine,
                                     CVectorSearcher* vectorSearcher,
                                     bool enableCache)
{
  if (database == nullptr)
  {
    CLog::LogF(LOGERROR, "Cannot initialize with null database pointer");
    return false;
  }

  if (embeddingEngine == nullptr)
  {
    CLog::LogF(LOGERROR, "Cannot initialize with null embedding engine pointer");
    return false;
  }

  if (vectorSearcher == nullptr)
  {
    CLog::LogF(LOGERROR, "Cannot initialize with null vector searcher pointer");
    return false;
  }

  m_database = database;
  m_embeddingEngine = embeddingEngine;
  m_vectorSearcher = vectorSearcher;
  m_cacheEnabled = enableCache;

  // Initialize keyword search component
  m_keywordSearch = std::make_unique<CSemanticSearch>();
  if (!m_keywordSearch->Initialize(database))
  {
    CLog::LogF(LOGERROR, "Failed to initialize keyword search component");
    return false;
  }

  // Initialize query cache if enabled
  if (enableCache)
  {
    m_cache = std::make_unique<CQueryCache>();
    m_cache->Initialize(50, 300, 3600); // 50MB cache, 5min TTL for results, 1hr for embeddings
    CLog::LogF(LOGDEBUG, "HybridSearchEngine: Query cache enabled");
  }

  CLog::LogF(LOGDEBUG, "HybridSearchEngine initialized successfully");
  return true;
}

bool CHybridSearchEngine::IsInitialized() const
{
  return m_database != nullptr && m_embeddingEngine != nullptr && m_vectorSearcher != nullptr &&
         m_keywordSearch != nullptr;
}

std::vector<HybridSearchResult> CHybridSearchEngine::Search(const std::string& query,
                                                            const HybridSearchOptions& options)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "Search called before initialization");
    return {};
  }

  if (query.empty())
  {
    CLog::LogF(LOGDEBUG, "Empty search query provided");
    return {};
  }

  CLog::LogF(LOGDEBUG, "Search query='{}' mode={}", query, static_cast<int>(options.mode));

  // Route to appropriate search implementation based on mode
  switch (options.mode)
  {
    case SearchMode::KeywordOnly:
      return SearchKeywordOnly(query, options);
    case SearchMode::SemanticOnly:
      return SearchSemanticOnly(query, options);
    case SearchMode::Hybrid:
    default:
      return SearchHybrid(query, options);
  }
}

std::vector<HybridSearchResult> CHybridSearchEngine::SearchKeywordOnly(
    const std::string& query,
    const HybridSearchOptions& options)
{
  CLog::LogF(LOGDEBUG, "Performing keyword-only search");

  // Configure FTS5 search options
  SearchOptions ftsOptions;
  ftsOptions.maxResults = options.maxResults;
  ftsOptions.mediaType = options.mediaType;
  ftsOptions.mediaId = options.mediaId;
  ftsOptions.minConfidence = options.minConfidence;

  // Execute keyword search
  auto keywordResults = m_keywordSearch->Search(query, ftsOptions);

  // Convert to hybrid results
  std::vector<HybridSearchResult> results;
  results.reserve(keywordResults.size());

  for (const auto& result : keywordResults)
  {
    HybridSearchResult hybridResult;
    hybridResult.chunkId = result.chunk.chunkId;
    hybridResult.chunk = result.chunk;
    hybridResult.keywordScore = result.score;
    hybridResult.vectorScore = 0.0f;
    hybridResult.combinedScore = result.score;
    hybridResult.snippet = result.snippet;
    hybridResult.formattedTimestamp = FormatTimestamp(result.chunk.startMs);

    results.push_back(std::move(hybridResult));
  }

  CLog::LogF(LOGDEBUG, "Keyword search returned {} results before filtering", results.size());

  // Apply extended filters (year, rating, duration, source type)
  results = ApplyExtendedFilters(results, options);

  CLog::LogF(LOGDEBUG, "Keyword search returned {} results after filtering", results.size());
  return results;
}

std::vector<HybridSearchResult> CHybridSearchEngine::SearchSemanticOnly(
    const std::string& query,
    const HybridSearchOptions& options)
{
  CLog::LogF(LOGDEBUG, "Performing semantic-only search");

  // Generate query embedding
  auto queryEmbedding = m_embeddingEngine->Embed(query);

  // Check if embedding generation succeeded
  bool embeddingValid = false;
  for (size_t i = 0; i < queryEmbedding.size(); ++i)
  {
    if (queryEmbedding[i] != 0.0f)
    {
      embeddingValid = true;
      break;
    }
  }

  if (!embeddingValid)
  {
    CLog::LogF(LOGWARNING, "Embedding generation failed for query: {}", query);
    return {};
  }

  // Execute vector search
  std::vector<CVectorSearcher::VectorResult> vectorResults;
  if (!options.mediaType.empty())
  {
    vectorResults =
        m_vectorSearcher->SearchSimilarByMediaType(queryEmbedding, options.mediaType,
                                                   options.maxResults);
  }
  else
  {
    vectorResults = m_vectorSearcher->SearchSimilar(queryEmbedding, options.maxResults);
  }

  // Convert to hybrid results
  std::vector<HybridSearchResult> results;
  results.reserve(vectorResults.size());

  for (const auto& result : vectorResults)
  {
    float similarity = CosineDistanceToSimilarity(result.distance);

    // Apply confidence threshold
    if (similarity < options.minConfidence)
      continue;

    results.push_back(EnrichResult(result.chunkId, 0.0f, similarity));
  }

  CLog::LogF(LOGDEBUG, "Semantic search returned {} results before filtering", results.size());

  // Apply extended filters (year, rating, duration, source type)
  results = ApplyExtendedFilters(results, options);

  CLog::LogF(LOGDEBUG, "Semantic search returned {} results after filtering", results.size());
  return results;
}

std::vector<HybridSearchResult> CHybridSearchEngine::SearchHybrid(
    const std::string& query,
    const HybridSearchOptions& options)
{
  CLog::LogF(LOGDEBUG, "Performing hybrid search");

  // Step 1: Generate query embedding
  auto queryEmbedding = m_embeddingEngine->Embed(query);

  // Check if embedding generation succeeded
  bool embeddingValid = false;
  for (size_t i = 0; i < queryEmbedding.size(); ++i)
  {
    if (queryEmbedding[i] != 0.0f)
    {
      embeddingValid = true;
      break;
    }
  }

  if (!embeddingValid)
  {
    CLog::LogF(LOGWARNING, "Embedding failed, falling back to keyword-only search");
    return SearchKeywordOnly(query, options);
  }

  // Step 2: Execute keyword search (FTS5)
  SearchOptions ftsOptions;
  ftsOptions.maxResults = options.keywordTopK;
  ftsOptions.mediaType = options.mediaType;
  ftsOptions.mediaId = options.mediaId;
  ftsOptions.minConfidence = options.minConfidence;

  auto keywordResults = m_keywordSearch->Search(query, ftsOptions);
  CLog::LogF(LOGDEBUG, "Keyword search returned {} candidates", keywordResults.size());

  // Step 3: Execute vector search
  std::vector<CVectorSearcher::VectorResult> vectorResults;
  if (!options.mediaType.empty())
  {
    vectorResults =
        m_vectorSearcher->SearchSimilarByMediaType(queryEmbedding, options.mediaType,
                                                   options.vectorTopK);
  }
  else
  {
    vectorResults = m_vectorSearcher->SearchSimilar(queryEmbedding, options.vectorTopK);
  }
  CLog::LogF(LOGDEBUG, "Vector search returned {} candidates", vectorResults.size());

  // Step 4: Combine results using Reciprocal Rank Fusion
  return CombineResultsRRF(keywordResults, vectorResults, options);
}

std::vector<HybridSearchResult> CHybridSearchEngine::CombineResultsRRF(
    const std::vector<SearchResult>& keywordResults,
    const std::vector<CVectorSearcher::VectorResult>& vectorResults,
    const HybridSearchOptions& options)
{
  // RRF constant (standard value from literature)
  const float k = 60.0f;

  // Map chunk_id -> (keywordRRF, vectorRRF)
  std::unordered_map<int64_t, std::pair<float, float>> scores;

  // Process keyword results - add RRF score based on rank
  for (size_t i = 0; i < keywordResults.size(); ++i)
  {
    int64_t chunkId = keywordResults[i].chunk.chunkId;
    float rrfScore = options.keywordWeight / (k + static_cast<float>(i) + 1.0f);
    scores[chunkId].first = rrfScore;
  }

  CLog::LogF(LOGDEBUG, "Processed {} keyword results for RRF", keywordResults.size());

  // Process vector results - add RRF score based on rank
  for (size_t i = 0; i < vectorResults.size(); ++i)
  {
    int64_t chunkId = vectorResults[i].chunkId;
    float rrfScore = options.vectorWeight / (k + static_cast<float>(i) + 1.0f);
    scores[chunkId].second = rrfScore;
  }

  CLog::LogF(LOGDEBUG, "Processed {} vector results for RRF", vectorResults.size());

  // Combine scores and prepare for sorting
  std::vector<std::tuple<int64_t, float, float, float>> combined;
  combined.reserve(scores.size());

  for (const auto& [chunkId, scorePair] : scores)
  {
    float totalScore = scorePair.first + scorePair.second;
    combined.emplace_back(chunkId, totalScore, scorePair.first, scorePair.second);
  }

  // Sort by total RRF score (descending)
  std::sort(combined.begin(), combined.end(), [](const auto& a, const auto& b) {
    return std::get<1>(a) > std::get<1>(b);
  });

  CLog::LogF(LOGDEBUG, "Combined {} unique chunks, sorting by RRF score", combined.size());

  // Build final results with enriched data
  std::vector<HybridSearchResult> results;
  results.reserve(std::min(static_cast<size_t>(options.maxResults), combined.size()));

  int count = 0;
  for (const auto& [chunkId, totalScore, kwScore, vecScore] : combined)
  {
    if (count >= options.maxResults)
      break;

    results.push_back(EnrichResult(chunkId, kwScore, vecScore));
    count++;
  }

  CLog::LogF(LOGDEBUG, "Hybrid search returned {} final results before filtering", results.size());

  // Apply extended filters (year, rating, duration, source type)
  results = ApplyExtendedFilters(results, options);

  CLog::LogF(LOGDEBUG, "Hybrid search returned {} final results after filtering", results.size());
  return results;
}

std::vector<HybridSearchResult> CHybridSearchEngine::FindSimilar(int64_t chunkId, int topK)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "FindSimilar called before initialization");
    return {};
  }

  CLog::LogF(LOGDEBUG, "Finding similar chunks to chunkId={}", chunkId);

  // Use vector searcher's FindSimilar method
  auto vectorResults = m_vectorSearcher->FindSimilar(chunkId, topK);

  // Convert to hybrid results
  std::vector<HybridSearchResult> results;
  results.reserve(vectorResults.size());

  for (const auto& result : vectorResults)
  {
    float similarity = CosineDistanceToSimilarity(result.distance);
    results.push_back(EnrichResult(result.chunkId, 0.0f, similarity));
  }

  CLog::LogF(LOGDEBUG, "Found {} similar chunks", results.size());
  return results;
}

std::vector<SemanticChunk> CHybridSearchEngine::GetResultContext(const HybridSearchResult& result,
                                                                 int64_t windowMs)
{
  if (!IsInitialized())
  {
    CLog::LogF(LOGERROR, "GetResultContext called before initialization");
    return {};
  }

  // Use the high-level search API's GetContext method
  return m_keywordSearch->GetContext(result.chunk.mediaId, result.chunk.mediaType,
                                     result.chunk.startMs, windowMs);
}

HybridSearchResult CHybridSearchEngine::EnrichResult(int64_t chunkId,
                                                     float keywordScore,
                                                     float vectorScore)
{
  HybridSearchResult result;
  result.chunkId = chunkId;
  result.keywordScore = keywordScore;
  result.vectorScore = vectorScore;
  result.combinedScore = keywordScore + vectorScore;

  // Load full chunk data from database
  SemanticChunk chunk;
  if (m_database->GetChunk(chunkId, chunk))
  {
    result.chunk = chunk;
    result.formattedTimestamp = FormatTimestamp(chunk.startMs);

    // Generate snippet (first 100 chars + ellipsis if needed)
    if (chunk.text.length() > 100)
    {
      result.snippet = chunk.text.substr(0, 100) + "...";
    }
    else
    {
      result.snippet = chunk.text;
    }
  }
  else
  {
    CLog::LogF(LOGWARNING, "Failed to load chunk data for chunkId={}", chunkId);
  }

  return result;
}

std::string CHybridSearchEngine::FormatTimestamp(int64_t ms)
{
  int hours = static_cast<int>(ms / 3600000);
  int minutes = static_cast<int>((ms % 3600000) / 60000);
  int seconds = static_cast<int>((ms % 60000) / 1000);

  if (hours > 0)
  {
    return StringUtils::Format("{}:{:02d}:{:02d}", hours, minutes, seconds);
  }
  else
  {
    return StringUtils::Format("{}:{:02d}", minutes, seconds);
  }
}

float CHybridSearchEngine::NormalizeBM25Score(float score, float maxScore)
{
  // Normalize BM25 score to 0-1 range
  if (maxScore <= 0.0f)
    return 0.0f;

  return score / maxScore;
}

float CHybridSearchEngine::CosineDistanceToSimilarity(float distance)
{
  // Convert cosine distance (0 = identical, 2 = opposite)
  // to similarity score (1 = identical, 0 = opposite)
  return 1.0f - (distance / 2.0f);
}

std::vector<HybridSearchResult> CHybridSearchEngine::ApplyExtendedFilters(
    const std::vector<HybridSearchResult>& results,
    const HybridSearchOptions& options)
{
  std::vector<HybridSearchResult> filtered;
  filtered.reserve(results.size());

  for (const auto& result : results)
  {
    // Apply source type filter
    if (!PassesSourceFilter(result.chunk, options))
    {
      CLog::LogF(LOGDEBUG, "Result chunkId={} filtered by source type", result.chunkId);
      continue;
    }

    // Apply genre filter (if genres specified)
    // NOTE: This requires querying video database for media metadata
    // TODO: Implement genre filtering by querying CVideoDatabase::GetMovieInfo or GetEpisodeInfo
    if (!options.genres.empty())
    {
      // For now, accept all results as we don't have genre data in chunks
      // In a full implementation, we would:
      // 1. Query video database for media_id's genre
      // 2. Check if any of the media's genres match options.genres
      CLog::LogF(LOGDEBUG, "Genre filter active but not yet implemented for chunkId={}",
                 result.chunkId);
    }

    // Apply year filter (if year range specified)
    // NOTE: This requires querying video database for media metadata
    // TODO: Implement year filtering by querying CVideoDatabase
    if (options.minYear > 0 || options.maxYear > 0)
    {
      // For now, accept all results as we don't have year data in chunks
      // In a full implementation, we would:
      // 1. Query video database for media_id's year
      // 2. Check if year falls within minYear/maxYear range
      CLog::LogF(LOGDEBUG, "Year filter active but not yet implemented for chunkId={}",
                 result.chunkId);
    }

    // Apply MPAA rating filter (if rating specified)
    // NOTE: This requires querying video database for media metadata
    // TODO: Implement rating filtering by querying CVideoDatabase
    if (!options.mpaaRating.empty() && options.mpaaRating != "All Ratings")
    {
      // For now, accept all results as we don't have rating data in chunks
      // In a full implementation, we would:
      // 1. Query video database for media_id's MPAA rating
      // 2. Check if rating matches options.mpaaRating
      CLog::LogF(LOGDEBUG, "Rating filter active but not yet implemented for chunkId={}",
                 result.chunkId);
    }

    // Apply duration filter (if duration range specified)
    // NOTE: This requires querying video database for media metadata
    // TODO: Implement duration filtering by querying CVideoDatabase
    if (options.minDurationMinutes > 0 || options.maxDurationMinutes > 0)
    {
      // For now, accept all results as we don't have duration data in chunks
      // In a full implementation, we would:
      // 1. Query video database for media_id's runtime/duration
      // 2. Check if duration falls within minDurationMinutes/maxDurationMinutes range
      CLog::LogF(LOGDEBUG, "Duration filter active but not yet implemented for chunkId={}",
                 result.chunkId);
    }

    // Result passed all filters
    filtered.push_back(result);
  }

  CLog::LogF(LOGDEBUG, "Applied extended filters: {} -> {} results", results.size(),
             filtered.size());

  return filtered;
}

bool CHybridSearchEngine::PassesSourceFilter(const SemanticChunk& chunk,
                                              const HybridSearchOptions& options)
{
  // Check if all source types are enabled (no filtering)
  if (options.includeSubtitles && options.includeTranscription && options.includeMetadata)
  {
    return true;
  }

  // Check if the chunk's source type is enabled
  switch (chunk.sourceType)
  {
    case SourceType::SUBTITLE:
      return options.includeSubtitles;
    case SourceType::TRANSCRIPTION:
      return options.includeTranscription;
    case SourceType::METADATA:
      return options.includeMetadata;
    default:
      return true; // Unknown source types are allowed by default
  }
}
