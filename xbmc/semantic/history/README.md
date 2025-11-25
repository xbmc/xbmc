# Search History Component

## Overview

The search history component manages persistent storage and retrieval of user search queries, enabling features like recent searches, autocomplete suggestions, and search analytics.

## Architecture

```
CSearchHistory (History management)
    │
    ├─ Per-profile storage (isolated histories)
    ├─ Privacy mode (opt-out of recording)
    └─ Automatic cleanup (size limits)

CSearchSuggestions (Autocomplete)
    │
    ├─ History-based suggestions
    ├─ Frequency-based ranking
    └─ Prefix matching
```

## Files

| File | Description |
|------|-------------|
| `SearchHistory.h/cpp` | Search history management and persistence |
| `SearchSuggestions.h/cpp` | Autocomplete suggestions engine |

## Key Classes

### CSearchHistory

**Manages search history** with per-profile isolation and privacy controls.

**Features**:
- Per-profile history tracking
- Configurable size limits (default: 100 recent searches)
- Privacy mode (disable recording)
- Click tracking (which results were selected)
- Frequency tracking (popular queries)

**Usage**:
```cpp
CSearchHistory history;
history.Initialize(&database);

// Add search
history.AddSearch("detective mystery", 8 /* result count */);

// Get recent searches
auto recent = history.GetRecentSearches(10);
for (const auto& entry : recent) {
    std::cout << entry.queryText << " (" << entry.resultCount << " results)\n";
}

// Get searches by prefix (for autocomplete)
auto suggestions = history.GetSearchesByPrefix("det", 5);
// Returns: ["detective mystery", "detective noir", ...]

// Clear history
history.ClearHistory();  // Current profile only
```

### CSearchSuggestions

**Provides autocomplete suggestions** based on history and frequency.

**Suggestion Sources**:
1. **User's search history** (personalized)
2. **Popular queries** (if sharing enabled, anonymized)
3. **Common phrases** in indexed content

**Ranking**:
```cpp
SuggestionScore = FrequencyWeight × Frequency +
                  RecencyWeight × Recency +
                  ClickWeight × ClickRate

Default weights:
  FrequencyWeight = 0.5
  RecencyWeight = 0.3
  ClickWeight = 0.2
```

**Usage**:
```cpp
CSearchSuggestions suggestions;
suggestions.Initialize(&database, &history);

// Get suggestions for partial query
auto results = suggestions.GetSuggestions("det", 5);

for (const auto& suggestion : results) {
    std::cout << suggestion.text
              << " (score: " << suggestion.score << ")\n";
}
```

## Database Schema

### semantic_search_history

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

CREATE INDEX idx_history_profile ON semantic_search_history(
  profile_id, timestamp DESC
);
CREATE INDEX idx_history_query ON semantic_search_history(query_text);
CREATE INDEX idx_history_timestamp ON semantic_search_history(timestamp DESC);
```

**Example Row**:
```
id: 42
profile_id: 1
query_text: detective solving mystery
result_count: 8
timestamp: 1732115535
clicked_results: ["4521", "6789"]
```

### Search Frequency Query

```sql
-- Get search frequency for a query
SELECT query_text, COUNT(*) as frequency
FROM semantic_search_history
WHERE profile_id = ? AND query_text = ?
GROUP BY query_text;
```

## Privacy Features

### Privacy Mode

**Disable history recording**:
```cpp
history.SetPrivacyMode(true);  // Stop recording

// Searches are not stored
history.AddSearch("private query", 5);  // No-op in privacy mode

// Previous history still accessible
auto recent = history.GetRecentSearches(10);  // Returns old searches

// Clear all history
history.ClearHistory();  // Remove all traces
```

**Configuration**:
```xml
<semantic>
  <history>
    <enabled>true</enabled>
    <privacymode>false</privacymode>
    <maxentries>100</maxentries>
  </history>
</semantic>
```

### Per-Profile Isolation

**Each Kodi profile** has separate history:
```cpp
// Profile 1
history.AddSearch("kids cartoons", 10);

// Profile 2
history.AddSearch("action movies", 15);

// Profile 1's history doesn't see Profile 2's searches
auto profile1History = history.GetRecentSearches(10, /*profileId=*/1);
// Returns: ["kids cartoons"]
```

**Master profile** can view all histories (admin setting):
```cpp
if (IsMasterProfile()) {
    auto allHistory = history.GetAllRecentSearches(100);
    // Returns searches from all profiles
}
```

### Data Retention

**Automatic cleanup** of old entries:
```cpp
// Keep only last 100 searches per profile
int removed = history.CleanupOldEntries(100);
```

**Scheduled cleanup** (background task):
```cpp
// Run weekly
if (ShouldRunWeeklyMaintenance()) {
    history.CleanupOldEntries(settings.maxHistorySize);
}
```

## Click Tracking

**Track which results were clicked**:
```cpp
// User clicked result with chunk_id 4521
history.UpdateClickedResult("detective mystery", 4521);

// Query database
auto entry = history.GetMostRecentSearch("detective mystery", profileId);
// entry.clickedResultIds = [4521, 6789, ...]

// Use for ranking (clicked results = more relevant)
float clickRate = entry.clickedResultIds.size() / entry.resultCount;
```

**Click-based re-ranking**:
```cpp
// Boost results that were clicked in past searches
for (auto& result : searchResults) {
    if (WasClickedBefore(result.chunkId, query)) {
        result.score *= 1.2;  // 20% boost
    }
}
```

## Autocomplete Suggestions

### Prefix Matching

**Basic prefix search**:
```sql
SELECT DISTINCT query_text, COUNT(*) as frequency
FROM semantic_search_history
WHERE profile_id = ?
  AND query_text LIKE 'det%'  -- Prefix match
GROUP BY query_text
ORDER BY frequency DESC, MAX(timestamp) DESC
LIMIT 5;
```

**Results**:
```
detective solving mystery (frequency: 3)
detective noir (frequency: 2)
detective interrogation (frequency: 1)
```

### Fuzzy Matching

**Levenshtein distance** for typo tolerance:
```cpp
std::vector<std::string> GetFuzzySuggestions(
    const std::string& prefix, int maxDistance = 2)
{
    auto allQueries = GetAllHistoricalQueries();
    std::vector<std::string> matches;

    for (const auto& query : allQueries) {
        int distance = LevenshteinDistance(prefix, query);
        if (distance <= maxDistance) {
            matches.push_back(query);
        }
    }

    return matches;
}
```

**Example**:
```
Input: "detectiv"
Fuzzy matches: "detective", "detectives"
```

### Contextual Suggestions

**Time-based** (day of week, time of day):
```cpp
// More kids content suggestions during daytime
// More adult content suggestions during evening
auto suggestions = GetContextualSuggestions(prefix, currentTime);
```

**Recent media** (searches related to recently added):
```cpp
// User added new Sherlock Holmes movie
// Suggest: "sherlock", "detective mystery", etc.
auto suggestions = GetSuggestionsForRecentMedia(recentMovies);
```

## Configuration

**advancedsettings.xml**:
```xml
<semantic>
  <history>
    <enabled>true</enabled>
    <privacymode>false</privacymode>
    <maxentries>100</maxentries>
    <autocompletethreshold>2</autocompletethreshold>  <!-- min chars for autocomplete -->
    <trackclicks>true</trackclicks>
  </history>

  <suggestions>
    <enabled>true</enabled>
    <maxsuggestions>10</maxsuggestions>
    <includepopular>false</includepopular>  <!-- include anonymized popular queries -->
    <fuzzymatching>true</fuzzymatching>
    <fuzzydistance>2</fuzzydistance>
  </suggestions>
</semantic>
```

## UI Integration

### Search Input Autocomplete

```cpp
void CGUIDialogSemanticSearch::OnSearchTextChanged(const std::string& text) {
    if (text.length() >= m_autocompleteThreshold) {
        // Get suggestions
        auto suggestions = m_suggestions->GetSuggestions(text, 10);

        // Show dropdown
        ShowSuggestionDropdown(suggestions);
    }
}

void CGUIDialogSemanticSearch::OnSuggestionSelected(const std::string& suggestion) {
    // Fill search input
    SetSearchText(suggestion);

    // Trigger search
    PerformSearch();
}
```

### History Dropdown

```cpp
void CGUIDialogSemanticSearch::OnSearchInputFocus() {
    // Show recent searches when input is focused
    auto recent = m_history->GetRecentSearches(10);
    ShowHistoryDropdown(recent);
}
```

**Display**:
```
┌─────────────────────────────────────┐
│ Recent Searches:                    │
│  detective solving mystery (8)      │
│  action scene explosion (12)        │
│  romantic conversation (5)          │
│  ...                                │
│ ──────────────────────────────────  │
│ [Clear History]                     │
└─────────────────────────────────────┘
```

## Performance Considerations

### Indexing

**Ensure indexes exist** for fast queries:
```sql
CREATE INDEX idx_history_profile_timestamp ON semantic_search_history(
  profile_id, timestamp DESC
);
CREATE INDEX idx_history_prefix ON semantic_search_history(
  query_text COLLATE NOCASE  -- Case-insensitive prefix matching
);
```

### Caching

**Cache popular queries** in memory:
```cpp
class CSearchSuggestions {
private:
    std::vector<std::string> m_cachedPopularQueries;
    std::chrono::steady_clock::time_point m_cacheTime;

    void RefreshPopularCache() {
        if (CacheExpired()) {
            m_cachedPopularQueries = LoadPopularQueries(50);
            m_cacheTime = std::chrono::steady_clock::now();
        }
    }

public:
    std::vector<std::string> GetSuggestions(const std::string& prefix) {
        RefreshPopularCache();
        return FilterByPrefix(m_cachedPopularQueries, prefix);
    }
};
```

### Throttling

**Debounce autocomplete** to avoid excessive queries:
```cpp
void CGUIDialogSemanticSearch::OnSearchTextChanged(const std::string& text) {
    // Cancel previous timer
    m_autocompleteTimer.Cancel();

    // Start new timer (300ms debounce)
    m_autocompleteTimer.Start(300, [this, text]() {
        auto suggestions = GetSuggestions(text);
        ShowSuggestions(suggestions);
    });
}
```

## Analytics (Optional)

**Aggregate statistics** (anonymized):
```cpp
struct SearchAnalytics {
    int totalSearches;
    int uniqueQueries;
    float avgResultCount;
    float avgClickRate;
    std::vector<std::string> topQueries;
};

SearchAnalytics GetAnalytics(int profileId) {
    // Query database for stats
    // ...
}
```

**Use cases**:
- Improve search quality
- Identify common queries
- Detect issues (many searches, no clicks = poor results)

## Testing

**Unit Tests**: `xbmc/semantic/test/TestSearchHistory.cpp`

**Test Cases**:
- Add/retrieve searches
- Per-profile isolation
- Privacy mode (no recording)
- Click tracking
- Autocomplete suggestions
- Fuzzy matching
- History cleanup

## Future Enhancements

- **Query expansion** based on history (synonyms)
- **Personalized ranking** using click history
- **Search sessions** (group related searches)
- **A/B testing** (test suggestion algorithms)
- **Export/import** history (backup/restore)
- **Cloud sync** (optional, opt-in, encrypted)

## See Also

- [GUIGuide.md](../docs/GUIGuide.md) - History UI and usage
- [APIReference.md](../docs/APIReference.md) - C++ API reference
- [TechnicalDesign.md](../docs/TechnicalDesign.md) - System architecture

---

*Last Updated: 2025-11-25*
