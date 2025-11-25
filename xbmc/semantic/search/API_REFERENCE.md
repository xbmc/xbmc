# CSemanticSearch API Reference

## Class: `KODI::SEMANTIC::CSemanticSearch`

High-level semantic search API that wraps FTS5 functionality with automatic query normalization and processing.

---

## Public Methods

### Initialization

#### `bool Initialize(CSemanticDatabase* database)`

Initialize the search interface with a database connection.

**Parameters:**
- `database` - Pointer to CSemanticDatabase instance (must remain valid)

**Returns:**
- `true` if successful
- `false` if database pointer is null

**Example:**
```cpp
CSemanticDatabase db;
db.Open();

CSemanticSearch search;
if (!search.Initialize(&db))
{
    CLog::LogF(LOGERROR, "Failed to initialize search");
}
```

---

#### `bool IsInitialized() const`

Check if the search interface has been initialized.

**Returns:**
- `true` if initialized with valid database
- `false` otherwise

**Example:**
```cpp
if (search.IsInitialized())
{
    auto results = search.Search("query");
}
```

---

### Search Operations

#### `std::vector<SearchResult> Search(const std::string& query, const SearchOptions& options = {})`

Perform a semantic search across all indexed content.

**Parameters:**
- `query` - User search query (will be normalized)
- `options` - Optional search filters and limits

**Returns:**
- Vector of `SearchResult` objects sorted by relevance (BM25 score)
- Empty vector if not initialized, query is empty, or error occurs

**Query Processing:**
1. Lowercase conversion
2. Whitespace normalization
3. FTS5 special character removal
4. Wildcard suffix addition for partial matching

**Example:**
```cpp
// Basic search
auto results = search.Search("batman fighting");

// With options
SearchOptions opts;
opts.maxResults = 20;
opts.mediaType = "movie";
opts.minConfidence = 0.8f;
auto filtered = search.Search("car chase", opts);
```

---

#### `std::vector<SearchResult> SearchInMedia(const std::string& query, int mediaId, const std::string& mediaType)`

Search within a specific media item only.

**Parameters:**
- `query` - Search query
- `mediaId` - Media item ID to search within
- `mediaType` - Media type ("movie", "episode", "musicvideo")

**Returns:**
- Vector of `SearchResult` objects from the specified media
- Empty vector on error or no results

**Example:**
```cpp
int movieId = 42;
auto results = search.SearchInMedia("final scene", movieId, "movie");

for (const auto& result : results)
{
    // All results are from movieId 42
    CLog::Log(LOGINFO, "Found at {}ms: {}",
              result.chunk.startMs, result.snippet);
}
```

---

### Context Retrieval

#### `std::vector<SemanticChunk> GetContext(int mediaId, const std::string& mediaType, int64_t timestampMs, int64_t windowMs = 60000)`

Get chunks within a time window around a specific timestamp.

**Parameters:**
- `mediaId` - Media item ID
- `mediaType` - Media type
- `timestampMs` - Center timestamp in milliseconds
- `windowMs` - Window size in milliseconds (default: 60000 = ±60s)

**Returns:**
- Vector of `SemanticChunk` objects within the time window
- Ordered by start timestamp
- Empty vector on error

**Example:**
```cpp
// Get context around 5-minute mark
int64_t timestamp = 300000;  // 5 minutes
int64_t window = 30000;      // ±30 seconds

auto context = search.GetContext(episodeId, "episode", timestamp, window);

// Returns chunks from 4:30 to 5:30
for (const auto& chunk : context)
{
    CLog::Log(LOGINFO, "{}-{}ms: {}",
              chunk.startMs, chunk.endMs, chunk.text);
}
```

---

#### `std::vector<SemanticChunk> GetMediaChunks(int mediaId, const std::string& mediaType)`

Get all indexed chunks for a media item.

**Parameters:**
- `mediaId` - Media item ID
- `mediaType` - Media type

**Returns:**
- Vector of all `SemanticChunk` objects for the media
- Ordered by start timestamp
- Empty vector on error or no chunks

**Example:**
```cpp
auto chunks = search.GetMediaChunks(movieId, "movie");

CLog::Log(LOGINFO, "Movie has {} chunks", chunks.size());

// Calculate total indexed duration
if (!chunks.empty())
{
    int64_t totalMs = chunks.back().endMs - chunks.front().startMs;
    CLog::Log(LOGINFO, "Indexed duration: {} minutes", totalMs / 60000);
}
```

---

### Status and Statistics

#### `bool IsMediaSearchable(int mediaId, const std::string& mediaType)`

Check if a media item is indexed and searchable.

**Parameters:**
- `mediaId` - Media item ID
- `mediaType` - Media type

**Returns:**
- `true` if media has completed indexing and has chunks
- `false` if not indexed, in progress, or failed

**Criteria for Searchable:**
- At least one source (subtitle/transcription/metadata) has status COMPLETED
- Chunk count > 0

**Example:**
```cpp
if (search.IsMediaSearchable(movieId, "movie"))
{
    // Safe to search
    auto results = search.SearchInMedia("explosion", movieId, "movie");
}
else
{
    // Show "Indexing in progress" message
    ShowIndexingDialog(movieId);
}
```

---

#### `IndexStats GetSearchStats()`

Get overall search database statistics.

**Returns:**
- `IndexStats` structure with counts and metrics
- Zero values on error

**IndexStats Structure:**
```cpp
struct IndexStats {
    int totalMedia;       // Total media items in system
    int indexedMedia;     // Media with completed indexing
    int totalChunks;      // Total number of chunks
    int totalWords;       // Approximate word count
    int queuedJobs;       // Pending indexing jobs
};
```

**Example:**
```cpp
IndexStats stats = search.GetSearchStats();

float indexingProgress =
    (float)stats.indexedMedia / stats.totalMedia * 100.0f;

CLog::Log(LOGINFO, "Indexing progress: {:.1f}%", indexingProgress);
CLog::Log(LOGINFO, "Total searchable chunks: {}", stats.totalChunks);
CLog::Log(LOGINFO, "Queued jobs: {}", stats.queuedJobs);
```

---

### Search History (Future Implementation)

#### `std::vector<std::string> GetSuggestions(const std::string& prefix, int maxSuggestions = 10)`

Get search suggestions based on history.

**Parameters:**
- `prefix` - Search prefix to match
- `maxSuggestions` - Maximum suggestions to return (default: 10)

**Returns:**
- Empty vector (not yet implemented)

**Future Behavior:**
- Returns recent/popular searches matching prefix
- Sorted by frequency and recency

**Example:**
```cpp
// When implemented:
auto suggestions = search.GetSuggestions("bat", 5);
// Returns: ["batman", "battle", "bathroom scene", ...]
```

---

#### `void RecordSearch(const std::string& query, int resultCount)`

Record a search for history and suggestions.

**Parameters:**
- `query` - The search query
- `resultCount` - Number of results returned

**Current Behavior:**
- Logs the call but does not persist data
- Automatically called by `Search()`

**Future Behavior:**
- Will insert into `semantic_search_history` table
- Used for suggestions and analytics

---

## Supporting Types

### SearchOptions

Configure search filters and limits.

```cpp
struct SearchOptions {
    int maxResults{50};              // Maximum results (default: 50)
    std::string mediaType;            // Filter by type (empty = all)
    int mediaId{-1};                  // Filter by ID (-1 = all)
    SourceType sourceType;            // Source type filter
    bool filterBySource{false};       // Enable source filtering
    float minConfidence{0.0f};        // Minimum confidence threshold
};
```

**Example:**
```cpp
SearchOptions opts;
opts.maxResults = 100;                // Get more results
opts.mediaType = "episode";           // Only episodes
opts.minConfidence = 0.9f;            // High confidence only
opts.filterBySource = true;
opts.sourceType = SourceType::SUBTITLE;  // Subtitles only

auto results = search.Search("dialogue", opts);
```

---

### SearchResult

Single search result with metadata.

```cpp
struct SearchResult {
    SemanticChunk chunk;    // The matching chunk data
    float score;            // BM25 relevance score (lower = better)
    std::string snippet;    // Highlighted snippet with <b> tags
};
```

**Example:**
```cpp
for (const auto& result : results)
{
    CLog::Log(LOGINFO, "Score: {:.2f}", result.score);
    CLog::Log(LOGINFO, "Media: {} (ID: {})",
              result.chunk.mediaType, result.chunk.mediaId);
    CLog::Log(LOGINFO, "Time: {}ms - {}ms",
              result.chunk.startMs, result.chunk.endMs);
    CLog::Log(LOGINFO, "Snippet: {}", result.snippet);
}
```

---

### SemanticChunk

Content chunk with timing and metadata.

```cpp
struct SemanticChunk {
    int chunkId;               // Unique chunk ID
    int mediaId;               // Parent media ID
    std::string mediaType;     // Media type
    SourceType sourceType;     // SUBTITLE/TRANSCRIPTION/METADATA
    std::string sourcePath;    // Source file path
    int startMs;               // Start timestamp (ms)
    int endMs;                 // End timestamp (ms)
    std::string text;          // Full text content
    std::string language;      // Language code
    float confidence;          // Confidence score (0.0-1.0)
    std::string createdAt;     // Creation timestamp
};
```

---

## Query Processing Details

### Normalization Pipeline

1. **Lowercase**: "BATMAN" → "batman"
2. **Trim**: "  batman  " → "batman"
3. **Deduplicate**: "batman    joker" → "batman joker"

### FTS5 Conversion

1. **Split**: "batman joker" → ["batman", "joker"]
2. **Escape**: Remove FTS5 operators `" - + * ( ) : ^`
3. **Wildcard**: "batman" → "batman*" (enables partial matching)
4. **Join**: ["batman*", "joker*"] → "batman* joker*"

### Special Character Handling

| Input | After Escape | FTS5 Query | Reason |
|-------|--------------|------------|--------|
| `don't` | `don't` | `don't*` | Apostrophes preserved |
| `car-chase` | `carchase` | `carchase*` | Hyphen removed (operator) |
| `"exact"` | `exact` | `exact*` | Quotes removed (operator) |
| `(scene)` | `scene` | `scene*` | Parens removed (operator) |

---

## Error Handling

All methods implement defensive error handling:

1. **Initialization Check**
   ```cpp
   if (!IsInitialized())
   {
       CLog::LogF(LOGERROR, "Method called before initialization");
       return {};  // Empty result
   }
   ```

2. **Exception Handling**
   ```cpp
   try
   {
       // Database operation
   }
   catch (...)
   {
       CLog::LogF(LOGERROR, "Operation failed");
   }
   return {};  // Safe return on error
   ```

3. **Empty Input Check**
   ```cpp
   if (query.empty())
   {
       CLog::LogF(LOGDEBUG, "Empty query provided");
       return {};
   }
   ```

---

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `Search()` | O(log n) | FTS5 index search |
| `SearchInMedia()` | O(log n) | FTS5 with filter |
| `GetContext()` | O(log n) | Indexed timestamp query |
| `GetMediaChunks()` | O(k) | k = chunk count for media |
| `IsMediaSearchable()` | O(1) | Single row lookup |
| `GetSearchStats()` | O(1) | Aggregate query |

**Query Processing Overhead:**
- Normalization: O(m) where m = query length
- Typically < 1ms for normal queries
- Negligible compared to database I/O

---

## Thread Safety

**Not Thread-Safe**: CSemanticSearch is not inherently thread-safe.

**Guidelines:**
- Create separate instances per thread
- Or protect shared instance with mutex
- Database pointer must remain valid for lifetime

**Example: Multi-threaded Usage**
```cpp
// Thread-local search instance
thread_local CSemanticSearch g_search;

void ThreadWorker(CSemanticDatabase* sharedDB)
{
    if (!g_search.IsInitialized())
    {
        g_search.Initialize(sharedDB);
    }

    auto results = g_search.Search("query");
}
```

---

## Dependencies

### Required Headers
```cpp
#include "semantic/search/SemanticSearch.h"
#include "semantic/SemanticDatabase.h"
```

### Linked Libraries
- Kodi core libraries (utils, dbwrappers)
- SQLite3 (via CDatabase)
- Standard C++ library

---

## Version History

**Wave 0 (v1.0)** - Initial Implementation
- Core search functionality
- Query normalization and FTS5 building
- Context retrieval
- Media-specific search
- Status and statistics
- Search history stubs

**Future (v1.1)** - Search History
- Implement `RecordSearch()`
- Implement `GetSuggestions()`
- Add `semantic_search_history` table

**Future (v2.0)** - Advanced Features
- Boolean operators (AND, OR, NOT)
- Phrase matching
- Fuzzy search
- Spell correction
- Result caching
- Async API
