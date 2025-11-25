# Semantic Search API Reference

## Overview

This document covers both the **C++ API** (for internal Kodi development) and the **JSON-RPC API** (for external clients, web interfaces, and remote control).

---

# JSON-RPC API

## Introduction

The Semantic Search JSON-RPC API provides remote access to all search and indexing functionality. Compatible with:
- Web applications
- Mobile apps
- Third-party remote controls
- Scripting and automation

**Base URL**: `http://kodi-host:8080/jsonrpc`

**Authentication**: HTTP Basic Auth (if enabled in settings)

---

## Common Request Format

```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.MethodName",
  "params": {
    "param1": "value1",
    "param2": 123
  },
  "id": 1
}
```

## Common Response Format

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "field1": "value1",
    "field2": 42
  }
}
```

## Error Response

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32601,
    "message": "Method not found"
  }
}
```

---

## Search Methods

### Semantic.Search

**Description**: Search indexed content using hybrid search (FTS5 + vector)

**Parameters**:

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `query` | string | ✓ | Search query (min 1 char) |
| `mode` | string | ✗ | Search mode: "hybrid", "keyword", "semantic" (default: "hybrid") |
| `media_type` | string | ✗ | Filter by type: "movie", "episode", "musicvideo", "all" (default: "all") |
| `max_results` | integer | ✗ | Maximum results to return (default: 20, max: 100) |
| `min_confidence` | number | ✗ | Minimum confidence score 0.0-1.0 (default: 0.0) |
| `genres` | array[string] | ✗ | Filter by genres (OR logic) |
| `min_year` | integer | ✗ | Minimum release year |
| `max_year` | integer | ✗ | Maximum release year |
| `rating` | string | ✗ | MPAA rating filter: "G", "PG", "PG-13", "R", "NC-17" |
| `min_duration` | integer | ✗ | Minimum duration in minutes |
| `max_duration` | integer | ✗ | Maximum duration in minutes |
| `sources` | array[string] | ✗ | Content sources: ["subtitle", "transcription", "metadata"] (default: all) |

**Returns**:

```json
{
  "results": [
    {
      "chunk_id": 12345,
      "media_id": 42,
      "media_type": "movie",
      "media_title": "The Dark Knight",
      "media_year": 2008,
      "timestamp_ms": 5025000,
      "timestamp_formatted": "01:23:45",
      "text": "Full chunk text content...",
      "snippet": "Matched <b>text</b> snippet...",
      "source_type": "subtitle",
      "combined_score": 0.95,
      "keyword_score": 0.87,
      "vector_score": 0.92,
      "confidence": 1.0
    }
  ],
  "total_results": 42,
  "query_time_ms": 38
}
```

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.Search",
    "params": {
      "query": "detective solving mystery",
      "mode": "hybrid",
      "media_type": "movie",
      "max_results": 10,
      "min_year": 2000
    },
    "id": 1
  }'
```

**Example Response**:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "results": [
      {
        "chunk_id": 4521,
        "media_id": 142,
        "media_type": "movie",
        "media_title": "Sherlock Holmes",
        "media_year": 2009,
        "timestamp_ms": 3245000,
        "timestamp_formatted": "00:54:05",
        "text": "Holmes examines the crime scene carefully, searching for clues...",
        "snippet": "Holmes examines the crime scene carefully, searching for <b>clues</b>...",
        "source_type": "subtitle",
        "combined_score": 0.94,
        "keyword_score": 0.89,
        "vector_score": 0.96,
        "confidence": 1.0
      }
    ],
    "total_results": 8,
    "query_time_ms": 42
  }
}
```

---

### Semantic.FindSimilar

**Description**: Find content similar to a specific chunk using vector similarity

**Parameters**:

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `chunk_id` | integer | ✗* | Chunk ID to find similar content for |
| `media_id` | integer | ✗* | Media ID (alternative to chunk_id) |
| `media_type` | string | ✗* | Media type if using media_id |
| `top_k` | integer | ✗ | Number of results (default: 20) |
| `max_distance` | number | ✗ | Maximum cosine distance 0.0-2.0 (default: 1.0) |

*Either `chunk_id` OR (`media_id` + `media_type`) required

**Returns**:

```json
{
  "results": [
    {
      "chunk_id": 67890,
      "media_id": 88,
      "media_type": "movie",
      "media_title": "The Prestige",
      "media_year": 2006,
      "timestamp_ms": 2134000,
      "timestamp_formatted": "00:35:34",
      "text": "Chunk text content...",
      "source_type": "subtitle",
      "similarity": 0.91,
      "distance": 0.09
    }
  ],
  "source_chunk": {
    "chunk_id": 4521,
    "media_title": "Sherlock Holmes",
    "timestamp_formatted": "00:54:05"
  },
  "total_results": 15
}
```

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.FindSimilar",
    "params": {
      "chunk_id": 4521,
      "top_k": 5
    },
    "id": 2
  }'
```

---

### Semantic.GetContext

**Description**: Get contextual chunks around a specific timestamp

**Parameters**:

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `media_id` | integer | ✓ | Media item ID |
| `media_type` | string | ✓ | Media type: "movie", "episode", "musicvideo" |
| `timestamp_ms` | integer | ✓ | Center timestamp in milliseconds |
| `window_ms` | integer | ✗ | Context window size in ms (default: 60000 = ±30s) |

**Returns**:

```json
{
  "chunks": [
    {
      "chunk_id": 100,
      "start_ms": 3215000,
      "end_ms": 3230000,
      "timestamp_formatted": "00:53:35 - 00:53:50",
      "text": "Previous context chunk...",
      "source_type": "subtitle"
    },
    {
      "chunk_id": 101,
      "start_ms": 3245000,
      "end_ms": 3260000,
      "timestamp_formatted": "00:54:05 - 00:54:20",
      "text": "Center chunk (matched)...",
      "source_type": "subtitle",
      "is_center": true
    },
    {
      "chunk_id": 102,
      "start_ms": 3275000,
      "end_ms": 3290000,
      "timestamp_formatted": "00:54:35 - 00:54:50",
      "text": "Following context chunk...",
      "source_type": "subtitle"
    }
  ],
  "media_title": "Sherlock Holmes",
  "total_chunks": 3
}
```

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.GetContext",
    "params": {
      "media_id": 142,
      "media_type": "movie",
      "timestamp_ms": 3245000,
      "window_ms": 30000
    },
    "id": 3
  }'
```

---

## Indexing Methods

### Semantic.GetIndexState

**Description**: Get indexing status for a media item

**Parameters**:

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `media_id` | integer | ✓ | Media item ID |
| `media_type` | string | ✓ | Media type: "movie", "episode", "musicvideo" |

**Returns**:

```json
{
  "media_id": 142,
  "media_type": "movie",
  "media_title": "Sherlock Holmes",
  "is_searchable": true,
  "indexing_status": "completed",
  "sources": {
    "subtitle": {
      "status": "completed",
      "chunk_count": 842,
      "language": "en",
      "indexed_at": "2024-11-20T14:32:15Z"
    },
    "transcription": {
      "status": "not_started",
      "chunk_count": 0
    },
    "metadata": {
      "status": "completed",
      "chunk_count": 12,
      "indexed_at": "2024-11-20T14:30:00Z"
    }
  },
  "total_chunks": 854,
  "has_embeddings": true,
  "embedding_count": 854,
  "last_updated": "2024-11-20T14:32:15Z"
}
```

**Status Values**:
- `not_started`: Not yet indexed
- `queued`: In indexing queue
- `in_progress`: Currently indexing
- `completed`: Indexing complete
- `failed`: Indexing failed
- `partial`: Some sources completed, others failed

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.GetIndexState",
    "params": {
      "media_id": 142,
      "media_type": "movie"
    },
    "id": 4
  }'
```

---

### Semantic.QueueIndex

**Description**: Queue a media item for indexing

**Parameters**:

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `media_id` | integer | ✓ | Media item ID |
| `media_type` | string | ✓ | Media type: "movie", "episode", "musicvideo" |
| `priority` | integer | ✗ | Queue priority 0-10 (default: 0, higher = sooner) |
| `force_reindex` | boolean | ✗ | Force reindex even if already indexed (default: false) |

**Returns**:

```json
{
  "success": true,
  "job_id": "idx_20241120_143215_142",
  "position_in_queue": 3,
  "estimated_start_time": "2024-11-20T14:35:00Z"
}
```

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.QueueIndex",
    "params": {
      "media_id": 256,
      "media_type": "movie",
      "priority": 5
    },
    "id": 5
  }'
```

---

### Semantic.QueueTranscription

**Description**: Queue a media item for audio transcription

**Parameters**:

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `media_id` | integer | ✓ | Media item ID |
| `media_type` | string | ✓ | Media type |
| `provider_id` | string | ✗ | Transcription provider (default: auto-select) |
| `language` | string | ✗ | Expected language code (default: auto-detect) |

**Returns**:

```json
{
  "success": true,
  "job_id": "trans_20241120_143500_256",
  "provider": "groq",
  "estimated_cost_usd": 0.12,
  "estimated_duration_sec": 180,
  "position_in_queue": 1
}
```

**Available Providers**:
- `groq`: Groq Whisper API (fast, low cost)
- `openai`: OpenAI Whisper API (accurate, higher cost)
- `local`: Local Whisper model (free, slower)

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.QueueTranscription",
    "params": {
      "media_id": 256,
      "media_type": "movie",
      "provider_id": "groq"
    },
    "id": 6
  }'
```

---

## Statistics Methods

### Semantic.GetStats

**Description**: Get overall semantic search statistics

**Parameters**: None

**Returns**:

```json
{
  "total_media": 1247,
  "indexed_media": 1183,
  "pending_media": 64,
  "total_chunks": 124583,
  "total_embeddings": 124583,
  "index_size_mb": 245.8,
  "embedding_size_mb": 186.2,
  "fts_index_size_mb": 42.1,
  "transcription_stats": {
    "total_transcribed": 342,
    "total_cost_usd": 42.35,
    "remaining_budget_usd": 57.65
  },
  "cache_stats": {
    "enabled": true,
    "hit_rate": 0.73,
    "size_mb": 12.4,
    "capacity_mb": 50.0
  },
  "performance": {
    "avg_search_time_ms": 38,
    "avg_embedding_time_ms": 22,
    "total_searches": 5842
  },
  "last_index_time": "2024-11-20T14:32:15Z"
}
```

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.GetStats",
    "params": {},
    "id": 7
  }'
```

---

### Semantic.GetProviders

**Description**: Get available transcription providers and their status

**Parameters**: None

**Returns**:

```json
{
  "providers": [
    {
      "id": "groq",
      "name": "Groq Whisper API",
      "type": "cloud",
      "available": true,
      "cost_per_minute_usd": 0.0001,
      "speed_factor": 10.0,
      "supported_languages": ["en", "es", "fr", "de", "it", "pt", "nl", "pl", "ru", "zh", "ja", "ko"],
      "requires_api_key": true,
      "api_key_configured": true
    },
    {
      "id": "openai",
      "name": "OpenAI Whisper API",
      "type": "cloud",
      "available": true,
      "cost_per_minute_usd": 0.006,
      "speed_factor": 5.0,
      "supported_languages": ["*"],
      "requires_api_key": true,
      "api_key_configured": false
    },
    {
      "id": "local",
      "name": "Local Whisper",
      "type": "local",
      "available": false,
      "cost_per_minute_usd": 0.0,
      "speed_factor": 0.2,
      "supported_languages": ["en", "es", "fr", "de"],
      "requires_api_key": false,
      "status": "Model not downloaded"
    }
  ],
  "default_provider": "groq",
  "total_budget_usd": 100.0,
  "used_budget_usd": 42.35,
  "remaining_budget_usd": 57.65
}
```

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.GetProviders",
    "params": {},
    "id": 8
  }'
```

---

### Semantic.EstimateCost

**Description**: Estimate transcription cost for a media item

**Parameters**:

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `media_id` | integer | ✓ | Media item ID |
| `media_type` | string | ✓ | Media type |
| `provider_id` | string | ✗ | Provider to use (default: default provider) |

**Returns**:

```json
{
  "media_id": 256,
  "media_title": "Inception",
  "duration_minutes": 148,
  "provider": "groq",
  "estimated_cost_usd": 0.0148,
  "estimated_time_seconds": 89
}
```

**Example Request**:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.EstimateCost",
    "params": {
      "media_id": 256,
      "media_type": "movie",
      "provider_id": "groq"
    },
    "id": 9
  }'
```

---

## Error Codes

| Code | Message | Description |
|------|---------|-------------|
| -32700 | Parse error | Invalid JSON |
| -32600 | Invalid request | Missing required fields |
| -32601 | Method not found | Unknown method |
| -32602 | Invalid params | Invalid parameter type/value |
| -32603 | Internal error | Server-side error |
| -32001 | Database error | Database operation failed |
| -32002 | Not indexed | Media item not indexed yet |
| -32003 | Indexing failed | Indexing operation failed |
| -32004 | Model not loaded | Embedding model not available |
| -32005 | Transcription failed | Transcription operation failed |
| -32006 | Budget exceeded | Transcription budget exhausted |
| -32007 | Provider unavailable | Transcription provider not available |

**Example Error**:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32002,
    "message": "Media item not indexed",
    "data": {
      "media_id": 999,
      "media_type": "movie",
      "status": "not_started"
    }
  }
}
```

---

# C++ API

## Core Classes

### CEmbeddingEngine

**Header**: `xbmc/semantic/embedding/EmbeddingEngine.h`

**Purpose**: Generate 384-dimensional embeddings using ONNX Runtime

#### Methods

##### `bool Initialize(modelPath, vocabPath, lazyLoad, idleTimeout)`

Initialize the embedding engine.

**Parameters**:
- `modelPath` (string): Path to ONNX model file
- `vocabPath` (string): Path to tokenizer vocabulary
- `lazyLoad` (bool): Load model on first use (default: true)
- `idleTimeout` (int): Seconds before unloading (default: 300)

**Returns**: `true` if successful

**Example**:
```cpp
#include "semantic/embedding/EmbeddingEngine.h"

CEmbeddingEngine engine;
if (!engine.Initialize("/path/to/model.onnx", "/path/to/vocab.txt"))
{
    CLog::LogF(LOGERROR, "Failed to initialize embedding engine");
    return false;
}
```

##### `Embedding Embed(const std::string& text)`

Generate embedding for single text.

**Parameters**:
- `text` (string): Input text

**Returns**: `std::array<float, 384>` embedding vector

**Throws**: `std::runtime_error` if not initialized

**Example**:
```cpp
auto embedding = engine.Embed("detective solving mystery");
// embedding.size() == 384
```

##### `std::vector<Embedding> EmbedBatch(const std::vector<std::string>& texts)`

Generate embeddings for multiple texts (more efficient).

**Parameters**:
- `texts` (vector<string>): Input texts

**Returns**: Vector of embeddings (one per input)

**Example**:
```cpp
std::vector<std::string> queries = {"action scene", "romantic comedy"};
auto embeddings = engine.EmbedBatch(queries);
// embeddings.size() == 2
```

##### `static float Similarity(const Embedding& a, const Embedding& b)`

Compute cosine similarity between two embeddings.

**Parameters**:
- `a` (Embedding): First embedding
- `b` (Embedding): Second embedding

**Returns**: Similarity score [-1, 1]

**Example**:
```cpp
float sim = CEmbeddingEngine::Similarity(emb1, emb2);
if (sim > 0.8) {
    // Highly similar
}
```

---

### CVectorSearcher

**Header**: `xbmc/semantic/search/VectorSearcher.h`

**Purpose**: Vector similarity search using sqlite-vec

#### Methods

##### `bool InsertVector(int64_t chunkId, const Embedding& embedding)`

Insert or update a vector.

**Example**:
```cpp
CVectorSearcher searcher;
int64_t chunkId = 12345;
auto embedding = engine.Embed("chunk text");
searcher.InsertVector(chunkId, embedding);
```

##### `bool InsertVectorBatch(const std::vector<std::pair<int64_t, Embedding>>& vectors)`

Insert multiple vectors efficiently.

**Example**:
```cpp
std::vector<std::pair<int64_t, Embedding>> batch;
for (size_t i = 0; i < chunks.size(); ++i) {
    batch.push_back({chunkIds[i], embeddings[i]});
}
searcher.InsertVectorBatch(batch);
```

##### `std::vector<VectorResult> SearchSimilar(const Embedding& query, int topK)`

Search for similar vectors.

**Example**:
```cpp
auto queryEmb = engine.Embed("search query");
auto results = searcher.SearchSimilar(queryEmb, 50);

for (const auto& result : results) {
    std::cout << "Chunk " << result.chunkId
              << " distance: " << result.distance << "\n";
}
```

##### `std::vector<VectorResult> FindSimilar(int64_t chunkId, int topK, float maxDistance)`

Find similar chunks to an existing chunk.

**Example**:
```cpp
auto similar = searcher.FindSimilar(12345, 20, 1.0f);
// Returns up to 20 similar chunks within distance 1.0
```

---

### CHybridSearchEngine

**Header**: `xbmc/semantic/search/HybridSearchEngine.h`

**Purpose**: Combine FTS5 + vector search with RRF fusion

#### Methods

##### `bool Initialize(database, embeddingEngine, vectorSearcher, enableCache)`

Initialize the hybrid search engine.

**Example**:
```cpp
CHybridSearchEngine engine;
engine.Initialize(&database, &embedder, &vectorSearcher, true);
```

##### `std::vector<HybridSearchResult> Search(const std::string& query, const HybridSearchOptions& options)`

Main search interface.

**Example**:
```cpp
HybridSearchOptions opts;
opts.mode = SearchMode::Hybrid;
opts.keywordWeight = 0.4f;
opts.vectorWeight = 0.6f;
opts.maxResults = 20;
opts.genres = {"Action", "Thriller"};
opts.minYear = 2020;

auto results = engine.Search("detective mystery", opts);

for (const auto& result : results) {
    std::cout << result.chunk.mediaType << " "
              << result.chunk.mediaId << ": "
              << result.snippet << " (score: "
              << result.combinedScore << ")\n";
}
```

##### `std::vector<HybridSearchResult> FindSimilar(int64_t chunkId, int topK)`

Find similar content.

**Example**:
```cpp
auto similar = engine.FindSimilar(12345, 10);
```

##### `std::vector<SemanticChunk> GetResultContext(const HybridSearchResult& result, int64_t windowMs)`

Get context around a result.

**Example**:
```cpp
auto context = engine.GetResultContext(results[0], 30000);
// Returns chunks within ±30 seconds
```

---

### CResultRanker

**Header**: `xbmc/semantic/search/ResultRanker.h`

**Purpose**: Merge ranked lists using various algorithms

#### Methods

##### `std::vector<RankedItem> Combine(list1, list2)`

Combine two ranked lists.

**Example**:
```cpp
RankingConfig config;
config.algorithm = RankingAlgorithm::RRF;
config.rrfK = 60.0f;

CResultRanker ranker(config);

std::vector<std::pair<int64_t, float>> keywordResults = {
    {101, 0.95}, {102, 0.87}, {103, 0.82}
};
std::vector<std::pair<int64_t, float>> vectorResults = {
    {102, 0.91}, {104, 0.88}, {101, 0.85}
};

auto merged = ranker.Combine(keywordResults, vectorResults);

for (const auto& item : merged) {
    std::cout << "ID: " << item.id
              << ", Combined: " << item.combinedScore
              << ", Keyword: " << item.score1
              << ", Vector: " << item.score2 << "\n";
}
```

---

### CPerformanceMonitor

**Header**: `xbmc/semantic/perf/PerformanceMonitor.h`

**Purpose**: Monitor and measure performance

#### Methods

##### `void RecordEmbedding(double durationMs, size_t batchSize)`

Record embedding timing.

**Example**:
```cpp
auto start = std::chrono::steady_clock::now();
auto emb = engine.Embed(text);
auto end = std::chrono::steady_clock::now();

double ms = std::chrono::duration<double, std::milli>(end - start).count();
CPerformanceMonitor::GetInstance().RecordEmbedding(ms, 1);
```

##### `PerformanceMetrics GetMetrics()`

Get current metrics.

**Example**:
```cpp
auto& perf = CPerformanceMonitor::GetInstance();
auto metrics = perf.GetMetrics();

std::cout << "Avg search time: " << metrics.avgSearchTime << "ms\n";
std::cout << "Cache hit rate: " << metrics.cacheHitRate * 100 << "%\n";
std::cout << "Total memory: " << metrics.currentMemoryUsage / 1024 / 1024 << "MB\n";
```

##### Timer (RAII)

Automatic timing measurement.

**Example**:
```cpp
{
    PERF_TIMER("embedding");
    auto emb = engine.Embed(text);
}  // Automatically records timing
```

---

## Data Structures

### HybridSearchOptions

```cpp
struct HybridSearchOptions {
    SearchMode mode{SearchMode::Hybrid};
    float keywordWeight{0.4f};
    float vectorWeight{0.6f};
    int keywordTopK{100};
    int vectorTopK{100};
    int maxResults{20};

    std::string mediaType;
    int mediaId{-1};
    float minConfidence{0.0f};

    std::vector<std::string> genres;
    int minYear{-1};
    int maxYear{-1};
    std::string mpaaRating;
    int minDurationMinutes{-1};
    int maxDurationMinutes{-1};

    bool includeSubtitles{true};
    bool includeTranscription{true};
    bool includeMetadata{true};
};
```

### HybridSearchResult

```cpp
struct HybridSearchResult {
    int64_t chunkId{-1};
    SemanticChunk chunk;

    float combinedScore{0.0f};
    float keywordScore{0.0f};
    float vectorScore{0.0f};

    std::string snippet;
    std::string formattedTimestamp;
};
```

### SemanticChunk

```cpp
struct SemanticChunk {
    int chunkId;
    int mediaId;
    std::string mediaType;
    SourceType sourceType;
    std::string sourcePath;
    int startMs;
    int endMs;
    std::string text;
    std::string language;
    float confidence;
    std::string createdAt;
};
```

---

## Threading and Thread Safety

### Thread-Safe Classes

- ✅ `CPerformanceMonitor` (singleton with mutex)
- ✅ `CQueryCache` (internal mutex)
- ✅ `CSearchHistory` (CCriticalSection)

### Not Thread-Safe

- ❌ `CEmbeddingEngine` (create per-thread)
- ❌ `CVectorSearcher` (share database connection)
- ❌ `CHybridSearchEngine` (create per-thread)
- ❌ `CResultRanker` (stateless, but no mutex)

### Best Practices

**Option 1: Thread-local instances**
```cpp
thread_local CEmbeddingEngine g_threadEngine;
thread_local CHybridSearchEngine g_threadSearch;
```

**Option 2: External locking**
```cpp
std::mutex g_searchMutex;

void ThreadSafeSearch(const std::string& query) {
    std::lock_guard<std::mutex> lock(g_searchMutex);
    auto results = searchEngine.Search(query);
}
```

---

## Usage Examples

### Complete Search Flow

```cpp
#include "semantic/SemanticDatabase.h"
#include "semantic/embedding/EmbeddingEngine.h"
#include "semantic/search/VectorSearcher.h"
#include "semantic/search/HybridSearchEngine.h"

// Initialize components
CSemanticDatabase db;
db.Open();

CEmbeddingEngine embedder;
embedder.Initialize("/path/to/model.onnx", "/path/to/vocab.txt");

CVectorSearcher vectorSearch;
vectorSearch.InitializeExtension(db.GetConnection());
vectorSearch.CreateVectorTable();

CHybridSearchEngine searchEngine;
searchEngine.Initialize(&db, &embedder, &vectorSearch);

// Perform search
HybridSearchOptions opts;
opts.mode = SearchMode::Hybrid;
opts.maxResults = 10;

auto results = searchEngine.Search("detective solving mystery", opts);

// Process results
for (const auto& result : results) {
    CLog::LogF(LOGINFO, "Found: {} - {} (score: {:.2f})",
               result.chunk.mediaType,
               result.formattedTimestamp,
               result.combinedScore);

    // Get context
    auto context = searchEngine.GetResultContext(result, 30000);
    for (const auto& ctx : context) {
        CLog::LogF(LOGDEBUG, "  [{}] {}",
                   ctx.startMs, ctx.text.substr(0, 50));
    }
}
```

### Indexing Pipeline

```cpp
#include "semantic/SemanticIndexService.h"

CSemanticIndexService indexService;
indexService.Initialize(&db, &embedder);

// Queue media for indexing
int movieId = 42;
indexService.QueueMediaIndex(movieId, "movie", 5 /* priority */);

// Monitor progress
auto progress = indexService.GetIndexingProgress(movieId, "movie");
std::cout << "Progress: " << progress.completedChunks
          << "/" << progress.totalChunks << "\n";

// Wait for completion
while (!indexService.IsMediaSearchable(movieId, "movie")) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

std::cout << "Indexing complete!\n";
```

---

*For GUI usage, see [GUIGuide.md](GUIGuide.md)*
*For performance optimization, see [PerformanceTuning.md](PerformanceTuning.md)*

---

*Last Updated: 2025-11-25*
*Phase 2 API Complete*
