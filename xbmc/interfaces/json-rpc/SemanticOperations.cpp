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
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "semantic/SemanticIndexService.h"
#include "semantic/SemanticTypes.h"
#include "semantic/search/SemanticSearch.h"
#include "semantic/transcription/TranscriptionProviderManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
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

// Static instances shared across all operations
static CSemanticDatabase* s_database = nullptr;
static CSemanticSearch* s_search = nullptr;
static bool s_initialized = false;

/*!
 * \brief Initialize or get the semantic database instance
 */
CSemanticDatabase* GetSemanticDatabase()
{
  if (!s_initialized)
  {
    // Try to get from SemanticIndexService first (preferred)
    auto* service = CServiceBroker::GetSemanticIndexService();
    if (service && service->IsRunning())
    {
      s_database = service->GetDatabase();
      if (s_database)
      {
        s_initialized = true;
        return s_database;
      }
    }

    // Fallback: create own instance
    static CSemanticDatabase fallbackDb;
    if (fallbackDb.Open())
    {
      s_database = &fallbackDb;
      s_initialized = true;
    }
  }

  return s_database;
}

/*!
 * \brief Get semantic search instance
 */
CSemanticSearch* GetSemanticSearch()
{
  static bool searchInitialized = false;
  static CSemanticSearch search;

  if (!searchInitialized)
  {
    CLog::Log(LOGINFO, "SemanticOperations: Initializing SemanticSearch...");
    auto* database = GetSemanticDatabase();
    if (database && search.Initialize(database))
    {
      s_search = &search;
      searchInitialized = true;
      CLog::Log(LOGINFO, "SemanticOperations: SemanticSearch initialized successfully");
    }
    else
    {
      CLog::Log(LOGERROR, "SemanticOperations: Failed to initialize SemanticSearch (db={})",
                database ? "available" : "null");
    }
  }

  return s_search;
}

/*!
 * \brief Get transcription provider manager instance
 */
CTranscriptionProviderManager* GetProviderManager()
{
  // Try to get from SemanticIndexService
  auto* service = CServiceBroker::GetSemanticIndexService();
  if (service && service->IsRunning())
  {
    return service->GetTranscriptionManager();
  }

  return nullptr;
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

  CLog::Log(LOGINFO, "SemanticOperations::Search: query='{}', has options={}",
            query, parameterObject.isMember("options"));

  if (parameterObject.isMember("options"))
  {
    const CVariant& opts = parameterObject["options"];

    // Note: JSON-RPC schema may inject default enum values even when user didn't specify them
    // We need to be careful to only use values that were explicitly provided

    // limit has a schema default of 50, so we accept any value
    if (opts.isMember("limit") && opts["limit"].asInteger() > 0)
      options.maxResults = static_cast<int>(opts["limit"].asInteger());

    // media_type: only set if provided and not the schema-injected first enum value
    // The schema has enum: ["movie", "episode", "musicvideo"] - "movie" is first
    // We'll accept any value, but the user should explicitly pass it
    // Since we can't tell if it was injected or user-provided, we'll leave this unfiltered
    // unless we explicitly check for an empty value
    if (opts.isMember("media_type"))
    {
      std::string mediaTypeVal = opts["media_type"].asString();
      // Only set if the value seems intentional (not relying on schema default detection)
      // For now, require explicit non-empty value from user's perspective
      // The schema injection issue means we need a different approach:
      // Don't filter by media_type at all unless media_id is also specified
      // This is a reasonable heuristic - users searching for specific media will provide both
      if (!mediaTypeVal.empty() && opts.isMember("media_id") && opts["media_id"].asInteger() > 0)
      {
        options.mediaType = mediaTypeVal;
      }
    }

    if (opts.isMember("media_id") && opts["media_id"].asInteger() > 0)
    {
      options.mediaId = static_cast<int>(opts["media_id"].asInteger());
    }

    // source_type: only enable source filtering if user explicitly provided a value
    // The schema has enum: ["subtitle", "transcription", "metadata"] - "subtitle" is first
    // We need to detect if this was schema-injected vs user-provided
    // Heuristic: if source_type is "subtitle" (first enum) and no other filters are set,
    // assume it was schema-injected
    if (opts.isMember("source_type"))
    {
      std::string sourceStr = opts["source_type"].asString();
      // Only enable source filtering if:
      // 1. The value is NOT the first enum value (definitely user-provided), OR
      // 2. Other filters are also set (suggesting intentional filtering)
      bool isDefaultEnumValue = (sourceStr == "subtitle");
      bool hasOtherFilters = (opts.isMember("media_id") && opts["media_id"].asInteger() > 0);

      if (!isDefaultEnumValue || hasOtherFilters)
      {
        options.sourceType = StringToSourceType(sourceStr);
        options.filterBySource = true;
      }
    }

    if (opts.isMember("min_confidence") && opts["min_confidence"].asDouble() > 0.0)
    {
      options.minConfidence = static_cast<float>(opts["min_confidence"].asDouble());
    }
  }

  CLog::Log(LOGINFO, "SemanticOperations::Search: options - maxResults={}, mediaType='{}', mediaId={}, filterBySource={}, sourceType={}",
            options.maxResults, options.mediaType, options.mediaId, options.filterBySource,
            static_cast<int>(options.sourceType));

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
  int64_t timestampMs = static_cast<int64_t>(parameterObject["timestamp_ms"].asInteger());

  // Optional window size (default 60 seconds)
  int64_t windowMs = 60000;
  if (parameterObject.isMember("window_ms"))
  {
    windowMs = static_cast<int64_t>(parameterObject["window_ms"].asInteger());
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
  if (!database->GetIndexState(mediaId, CMediaTypes::FromString(mediaType), state))
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
  bool exists = database->GetIndexState(mediaId, CMediaTypes::FromString(mediaType), state);

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

  // Get the semantic index service and queue for transcription
  auto* service = CServiceBroker::GetSemanticIndexService();
  if (!service || !service->IsRunning())
  {
    CLog::Log(LOGERROR, "SemanticOperations::QueueTranscription: Service not running");
    return FailedToExecute;
  }

  // Queue the item for transcription via the service (this sets transcribe=true)
  service->QueueTranscription(mediaId, mediaType);

  // Update provider if specified
  if (!providerId.empty())
  {
    auto* database = GetSemanticDatabase();
    if (database)
    {
      SemanticIndexState state;
      if (database->GetIndexState(mediaId, mediaType, state))
      {
        state.transcriptionProvider = providerId;
        database->UpdateIndexState(state);
      }
    }
  }

  CLog::Log(LOGINFO, "SemanticOperations::QueueTranscription: Queued {} {} for transcription",
            mediaType, mediaId);

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

  // Embedding stats (Phase 2 feature - not yet available)
  result["embedding_count"] = 0;

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

  // Calculate estimated cost using provider's EstimateCost method
  float durationMinutes = runtime / 60.0f;
  float costPerMinute = provider->EstimateCost(60000); // Cost for 1 minute (60000ms)
  float estimatedCost = provider->EstimateCost(runtime * 1000); // runtime is in seconds

  // Get provider info for additional details
  auto providerInfoList = providerManager->GetProviderInfoList();
  bool isLocal = false;
  for (const auto& info : providerInfoList)
  {
    if (info.id == actualProviderId)
    {
      isLocal = info.isLocal;
      break;
    }
  }

  result["media_id"] = mediaId;
  result["media_type"] = mediaType;
  result["provider_id"] = actualProviderId;
  result["duration_seconds"] = runtime;
  result["duration_minutes"] = static_cast<double>(durationMinutes);
  result["cost_per_minute"] = static_cast<double>(costPerMinute);
  result["estimated_cost_usd"] = static_cast<double>(estimatedCost);
  result["is_local"] = isLocal;

  return OK;
}

JSONRPC_STATUS CSemanticOperations::Configure(const std::string& method,
                                              ITransportLayer* transport,
                                              IClient* client,
                                              const CVariant& parameterObject,
                                              CVariant& result)
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return InternalError;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return InternalError;

  bool anyChanges = false;

  // Configure Groq API key
  if (parameterObject.isMember("groq_api_key"))
  {
    std::string apiKey = parameterObject["groq_api_key"].asString();
    if (settings->SetString(CSettings::SETTING_SEMANTIC_GROQ_APIKEY, apiKey))
    {
      CLog::Log(LOGINFO, "SemanticOperations::Configure: Groq API key {}",
                apiKey.empty() ? "cleared" : "set");
      anyChanges = true;
    }
  }

  // Configure auto-transcribe
  if (parameterObject.isMember("auto_transcribe"))
  {
    bool autoTranscribe = parameterObject["auto_transcribe"].asBoolean();
    if (settings->SetBool(CSettings::SETTING_SEMANTIC_AUTOTRANSCRIBE, autoTranscribe))
    {
      CLog::Log(LOGINFO, "SemanticOperations::Configure: Auto-transcribe {}",
                autoTranscribe ? "enabled" : "disabled");
      anyChanges = true;
    }
  }

  // Configure monthly budget
  if (parameterObject.isMember("monthly_budget"))
  {
    double budget = parameterObject["monthly_budget"].asDouble();
    if (settings->SetNumber(CSettings::SETTING_SEMANTIC_MAXCOST, budget))
    {
      CLog::Log(LOGINFO, "SemanticOperations::Configure: Monthly budget set to ${:.2f}", budget);
      anyChanges = true;
    }
  }

  // Save settings if any changes were made
  if (anyChanges)
  {
    settings->Save();
  }

  result["success"] = true;

  // Return current configuration state
  CVariant configured;
  configured["groq_configured"] = !settings->GetString(CSettings::SETTING_SEMANTIC_GROQ_APIKEY).empty();
  configured["auto_transcribe"] = settings->GetBool(CSettings::SETTING_SEMANTIC_AUTOTRANSCRIBE);
  configured["monthly_budget"] = settings->GetNumber(CSettings::SETTING_SEMANTIC_MAXCOST);

  // Get budget usage from provider manager
  auto* providerManager = GetProviderManager();
  if (providerManager)
  {
    configured["budget_used"] = static_cast<double>(providerManager->GetTotalCost("groq"));
    configured["budget_remaining"] = static_cast<double>(providerManager->GetRemainingBudget());
  }

  result["configured"] = configured;

  return OK;
}

JSONRPC_STATUS CSemanticOperations::GetConfig(const std::string& method,
                                              ITransportLayer* transport,
                                              IClient* client,
                                              const CVariant& parameterObject,
                                              CVariant& result)
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return InternalError;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return InternalError;

  result["groq_configured"] = !settings->GetString(CSettings::SETTING_SEMANTIC_GROQ_APIKEY).empty();
  result["auto_transcribe"] = settings->GetBool(CSettings::SETTING_SEMANTIC_AUTOTRANSCRIBE);
  result["monthly_budget"] = settings->GetNumber(CSettings::SETTING_SEMANTIC_MAXCOST);

  // Get budget usage from provider manager
  auto* providerManager = GetProviderManager();
  if (providerManager)
  {
    result["budget_used"] = static_cast<double>(providerManager->GetTotalCost("groq"));
    result["budget_remaining"] = static_cast<double>(providerManager->GetRemainingBudget());
  }
  else
  {
    result["budget_used"] = 0.0;
    result["budget_remaining"] = settings->GetNumber(CSettings::SETTING_SEMANTIC_MAXCOST);
  }

  return OK;
}
