# Technical Design Document: Kodi Semantic Search

## Overview

This document provides a comprehensive technical design for Kodi's semantic search system, covering architecture, data flow, component interactions, and extension points.

**Version**: Phase 2 Complete (Vector Search + Hybrid)
**Last Updated**: 2025-11-25

---

## Table of Contents

1. [System Architecture](#system-architecture)
2. [Component Design](#component-design)
3. [Data Flow](#data-flow)
4. [Database Schema](#database-schema)
5. [Search Pipeline](#search-pipeline)
6. [Indexing Pipeline](#indexing-pipeline)
7. [Extension Points](#extension-points)
8. [Security Considerations](#security-considerations)
9. [Scalability](#scalability)
10. [Future Roadmap](#future-roadmap)

---

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Presentation Layer                       │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │ GUI Dialog       │  │ JSON-RPC API     │  │ Voice Input  │  │
│  │ (CGUIDialog)     │  │ (JSONRPC Server) │  │ (Platform)   │  │
│  └──────────────────┘  └──────────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                         Service Layer                           │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │              CSemanticIndexService                        │  │
│  │  • Indexing orchestration                                 │  │
│  │  • Job queue management                                   │  │
│  │  • Progress tracking                                      │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Search Layer                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │ CHybridSearch    │  │ CResultRanker    │  │ CResultEnricher│
│  │ Engine           │  │ (RRF Fusion)     │  │               │  │
│  └──────────────────┘  └──────────────────┘  └──────────────┘  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │ CSemanticSearch  │  │ CVectorSearcher  │  │ CContextProvider│
│  │ (FTS5/BM25)      │  │ (sqlite-vec)     │  │               │  │
│  └──────────────────┘  └──────────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Processing Layer                          │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │ CEmbeddingEngine │  │ CContentParser   │  │ CChunkProcessor│
│  │ (ONNX Runtime)   │  │ (Subtitle, etc.) │  │               │  │
│  └──────────────────┘  └──────────────────┘  └──────────────┘  │
│  ┌──────────────────┐  ┌──────────────────┐                    │
│  │ Tokenizer        │  │ Transcription    │                    │
│  │ (WordPiece)      │  │ Provider         │                    │
│  └──────────────────┘  └──────────────────┘                    │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Storage Layer                              │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │              CSemanticDatabase (SQLite)                   │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │  │
│  │  │ FTS5 Index   │  │ vec0 Index   │  │ Metadata     │    │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘    │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Infrastructure Layer                         │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │ CPerformance     │  │ CMemoryManager   │  │ CQueryCache  │  │
│  │ Monitor          │  │                  │  │              │  │
│  └──────────────────┘  └──────────────────┘  └──────────────┘  │
│  ┌──────────────────┐  ┌──────────────────┐                    │
│  │ CSearchHistory   │  │ CFilterPresets   │                    │
│  │                  │  │                  │                    │
│  └──────────────────┘  └──────────────────┘                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Component Design

### Core Components

#### CSemanticDatabase

**Responsibilities**:
- SQLite database management
- Schema creation and migration
- Transaction management
- Connection pooling

**Key Methods**:
```cpp
class CSemanticDatabase : public CDatabase {
public:
  bool Open() override;
  bool CreateTables();
  bool MigrateSchema(int fromVersion, int toVersion);

  // Chunk operations
  int64_t InsertChunk(const SemanticChunk& chunk);
  bool UpdateChunk(int64_t chunkId, const SemanticChunk& chunk);
  bool DeleteChunk(int64_t chunkId);
  SemanticChunk GetChunk(int64_t chunkId);

  // Index state
  bool UpdateIndexState(int mediaId, const std::string& mediaType, ...);
  IndexState GetIndexState(int mediaId, const std::string& mediaType);
};
```

**Database Location**:
- Path: `special://masterprofile/Database/SemanticSearch.db`
- SQLite version: 3.41.0+
- Extensions: FTS5, sqlite-vec

---

#### CEmbeddingEngine

**Responsibilities**:
- ONNX model loading/unloading
- Text tokenization (WordPiece)
- Embedding generation (single/batch)
- Lazy loading and memory management

**Architecture**:
```cpp
class CEmbeddingEngine {
private:
  class Impl {
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Tokenizer> m_tokenizer;

    bool m_isLoaded{false};
    std::chrono::steady_clock::time_point m_lastUsed;
    int m_idleTimeoutSec{300};
  };

  std::unique_ptr<Impl> m_impl;

  void CheckAndUnloadIfIdle();
  void EnsureModelLoaded();

public:
  Embedding Embed(const std::string& text);
  std::vector<Embedding> EmbedBatch(const std::vector<std::string>& texts);
};
```

**PIMPL Pattern**: Hides ONNX Runtime dependencies from header

**Model Format**: ONNX (Open Neural Network Exchange)

---

#### CVectorSearcher

**Responsibilities**:
- Vector storage using sqlite-vec
- k-NN similarity search
- Batch vector operations
- Index maintenance

**Index Structure**:
```sql
CREATE VIRTUAL TABLE semantic_vectors USING vec0(
  chunk_id INTEGER PRIMARY KEY,
  embedding float[384]
);
```

**Search Algorithm**:
- Small datasets (<10K): Brute-force (exact)
- Large datasets (>10K): HNSW (approximate)

**Distance Metric**: Cosine distance (1 - cosine similarity)

---

#### CHybridSearchEngine

**Responsibilities**:
- Orchestrate FTS5 + vector search
- Merge results using RRF
- Filter and enrich results
- Cache management

**Search Flow**:
```cpp
class CHybridSearchEngine {
private:
  std::vector<HybridSearchResult> SearchKeywordOnly(...);
  std::vector<HybridSearchResult> SearchSemanticOnly(...);
  std::vector<HybridSearchResult> SearchHybrid(...);

  std::vector<HybridSearchResult> CombineResultsRRF(
    const std::vector<SearchResult>& keywordResults,
    const std::vector<VectorResult>& vectorResults,
    const HybridSearchOptions& options);

  HybridSearchResult EnrichResult(int64_t chunkId, float kwScore, float vecScore);

public:
  std::vector<HybridSearchResult> Search(
    const std::string& query,
    const HybridSearchOptions& options);
};
```

**Decision Tree**:
```
Query → Mode?
  ├─ KeywordOnly → FTS5 Search → Results
  ├─ SemanticOnly → Generate Embedding → Vector Search → Results
  └─ Hybrid → Parallel:
      ├─ FTS5 Search → topK results
      └─ Vector Search → topK results
      → RRF Fusion → Sorted Results → Enrich → Return
```

---

#### CResultRanker

**Responsibilities**:
- Implement ranking algorithms (RRF, Linear, Borda, CombMNZ)
- Score normalization
- Rank fusion

**RRF Algorithm**:
```cpp
float CalculateRRFScore(const RankMap& ranks, float k) {
  float score = 0.0f;
  for (const auto& [listId, rank] : ranks) {
    score += 1.0f / (k + rank);
  }
  return score;
}

std::vector<RankedItem> CombineRRF(
  const std::vector<std::pair<int64_t, float>>& list1,
  const std::vector<std::pair<int64_t, float>>& list2)
{
  // Build rank maps
  std::unordered_map<int64_t, int> rank1, rank2;
  for (size_t i = 0; i < list1.size(); ++i) {
    rank1[list1[i].first] = i + 1;  // 1-indexed
  }
  for (size_t i = 0; i < list2.size(); ++i) {
    rank2[list2[i].first] = i + 1;
  }

  // Calculate RRF scores
  std::unordered_map<int64_t, float> rrfScores;
  std::unordered_set<int64_t> allIds;

  for (const auto& [id, _] : list1) allIds.insert(id);
  for (const auto& [id, _] : list2) allIds.insert(id);

  for (int64_t id : allIds) {
    float score = 0.0f;
    if (rank1.count(id)) {
      score += 1.0f / (m_config.rrfK + rank1[id]);
    }
    if (rank2.count(id)) {
      score += 1.0f / (m_config.rrfK + rank2[id]);
    }
    rrfScores[id] = score;
  }

  // Sort by RRF score
  // ... (build RankedItem vector and sort)
}
```

---

#### CSemanticIndexService

**Responsibilities**:
- Indexing job queue
- Worker thread pool
- Progress tracking
- Error handling and retry

**Job Queue**:
```cpp
struct IndexingJob {
  std::string jobId;
  int mediaId;
  std::string mediaType;
  int priority;  // 0-10, higher = sooner
  std::chrono::system_clock::time_point queuedAt;
  std::chrono::system_clock::time_point startedAt;
  JobStatus status;  // QUEUED, IN_PROGRESS, COMPLETED, FAILED
  std::string errorMessage;
};

class CSemanticIndexService {
private:
  std::priority_queue<IndexingJob> m_jobQueue;
  std::vector<std::thread> m_workers;
  std::mutex m_queueMutex;
  std::condition_variable m_queueCV;

  void WorkerThread();
  void ProcessJob(const IndexingJob& job);

public:
  std::string QueueMediaIndex(int mediaId, const std::string& mediaType, int priority);
  IndexingProgress GetProgress(const std::string& jobId);
  bool CancelJob(const std::string& jobId);
};
```

**Worker Pool**:
- Configurable thread count (default: CPU cores)
- Priority-based job scheduling
- Automatic retry on failure (up to 3 times)

---

### Supporting Components

#### CPerformanceMonitor

**Singleton pattern** for global access:
```cpp
class CPerformanceMonitor {
private:
  CPerformanceMonitor() = default;
  static CPerformanceMonitor s_instance;

  mutable std::mutex m_mutex;
  PerformanceMetrics m_metrics;

public:
  static CPerformanceMonitor& GetInstance() {
    return s_instance;
  }

  void RecordEmbedding(double durationMs, size_t batchSize);
  void RecordSearch(double durationMs, const std::string& searchType);
  PerformanceMetrics GetMetrics() const;
};
```

**Metrics Collected**:
- Timing: embedding, FTS5 search, vector search, hybrid search
- Memory: model, cache, index, total
- Throughput: searches/sec, embeddings/sec
- Cache: hit rate, size, capacity

---

#### CQueryCache

**LRU Cache** with TTL:
```cpp
template<typename KeyType, typename ValueType>
class CLRUCache {
private:
  size_t m_maxSize;
  int m_ttlSeconds;

  struct LRUNode {
    KeyType key;
    ValueType value;
    std::chrono::steady_clock::time_point timestamp;
  };

  std::list<LRUNode> m_lruList;
  std::unordered_map<KeyType, typename std::list<LRUNode>::iterator> m_cache;

public:
  void Put(const KeyType& key, const ValueType& value);
  std::optional<ValueType> Get(const KeyType& key);
};
```

**Cache Hierarchy**:
1. **L1: Embedding Cache** - Stores query embeddings (1-hour TTL)
2. **L2: Result Cache** - Stores search results (5-minute TTL)
3. **L3: SQLite Page Cache** - OS-level disk cache

---

#### CSearchHistory

**Per-profile history tracking**:
```cpp
class CSearchHistory {
private:
  CSemanticDatabase* m_database;
  bool m_privacyMode{false};
  int m_maxHistorySize{100};

public:
  bool AddSearch(const std::string& queryText, int resultCount);
  std::vector<SearchHistoryEntry> GetRecentSearches(int limit, int profileId);
  std::vector<SearchHistoryEntry> GetSearchesByPrefix(const std::string& prefix);
  bool ClearHistory(int profileId = -1);
};
```

**Privacy Considerations**:
- History stored per-profile (isolated)
- Privacy mode disables recording
- Clear history option
- No cloud sync (local only)

---

## Data Flow

### Search Query Flow

```
User enters query: "detective solving mystery"
       │
       ▼
┌──────────────────────────────────────────────┐
│ 1. Query Normalization                       │
│    - Lowercase                                │
│    - Trim whitespace                          │
│    - Remove special chars                     │
│    Result: "detective solving mystery"        │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 2. Cache Check                                │
│    Hash(query + options) → cache key          │
│    Cache hit? → Return cached results         │
│    Cache miss? → Continue                     │
└──────────────────────────────────────────────┘
       │ (cache miss)
       ▼
┌──────────────────────────────────────────────┐
│ 3. Parallel Search (Hybrid Mode)             │
│                                               │
│    ┌─────────────────┐  ┌─────────────────┐  │
│    │ FTS5 Search     │  │ Vector Search   │  │
│    │                 │  │                 │  │
│    │ 1. Build FTS5   │  │ 1. Check cache  │  │
│    │    query        │  │ 2. Generate     │  │
│    │ 2. Execute      │  │    embedding    │  │
│    │    SELECT...    │  │ 3. k-NN search  │  │
│    │    MATCH        │  │ 4. Retrieve     │  │
│    │ 3. Get topK=100 │  │    topK=100     │  │
│    │                 │  │                 │  │
│    └─────────────────┘  └─────────────────┘  │
│           │                      │            │
│           └──────────┬───────────┘            │
│                      ▼                        │
│           ┌──────────────────────┐            │
│           │ RRF Fusion           │            │
│           │ - Rank-based merge   │            │
│           │ - Score calculation  │            │
│           │ - Sort by RRF score  │            │
│           └──────────────────────┘            │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 4. Result Enrichment                          │
│    For each result:                           │
│    - Fetch full chunk data                    │
│    - Generate snippet with highlights         │
│    - Format timestamp (HH:MM:SS)              │
│    - Add media metadata (title, year, etc.)   │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 5. Filter Application                         │
│    - Media type filter                        │
│    - Genre filter (OR within list)            │
│    - Year range filter                        │
│    - Rating filter                            │
│    - Duration filter                          │
│    - Source filter                            │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 6. Cache & Return                             │
│    - Store in result cache (5min TTL)         │
│    - Return top maxResults (default: 20)      │
└──────────────────────────────────────────────┘
       │
       ▼
    Results displayed to user
```

---

### Indexing Flow

```
New movie added: "Inception" (ID: 256)
       │
       ▼
┌──────────────────────────────────────────────┐
│ 1. Queue Indexing Job                        │
│    - Create IndexingJob                       │
│    - Set priority (user-initiated = 5)        │
│    - Add to priority queue                    │
│    - Notify worker threads                    │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 2. Worker Picks Up Job                       │
│    - Thread from pool acquires job            │
│    - Update status: IN_PROGRESS               │
│    - Load media metadata from VideoDatabase  │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 3. Content Extraction                         │
│                                               │
│    ┌─────────────┐  ┌─────────────┐          │
│    │ Subtitles   │  │ Metadata    │          │
│    │             │  │             │          │
│    │ 1. Find SRT │  │ 1. Title    │          │
│    │ 2. Parse    │  │ 2. Plot     │          │
│    │ 3. Extract  │  │ 3. Cast     │          │
│    │    lines    │  │ 4. Tags     │          │
│    │ 4. Timestamps│ │             │          │
│    └─────────────┘  └─────────────┘          │
│           │                 │                 │
│           └────────┬────────┘                 │
│                    ▼                          │
│         ┌──────────────────────┐              │
│         │ Raw Content Chunks   │              │
│         │ (text + timestamps)  │              │
│         └──────────────────────┘              │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 4. Chunk Processing                           │
│    For each chunk:                            │
│    - Insert into semantic_chunks table        │
│    - Insert into FTS5 index                   │
│                                               │
│    Example:                                   │
│    Chunk #1: [00:01:23-00:01:28]              │
│    Text: "We need to go deeper"               │
│    Source: subtitle                           │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 5. Embedding Generation                       │
│    - Collect all chunk texts                  │
│    - Batch embed (100 chunks at a time)       │
│    - CEmbeddingEngine::EmbedBatch()           │
│                                               │
│    Batch 1 (100 chunks): ~1.5 seconds         │
│    Batch 2 (100 chunks): ~1.5 seconds         │
│    ...                                        │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 6. Vector Insertion                           │
│    - Pair (chunk_id, embedding)               │
│    - Batch insert into semantic_vectors       │
│    - CVectorSearcher::InsertVectorBatch()     │
│                                               │
│    All vectors in single transaction          │
└──────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────┐
│ 7. Finalize                                   │
│    - Update index_state table                 │
│    - Set status: COMPLETED                    │
│    - Record completion time                   │
│    - Update job in queue                      │
│    - Emit completion event                    │
└──────────────────────────────────────────────┘
       │
       ▼
    Media is now searchable
```

---

## Database Schema

### Core Tables

#### semantic_chunks

```sql
CREATE TABLE semantic_chunks (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  media_id INTEGER NOT NULL,
  media_type TEXT NOT NULL,  -- 'movie', 'episode', 'musicvideo'
  source_type TEXT NOT NULL,  -- 'subtitle', 'transcription', 'metadata'
  source_path TEXT,
  start_ms INTEGER NOT NULL,
  end_ms INTEGER NOT NULL,
  text TEXT NOT NULL,
  language TEXT,
  confidence REAL DEFAULT 1.0,
  created_at TEXT DEFAULT CURRENT_TIMESTAMP,

  FOREIGN KEY (media_id) REFERENCES movie(idMovie)  -- Conceptual
);

CREATE INDEX idx_chunks_media ON semantic_chunks(media_id, media_type);
CREATE INDEX idx_chunks_timestamp ON semantic_chunks(start_ms, end_ms);
CREATE INDEX idx_chunks_source ON semantic_chunks(source_type);
```

**Typical Row**:
```
id: 4521
media_id: 142
media_type: movie
source_type: subtitle
source_path: /path/to/Inception.srt
start_ms: 83000
end_ms: 88000
text: We need to go deeper
language: en
confidence: 1.0
created_at: 2024-11-20T14:32:15Z
```

---

#### semantic_chunks_fts

**FTS5 Virtual Table**:
```sql
CREATE VIRTUAL TABLE semantic_chunks_fts USING fts5(
  text,
  content=semantic_chunks,
  content_rowid=id,
  tokenize='unicode61 remove_diacritics 1'
);

-- Triggers to keep FTS5 in sync
CREATE TRIGGER chunks_fts_insert AFTER INSERT ON semantic_chunks BEGIN
  INSERT INTO semantic_chunks_fts(rowid, text) VALUES (new.id, new.text);
END;

CREATE TRIGGER chunks_fts_delete AFTER DELETE ON semantic_chunks BEGIN
  DELETE FROM semantic_chunks_fts WHERE rowid = old.id;
END;

CREATE TRIGGER chunks_fts_update AFTER UPDATE ON semantic_chunks BEGIN
  UPDATE semantic_chunks_fts SET text = new.text WHERE rowid = new.id;
END;
```

**Search Query**:
```sql
SELECT rowid, rank
FROM semantic_chunks_fts
WHERE text MATCH 'detective* solving* mystery*'
ORDER BY rank
LIMIT 100;
```

---

#### semantic_vectors

**Vector Virtual Table** (sqlite-vec):
```sql
CREATE VIRTUAL TABLE semantic_vectors USING vec0(
  chunk_id INTEGER PRIMARY KEY,
  embedding float[384]
);
```

**Insertion**:
```sql
INSERT INTO semantic_vectors (chunk_id, embedding)
VALUES (?, ?);  -- ? = chunk_id, ? = blob of 384 floats
```

**Search**:
```sql
SELECT chunk_id, distance
FROM semantic_vectors
WHERE embedding MATCH ?  -- ? = query embedding blob
ORDER BY distance
LIMIT 100;
```

**Internal Structure** (HNSW graph for large indexes):
- Nodes: chunk_id + embedding
- Edges: Nearest neighbors in embedding space
- Layers: Hierarchical graph for fast search

---

#### index_state

```sql
CREATE TABLE index_state (
  media_id INTEGER NOT NULL,
  media_type TEXT NOT NULL,
  subtitle_status TEXT DEFAULT 'not_started',
  transcription_status TEXT DEFAULT 'not_started',
  metadata_status TEXT DEFAULT 'not_started',
  subtitle_chunk_count INTEGER DEFAULT 0,
  transcription_chunk_count INTEGER DEFAULT 0,
  metadata_chunk_count INTEGER DEFAULT 0,
  subtitle_indexed_at TEXT,
  transcription_indexed_at TEXT,
  metadata_indexed_at TEXT,
  has_embeddings BOOLEAN DEFAULT 0,
  embedding_count INTEGER DEFAULT 0,
  last_updated TEXT DEFAULT CURRENT_TIMESTAMP,

  PRIMARY KEY (media_id, media_type)
);
```

**Status Values**:
- `not_started`: Not yet indexed
- `queued`: In indexing queue
- `in_progress`: Currently indexing
- `completed`: Indexing complete
- `failed`: Indexing failed
- `partial`: Some chunks indexed, some failed

---

#### semantic_search_history

```sql
CREATE TABLE semantic_search_history (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  profile_id INTEGER NOT NULL,
  query_text TEXT NOT NULL,
  result_count INTEGER,
  timestamp INTEGER NOT NULL,  -- Unix timestamp
  clicked_results TEXT,  -- JSON array of chunk_ids

  FOREIGN KEY (profile_id) REFERENCES profiles(idProfile)
);

CREATE INDEX idx_history_profile ON semantic_search_history(profile_id, timestamp DESC);
CREATE INDEX idx_history_query ON semantic_search_history(query_text);
```

**Example Row**:
```
id: 42
profile_id: 1
query_text: detective solving mystery
result_count: 8
timestamp: 1732115535
clicked_results: [4521, 6789]
```

---

### Schema Migrations

**Version tracking**:
```sql
CREATE TABLE schema_version (
  version INTEGER PRIMARY KEY,
  applied_at TEXT DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO schema_version (version) VALUES (2);  -- Current: Phase 2
```

**Migration Path**:
- **Version 0** → **Version 1**: Add FTS5 index
- **Version 1** → **Version 2**: Add vector table, embeddings
- **Version 2** → **Version 3** (future): Add cross-encoder re-ranking

**Migration Example**:
```cpp
bool CSemanticDatabase::MigrateToV2() {
  BeginTransaction();

  try {
    // Create vector table
    Execute("CREATE VIRTUAL TABLE semantic_vectors USING vec0(...)");

    // Add embedding columns
    Execute("ALTER TABLE index_state ADD COLUMN has_embeddings BOOLEAN");
    Execute("ALTER TABLE index_state ADD COLUMN embedding_count INTEGER");

    // Update version
    Execute("INSERT INTO schema_version (version) VALUES (2)");

    CommitTransaction();
    return true;
  } catch (...) {
    RollbackTransaction();
    return false;
  }
}
```

---

## Search Pipeline

### Query Normalization

**Steps**:
1. **Lowercase**: `"Detective"` → `"detective"`
2. **Trim**: `"  detective  "` → `"detective"`
3. **Deduplicate whitespace**: `"detective   mystery"` → `"detective mystery"`
4. **Remove FTS5 special chars**: `"car-chase"` → `"car chase"`

**Implementation**:
```cpp
std::string NormalizeQuery(const std::string& query) {
  std::string normalized = query;

  // Lowercase
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Trim
  normalized = StringUtils::Trim(normalized);

  // Remove FTS5 operators
  static const std::string operators = "\"+-*():^";
  for (char op : operators) {
    normalized.erase(std::remove(normalized.begin(), normalized.end(), op),
                     normalized.end());
  }

  return normalized;
}
```

---

### FTS5 Query Building

**Process**:
1. Split into words: `"detective solving"` → `["detective", "solving"]`
2. Escape special chars (if any remain)
3. Add wildcard suffix: `["detective*", "solving*"]`
4. Join: `"detective* solving*"`

**Why wildcards?**
- Enables partial matching: "detect" matches "detective"
- Better user experience (no need for exact spelling)

**Code**:
```cpp
std::string BuildFTS5Query(const std::string& normalized) {
  std::vector<std::string> words = StringUtils::Split(normalized, " ");

  std::vector<std::string> ftsWords;
  for (const auto& word : words) {
    if (!word.empty()) {
      ftsWords.push_back(word + "*");
    }
  }

  return StringUtils::Join(ftsWords, " ");
}
```

---

### Vector Search Process

**Workflow**:
```cpp
std::vector<VectorResult> VectorSearch(const std::string& query, int topK) {
  // 1. Check embedding cache
  auto cachedEmbedding = cache.GetEmbedding(query);

  Embedding queryEmbedding;
  if (cachedEmbedding.has_value()) {
    queryEmbedding = cachedEmbedding.value();
  } else {
    // 2. Generate embedding
    queryEmbedding = embeddingEngine.Embed(query);

    // 3. Cache for future use
    cache.PutEmbedding(query, queryEmbedding);
  }

  // 4. Perform k-NN search
  auto results = vectorSearcher.SearchSimilar(queryEmbedding, topK);

  // 5. Convert distance to similarity
  for (auto& result : results) {
    result.similarity = 1.0f - result.distance;
  }

  return results;
}
```

---

### RRF Fusion

**Mathematical Formula**:
```
RRF(d) = Σ [ 1 / (k + rank_i(d)) ]

Where:
- d = document/chunk
- k = constant (typically 60)
- rank_i(d) = rank of d in list i (1-indexed)
- Σ = sum over all lists where d appears
```

**Example**:
```
Keyword results: [A(#1), B(#2), C(#3)]
Vector results:  [B(#1), D(#2), A(#3)]
k = 60

RRF(A) = 1/(60+1) + 1/(60+3) = 0.0164 + 0.0159 = 0.0323
RRF(B) = 1/(60+2) + 1/(60+1) = 0.0161 + 0.0164 = 0.0325 ← highest
RRF(C) = 1/(60+3) + 0 = 0.0159
RRF(D) = 0 + 1/(60+2) = 0.0161

Final ranking: B, A, D, C
```

**Why RRF?**
- Scale-independent (doesn't care about raw scores)
- Handles different score ranges (BM25 vs cosine)
- No normalization needed
- Proven effective in IR research

---

## Indexing Pipeline

### Subtitle Parsing

**Supported Formats**:
- SRT (SubRip)
- ASS/SSA (Advanced SubStation Alpha)
- VTT (WebVTT)

**SRT Parser**:
```cpp
std::vector<SubtitleChunk> ParseSRT(const std::string& filePath) {
  std::vector<SubtitleChunk> chunks;
  std::ifstream file(filePath);
  std::string line;

  while (std::getline(file, line)) {
    // Parse index number
    int index = std::stoi(line);

    // Parse timestamps
    std::getline(file, line);
    auto [startMs, endMs] = ParseTimestamp(line);

    // Parse text (multiple lines until blank)
    std::string text;
    while (std::getline(file, line) && !line.empty()) {
      text += line + " ";
    }

    chunks.push_back({index, startMs, endMs, text});
  }

  return chunks;
}
```

**Timestamp Format**: `00:01:23,456 --> 00:01:28,789`

---

### Chunking Strategy

**Goals**:
- Keep chunks small enough for relevance
- Keep chunks large enough for context
- Respect natural boundaries (sentences, subtitles)

**Subtitle Chunking**:
- Each subtitle = 1 chunk
- Typical duration: 2-5 seconds
- Typical length: 5-20 words

**Transcription Chunking**:
- Based on silence detection or fixed duration
- Typical duration: 10-30 seconds
- Typical length: 50-150 words

**Metadata Chunking**:
- Title: 1 chunk
- Plot/description: 1-3 chunks (split at sentence boundaries if >200 words)
- Cast/crew: 1 chunk per person (with role)

---

### Embedding Generation

**Batch Processing**:
```cpp
void IndexMedia(int mediaId, const std::string& mediaType) {
  // 1. Extract chunks
  auto chunks = ExtractChunks(mediaId, mediaType);

  // 2. Insert chunks into database
  std::vector<int64_t> chunkIds;
  for (const auto& chunk : chunks) {
    int64_t chunkId = database.InsertChunk(chunk);
    chunkIds.push_back(chunkId);
  }

  // 3. Generate embeddings in batches
  constexpr size_t BATCH_SIZE = 100;
  std::vector<std::pair<int64_t, Embedding>> vectors;

  for (size_t i = 0; i < chunks.size(); i += BATCH_SIZE) {
    size_t batchEnd = std::min(i + BATCH_SIZE, chunks.size());

    // Extract batch texts
    std::vector<std::string> batchTexts;
    for (size_t j = i; j < batchEnd; ++j) {
      batchTexts.push_back(chunks[j].text);
    }

    // Generate batch embeddings
    auto embeddings = embeddingEngine.EmbedBatch(batchTexts);

    // Pair with chunk IDs
    for (size_t j = 0; j < embeddings.size(); ++j) {
      vectors.push_back({chunkIds[i + j], embeddings[j]});
    }
  }

  // 4. Insert vectors in single transaction
  vectorSearcher.InsertVectorBatch(vectors);

  // 5. Update index state
  database.UpdateIndexState(mediaId, mediaType, IndexStatus::COMPLETED);
}
```

**Performance**:
- Batch size 100: ~1.5 seconds per batch
- 1000 chunks: ~15 seconds total embedding time
- Plus database I/O: ~5 seconds
- **Total**: ~20-25 seconds for typical movie

---

## Extension Points

### Custom Tokenizers

**Interface**:
```cpp
class ITokenizer {
public:
  virtual ~ITokenizer() = default;

  virtual std::vector<int> Tokenize(const std::string& text) = 0;
  virtual std::string Detokenize(const std::vector<int>& tokens) = 0;
  virtual int GetVocabSize() const = 0;
};
```

**Usage**:
```cpp
class CustomTokenizer : public ITokenizer {
  // Implement custom tokenization logic
};

embeddingEngine.SetTokenizer(std::make_unique<CustomTokenizer>());
```

---

### Custom Embedding Models

**Requirements**:
- ONNX format (.onnx file)
- Output: float32 tensor [batch_size, embedding_dim]
- Compatible with ONNX Runtime

**Configuration**:
```xml
<embeddingmodel>
  <modelpath>special://masterprofile/semantic/models/custom-model.onnx</modelpath>
  <vocabpath>special://masterprofile/semantic/models/custom-vocab.txt</vocabpath>
  <embeddingdim>384</embeddingdim>  <!-- Must match model output -->
</embeddingmodel>
```

---

### Custom Ranking Algorithms

**Interface**:
```cpp
// Extend CResultRanker with custom algorithm
class CResultRanker {
public:
  enum class RankingAlgorithm {
    RRF, Linear, Borda, CombMNZ,
    Custom  // Add custom type
  };

  void SetCustomCombiner(
    std::function<std::vector<RankedItem>(
      const std::vector<std::pair<int64_t, float>>&,
      const std::vector<std::pair<int64_t, float>>&)> combiner);
};
```

**Example**:
```cpp
ranker.SetCustomCombiner([](const auto& list1, const auto& list2) {
  // Custom ranking logic
  return customRankedResults;
});
```

---

### Custom Content Parsers

**Interface**:
```cpp
class IContentParser {
public:
  virtual ~IContentParser() = default;

  virtual bool CanParse(const std::string& filePath) const = 0;
  virtual std::vector<SemanticChunk> Parse(const std::string& filePath) = 0;
};
```

**Example** (custom subtitle format):
```cpp
class CustomSubtitleParser : public IContentParser {
public:
  bool CanParse(const std::string& filePath) const override {
    return StringUtils::EndsWith(filePath, ".custom");
  }

  std::vector<SemanticChunk> Parse(const std::string& filePath) override {
    // Custom parsing logic
  }
};

// Register parser
indexService.RegisterContentParser(std::make_unique<CustomSubtitleParser>());
```

---

### Custom Filters

**Create filter class**:
```cpp
class CustomFilter {
public:
  bool Apply(const HybridSearchResult& result) const {
    // Custom filter logic
    return meetsCondition;
  }
};
```

**Use in search**:
```cpp
auto results = searchEngine.Search(query, options);

CustomFilter filter;
auto filtered = FilterResults(results, filter);
```

---

## Security Considerations

### Input Validation

**Query Length**:
```cpp
constexpr size_t MAX_QUERY_LENGTH = 1000;

if (query.length() > MAX_QUERY_LENGTH) {
  throw std::invalid_argument("Query too long");
}
```

**SQL Injection Prevention**:
- All database queries use prepared statements
- No raw SQL from user input

**Path Traversal Prevention**:
```cpp
std::string SanitizePath(const std::string& path) {
  // Remove ".." and ensure within allowed directories
  if (path.find("..") != std::string::npos) {
    throw std::invalid_argument("Invalid path");
  }
  return path;
}
```

---

### API Authentication

**JSON-RPC**:
- HTTP Basic Auth (if enabled in settings)
- Optional API key for external clients
- Rate limiting to prevent abuse

**Internal**:
- No authentication needed (same process)

---

### Privacy

**Data Storage**:
- All data stored locally (no cloud sync)
- Per-profile isolation
- Privacy mode disables history

**Transcription**:
- User choice: local or cloud provider
- API keys stored encrypted
- Cost tracking and budget limits

---

## Scalability

### Dataset Size Limits

| Component | Small | Medium | Large | Very Large |
|-----------|-------|--------|-------|------------|
| **Chunks** | <10K | 10K-100K | 100K-1M | >1M |
| **FTS5** | <1ms | 5-20ms | 20-100ms | 100-500ms |
| **Vector (brute)** | 5ms | 50ms | N/A | N/A |
| **Vector (HNSW)** | N/A | N/A | 20-40ms | 40-100ms |
| **DB Size** | <10 MB | 10-100 MB | 100MB-1GB | >1GB |
| **Memory** | 200 MB | 300 MB | 500 MB | 1+ GB |

---

### Performance Optimization Strategies

**For Large Libraries (>100K chunks)**:

1. **Increase database cache**:
   ```xml
   <database>
     <cachesize>-50000</cachesize>  <!-- 50 MB -->
   </database>
   ```

2. **Use filtered search**:
   ```cpp
   // Filter candidates first, then search
   opts.mediaType = "movie";  // Reduces search space
   ```

3. **Periodic index maintenance**:
   ```cpp
   // Weekly during off-hours
   database.RebuildFTSIndex();
   vectorSearcher.RebuildIndex();
   database.Analyze();
   ```

4. **Consider partitioning** (future):
   - Separate databases per media type
   - Shard by year/decade

---

## Future Roadmap

### Phase 3: Advanced Features

**1. GPU Acceleration**
- CUDA/OpenCL for embedding generation
- 5-10x speedup on compatible hardware
- Fallback to CPU if unavailable

**2. Cross-Encoder Re-ranking**
- More accurate but slower re-ranking
- Use as final stage on top-N results
- 2-5% relevance improvement

**3. Query Expansion**
- Automatic synonym expansion
- Concept-based expansion using embeddings
- Improves recall for vague queries

**4. Multilingual Support**
- Multilingual embedding models
- Language-specific tokenizers
- Cross-lingual search (query in English, find Spanish content)

---

### Phase 4: Intelligence & Learning

**1. Relevance Feedback**
- Learn from clicked results
- Personalize ranking per user
- Improve query understanding

**2. Auto-categorization**
- Cluster similar content
- Suggest tags/categories
- Content discovery

**3. Contextual Search**
- "Find more like this scene"
- Temporal context awareness
- Character/actor tracking

---

### Phase 5: Platform & Integration

**1. Mobile Optimization**
- Quantized models (smaller, faster)
- Reduced memory footprint
- Offline-first design

**2. Cloud Sync** (Optional)
- Sync search history across devices
- Backup embeddings (save indexing time)
- Privacy-preserving design

**3. Third-Party Integrations**
- Plugin API for external services
- Custom embedding providers
- External knowledge bases

---

*For user guide, see [GUIGuide.md](GUIGuide.md)*
*For API reference, see [APIReference.md](APIReference.md)*
*For performance tuning, see [PerformanceTuning.md](PerformanceTuning.md)*

---

*Last Updated: 2025-11-25*
*Phase 2 Technical Design Complete*
