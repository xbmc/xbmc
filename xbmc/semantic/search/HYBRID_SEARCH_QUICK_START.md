# CHybridSearchEngine Quick Start Guide

## Overview
CHybridSearchEngine combines FTS5 keyword search (BM25) with vector similarity search using **Reciprocal Rank Fusion (RRF)** for optimal relevance.

## Basic Usage

```cpp
#include "semantic/search/HybridSearchEngine.h"

// Initialize (assumes database, embedding engine, and vector searcher are ready)
CHybridSearchEngine hybridSearch;
hybridSearch.Initialize(&database, &embeddingEngine, vectorSearcher);

// Perform a hybrid search
auto results = hybridSearch.Search("detective solving mystery");

// Process results
for (const auto& result : results)
{
    std::cout << result.formattedTimestamp << ": " << result.snippet << std::endl;
    std::cout << "  Combined: " << result.combinedScore
              << " (kw:" << result.keywordScore
              << " vec:" << result.vectorScore << ")" << std::endl;
}
```

## Search Modes

| Mode | Speed | Best For | When to Use |
|------|-------|----------|-------------|
| **Hybrid** (default) | Medium | General search | Want best relevance |
| **KeywordOnly** | Fastest | Exact matches | Need speed, specific terms |
| **SemanticOnly** | Fast | Concepts | Want similar meaning, not exact words |

```cpp
HybridSearchOptions options;

// Hybrid mode (default)
options.mode = SearchMode::Hybrid;
options.keywordWeight = 0.4f;  // BM25 weight
options.vectorWeight = 0.6f;   // Vector weight

// Keyword-only mode
options.mode = SearchMode::KeywordOnly;

// Semantic-only mode
options.mode = SearchMode::SemanticOnly;

auto results = hybridSearch.Search("query", options);
```

## Configuration Options

```cpp
HybridSearchOptions options;

// Search behavior
options.mode = SearchMode::Hybrid;
options.keywordWeight = 0.4f;    // Keyword importance (0-1)
options.vectorWeight = 0.6f;     // Vector importance (0-1)

// Result limits
options.keywordTopK = 100;       // Keyword candidates to consider
options.vectorTopK = 100;        // Vector candidates to consider
options.maxResults = 20;         // Final results to return

// Filters
options.mediaType = "movie";     // Filter by media type
options.mediaId = 12345;         // Filter by specific media
options.minConfidence = 0.5f;    // Minimum confidence threshold
```

## Common Scenarios

### 1. General Search (Best Relevance)
```cpp
auto results = hybridSearch.Search("time travel paradox");
// Uses default hybrid mode with balanced weights
```

### 2. Fast Exact Match
```cpp
HybridSearchOptions options;
options.mode = SearchMode::KeywordOnly;
options.maxResults = 50;
auto results = hybridSearch.Search("police chief badge", options);
```

### 3. Conceptual Similarity
```cpp
HybridSearchOptions options;
options.mode = SearchMode::SemanticOnly;
auto results = hybridSearch.Search("fear and anxiety", options);
// May return: "worried", "scared", "nervous", etc.
```

### 4. Search Within Media
```cpp
HybridSearchOptions options;
options.mediaType = "episode";
options.mediaId = 12345;
auto results = hybridSearch.Search("final battle", options);
```

### 5. Find Similar Content
```cpp
// Find chunks similar to a specific chunk
int64_t sourceChunkId = 42;
auto similar = hybridSearch.FindSimilar(sourceChunkId, 15);
```

### 6. Get Context Around Result
```cpp
auto results = hybridSearch.Search("hero saves day");
if (!results.empty())
{
    // Get 30 seconds of context around result
    auto context = hybridSearch.GetResultContext(results[0], 30000);
}
```

## Weight Tuning Guide

| Scenario | keywordWeight | vectorWeight | Why |
|----------|--------------|-------------|-----|
| General | 0.4 | 0.6 | Balanced (default) |
| Exact terms | 0.7 | 0.3 | Prioritize BM25 |
| Concepts | 0.3 | 0.7 | Prioritize semantics |
| Names/places | 0.8 | 0.2 | Exact matches critical |
| Questions | 0.3 | 0.7 | Meaning > exact words |

## RRF Algorithm (Quick Explanation)

**Reciprocal Rank Fusion** combines rankings without score normalization:

```
RRF_score = keywordWeight/(60 + keyword_rank) + vectorWeight/(60 + vector_rank)
```

**Example**:
```
Query: "mystery detective"

Keyword results:     Vector results:
1. chunk_42          1. chunk_17
2. chunk_17          2. chunk_99
3. chunk_99          3. chunk_42

RRF scores (weights: 0.4 kw, 0.6 vec):
chunk_17: 0.4/(60+1) + 0.6/(60+0) = 0.01656 ✓ Winner!
chunk_42: 0.4/(60+0) + 0.6/(60+2) = 0.01635
chunk_99: 0.4/(60+2) + 0.6/(60+1) = 0.01629
```

Chunks appearing in both rankings get boosted!

## Result Structure

```cpp
struct HybridSearchResult
{
    int64_t chunkId;              // Database chunk ID
    SemanticChunk chunk;          // Full chunk data

    float combinedScore;          // Total RRF score
    float keywordScore;           // BM25 contribution
    float vectorScore;            // Vector contribution

    std::string snippet;          // Text preview (100 chars)
    std::string formattedTimestamp; // "1:23:45" format
};
```

## Performance Tips

1. **Use appropriate mode**: Keyword-only is faster for exact matches
2. **Adjust topK values**: Lower for small collections, higher for large
3. **Filter early**: Use mediaType/mediaId to reduce search space
4. **Cache frequent queries**: Store popular search results
5. **Monitor latency**: Adjust parameters if searches are slow

## Error Handling

```cpp
if (!hybridSearch.Initialize(&database, &embeddingEngine, vectorSearcher))
{
    // Handle initialization failure
}

if (!hybridSearch.IsInitialized())
{
    // Engine not ready
}

auto results = hybridSearch.Search("query");
if (results.empty())
{
    // No results found
}
```

## Integration Example

```cpp
class CSemanticSearchManager
{
public:
    bool Initialize()
    {
        // Open database
        if (!m_database.Open())
            return false;

        // Initialize embedding engine
        if (!m_embeddingEngine.Initialize(modelPath, vocabPath))
            return false;

        // Get vector searcher from database
        m_vectorSearcher = m_database.GetVectorSearcher();

        // Initialize hybrid search
        return m_hybridSearch.Initialize(
            &m_database,
            &m_embeddingEngine,
            m_vectorSearcher);
    }

    std::vector<HybridSearchResult> Search(const std::string& query)
    {
        return m_hybridSearch.Search(query);
    }

private:
    CSemanticDatabase m_database;
    CEmbeddingEngine m_embeddingEngine;
    CVectorSearcher* m_vectorSearcher{nullptr};
    CHybridSearchEngine m_hybridSearch;
};
```

## Files & Documentation

- **HybridSearchEngine.h** - Header with API definitions
- **HybridSearchEngine.cpp** - Implementation with RRF algorithm
- **HYBRID_SEARCH_EXAMPLE.md** - Comprehensive usage examples
- **TASK_P2-6_COMPLETE.md** - Full implementation details and RRF explanation

## Next Steps

1. Read **HYBRID_SEARCH_EXAMPLE.md** for detailed examples
2. Review **TASK_P2-6_COMPLETE.md** for algorithm details
3. Integrate into your search workflow
4. Tune weights based on your content and user feedback
5. Monitor performance and adjust parameters as needed

## Key Takeaways

- ✅ Hybrid mode provides best relevance by combining BM25 and vector search
- ✅ RRF algorithm is simple, robust, and proven effective
- ✅ Three modes support different use cases and performance needs
- ✅ Configurable weights allow tuning for specific content types
- ✅ Automatic fallback ensures robustness when embedding fails
- ✅ Result enrichment provides snippets and formatted timestamps
