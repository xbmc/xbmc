# TDD: Semantic Vector Search - Technical Design Document

## Document Information

| Field | Value |
|-------|-------|
| **Feature** | Semantic Vector Search & Smart Discovery |
| **Related PRD** | `prd-2.md` |
| **Depends On** | PR #1 (prd-1.md, tdd-1-semantic-index.md) |
| **Version** | 1.0 |

> This document contains implementation details, code samples, and technical specifications extracted from the PRD. For requirements and user stories, see `prd-2.md`.

---

## 1. Directory Structure

```
xbmc/
├── semantic/
│   │
│   │   # Existing from PR #1
│   ├── SemanticIndexService.h/.cpp
│   ├── SemanticDatabase.h/.cpp
│   ├── SemanticSearch.h/.cpp
│   │
│   │   # NEW in PR #2
│   ├── embedding/
│   │   ├── EmbeddingEngine.h
│   │   ├── EmbeddingEngine.cpp          # ONNX model loading and inference (~400 lines)
│   │   ├── Tokenizer.h
│   │   ├── Tokenizer.cpp                # WordPiece tokenization (~300 lines)
│   │   ├── ModelManager.h
│   │   └── ModelManager.cpp             # Model download/versioning (~200 lines)
│   │
│   ├── search/
│   │   ├── HybridSearchEngine.h
│   │   ├── HybridSearchEngine.cpp       # Combined FTS+vector search (~500 lines)
│   │   ├── VectorSearcher.h
│   │   ├── VectorSearcher.cpp           # sqlite-vec queries (~200 lines)
│   │   ├── ResultRanker.h
│   │   └── ResultRanker.cpp             # RRF implementation (~150 lines)
│   │
│   └── models/
│       └── vocab.txt                    # WordPiece vocabulary file
│
├── dialogs/
│   ├── GUIDialogSemanticSearch.h
│   └── GUIDialogSemanticSearch.cpp      # Main search dialog (~800 lines)
│
├── windows/
│   └── GUIWindowSemanticResults.h/.cpp  # Full-screen results view (~400 lines)
│
└── lib/
    └── sqlite-vec/
        ├── sqlite-vec.h
        └── sqlite-vec.c                  # Vendored sqlite-vec source

addons/
└── skin.estuary/
    └── xml/
        ├── DialogSemanticSearch.xml     # Search dialog layout
        ├── MySemanticResults.xml        # Results window layout
        └── Includes_SemanticWidgets.xml # Reusable result item templates

Total new code in PR #2: ~3,500 lines
Combined with PR #1: ~8,500 lines total
```

---

## 2. Class Relationships

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          CLASS RELATIONSHIPS                             │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────┐         ┌─────────────────────┐
│ GUIDialogSemantic   │────────▶│ CHybridSearchEngine │
│ Search              │         │                     │
├─────────────────────┤         ├─────────────────────┤
│ - m_searchEngine    │         │ - m_ftsSearcher     │
│ - m_results         │         │ - m_vectorSearcher  │
│ - m_searchHistory   │         │ - m_ranker          │
├─────────────────────┤         │ - m_embedder        │
│ + OnSearch()        │         ├─────────────────────┤
│ + OnResultSelect()  │         │ + Search()          │
│ + OnPlayFromTime()  │         │ + SearchWithFilters │
└─────────────────────┘         └──────────┬──────────┘
                                           │
                    ┌──────────────────────┼──────────────────────┐
                    │                      │                      │
                    ▼                      ▼                      ▼
         ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
         │ CFTSSearcher    │    │CVectorSearcher  │    │ CResultRanker   │
         │ (from PR #1)    │    │                 │    │                 │
         ├─────────────────┤    ├─────────────────┤    ├─────────────────┤
         │ + Search()      │    │ - m_database    │    │ + RankRRF()     │
         │ + BuildQuery()  │    ├─────────────────┤    │ + Deduplicate() │
         └─────────────────┘    │ + SearchSimilar │    │ + MergeResults()│
                                │ + GetTopK()     │    └─────────────────┘
                                └────────┬────────┘
                                         │
                                         ▼
                                ┌─────────────────┐
                                │CEmbeddingEngine │
                                ├─────────────────┤
                                │ - m_session     │ (ONNX Runtime)
                                │ - m_tokenizer   │
                                ├─────────────────┤
                                │ + Embed()       │
                                │ + EmbedBatch()  │
                                │ + LoadModel()   │
                                └────────┬────────┘
                                         │
                                         ▼
                                ┌─────────────────┐
                                │   CTokenizer    │
                                ├─────────────────┤
                                │ - m_vocab       │
                                ├─────────────────┤
                                │ + Tokenize()    │
                                │ + Encode()      │
                                └─────────────────┘
```

---

## 3. Database Schema

### 3.1 Vector Storage Schema

```sql
-- ============================================================================
-- VECTOR EMBEDDING STORAGE (PR #2 additions)
-- ============================================================================

-- sqlite-vec virtual table for vector similarity search
-- Requires sqlite-vec extension to be loaded
CREATE VIRTUAL TABLE semantic_vectors USING vec0(
    chunk_id INTEGER PRIMARY KEY,
    embedding FLOAT[384] distance_metric=cosine
);

-- Embedding generation state (extends semantic_index_state from PR #1)
ALTER TABLE semantic_index_state ADD COLUMN embedding_status TEXT
    DEFAULT 'pending' CHECK (embedding_status IN (
        'pending', 'processing', 'complete', 'failed'
    ));
ALTER TABLE semantic_index_state ADD COLUMN embedding_progress REAL DEFAULT 0;
ALTER TABLE semantic_index_state ADD COLUMN embedding_error TEXT;
ALTER TABLE semantic_index_state ADD COLUMN embeddings_count INTEGER DEFAULT 0;

-- Model versioning for future upgrades
CREATE TABLE semantic_embedding_models (
    model_id TEXT PRIMARY KEY,
    model_name TEXT NOT NULL,
    model_version TEXT NOT NULL,
    dimensions INTEGER NOT NULL,
    model_path TEXT,
    vocab_path TEXT,
    is_active INTEGER DEFAULT 0,
    downloaded_at INTEGER,
    file_hash TEXT
);

INSERT INTO semantic_embedding_models
    (model_id, model_name, model_version, dimensions, is_active)
VALUES
    ('minilm-l6-v2', 'all-MiniLM-L6-v2', '2.0', 384, 1);

-- Search history for suggestions and analytics
CREATE TABLE semantic_search_history (
    search_id INTEGER PRIMARY KEY AUTOINCREMENT,
    query TEXT NOT NULL,
    query_normalized TEXT NOT NULL,
    result_count INTEGER,
    clicked_chunk_id INTEGER,
    clicked_media_id INTEGER,
    search_duration_ms INTEGER,
    searched_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

CREATE INDEX idx_search_history_query ON semantic_search_history(query_normalized);
CREATE INDEX idx_search_history_time ON semantic_search_history(searched_at DESC);

-- Precomputed query suggestions
CREATE TABLE semantic_suggestions (
    suggestion_id INTEGER PRIMARY KEY AUTOINCREMENT,
    text TEXT NOT NULL UNIQUE,
    frequency INTEGER DEFAULT 1,
    last_used INTEGER
);
```

### 3.2 sqlite-vec Integration

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     sqlite-vec OVERVIEW                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  sqlite-vec is a SQLite extension for vector similarity search:         │
│  - https://github.com/asg017/sqlite-vec                                 │
│  - Single C file, easy to embed                                         │
│  - Supports cosine, L2, and inner product distance                      │
│  - Approximate nearest neighbor (ANN) for scale                         │
│  - ~50KB compiled size                                                  │
│                                                                          │
│  Integration approach:                                                   │
│  1. Bundle sqlite-vec source with Kodi                                  │
│  2. Load as extension when opening SemanticIndex.db                     │
│  3. Create virtual table for vector storage                             │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

```sql
-- Virtual table for vector storage and search
CREATE VIRTUAL TABLE semantic_vectors USING vec0(
    chunk_id INTEGER PRIMARY KEY,
    embedding FLOAT[384] distance_metric=cosine
);

-- Insert embedding
INSERT INTO semantic_vectors (chunk_id, embedding)
VALUES (123, vec_f32(?));  -- ? is float array

-- Similarity search
SELECT
    chunk_id,
    distance
FROM semantic_vectors
WHERE embedding MATCH vec_f32(?)  -- query vector
  AND k = 50                       -- top 50 results
ORDER BY distance;

-- With filtering (requires join)
SELECT
    v.chunk_id,
    v.distance,
    c.text,
    c.start_ms
FROM semantic_vectors v
JOIN semantic_chunks c ON v.chunk_id = c.chunk_id
WHERE v.embedding MATCH vec_f32(?)
  AND c.media_type = 'movie'
  AND k = 100
ORDER BY v.distance
LIMIT 20;
```

### 3.3 Query Examples

```sql
-- Hybrid search query (simplified)
WITH fts_results AS (
    SELECT
        c.chunk_id,
        c.media_id,
        c.text,
        c.start_ms,
        bm25(semantic_fts) AS fts_score,
        ROW_NUMBER() OVER (ORDER BY bm25(semantic_fts)) AS fts_rank
    FROM semantic_fts f
    JOIN semantic_chunks c ON f.rowid = c.chunk_id
    WHERE semantic_fts MATCH ?
    LIMIT 50
),
vec_results AS (
    SELECT
        v.chunk_id,
        v.distance AS vec_score,
        ROW_NUMBER() OVER (ORDER BY v.distance) AS vec_rank
    FROM semantic_vectors v
    WHERE v.embedding MATCH vec_f32(?)
      AND k = 50
)
SELECT
    COALESCE(f.chunk_id, v.chunk_id) AS chunk_id,
    f.media_id,
    f.text,
    f.start_ms,
    -- RRF score calculation
    COALESCE(0.4 / (60.0 + f.fts_rank), 0) +
    COALESCE(0.6 / (60.0 + v.vec_rank), 0) AS combined_score
FROM fts_results f
FULL OUTER JOIN vec_results v ON f.chunk_id = v.chunk_id
ORDER BY combined_score DESC
LIMIT 20;

-- Get chunk context (surrounding text)
SELECT
    c.*,
    LAG(c.text, 1) OVER (ORDER BY c.start_ms) AS prev_text,
    LEAD(c.text, 1) OVER (ORDER BY c.start_ms) AS next_text
FROM semantic_chunks c
WHERE c.media_id = ?
  AND c.start_ms BETWEEN ? - 30000 AND ? + 30000
ORDER BY c.start_ms;

-- Search suggestions based on history
SELECT text, frequency
FROM semantic_suggestions
WHERE text LIKE ? || '%'
ORDER BY frequency DESC
LIMIT 10;
```

---

## 4. C++ API Design

### 4.1 HybridSearchEngine.h

```cpp
// ============================================================================
// HybridSearchEngine.h - Combined semantic + keyword search
// ============================================================================

#pragma once

#include "SemanticSearch.h"  // From PR #1
#include "embedding/EmbeddingEngine.h"

namespace KODI::SEMANTIC
{

// Extended search options for hybrid search
struct HybridSearchOptions : public SearchOptions
{
    // Search mode
    enum class Mode
    {
        Hybrid,      // Combined FTS + vector (default)
        KeywordOnly, // FTS only (faster, exact matches)
        SemanticOnly // Vector only (conceptual matches)
    };
    Mode mode = Mode::Hybrid;

    // Ranking weights (must sum to 1.0)
    float ftsWeight = 0.4f;
    float vectorWeight = 0.6f;

    // RRF parameter
    float rrfK = 60.0f;

    // Result enrichment
    bool includeContext = true;
    int contextChunks = 2;        // Chunks before/after match

    // Feedback for ranking adjustment
    bool usePersonalization = false;
};

struct EnrichedSearchResult : public SearchResult
{
    // Additional context
    std::string contextBefore;
    std::string contextAfter;

    // Score breakdown
    float ftsScore = 0.0f;
    float vectorScore = 0.0f;
    float combinedScore = 0.0f;

    // For display
    std::string formattedTimestamp;    // "1:23:45"
    std::string thumbnailPath;
    std::string mediaTitle;
    std::string showTitle;             // For episodes
    int seasonNumber = 0;
    int episodeNumber = 0;
};

class CHybridSearchEngine
{
public:
    CHybridSearchEngine();
    ~CHybridSearchEngine();

    /// Initialize engine (load embedding model)
    bool Initialize();

    /// Check if semantic search is available
    bool IsSemanticAvailable() const;

    /// Get embedding model info
    std::string GetModelName() const;
    int GetEmbeddingDimensions() const;

    // ========================================================================
    // Search Operations
    // ========================================================================

    /// Perform hybrid semantic + keyword search
    std::vector<EnrichedSearchResult> Search(
        const std::string& query,
        const HybridSearchOptions& options = {});

    /// Search with pre-computed query embedding (for repeated searches)
    std::vector<EnrichedSearchResult> SearchWithEmbedding(
        const std::string& query,
        const Embedding& queryEmbedding,
        const HybridSearchOptions& options = {});

    /// Generate embedding for a query (for external use)
    Embedding EmbedQuery(const std::string& query);

    // ========================================================================
    // Suggestions
    // ========================================================================

    /// Get search suggestions based on query prefix and history
    std::vector<std::string> GetSuggestions(
        const std::string& prefix,
        int maxResults = 10);

    /// Record search for history/suggestions
    void RecordSearch(
        const std::string& query,
        int resultCount,
        int selectedChunkId = -1);

    // ========================================================================
    // Context Retrieval
    // ========================================================================

    /// Get full context around a search result
    std::vector<EnrichedSearchResult> GetExpandedContext(
        int chunkId,
        int windowMs = 120000);  // 2 minutes

    /// Get semantically similar chunks (for "more like this")
    std::vector<EnrichedSearchResult> FindSimilar(
        int chunkId,
        int maxResults = 10);

private:
    std::unique_ptr<CEmbeddingEngine> m_embedder;
    std::unique_ptr<CSemanticDatabase> m_database;

    // Search helpers
    std::vector<SearchResult> SearchFTS(const std::string& query, int limit);
    std::vector<SearchResult> SearchVector(const Embedding& embedding, int limit);
    std::vector<EnrichedSearchResult> MergeAndRank(
        const std::vector<SearchResult>& ftsResults,
        const std::vector<SearchResult>& vecResults,
        const HybridSearchOptions& options);

    // Result enrichment
    void EnrichResults(std::vector<EnrichedSearchResult>& results,
                       const HybridSearchOptions& options);
};

} // namespace KODI::SEMANTIC
```

### 4.2 EmbeddingEngine.cpp

```cpp
// EmbeddingEngine.cpp - ONNX Runtime based embedding generation

#include "EmbeddingEngine.h"
#include "Tokenizer.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"

#include <onnxruntime_cxx_api.h>

namespace KODI::SEMANTIC
{

class CEmbeddingEngine::Impl
{
public:
    Ort::Env m_env{ORT_LOGGING_LEVEL_WARNING, "SemanticEmbedding"};
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<CTokenizer> m_tokenizer;

    Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(
        OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<const char*> m_inputNames = {"input_ids", "attention_mask", "token_type_ids"};
    std::vector<const char*> m_outputNames = {"last_hidden_state"};

    bool m_initialized = false;
};

CEmbeddingEngine::CEmbeddingEngine()
    : m_impl(std::make_unique<Impl>())
{
}

CEmbeddingEngine::~CEmbeddingEngine() = default;

bool CEmbeddingEngine::Initialize(const std::string& modelPath, const std::string& vocabPath)
{
    try
    {
        // Load tokenizer
        m_impl->m_tokenizer = std::make_unique<CTokenizer>();
        if (!m_impl->m_tokenizer->Load(vocabPath))
        {
            CLog::Log(LOGERROR, "SemanticEmbedding: Failed to load tokenizer vocab");
            return false;
        }

        // Configure ONNX session
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(2);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // Enable CPU optimizations
        #ifdef __AVX2__
        sessionOptions.AddConfigEntry("session.intra_op.allow_spinning", "1");
        #endif

        // Load model
        #ifdef _WIN32
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        m_impl->m_session = std::make_unique<Ort::Session>(
            m_impl->m_env, wModelPath.c_str(), sessionOptions);
        #else
        m_impl->m_session = std::make_unique<Ort::Session>(
            m_impl->m_env, modelPath.c_str(), sessionOptions);
        #endif

        m_impl->m_initialized = true;

        CLog::Log(LOGINFO, "SemanticEmbedding: Model loaded successfully");
        return true;
    }
    catch (const Ort::Exception& e)
    {
        CLog::Log(LOGERROR, "SemanticEmbedding: ONNX error: {}", e.what());
        return false;
    }
}

Embedding CEmbeddingEngine::Embed(const std::string& text)
{
    auto batch = EmbedBatch({text});
    return batch.empty() ? Embedding{} : batch[0];
}

std::vector<Embedding> CEmbeddingEngine::EmbedBatch(const std::vector<std::string>& texts)
{
    if (!m_impl->m_initialized || texts.empty())
        return {};

    try
    {
        const size_t batchSize = texts.size();
        const size_t maxLength = 256;  // Model's max sequence length

        // Tokenize all texts
        std::vector<std::vector<int64_t>> allInputIds;
        std::vector<std::vector<int64_t>> allAttentionMask;
        std::vector<std::vector<int64_t>> allTokenTypeIds;

        for (const auto& text : texts)
        {
            auto tokens = m_impl->m_tokenizer->Encode(text, maxLength);

            std::vector<int64_t> inputIds(tokens.begin(), tokens.end());
            std::vector<int64_t> attentionMask(tokens.size(), 1);
            std::vector<int64_t> tokenTypeIds(tokens.size(), 0);

            // Pad to maxLength
            while (inputIds.size() < maxLength)
            {
                inputIds.push_back(0);      // PAD token
                attentionMask.push_back(0);
                tokenTypeIds.push_back(0);
            }

            allInputIds.push_back(std::move(inputIds));
            allAttentionMask.push_back(std::move(attentionMask));
            allTokenTypeIds.push_back(std::move(tokenTypeIds));
        }

        // Flatten for ONNX
        std::vector<int64_t> flatInputIds;
        std::vector<int64_t> flatAttentionMask;
        std::vector<int64_t> flatTokenTypeIds;

        for (size_t i = 0; i < batchSize; ++i)
        {
            flatInputIds.insert(flatInputIds.end(),
                allInputIds[i].begin(), allInputIds[i].end());
            flatAttentionMask.insert(flatAttentionMask.end(),
                allAttentionMask[i].begin(), allAttentionMask[i].end());
            flatTokenTypeIds.insert(flatTokenTypeIds.end(),
                allTokenTypeIds[i].begin(), allTokenTypeIds[i].end());
        }

        // Create tensors
        std::vector<int64_t> inputShape = {static_cast<int64_t>(batchSize),
                                            static_cast<int64_t>(maxLength)};

        Ort::Value inputIdsTensor = Ort::Value::CreateTensor<int64_t>(
            m_impl->m_memoryInfo, flatInputIds.data(), flatInputIds.size(),
            inputShape.data(), inputShape.size());

        Ort::Value attentionMaskTensor = Ort::Value::CreateTensor<int64_t>(
            m_impl->m_memoryInfo, flatAttentionMask.data(), flatAttentionMask.size(),
            inputShape.data(), inputShape.size());

        Ort::Value tokenTypeIdsTensor = Ort::Value::CreateTensor<int64_t>(
            m_impl->m_memoryInfo, flatTokenTypeIds.data(), flatTokenTypeIds.size(),
            inputShape.data(), inputShape.size());

        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(inputIdsTensor));
        inputTensors.push_back(std::move(attentionMaskTensor));
        inputTensors.push_back(std::move(tokenTypeIdsTensor));

        // Run inference
        auto outputTensors = m_impl->m_session->Run(
            Ort::RunOptions{nullptr},
            m_impl->m_inputNames.data(), inputTensors.data(), inputTensors.size(),
            m_impl->m_outputNames.data(), m_impl->m_outputNames.size());

        // Extract embeddings (mean pooling over sequence dimension)
        auto& outputTensor = outputTensors[0];
        auto outputShape = outputTensor.GetTensorTypeAndShapeInfo().GetShape();
        // Shape: [batch_size, sequence_length, hidden_size]

        const float* outputData = outputTensor.GetTensorData<float>();
        const size_t hiddenSize = outputShape[2];  // 384 for MiniLM

        std::vector<Embedding> embeddings(batchSize);

        for (size_t b = 0; b < batchSize; ++b)
        {
            // Mean pooling: average over non-padded tokens
            Embedding embedding{};
            int validTokens = 0;

            for (size_t t = 0; t < maxLength; ++t)
            {
                if (allAttentionMask[b][t] == 0)
                    continue;

                validTokens++;
                for (size_t h = 0; h < hiddenSize && h < EMBEDDING_DIM; ++h)
                {
                    size_t idx = b * maxLength * hiddenSize + t * hiddenSize + h;
                    embedding[h] += outputData[idx];
                }
            }

            // Average and normalize
            if (validTokens > 0)
            {
                float norm = 0.0f;
                for (size_t h = 0; h < EMBEDDING_DIM; ++h)
                {
                    embedding[h] /= validTokens;
                    norm += embedding[h] * embedding[h];
                }
                norm = std::sqrt(norm);
                if (norm > 0)
                {
                    for (size_t h = 0; h < EMBEDDING_DIM; ++h)
                        embedding[h] /= norm;
                }
            }

            embeddings[b] = embedding;
        }

        return embeddings;
    }
    catch (const Ort::Exception& e)
    {
        CLog::Log(LOGERROR, "SemanticEmbedding: Inference error: {}", e.what());
        return {};
    }
}

float CEmbeddingEngine::Similarity(const Embedding& a, const Embedding& b)
{
    // Cosine similarity (vectors are already normalized)
    float dot = 0.0f;
    for (size_t i = 0; i < EMBEDDING_DIM; ++i)
        dot += a[i] * b[i];
    return dot;
}

} // namespace KODI::SEMANTIC
```

### 4.3 ResultRanker.cpp - Reciprocal Rank Fusion

```cpp
// ResultRanker.cpp - Reciprocal Rank Fusion implementation

#include "ResultRanker.h"
#include <algorithm>
#include <unordered_map>

namespace KODI::SEMANTIC
{

std::vector<EnrichedSearchResult> CResultRanker::RankRRF(
    const std::vector<SearchResult>& ftsResults,
    const std::vector<SearchResult>& vecResults,
    float ftsWeight,
    float vecWeight,
    float k)
{
    // RRF formula: score(d) = Σ (weight / (k + rank(d)))
    // where rank starts at 1

    std::unordered_map<int64_t, float> scores;
    std::unordered_map<int64_t, SearchResult> resultMap;
    std::unordered_map<int64_t, float> ftsScores;
    std::unordered_map<int64_t, float> vecScores;

    // Score FTS results
    for (size_t i = 0; i < ftsResults.size(); ++i)
    {
        int64_t id = ftsResults[i].chunkId;
        float rank = static_cast<float>(i + 1);

        scores[id] += ftsWeight / (k + rank);
        ftsScores[id] = 1.0f / rank;  // Normalized rank score
        resultMap[id] = ftsResults[i];
    }

    // Score vector results
    for (size_t i = 0; i < vecResults.size(); ++i)
    {
        int64_t id = vecResults[i].chunkId;
        float rank = static_cast<float>(i + 1);

        scores[id] += vecWeight / (k + rank);
        vecScores[id] = vecResults[i].score;  // Original similarity score

        if (resultMap.find(id) == resultMap.end())
            resultMap[id] = vecResults[i];
    }

    // Sort by combined score
    std::vector<std::pair<int64_t, float>> sortedScores(scores.begin(), scores.end());
    std::sort(sortedScores.begin(), sortedScores.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Build enriched results
    std::vector<EnrichedSearchResult> results;
    results.reserve(sortedScores.size());

    for (const auto& [id, score] : sortedScores)
    {
        EnrichedSearchResult enriched;

        // Copy base result
        const auto& base = resultMap[id];
        enriched.chunkId = base.chunkId;
        enriched.mediaId = base.mediaId;
        enriched.mediaType = base.mediaType;
        enriched.matchedText = base.matchedText;
        enriched.startMs = base.startMs;
        enriched.endMs = base.endMs;
        enriched.sourceType = base.sourceType;
        enriched.confidence = base.confidence;

        // Add score breakdown
        enriched.combinedScore = score;
        enriched.ftsScore = ftsScores.count(id) ? ftsScores[id] : 0.0f;
        enriched.vectorScore = vecScores.count(id) ? vecScores[id] : 0.0f;

        results.push_back(enriched);
    }

    return results;
}

std::vector<EnrichedSearchResult> CResultRanker::Deduplicate(
    std::vector<EnrichedSearchResult>& results,
    int64_t windowMs)
{
    // Remove results that are too close in time within the same media
    // Keep the highest scoring one

    std::vector<EnrichedSearchResult> deduplicated;

    for (const auto& result : results)
    {
        bool isDuplicate = false;

        for (auto& existing : deduplicated)
        {
            if (existing.mediaId != result.mediaId)
                continue;

            if (!result.startMs || !existing.startMs)
                continue;

            int64_t timeDiff = std::abs(*result.startMs - *existing.startMs);
            if (timeDiff < windowMs)
            {
                // Keep higher score
                if (result.combinedScore > existing.combinedScore)
                {
                    existing = result;
                }
                isDuplicate = true;
                break;
            }
        }

        if (!isDuplicate)
            deduplicated.push_back(result);
    }

    return deduplicated;
}

} // namespace KODI::SEMANTIC
```

---

## 5. JSON-RPC API Extensions

```json
// ============================================================================
// Semantic.HybridSearch - Enhanced semantic search
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.HybridSearch",
    "params": {
        "query": "the scene where they plan the heist",
        "options": {
            "mode": "hybrid",           // "hybrid", "keyword", "semantic"
            "limit": 20,
            "fts_weight": 0.4,
            "vector_weight": 0.6,
            "include_context": true,
            "context_chunks": 2,
            "media_types": ["movie"],
            "min_score": 0.3
        }
    },
    "id": 1
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "results": [
            {
                "chunk_id": 12345,
                "media_id": 789,
                "media_type": "movie",
                "title": "Heat",
                "year": 1995,
                "text": "I told you. I'm never going back.",
                "context_before": "What you're telling me is, you don't...",
                "context_after": "Then don't take down scores.",
                "start_ms": 5023000,
                "end_ms": 5027000,
                "timestamp": "1:23:43",
                "thumbnail": "/path/to/thumb.jpg",
                "scores": {
                    "fts": 0.72,
                    "vector": 0.91,
                    "combined": 0.84
                },
                "source": "subtitle_srt"
            }
        ],
        "query_embedding_time_ms": 12,
        "search_time_ms": 45,
        "total_time_ms": 57
    },
    "id": 1
}

// ============================================================================
// Semantic.FindSimilar - Find semantically similar content
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.FindSimilar",
    "params": {
        "chunk_id": 12345,
        "limit": 10,
        "exclude_same_media": true
    },
    "id": 2
}

// ============================================================================
// Semantic.GetSuggestions - Search autocomplete
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.GetSuggestions",
    "params": {
        "prefix": "the scene where",
        "limit": 10
    },
    "id": 3
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "suggestions": [
            "the scene where they meet",
            "the scene where he dies",
            "the scene where she confesses",
            "the scene where they fight"
        ]
    },
    "id": 3
}

// ============================================================================
// Semantic.GetEmbeddingStatus - Check embedding generation progress
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.GetEmbeddingStatus",
    "params": {},
    "id": 4
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "total_chunks": 250000,
        "embedded_chunks": 185000,
        "progress_percent": 74,
        "estimated_remaining_minutes": 25,
        "model": {
            "name": "all-MiniLM-L6-v2",
            "dimensions": 384,
            "version": "2.0"
        }
    },
    "id": 4
}
```

---

## 6. GUI Implementation

### 6.1 DialogSemanticSearch.xml

```xml
<!-- DialogSemanticSearch.xml -->
<?xml version="1.0" encoding="UTF-8"?>
<window id="13500" type="dialog">
    <defaultcontrol always="true">9000</defaultcontrol>
    <coordinates>
        <origin x="320" y="120"/>
        <posx>0</posx>
        <posy>0</posy>
    </coordinates>
    <width>1280</width>
    <height>840</height>

    <controls>
        <!-- Background -->
        <control type="image">
            <posx>0</posx>
            <posy>0</posy>
            <width>1280</width>
            <height>840</height>
            <texture colordiffuse="DD000000">common/dialogbg.png</texture>
        </control>

        <!-- Title -->
        <control type="label">
            <posx>40</posx>
            <posy>20</posy>
            <width>1200</width>
            <height>40</height>
            <label>$LOCALIZE[39700]</label>  <!-- "Semantic Search" -->
            <font>font_title</font>
            <align>center</align>
        </control>

        <!-- Search Input Group -->
        <control type="group">
            <posx>40</posx>
            <posy>80</posy>
            <width>1200</width>

            <!-- Search Edit Box -->
            <control type="edit" id="9000">
                <posx>0</posx>
                <posy>0</posy>
                <width>1120</width>
                <height>50</height>
                <label>$LOCALIZE[39701]</label>  <!-- "Search your library..." -->
                <hinttext>$LOCALIZE[39702]</hinttext>
                <font>font13</font>
                <textoffsetx>15</textoffsetx>
                <aligny>center</aligny>
                <texturefocus>buttons/edit-focus.png</texturefocus>
                <texturenofocus>buttons/edit-nofocus.png</texturenofocus>
                <onup>9003</onup>
                <ondown>9001</ondown>
            </control>

            <!-- Voice Search Button -->
            <control type="button" id="9005">
                <posx>1130</posx>
                <posy>0</posy>
                <width>50</width>
                <height>50</height>
                <label></label>
                <texturefocus>buttons/mic-focus.png</texturefocus>
                <texturenofocus>buttons/mic-nofocus.png</texturenofocus>
                <onclick>VoiceSearch</onclick>
                <onleft>9000</onleft>
                <visible>System.HasAddon(service.voice)</visible>
            </control>
        </control>

        <!-- Recent Searches (visible when no query) -->
        <control type="group" id="9010">
            <visible>String.IsEmpty(Control.GetLabel(9000))</visible>
            <posx>40</posx>
            <posy>150</posy>
            <width>1200</width>

            <control type="label">
                <posx>0</posx>
                <posy>0</posy>
                <label>$LOCALIZE[39703]</label>  <!-- "Recent Searches" -->
                <font>font12_caps</font>
                <textcolor>grey</textcolor>
            </control>

            <control type="list" id="9011">
                <posx>0</posx>
                <posy>30</posy>
                <width>1200</width>
                <height>100</height>
                <orientation>horizontal</orientation>
                <itemlayout width="380" height="40">
                    <control type="label">
                        <posx>10</posx>
                        <posy>10</posy>
                        <width>360</width>
                        <label>$INFO[ListItem.Label]</label>
                        <font>font12</font>
                    </control>
                </itemlayout>
                <focusedlayout width="380" height="40">
                    <control type="image">
                        <width>380</width>
                        <height>40</height>
                        <texture>buttons/focus.png</texture>
                    </control>
                    <control type="label">
                        <posx>10</posx>
                        <posy>10</posy>
                        <width>360</width>
                        <label>$INFO[ListItem.Label]</label>
                        <font>font12</font>
                    </control>
                </focusedlayout>
                <onup>9000</onup>
                <ondown>9001</ondown>
            </control>
        </control>

        <!-- Filter Bar -->
        <control type="group">
            <visible>!String.IsEmpty(Control.GetLabel(9000))</visible>
            <posx>40</posx>
            <posy>150</posy>
            <width>1200</width>

            <control type="label">
                <posx>0</posx>
                <posy>5</posy>
                <label>$LOCALIZE[39704]</label>  <!-- "Type:" -->
                <font>font12</font>
            </control>

            <control type="spincontrolex" id="9020">
                <posx>60</posx>
                <posy>0</posy>
                <width>150</width>
                <height>35</height>
                <font>font12</font>
                <onleft>9000</onleft>
                <onright>9021</onright>
            </control>

            <control type="label">
                <posx>230</posx>
                <posy>5</posy>
                <label>$LOCALIZE[39705]</label>  <!-- "Genre:" -->
                <font>font12</font>
            </control>

            <control type="spincontrolex" id="9021">
                <posx>300</posx>
                <posy>0</posy>
                <width>150</width>
                <height>35</height>
                <font>font12</font>
                <onleft>9020</onleft>
                <onright>9022</onright>
            </control>
        </control>

        <!-- Results List -->
        <control type="panel" id="9001">
            <posx>40</posx>
            <posy>210</posy>
            <width>1200</width>
            <height>580</height>
            <onup>9000</onup>
            <ondown>9003</ondown>
            <pagecontrol>9002</pagecontrol>
            <scrolltime>200</scrolltime>
            <itemlayout width="1200" height="110">
                <control type="image">
                    <posx>0</posx>
                    <posy>5</posy>
                    <width>160</width>
                    <height>90</height>
                    <texture>$INFO[ListItem.Thumb]</texture>
                    <aspectratio>scale</aspectratio>
                </control>
                <control type="label">
                    <posx>175</posx>
                    <posy>5</posy>
                    <width>800</width>
                    <height>30</height>
                    <label>$INFO[ListItem.Label]</label>
                    <font>font13_title</font>
                </control>
                <control type="textbox">
                    <posx>175</posx>
                    <posy>35</posy>
                    <width>900</width>
                    <height>40</height>
                    <label>$INFO[ListItem.Label2]</label>
                    <font>font12</font>
                    <textcolor>grey</textcolor>
                </control>
                <control type="label">
                    <posx>175</posx>
                    <posy>75</posy>
                    <width>400</width>
                    <label>-> $INFO[ListItem.Property(timestamp)]  |  Semantic: $INFO[ListItem.Property(vec_score)]%  Keyword: $INFO[ListItem.Property(fts_score)]%</label>
                    <font>font10</font>
                    <textcolor>blue</textcolor>
                </control>
            </itemlayout>
            <focusedlayout width="1200" height="110">
                <control type="image">
                    <posx>0</posx>
                    <posy>0</posy>
                    <width>1200</width>
                    <height>110</height>
                    <texture colordiffuse="44FFFFFF">common/highlight.png</texture>
                </control>
                <control type="image">
                    <posx>0</posx>
                    <posy>5</posy>
                    <width>160</width>
                    <height>90</height>
                    <texture>$INFO[ListItem.Thumb]</texture>
                    <aspectratio>scale</aspectratio>
                </control>
                <control type="label">
                    <posx>175</posx>
                    <posy>5</posy>
                    <width>800</width>
                    <height>30</height>
                    <label>$INFO[ListItem.Label]</label>
                    <font>font13_title</font>
                </control>
                <control type="textbox">
                    <posx>175</posx>
                    <posy>35</posy>
                    <width>900</width>
                    <height>40</height>
                    <label>$INFO[ListItem.Label2]</label>
                    <font>font12</font>
                </control>
                <control type="label">
                    <posx>175</posx>
                    <posy>75</posy>
                    <width>400</width>
                    <label>-> $INFO[ListItem.Property(timestamp)]  |  Semantic: $INFO[ListItem.Property(vec_score)]%  Keyword: $INFO[ListItem.Property(fts_score)]%</label>
                    <font>font10</font>
                    <textcolor>blue</textcolor>
                </control>
                <control type="button" id="9100">
                    <posx>1100</posx>
                    <posy>30</posy>
                    <width>80</width>
                    <height>40</height>
                    <label>Play</label>
                    <font>font12</font>
                    <texturefocus>buttons/button-focus.png</texturefocus>
                    <texturenofocus>buttons/button-nofocus.png</texturenofocus>
                </control>
            </focusedlayout>
        </control>

        <!-- Scroll Bar -->
        <control type="scrollbar" id="9002">
            <posx>1250</posx>
            <posy>210</posy>
            <width>15</width>
            <height>580</height>
            <orientation>vertical</orientation>
        </control>

        <!-- Status Bar -->
        <control type="group">
            <posx>40</posx>
            <posy>800</posy>
            <width>1200</width>

            <control type="label" id="9003">
                <posx>0</posx>
                <posy>0</posy>
                <width>600</width>
                <label>$INFO[Window(Home).Property(SemanticStatus)]</label>
                <font>font10</font>
                <textcolor>grey</textcolor>
            </control>

            <control type="label">
                <posx>800</posx>
                <posy>0</posy>
                <width>400</width>
                <align>right</align>
                <label>$INFO[Window(Home).Property(SemanticResultCount)]</label>
                <font>font10</font>
            </control>
        </control>

        <!-- Close Button -->
        <control type="button" id="9099">
            <posx>1200</posx>
            <posy>10</posy>
            <width>50</width>
            <height>50</height>
            <label></label>
            <texturefocus>buttons/close-focus.png</texturefocus>
            <texturenofocus>buttons/close-nofocus.png</texturenofocus>
            <onclick>Close</onclick>
        </control>

    </controls>
</window>
```

### 6.2 GUIDialogSemanticSearch.cpp

```cpp
// GUIDialogSemanticSearch.cpp

#include "GUIDialogSemanticSearch.h"
#include "semantic/search/HybridSearchEngine.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIListContainer.h"
#include "input/actions/ActionIDs.h"
#include "video/VideoDatabase.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"

namespace KODI::SEMANTIC
{

#define CONTROL_SEARCH_EDIT     9000
#define CONTROL_RESULTS_LIST    9001
#define CONTROL_RECENT_LIST     9011
#define CONTROL_FILTER_TYPE     9020
#define CONTROL_FILTER_GENRE    9021

CGUIDialogSemanticSearch::CGUIDialogSemanticSearch()
    : CGUIDialog(WINDOW_DIALOG_SEMANTIC_SEARCH, "DialogSemanticSearch.xml")
    , m_searchEngine(std::make_unique<CHybridSearchEngine>())
{
    m_loadType = KEEP_IN_MEMORY;
}

bool CGUIDialogSemanticSearch::OnMessage(CGUIMessage& message)
{
    switch (message.GetMessage())
    {
        case GUI_MSG_WINDOW_INIT:
        {
            CGUIDialog::OnMessage(message);

            // Initialize search engine
            if (!m_searchEngine->IsSemanticAvailable())
            {
                // Fall back to keyword-only mode
                SetProperty("SemanticStatus",
                    g_localizeStrings.Get(39710));  // "Semantic search unavailable"
            }

            // Load recent searches
            LoadRecentSearches();

            // Focus search input
            SET_CONTROL_FOCUS(CONTROL_SEARCH_EDIT, 0);

            return true;
        }

        case GUI_MSG_CLICKED:
        {
            int control = message.GetSenderId();

            if (control == CONTROL_SEARCH_EDIT)
            {
                OnSearch();
                return true;
            }
            else if (control == CONTROL_RESULTS_LIST)
            {
                OnResultSelected();
                return true;
            }
            else if (control == CONTROL_RECENT_LIST)
            {
                OnRecentSearchSelected();
                return true;
            }
            break;
        }

        case GUI_MSG_SETFOCUS:
        {
            break;
        }
    }

    return CGUIDialog::OnMessage(message);
}

bool CGUIDialogSemanticSearch::OnAction(const CAction& action)
{
    switch (action.GetID())
    {
        case ACTION_SEARCH:
        case ACTION_FILTER:
            SET_CONTROL_FOCUS(CONTROL_SEARCH_EDIT, 0);
            return true;

        case ACTION_VOICE_SEARCH:
            OnVoiceSearch();
            return true;

        case ACTION_PLAYER_PLAY:
        {
            if (GetFocusedControlID() == CONTROL_RESULTS_LIST)
            {
                OnPlaySelected();
                return true;
            }
            break;
        }

        case ACTION_CONTEXT_MENU:
        {
            if (GetFocusedControlID() == CONTROL_RESULTS_LIST)
            {
                OnResultContextMenu();
                return true;
            }
            break;
        }
    }

    return CGUIDialog::OnAction(action);
}

void CGUIDialogSemanticSearch::OnSearch()
{
    CGUIEditControl* edit = dynamic_cast<CGUIEditControl*>(
        GetControl(CONTROL_SEARCH_EDIT));
    if (!edit)
        return;

    std::string query = edit->GetLabel2();
    StringUtils::Trim(query);

    if (query.empty())
        return;

    SetProperty("SemanticStatus", g_localizeStrings.Get(39711));  // "Searching..."

    HybridSearchOptions options;
    options.maxResults = 50;
    options.includeContext = true;
    options.contextChunks = 1;

    ApplyFilters(options);

    auto results = m_searchEngine->Search(query, options);

    CGUIListContainer* list = dynamic_cast<CGUIListContainer*>(
        GetControl(CONTROL_RESULTS_LIST));
    if (!list)
        return;

    CFileItemList items;
    for (const auto& result : results)
    {
        CFileItemPtr item = ResultToFileItem(result);
        items.Add(item);
    }

    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_RESULTS_LIST, 0, 0, &items);
    OnMessage(msg);

    SetProperty("SemanticResultCount",
        StringUtils::Format(g_localizeStrings.Get(39712), results.size()));
    SetProperty("SemanticStatus", "");

    m_searchEngine->RecordSearch(query, results.size());
    AddToRecentSearches(query);
}

CFileItemPtr CGUIDialogSemanticSearch::ResultToFileItem(
    const EnrichedSearchResult& result)
{
    CFileItemPtr item = std::make_shared<CFileItem>(result.mediaTitle);

    item->SetPath(result.mediaPath);

    std::string contextText = result.text;
    if (!result.contextBefore.empty())
        contextText = "..." + result.contextBefore + " " + contextText;
    if (!result.contextAfter.empty())
        contextText += " " + result.contextAfter + "...";
    item->SetLabel2(contextText);

    item->SetProperty("chunk_id", result.chunkId);
    item->SetProperty("media_id", result.mediaId);
    item->SetProperty("start_ms", static_cast<int64_t>(result.startMs.value_or(0)));
    item->SetProperty("timestamp", result.formattedTimestamp);
    item->SetProperty("fts_score", StringUtils::Format("{:.0f}", result.ftsScore * 100));
    item->SetProperty("vec_score", StringUtils::Format("{:.0f}", result.vectorScore * 100));

    if (!result.thumbnailPath.empty())
        item->SetArt("thumb", result.thumbnailPath);

    return item;
}

void CGUIDialogSemanticSearch::OnPlaySelected()
{
    CGUIListContainer* list = dynamic_cast<CGUIListContainer*>(
        GetControl(CONTROL_RESULTS_LIST));
    if (!list)
        return;

    CFileItemPtr item = list->GetSelectedItem();
    if (!item)
        return;

    int64_t startMs = item->GetProperty("start_ms").asInteger();

    Close();

    CFileItem playItem(*item);
    playItem.m_lStartOffset = static_cast<int>(startMs / 1000);

    g_application.PlayFile(playItem, "");
}

void CGUIDialogSemanticSearch::OnResultContextMenu()
{
    CGUIListContainer* list = dynamic_cast<CGUIListContainer*>(
        GetControl(CONTROL_RESULTS_LIST));
    if (!list)
        return;

    CFileItemPtr item = list->GetSelectedItem();
    if (!item)
        return;

    CContextButtons buttons;
    buttons.Add(1, g_localizeStrings.Get(208));   // "Play"
    buttons.Add(2, g_localizeStrings.Get(39720)); // "Play from beginning"
    buttons.Add(3, g_localizeStrings.Get(39721)); // "Show in library"
    buttons.Add(4, g_localizeStrings.Get(39722)); // "Find similar scenes"
    buttons.Add(5, g_localizeStrings.Get(39723)); // "See more context"

    int choice = CGUIDialogContextMenu::Show(buttons);

    switch (choice)
    {
        case 1:
            OnPlaySelected();
            break;
        case 2:
            OnPlayFromBeginning(item);
            break;
        case 3:
            OnShowInLibrary(item);
            break;
        case 4:
            OnFindSimilar(item);
            break;
        case 5:
            OnShowContext(item);
            break;
    }
}

void CGUIDialogSemanticSearch::OnFindSimilar(const CFileItemPtr& item)
{
    int chunkId = item->GetProperty("chunk_id").asInteger();

    auto similar = m_searchEngine->FindSimilar(chunkId, 20);

    CFileItemList items;
    for (const auto& result : similar)
    {
        items.Add(ResultToFileItem(result));
    }

    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_RESULTS_LIST, 0, 0, &items);
    OnMessage(msg);

    SetProperty("SemanticStatus", g_localizeStrings.Get(39724));  // "Similar scenes"
}

void CGUIDialogSemanticSearch::OnVoiceSearch()
{
    CGUIMessage msg(GUI_MSG_SET_LABELS, GetID(), CONTROL_SEARCH_EDIT);
    msg.SetLabel(g_localizeStrings.Get(39725));  // "Listening..."
    OnMessage(msg);

    #if !defined(TARGET_ANDROID) && !defined(TARGET_DARWIN_IOS)
    CGUIDialogOK::ShowAndGetInput(
        g_localizeStrings.Get(39726),  // "Voice Search"
        g_localizeStrings.Get(39727)); // "Voice search not available on this platform"
    #endif
}

void CGUIDialogSemanticSearch::LoadRecentSearches()
{
    m_recentSearches = m_searchEngine->GetRecentSearches(10);

    CGUIListContainer* list = dynamic_cast<CGUIListContainer*>(
        GetControl(CONTROL_RECENT_LIST));
    if (!list)
        return;

    CFileItemList items;
    for (const auto& search : m_recentSearches)
    {
        CFileItemPtr item = std::make_shared<CFileItem>(search);
        items.Add(item);
    }

    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_RECENT_LIST, 0, 0, &items);
    OnMessage(msg);
}

void CGUIDialogSemanticSearch::AddToRecentSearches(const std::string& query)
{
    m_recentSearches.erase(
        std::remove(m_recentSearches.begin(), m_recentSearches.end(), query),
        m_recentSearches.end());

    m_recentSearches.insert(m_recentSearches.begin(), query);

    if (m_recentSearches.size() > 20)
        m_recentSearches.resize(20);

    m_searchEngine->SaveRecentSearches(m_recentSearches);
}

} // namespace KODI::SEMANTIC
```

---

## 7. Migration Code

```cpp
// Run after PR #2 code is deployed
void CSemanticIndexService::MigrateToVectors()
{
    // 1. Load embedding model
    m_embedder->Initialize(modelPath, vocabPath);

    // 2. Process chunks in batches
    const int batchSize = 32;
    int64_t lastId = 0;

    while (true)
    {
        auto chunks = m_database->GetChunksWithoutEmbeddings(batchSize, lastId);
        if (chunks.empty())
            break;

        std::vector<std::string> texts;
        for (const auto& chunk : chunks)
            texts.push_back(chunk.text);

        auto embeddings = m_embedder->EmbedBatch(texts);

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            m_database->InsertEmbedding(chunks[i].chunkId, embeddings[i]);
            lastId = chunks[i].chunkId;
        }

        // Progress notification
        NotifyMigrationProgress(lastId);
    }
}
```

---

## 8. Testing

### 8.1 Embedding Engine Tests

```cpp
// test/TestEmbeddingEngine.cpp

TEST(EmbeddingEngine, Embed_SimilarTexts_HighSimilarity)
{
    CEmbeddingEngine engine;
    engine.Initialize(GetTestModelPath(), GetTestVocabPath());

    auto emb1 = engine.Embed("The movie was really exciting and fun");
    auto emb2 = engine.Embed("The film was very thrilling and enjoyable");
    auto emb3 = engine.Embed("I need to buy groceries tomorrow");

    float sim12 = CEmbeddingEngine::Similarity(emb1, emb2);
    float sim13 = CEmbeddingEngine::Similarity(emb1, emb3);

    EXPECT_GT(sim12, 0.7f);  // Similar meanings should be high
    EXPECT_LT(sim13, 0.3f);  // Unrelated should be low
}

TEST(EmbeddingEngine, EmbedBatch_ConsistentWithSingle)
{
    CEmbeddingEngine engine;
    engine.Initialize(GetTestModelPath(), GetTestVocabPath());

    std::vector<std::string> texts = {
        "Hello world",
        "How are you",
        "The quick brown fox"
    };

    auto batchResults = engine.EmbedBatch(texts);

    for (size_t i = 0; i < texts.size(); ++i)
    {
        auto single = engine.Embed(texts[i]);
        float similarity = CEmbeddingEngine::Similarity(single, batchResults[i]);
        EXPECT_GT(similarity, 0.99f);  // Should be nearly identical
    }
}
```

### 8.2 Hybrid Search Tests

```cpp
// test/TestHybridSearch.cpp

class HybridSearchTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_engine = std::make_unique<CHybridSearchEngine>();
        m_engine->InitializeForTest(":memory:");

        // Insert test data with embeddings
        InsertChunkWithEmbedding(1, "movie",
            "I'm going to make him an offer he can't refuse", 1000, 5000);
        InsertChunkWithEmbedding(1, "movie",
            "Leave the gun, take the cannoli", 10000, 14000);
        InsertChunkWithEmbedding(2, "movie",
            "Here's looking at you, kid", 5000, 8000);
    }

    std::unique_ptr<CHybridSearchEngine> m_engine;
};

TEST_F(HybridSearchTest, Search_SemanticMatch_FindsRelated)
{
    // Search for concept, not exact words
    auto results = m_engine->Search("mafia boss makes a deal");

    ASSERT_GE(results.size(), 1);
    EXPECT_EQ(results[0].mediaId, 1);  // Should find Godfather quote
    EXPECT_TRUE(results[0].matchedText.find("offer") != std::string::npos);
}

TEST_F(HybridSearchTest, Search_ExactMatch_RanksHigher)
{
    auto results = m_engine->Search("cannoli gun");

    ASSERT_GE(results.size(), 1);
    EXPECT_TRUE(results[0].matchedText.find("cannoli") != std::string::npos);
    EXPECT_GT(results[0].ftsScore, 0.5f);  // High keyword score
}

TEST_F(HybridSearchTest, Search_HybridMode_CombinesBothSignals)
{
    HybridSearchOptions options;
    options.mode = HybridSearchOptions::Mode::Hybrid;

    auto results = m_engine->Search("romantic farewell", options);

    // "Here's looking at you, kid" should rank well semantically
    ASSERT_GE(results.size(), 1);
    EXPECT_GT(results[0].vectorScore, 0.5f);
}
```

### 8.3 GUI Tests

```cpp
// test/TestSemanticSearchDialog.cpp

TEST(SemanticSearchDialog, OnSearch_PopulatesResults)
{
    CGUIDialogSemanticSearchMock dialog;
    dialog.Initialize();

    dialog.SetSearchText("test query");
    dialog.OnSearch();

    EXPECT_GT(dialog.GetResultCount(), 0);
    EXPECT_FALSE(dialog.GetProperty("SemanticStatus").asString().empty());
}

TEST(SemanticSearchDialog, OnResultSelect_PlaysAtTimestamp)
{
    CGUIDialogSemanticSearchMock dialog;
    dialog.Initialize();

    EnrichedSearchResult result;
    result.mediaPath = "/test/movie.mkv";
    result.startMs = 123000;
    dialog.AddMockResult(result);

    dialog.SelectResult(0);
    dialog.OnPlaySelected();

    EXPECT_EQ(g_applicationMock.GetLastPlayedPath(), "/test/movie.mkv");
    EXPECT_EQ(g_applicationMock.GetLastPlayedOffset(), 123);  // seconds
}
```

### 8.4 Performance Benchmarks

```cpp
// test/BenchmarkSemanticSearch.cpp

BENCHMARK(EmbeddingGeneration_Single)
{
    CEmbeddingEngine engine;
    engine.Initialize(modelPath, vocabPath);

    for (auto _ : state)
    {
        auto emb = engine.Embed("This is a test sentence for benchmarking");
        benchmark::DoNotOptimize(emb);
    }
}
// Target: <10ms per embedding

BENCHMARK(EmbeddingGeneration_Batch32)
{
    CEmbeddingEngine engine;
    engine.Initialize(modelPath, vocabPath);

    std::vector<std::string> texts(32, "Test sentence for batch processing");

    for (auto _ : state)
    {
        auto embs = engine.EmbedBatch(texts);
        benchmark::DoNotOptimize(embs);
    }
}
// Target: <200ms for 32 embeddings

BENCHMARK(HybridSearch_100kChunks)
{
    CHybridSearchEngine engine;
    SetupLargeTestDatabase(engine, 100000);  // 100k chunks

    for (auto _ : state)
    {
        auto results = engine.Search("test query phrase");
        benchmark::DoNotOptimize(results);
    }
}
// Target: <200ms for hybrid search on 100k chunks
```

---

## 9. Model Information

### 9.1 Model Comparison

| Model | Size | Dim | Speed | Quality |
|-------|------|-----|-------|---------|
| all-MiniLM-L6-v2 | 90MB | 384 | 5ms | Good |
| all-MiniLM-L12-v2 | 120MB | 384 | 10ms | Better |
| all-mpnet-base-v2 | 420MB | 768 | 25ms | Best |
| bge-small-en-v1.5 | 130MB | 384 | 6ms | Good |

**Selection: all-MiniLM-L6-v2**
- Best balance of size, speed, and quality
- Widely used and well-tested
- Apache 2.0 license

### 9.2 sqlite-vec vs Alternatives

| Solution | Pros | Cons |
|----------|------|------|
| **sqlite-vec** | Single file, easy embed | Newer, less tested |
| FAISS | Very fast, mature | Large dependency, complex |
| Annoy | Simple, fast | No updates, write-once |
| hnswlib | Fast, small | C++ only, no persistence |

**Selection: sqlite-vec**
- Integrates naturally with existing SQLite usage
- Single C file to vendor
- Active development
- Good enough performance for our scale

---

## 10. References

- [sqlite-vec GitHub](https://github.com/asg017/sqlite-vec)
- [Sentence Transformers](https://www.sbert.net/)
- [ONNX Runtime](https://onnxruntime.ai/)
- [Reciprocal Rank Fusion](https://plg.uwaterloo.ca/~gvcormac/cormacksigir09-rrf.pdf)
