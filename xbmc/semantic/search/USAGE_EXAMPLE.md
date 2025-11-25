# CSemanticSearch Usage Examples

## Overview

The `CSemanticSearch` class provides a high-level, user-friendly API for semantic search functionality. It wraps the FTS5 full-text search capabilities of `CSemanticDatabase` with automatic query normalization, FTS5 query building, and result processing.

## Basic Initialization

```cpp
#include "semantic/search/SemanticSearch.h"
#include "semantic/SemanticDatabase.h"

// Create and initialize database
CSemanticDatabase database;
if (!database.Open())
{
    // Handle error
    return;
}

// Create and initialize search interface
CSemanticSearch search;
if (!search.Initialize(&database))
{
    // Handle error
    return;
}
```

## Simple Search

```cpp
// Basic search - query is automatically normalized and converted to FTS5
std::vector<SearchResult> results = search.Search("batman fighting joker");

for (const auto& result : results)
{
    CLog::Log(LOGINFO, "Found in {} at {}ms: {}",
              result.chunk.mediaType,
              result.chunk.startMs,
              result.snippet);
    CLog::Log(LOGINFO, "Relevance score: {}", result.score);
}
```

## Search with Options

```cpp
SearchOptions options;
options.maxResults = 20;
options.mediaType = "movie";
options.minConfidence = 0.8f;

// Search only in movies with high confidence
std::vector<SearchResult> results = search.Search("car chase scene", options);
```

## Search Within Specific Media

```cpp
int movieId = 42;
std::string mediaType = "movie";

// Search only within a specific movie
std::vector<SearchResult> results = search.SearchInMedia(
    "final battle",
    movieId,
    mediaType
);
```

## Get Context Around a Timestamp

```cpp
int episodeId = 123;
std::string mediaType = "episode";
int64_t timestampMs = 300000;  // 5 minutes into the episode
int64_t windowMs = 30000;      // ±30 seconds

// Get all chunks within 30 seconds of the timestamp
std::vector<SemanticChunk> context = search.GetContext(
    episodeId,
    mediaType,
    timestampMs,
    windowMs
);

for (const auto& chunk : context)
{
    CLog::Log(LOGINFO, "{}-{}ms: {}",
              chunk.startMs, chunk.endMs, chunk.text);
}
```

## Get All Chunks for a Media Item

```cpp
int movieId = 42;
std::string mediaType = "movie";

// Retrieve all indexed chunks for a movie
std::vector<SemanticChunk> chunks = search.GetMediaChunks(movieId, mediaType);

CLog::Log(LOGINFO, "Movie has {} indexed chunks", chunks.size());
```

## Check if Media is Searchable

```cpp
int movieId = 42;
std::string mediaType = "movie";

if (search.IsMediaSearchable(movieId, mediaType))
{
    CLog::Log(LOGINFO, "Movie {} is indexed and searchable", movieId);

    // Proceed with search
    auto results = search.SearchInMedia("explosions", movieId, mediaType);
}
else
{
    CLog::Log(LOGINFO, "Movie {} is not yet indexed", movieId);
}
```

## Get Search Statistics

```cpp
IndexStats stats = search.GetSearchStats();

CLog::Log(LOGINFO, "Total media items: {}", stats.totalMedia);
CLog::Log(LOGINFO, "Indexed media items: {}", stats.indexedMedia);
CLog::Log(LOGINFO, "Total chunks: {}", stats.totalChunks);
CLog::Log(LOGINFO, "Queued indexing jobs: {}", stats.queuedJobs);
```

## Query Normalization Examples

The `CSemanticSearch` class automatically normalizes queries:

| User Input | Normalized | FTS5 Query |
|------------|-----------|------------|
| "Batman FIGHTS joker" | "batman fights joker" | "batman* fights* joker*" |
| "  extra   spaces  " | "extra spaces" | "extra* spaces*" |
| "car-chase" | "car-chase" | "carchase*" |
| "don't" | "don't" | "don't*" |

Special FTS5 characters (`" - + * ( ) : ^`) are automatically removed to prevent query syntax errors.

## Search History (Future Feature)

The following methods are stubbed for future implementation:

```cpp
// Get search suggestions based on history
std::vector<std::string> suggestions = search.GetSuggestions("bat", 10);

// Record a search (automatically called by Search())
search.RecordSearch("batman", 42);
```

These will be fully implemented when the `semantic_search_history` table is added to the database schema.

## Integration with UI

### Example: Search Dialog Handler

```cpp
void OnSearchSubmit(const std::string& userQuery)
{
    if (!m_search.IsInitialized())
    {
        ShowError("Search not available");
        return;
    }

    // Get statistics to show in UI
    IndexStats stats = m_search.GetSearchStats();
    if (stats.indexedMedia == 0)
    {
        ShowMessage("No media has been indexed yet");
        return;
    }

    // Perform search
    std::vector<SearchResult> results = m_search.Search(userQuery);

    if (results.empty())
    {
        ShowMessage("No results found for: " + userQuery);
        return;
    }

    // Display results in list
    PopulateResultsList(results);
}
```

### Example: Jump to Timestamp

```cpp
void OnResultSelected(const SearchResult& result)
{
    // Jump to the timestamp in the media player
    int mediaId = result.chunk.mediaId;
    std::string mediaType = result.chunk.mediaType;
    int64_t timestampMs = result.chunk.startMs;

    // Load media and seek to timestamp
    PlayMedia(mediaId, mediaType, timestampMs);

    // Optionally show context
    auto context = m_search.GetContext(mediaId, mediaType, timestampMs, 60000);
    ShowContextPanel(context);
}
```

## Error Handling

All methods check initialization status and return empty results on error:

```cpp
if (!search.IsInitialized())
{
    // Returns empty vector
    auto results = search.Search("query");
    // results.empty() == true
}
```

Errors are logged via `CLog::LogF()` for debugging:
- `LOGERROR`: Critical errors (null database, uninitialized)
- `LOGWARNING`: Expected failures (no results, media not found)
- `LOGDEBUG`: Normal operations (query processing, result counts)

## Performance Considerations

1. **FTS5 Index**: All searches use SQLite's FTS5 full-text index for fast results
2. **BM25 Ranking**: Results are automatically ranked by relevance using BM25 algorithm
3. **Wildcard Searches**: Each term uses suffix wildcards (`term*`) for partial matching
4. **Query Complexity**: Simple boolean AND search between terms (all terms must match)

## Future Enhancements

- Search history and suggestions (requires schema update)
- Boolean operators (OR, NOT, phrase matching)
- Fuzzy matching and spell correction
- Search result caching
- Async search API for long-running queries
