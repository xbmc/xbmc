# HybridSearchEngine Usage Examples

This document demonstrates how to use the CHybridSearchEngine class for combining FTS5 keyword search with vector similarity search.

## Overview

CHybridSearchEngine provides three search modes:
- **Hybrid Mode** (default): Combines FTS5 and vector search using Reciprocal Rank Fusion (RRF)
- **Keyword-Only Mode**: Fast BM25-based full-text search
- **Semantic-Only Mode**: Vector similarity search for conceptual matches

## Basic Setup

```cpp
#include "semantic/search/HybridSearchEngine.h"
#include "semantic/search/VectorSearcher.h"
#include "semantic/SemanticDatabase.h"
#include "semantic/embedding/EmbeddingEngine.h"

// Initialize components
CSemanticDatabase database;
database.Open();

CEmbeddingEngine embeddingEngine;
embeddingEngine.Initialize("/path/to/model.onnx", "/path/to/vocab.txt");

// Get the database's vector searcher (created during Open())
CVectorSearcher* vectorSearcher = database.GetVectorSearcher();

// Initialize hybrid search engine
CHybridSearchEngine hybridSearch;
hybridSearch.Initialize(&database, &embeddingEngine, vectorSearcher);
```

## Example 1: Basic Hybrid Search

```cpp
// Perform a hybrid search with default settings
auto results = hybridSearch.Search("detective solving mystery");

// Process results
for (const auto& result : results)
{
    std::cout << "Chunk ID: " << result.chunkId << std::endl;
    std::cout << "Combined Score: " << result.combinedScore << std::endl;
    std::cout << "Keyword Score: " << result.keywordScore << std::endl;
    std::cout << "Vector Score: " << result.vectorScore << std::endl;
    std::cout << "Timestamp: " << result.formattedTimestamp << std::endl;
    std::cout << "Snippet: " << result.snippet << std::endl;
    std::cout << "Full text: " << result.chunk.text << std::endl;
    std::cout << "---" << std::endl;
}
```

## Example 2: Configuring Search Options

```cpp
HybridSearchOptions options;

// Set search mode
options.mode = SearchMode::Hybrid;

// Adjust weighting (default: 0.4 keyword, 0.6 vector)
options.keywordWeight = 0.5f;  // Increase keyword importance
options.vectorWeight = 0.5f;   // Equal weighting

// Configure candidate pool sizes
options.keywordTopK = 200;  // Get top 200 keyword results
options.vectorTopK = 200;   // Get top 200 vector results
options.maxResults = 30;    // Return top 30 combined results

// Add filters
options.mediaType = "movie";  // Only search movies
options.minConfidence = 0.5f; // Minimum confidence threshold

auto results = hybridSearch.Search("time travel paradox", options);
```

## Example 3: Keyword-Only Search (Fastest)

```cpp
HybridSearchOptions options;
options.mode = SearchMode::KeywordOnly;
options.maxResults = 50;

// Fast exact-match search using BM25
auto results = hybridSearch.Search("police chief badge", options);
```

## Example 4: Semantic-Only Search (Conceptual)

```cpp
HybridSearchOptions options;
options.mode = SearchMode::SemanticOnly;
options.maxResults = 20;

// Find conceptually similar content (may not contain exact words)
auto results = hybridSearch.Search("fear and anxiety", options);
// May return results about "worried", "scared", "nervous", etc.
```

## Example 5: Search Within Specific Media

```cpp
HybridSearchOptions options;
options.mediaType = "episode";
options.mediaId = 12345;  // Specific episode ID
options.maxResults = 10;

// Search within a single episode
auto results = hybridSearch.Search("final confrontation", options);
```

## Example 6: Find Similar Chunks

```cpp
// Find chunks similar to a specific chunk (useful for "More Like This")
int64_t sourceChunkId = 42;
int topK = 15;

auto similarResults = hybridSearch.FindSimilar(sourceChunkId, topK);

for (const auto& result : similarResults)
{
    std::cout << "Similar chunk: " << result.snippet << std::endl;
    std::cout << "Similarity: " << result.vectorScore << std::endl;
}
```

## Example 7: Get Temporal Context

```cpp
// Perform search
auto results = hybridSearch.Search("hero saves day");

if (!results.empty())
{
    // Get context around the first result (30 seconds before and after)
    auto context = hybridSearch.GetResultContext(results[0], 30000);

    std::cout << "Context around result:" << std::endl;
    for (const auto& chunk : context)
    {
        std::cout << "[" << chunk.startMs << "ms] " << chunk.text << std::endl;
    }
}
```

## Example 8: Custom Weight Profiles

```cpp
// Profile 1: Favor exact matches (keyword-heavy)
HybridSearchOptions exactMatchProfile;
exactMatchProfile.keywordWeight = 0.7f;
exactMatchProfile.vectorWeight = 0.3f;

// Profile 2: Favor semantic similarity (vector-heavy)
HybridSearchOptions semanticProfile;
semanticProfile.keywordWeight = 0.3f;
semanticProfile.vectorWeight = 0.7f;

// Profile 3: Balanced (default)
HybridSearchOptions balancedProfile;
balancedProfile.keywordWeight = 0.4f;
balancedProfile.vectorWeight = 0.6f;

// Use appropriate profile based on query type
auto results = hybridSearch.Search("quantum physics", semanticProfile);
```

## Example 9: Multi-Result Processing

```cpp
auto results = hybridSearch.Search("car chase explosion");

// Group results by media
std::map<int, std::vector<HybridSearchResult>> resultsByMedia;
for (const auto& result : results)
{
    resultsByMedia[result.chunk.mediaId].push_back(result);
}

// Process each media
for (const auto& [mediaId, mediaResults] : resultsByMedia)
{
    std::cout << "Media ID: " << mediaId << std::endl;
    std::cout << "Matches: " << mediaResults.size() << std::endl;

    // Show best match
    if (!mediaResults.empty())
    {
        std::cout << "Best: " << mediaResults[0].snippet << std::endl;
    }
}
```

## Example 10: Adaptive Search Strategy

```cpp
std::vector<HybridSearchResult> AdaptiveSearch(
    CHybridSearchEngine& engine,
    const std::string& query)
{
    // Start with hybrid search
    HybridSearchOptions options;
    options.mode = SearchMode::Hybrid;
    options.maxResults = 20;

    auto results = engine.Search(query, options);

    // If hybrid returns few results, try keyword-only
    if (results.size() < 5)
    {
        CLog::LogF(LOGINFO, "Hybrid returned {} results, trying keyword-only",
                   results.size());
        options.mode = SearchMode::KeywordOnly;
        options.maxResults = 20;
        results = engine.Search(query, options);
    }

    // If still few results, try semantic-only
    if (results.size() < 5)
    {
        CLog::LogF(LOGINFO, "Keyword returned {} results, trying semantic-only",
                   results.size());
        options.mode = SearchMode::SemanticOnly;
        options.maxResults = 20;
        results = engine.Search(query, options);
    }

    return results;
}
```

## Understanding Reciprocal Rank Fusion (RRF)

The hybrid mode uses RRF to combine keyword and vector search rankings:

```
RRF_score(chunk) = keyword_weight / (k + keyword_rank) +
                   vector_weight / (k + vector_rank)

where k = 60 (standard RRF constant)
```

### RRF Benefits:
1. **Rank-based**: Uses positions rather than raw scores (more robust)
2. **Cross-metric**: Combines different scoring systems (BM25 and cosine similarity)
3. **Proven**: Well-researched algorithm used in production search systems
4. **Configurable**: Adjust weights based on use case

### Example RRF Calculation:

```
Query: "detective mystery"

Keyword results (BM25):
1. chunk_42 (rank=0)  -> RRF = 0.4 / (60 + 0) = 0.00667
2. chunk_17 (rank=1)  -> RRF = 0.4 / (60 + 1) = 0.00656
3. chunk_99 (rank=2)  -> RRF = 0.4 / (60 + 2) = 0.00645

Vector results (cosine):
1. chunk_17 (rank=0)  -> RRF = 0.6 / (60 + 0) = 0.01000
2. chunk_55 (rank=1)  -> RRF = 0.6 / (60 + 1) = 0.00984
3. chunk_42 (rank=2)  -> RRF = 0.6 / (60 + 2) = 0.00968

Combined scores:
chunk_17: 0.00656 + 0.01000 = 0.01656 (1st - appears in both, high in vector)
chunk_42: 0.00667 + 0.00968 = 0.01635 (2nd - appears in both, high in keyword)
chunk_55: 0.00000 + 0.00984 = 0.00984 (3rd - vector only)
chunk_99: 0.00645 + 0.00000 = 0.00645 (4th - keyword only)
```

## Performance Considerations

### Hybrid Mode
- **Speed**: Moderate (runs both searches in parallel)
- **Recall**: Highest (finds both exact and semantic matches)
- **Precision**: Highest (RRF combines best of both)
- **Use when**: You want optimal relevance

### Keyword-Only Mode
- **Speed**: Fastest (FTS5 is very efficient)
- **Recall**: Good for exact matches
- **Precision**: Good for specific terms
- **Use when**: You need speed or have specific terms

### Semantic-Only Mode
- **Speed**: Fast (vector search is efficient)
- **Recall**: Good for concepts
- **Precision**: May return unexpected matches
- **Use when**: You want conceptual similarity

## Best Practices

1. **Default to Hybrid**: Start with hybrid mode for best results
2. **Adjust Weights**: Tune keyword/vector weights based on your content
3. **Filter Early**: Use mediaType and mediaId filters to reduce search space
4. **Monitor Performance**: Track search times and adjust topK values
5. **Cache Results**: Consider caching frequently searched queries
6. **Explain Results**: Show users why results matched (keyword vs semantic)
7. **Test Edge Cases**: Try short queries, long queries, misspellings, etc.

## Error Handling

```cpp
CHybridSearchEngine engine;
if (!engine.Initialize(&database, &embeddingEngine, vectorSearcher))
{
    CLog::LogF(LOGERROR, "Failed to initialize hybrid search engine");
    // Handle error
}

if (!engine.IsInitialized())
{
    CLog::LogF(LOGERROR, "Engine not initialized before search");
    return;
}

auto results = engine.Search("query");
if (results.empty())
{
    CLog::LogF(LOGINFO, "No results found for query");
    // Show "no results" message to user
}
```

## Integration with Kodi UI

```cpp
// Example: Search from GUI
void CGUIDialogSemanticSearch::OnSearch()
{
    std::string query = GetSearchText();

    HybridSearchOptions options;
    options.maxResults = 50;

    // Show progress
    ShowBusyDialog();

    // Perform search
    auto results = m_hybridSearch->Search(query, options);

    HideBusyDialog();

    // Update UI with results
    for (const auto& result : results)
    {
        AddResult(result.chunk.mediaId,
                 result.chunk.mediaType,
                 result.formattedTimestamp,
                 result.snippet,
                 result.combinedScore);
    }
}
```
