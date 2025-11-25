/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SemanticOperations.h"

#include "ServiceBroker.h"
#include "media/MediaType.h"
#include "semantic/SemanticDatabase.h"
#include "semantic/SemanticTypes.h"
#include "semantic/search/SemanticSearch.h"
#include "semantic/transcription/TranscriptionProviderManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"

#include <chrono>

using namespace JSONRPC;
using namespace KODI::SEMANTIC;

namespace
{
/*!
 * \brief Format milliseconds to HH:MM:SS.mmm
 */
std::string FormatTimestamp(int64_t milliseconds)
{
  int64_t ms = milliseconds % 1000;
  int64_t seconds = (milliseconds / 1000) % 60;
  int64_t minutes = (milliseconds / 60000) % 60;
  int64_t hours = milliseconds / 3600000;

  return StringUtils::Format("%02d:%02d:%02d.%03d", static_cast<int>(hours),
                             static_cast<int>(minutes), static_cast<int>(seconds),
                             static_cast<int>(ms));
}

/*!
 * \brief Get semantic database instance
 */
CSemanticDatabase* GetSemanticDatabase()
{
  // TODO: Add to ServiceBroker once semantic services are integrated
  static CSemanticDatabase database;
  static bool initialized = false;

  if (!initialized)
  {
    if (database.Open())
      initialized = true;
    else
      return nullptr;
  }

  return &database;
}

/*!
 * \brief Get semantic search instance
 */
CSemanticSearch* GetSemanticSearch()
{
  static CSemanticSearch search;
  static bool initialized = false;

  if (!initialized)
  {
    auto* database = GetSemanticDatabase();
    if (database && search.Initialize(database))
      initialized = true;
    else
      return nullptr;
  }

  return &search;
}

/*!
 * \brief Get transcription provider manager instance
 */
CTranscriptionProviderManager* GetProviderManager()
{
  static CTranscriptionProviderManager manager;
  static bool initialized = false;

  if (!initialized)
  {
    if (manager.Initialize())
      initialized = true;
    else
      return nullptr;
  }

  return &manager;
}

} // anonymous namespace

JSONRPC_STATUS CSemanticOperations::Search(const std::string& method,
                                           ITransportLayer* transport,
                                           IClient* client,
                                           const CVariant& parameterObject,
                                           CVariant& result)
{
  // Validate required parameters
  if (!parameterObject.isMember("query") || parameterObject["query"].asString().empty())
  {
    return InvalidParams;
  }

  std::string query = parameterObject["query"].asString();

  // Parse options
  SearchOptions options;

  if (parameterObject.isMember("options"))
  {
    const CVariant& opts = parameterObject["options"];

    if (opts.isMember("limit"))
      options.maxResults = static_cast<int>(opts["limit"].asInteger());

    if (opts.isMember("media_type") && !opts["media_type"].asString().empty())
    {
      options.mediaType = opts["media_type"].asString();
    }

    if (opts.isMember("media_id"))
    {
      options.mediaId = static_cast<int>(opts["media_id"].asInteger());
    }

    if (opts.isMember("source_type"))
    {
      std::string sourceStr = opts["source_type"].asString();
      options.sourceType = StringToSourceType(sourceStr);
      options.filterBySource = true;
    }

    if (opts.isMember("min_confidence"))
    {
      options.minConfidence = static_cast<float>(opts["min_confidence"].asDouble());
    }
  }

  // Get search service
  auto* searchEngine = GetSemanticSearch();
  if (!searchEngine || !searchEngine->IsInitialized())
  {
    return FailedToExecute;
  }

  // Execute search
  auto start = std::chrono::high_resolution_clock::now();
  auto searchResults = searchEngine->Search(query, options);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Build result
  CVariant results = CVariant(CVariant::VariantTypeArray);
  for (const auto& sr : searchResults)
  {
    CVariant item;
    item["chunk_id"] = sr.chunk.chunkId;
    item["media_id"] = sr.chunk.mediaId;
    item["media_type"] = sr.chunk.mediaType;
    item["text"] = sr.chunk.text;
    item["snippet"] = sr.snippet;

    if (sr.chunk.startMs > 0)
    {
      item["start_ms"] = sr.chunk.startMs;
      item["end_ms"] = sr.chunk.endMs;
      item["timestamp"] = FormatTimestamp(sr.chunk.startMs);
    }

    item["source"] = SourceTypeToString(sr.chunk.sourceType);
    item["confidence"] = static_cast<double>(sr.chunk.confidence);
    item["score"] = static_cast<double>(sr.score);

    results.push_back(item);
  }

  result["results"] = results;
  result["total_results"] = static_cast<int>(searchResults.size());
  result["query_time_ms"] = static_cast<int>(duration.count());

  return OK;
}

JSONRPC_STATUS CSemanticOperations::GetContext(const std::string& method,
                                               ITransportLayer* transport,
                                               IClient* client,
                                               const CVariant& parameterObject,
                                               CVariant& result)
{
  // Validate parameters
  if (!parameterObject.isMember("media_id") || !parameterObject.isMember("media_type") ||
      !parameterObject.isMember("timestamp_ms"))
  {
    return InvalidParams;
  }

  int mediaId = static_cast<int>(parameterObject["media_id"].asInteger());
  std::string mediaType = parameterObject["media_type"].asString();
  int64_t timestampMs = parameterObject["timestamp_ms"].asInteger64();

  // Optional window size (default 60 seconds)
  int64_t windowMs = 60000;
  if (parameterObject.isMember("window_ms"))
  {
    windowMs = parameterObject["window_ms"].asInteger64();
  }

  auto* searchEngine = GetSemanticSearch();
  if (!searchEngine || !searchEngine->IsInitialized())
  {
    return FailedToExecute;
  }

  // Get context chunks
  auto chunks = searchEngine->GetContext(mediaId, mediaType, timestampMs, windowMs);

  // Build result
  CVariant contextChunks = CVariant(CVariant::VariantTypeArray);
  for (const auto& chunk : chunks)
  {
    CVariant item;
    item["chunk_id"] = chunk.chunkId;
    item["text"] = chunk.text;
    item["start_ms"] = chunk.startMs;
    item["end_ms"] = chunk.endMs;
    item["timestamp"] = FormatTimestamp(chunk.startMs);
    item["source"] = SourceTypeToString(chunk.sourceType);
    item["confidence"] = static_cast<double>(chunk.confidence);

    contextChunks.push_back(item);
  }

  result["chunks"] = contextChunks;
  result["media_id"] = mediaId;
  result["media_type"] = mediaType;
  result["center_timestamp_ms"] = timestampMs;
  result["window_ms"] = windowMs;

  return OK;
}

JSONRPC_STATUS CSemanticOperations::FindSimilar(const std::string& method,
                                                ITransportLayer* transport,
                                                IClient* client,
                                                const CVariant& parameterObject,
                                                CVariant& result)
{
  // This requires vector search which is part of Wave 1
  // For now, return a not implemented status
  // TODO: Implement once embedding generation is available

  result["error"] = "Vector similarity search not yet implemented. This feature requires "
                    "embedding generation (Wave 1).";
  return MethodNotFound;
}

JSONRPC_STATUS CSemanticOperations::GetIndexState(const std::string& method,
                                                  ITransportLayer* transport,
                                                  IClient* client,
                                                  const CVariant& parameterObject,
                                                  CVariant& result)
{
  if (!parameterObject.isMember("media_id") || !parameterObject.isMember("media_type"))
  {
    return InvalidParams;
  }

  int mediaId = static_cast<int>(parameterObject["media_id"].asInteger());
  std::string mediaType = parameterObject["media_type"].asString();

  auto* database = GetSemanticDatabase();
  if (!database)
    return FailedToExecute;

  SemanticIndexState state;
  if (!database->GetIndexState(mediaId, MediaTypeFromString(mediaType), state))
  {
    return InvalidParams;
  }

  result["media_id"] = state.mediaId;
  result["media_type"] = state.mediaType;
  result["path"] = state.mediaPath;

  result["subtitle_status"] = IndexStatusToString(state.subtitleStatus);
  result["transcription_status"] = IndexStatusToString(state.transcriptionStatus);
  result["transcription_provider"] = state.transcriptionProvider;
  result["transcription_progress"] = static_cast<double>(state.transcriptionProgress);
  result["metadata_status"] = IndexStatusToString(state.metadataStatus);
  result["embedding_status"] = IndexStatusToString(state.embeddingStatus);
  result["embedding_progress"] = static_cast<double>(state.embeddingProgress);

  result["is_searchable"] = (state.subtitleStatus == IndexStatus::COMPLETED ||
                             state.transcriptionStatus == IndexStatus::COMPLETED);
  result["chunk_count"] = state.chunkCount;
  result["embeddings_count"] = state.embeddingsCount;
  result["priority"] = state.priority;

  return OK;
}

JSONRPC_STATUS CSemanticOperations::QueueIndex(const std::string& method,
                                               ITransportLayer* transport,
                                               IClient* client,
                                               const CVariant& parameterObject,
                                               CVariant& result)
{
  if (!parameterObject.isMember("media_id") || !parameterObject.isMember("media_type"))
  {
    return InvalidParams;
  }

  int mediaId = static_cast<int>(parameterObject["media_id"].asInteger());
  std::string mediaType = parameterObject["media_type"].asString();

  // Optional priority
  int priority = 0;
  if (parameterObject.isMember("priority"))
  {
    priority = static_cast<int>(parameterObject["priority"].asInteger());
  }

  auto* database = GetSemanticDatabase();
  if (!database)
    return FailedToExecute;

  // Get or create index state
  SemanticIndexState state;
  bool exists = database->GetIndexState(mediaId, MediaTypeFromString(mediaType), state);

  if (!exists)
  {
    // Create new index state
    state.mediaId = mediaId;
    state.mediaType = mediaType;
    state.subtitleStatus = IndexStatus::PENDING;
    state.transcriptionStatus = IndexStatus::PENDING;
    state.metadataStatus = IndexStatus::PENDING;
    state.embeddingStatus = IndexStatus::PENDING;
    state.priority = priority;
  }
  else
  {
    // Update priority if specified
    if (priority > 0)
      state.priority = priority;
  }

  if (!database->UpdateIndexState(state))
  {
    return FailedToExecute;
  }

  result["queued"] = true;
  result["media_id"] = mediaId;
  result["media_type"] = mediaType;

  return OK;
}

JSONRPC_STATUS CSemanticOperations::QueueTranscription(const std::string& method,
                                                       ITransportLayer* transport,
                                                       IClient* client,
                                                       const CVariant& parameterObject,
                                                       CVariant& result)
{
  if (!parameterObject.isMember("media_id") || !parameterObject.isMember("media_type"))
  {
    return InvalidParams;
  }

  int mediaId = static_cast<int>(parameterObject["media_id"].asInteger());
  std::string mediaType = parameterObject["media_type"].asString();

  // Optional provider ID
  std::string providerId;
  if (parameterObject.isMember("provider_id"))
  {
    providerId = parameterObject["provider_id"].asString();
  }

  auto* database = GetSemanticDatabase();
  if (!database)
    return FailedToExecute;

  // Get or create index state
  SemanticIndexState state;
  bool exists = database->GetIndexState(mediaId, MediaTypeFromString(mediaType), state);

  if (!exists)
  {
    // Create new index state
    state.mediaId = mediaId;
    state.mediaType = mediaType;
    state.subtitleStatus = IndexStatus::PENDING;
    state.transcriptionStatus = IndexStatus::PENDING;
    state.metadataStatus = IndexStatus::PENDING;
    state.embeddingStatus = IndexStatus::PENDING;
  }

  // Set transcription to pending
  state.transcriptionStatus = IndexStatus::PENDING;
  state.transcriptionProgress = 0.0f;
  if (!providerId.empty())
  {
    state.transcriptionProvider = providerId;
  }

  if (!database->UpdateIndexState(state))
  {
    return FailedToExecute;
  }

  result["queued"] = true;
  result["media_id"] = mediaId;
  result["media_type"] = mediaType;
  if (!providerId.empty())
  {
    result["provider_id"] = providerId;
  }

  return OK;
}

JSONRPC_STATUS CSemanticOperations::GetStats(const std::string& method,
                                             ITransportLayer* transport,
                                             IClient* client,
                                             const CVariant& parameterObject,
                                             CVariant& result)
{
  auto* database = GetSemanticDatabase();
  if (!database)
    return FailedToExecute;

  IndexStats stats = database->GetStats();

  result["total_media"] = stats.totalMedia;
  result["indexed_media"] = stats.indexedMedia;
  result["total_chunks"] = stats.totalChunks;
  result["total_words"] = stats.totalWords;
  result["queued_jobs"] = stats.queuedJobs;

  // Provider usage
  auto* providerManager = GetProviderManager();
  if (providerManager)
  {
    result["total_cost_usd"] = static_cast<double>(providerManager->GetTotalCost());
    result["monthly_cost_usd"] = static_cast<double>(providerManager->GetMonthlyUsage(""));
    result["budget_exceeded"] = providerManager->IsBudgetExceeded();
    result["remaining_budget_usd"] = static_cast<double>(providerManager->GetRemainingBudget());
  }
  else
  {
    result["total_cost_usd"] = 0.0;
    result["monthly_cost_usd"] = 0.0;
    result["budget_exceeded"] = false;
    result["remaining_budget_usd"] = 0.0;
  }

  // Embedding stats
  int64_t embeddingCount = database->GetEmbeddingCount();
  result["embedding_count"] = embeddingCount >= 0 ? embeddingCount : 0;

  return OK;
}

JSONRPC_STATUS CSemanticOperations::GetProviders(const std::string& method,
                                                 ITransportLayer* transport,
                                                 IClient* client,
                                                 const CVariant& parameterObject,
                                                 CVariant& result)
{
  auto* providerManager = GetProviderManager();
  if (!providerManager)
    return FailedToExecute;

  auto providerInfoList = providerManager->GetProviderInfoList();

  CVariant providers = CVariant(CVariant::VariantTypeArray);
  for (const auto& info : providerInfoList)
  {
    CVariant provider;
    provider["id"] = info.id;
    provider["name"] = info.name;
    provider["is_configured"] = info.isConfigured;
    provider["is_available"] = info.isAvailable;
    provider["is_local"] = info.isLocal;
    provider["cost_per_minute"] = static_cast<double>(info.costPerMinute);

    providers.push_back(provider);
  }

  result["providers"] = providers;
  result["default_provider"] = providerManager->GetDefaultProviderId();

  return OK;
}

JSONRPC_STATUS CSemanticOperations::EstimateCost(const std::string& method,
                                                 ITransportLayer* transport,
                                                 IClient* client,
                                                 const CVariant& parameterObject,
                                                 CVariant& result)
{
  if (!parameterObject.isMember("media_id") || !parameterObject.isMember("media_type"))
  {
    return InvalidParams;
  }

  int mediaId = static_cast<int>(parameterObject["media_id"].asInteger());
  std::string mediaType = parameterObject["media_type"].asString();

  // Optional provider ID
  std::string providerId;
  if (parameterObject.isMember("provider_id"))
  {
    providerId = parameterObject["provider_id"].asString();
  }

  // Get provider info
  auto* providerManager = GetProviderManager();
  if (!providerManager)
    return FailedToExecute;

  // Get media duration from video database
  CVideoDatabase videoDb;
  if (!videoDb.Open())
    return InternalError;

  int runtime = 0; // in seconds
  if (mediaType == "movie")
  {
    CVideoInfoTag details;
    if (videoDb.GetMovieInfo("", details, mediaId))
    {
      runtime = details.GetDuration();
    }
  }
  else if (mediaType == "episode")
  {
    CVideoInfoTag details;
    if (videoDb.GetEpisodeInfo("", details, mediaId))
    {
      runtime = details.GetDuration();
    }
  }
  else if (mediaType == "musicvideo")
  {
    CVideoInfoTag details;
    if (videoDb.GetMusicVideoInfo("", details, mediaId))
    {
      runtime = details.GetDuration();
    }
  }

  videoDb.Close();

  if (runtime <= 0)
  {
    result["error"] = "Could not determine media duration";
    return InvalidParams;
  }

  // Get provider or use default
  std::string actualProviderId = providerId;
  if (actualProviderId.empty())
  {
    actualProviderId = providerManager->GetDefaultProviderId();
  }

  auto* provider = providerManager->GetProvider(actualProviderId);
  if (!provider)
  {
    result["error"] = "Provider not found";
    return InvalidParams;
  }

  // Calculate estimated cost
  float durationMinutes = runtime / 60.0f;
  float costPerMinute = provider->GetCostPerMinute();
  float estimatedCost = durationMinutes * costPerMinute;

  result["media_id"] = mediaId;
  result["media_type"] = mediaType;
  result["provider_id"] = actualProviderId;
  result["duration_seconds"] = runtime;
  result["duration_minutes"] = static_cast<double>(durationMinutes);
  result["cost_per_minute"] = static_cast<double>(costPerMinute);
  result["estimated_cost_usd"] = static_cast<double>(estimatedCost);
  result["is_local"] = provider->IsLocal();

  return OK;
}
