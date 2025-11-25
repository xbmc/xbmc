# Search Filters Component

## Overview

The filters component provides comprehensive filtering capabilities for semantic search results, including media type, genre, year, rating, duration, and content source filters.

## Architecture

```
CSearchFilters (Main filter container)
    │
    ├─ Media Type Filter (movies, TV, music videos)
    ├─ Genre Filter (multi-select, OR logic)
    ├─ Year Range Filter (min/max)
    ├─ Rating Filter (MPAA ratings)
    ├─ Duration Filter (length-based)
    └─ Source Filter (subtitle, transcription, metadata)

CFilterPresetManager (Save/load filter combinations)
    │
    └─ FilterPreset (Named filter configuration)
```

## Files

| File | Description |
|------|-------------|
| `SearchFilters.h/cpp` | Main filter container and logic |
| `FilterPreset.h/cpp` | Preset management (save/load filters) |

## Key Classes

### CSearchFilters

**Comprehensive filter container** managing all filter types.

**Filter Types**:
- **Media Type**: Movies, TV Shows, Music Videos, All
- **Genre**: Multi-select with OR logic (Action OR Thriller)
- **Year Range**: Min/max year filters
- **Rating**: MPAA ratings (G, PG, PG-13, R, NC-17)
- **Duration**: Short (<30min), Medium (30-90min), Long (>90min), Custom
- **Source**: Subtitle, Transcription, Metadata (multi-select)

**Usage**:
```cpp
CSearchFilters filters;

// Media type
filters.SetMediaType(MediaTypeFilter::Movies);

// Genres (OR logic)
filters.AddGenre("Action");
filters.AddGenre("Thriller");

// Year range
filters.SetYearRange(2020, 2024);

// Rating
filters.SetRating(RatingFilter::PG13);

// Duration
filters.SetDuration(DurationFilter::Long);  // >90 minutes

// Sources
filters.SetIncludeSubtitles(true);
filters.SetIncludeTranscription(false);
filters.SetIncludeMetadata(true);

// Apply to search
HybridSearchOptions opts;
opts.genres = filters.GetGenres();
opts.minYear = filters.GetYearRange().minYear;
opts.maxYear = filters.GetYearRange().maxYear;
// ... set other options

auto results = searchEngine.Search(query, opts);
```

### FilterPreset

**Named filter configurations** for quick reuse.

**Built-in Presets**:
- **Recent Releases**: Last 2 years, all types
- **Classic Movies**: Movies before 2000
- **Family Content**: G/PG ratings
- **Recent TV**: TV shows, last 5 years

**Custom Presets**:
```cpp
CFilterPresetManager presetMgr;

// Create custom preset
FilterPreset actionPreset;
actionPreset.name = "Recent Action Movies";
actionPreset.mediaType = MediaTypeFilter::Movies;
actionPreset.genres = {"Action"};
actionPreset.minYear = 2020;
actionPreset.mpaaRating = "PG-13";

// Save preset
presetMgr.SavePreset(actionPreset);

// Load preset
auto preset = presetMgr.LoadPreset("Recent Action Movies");
if (preset.has_value()) {
    filters.ApplyPreset(preset.value());
}
```

## Filter Logic

### AND Between Filter Types

Different filter types are combined with **AND** logic:
```
Results = Movies AND (Action OR Thriller) AND (2020-2024) AND PG-13
```

### OR Within Multi-Select

Multi-select filters (genres, sources) use **OR** logic:
```
Genres: Action OR Thriller (matches either)
Sources: Subtitle OR Metadata (matches either)
```

### Range Filters

Year and duration use **inclusive ranges**:
```
Year Range: [2020, 2024]
  → Matches: 2020, 2021, 2022, 2023, 2024

Duration: Long (>90 minutes)
  → Matches: 91, 120, 180, etc.
```

## Filter Application

### In Database Query

**FTS5 Filtering**:
```sql
SELECT c.*, rank
FROM semantic_chunks c
JOIN semantic_chunks_fts fts ON c.id = fts.rowid
WHERE fts.text MATCH ?
  AND c.media_type = 'movie'  -- Media type filter
  AND c.source_type IN ('subtitle', 'metadata')  -- Source filter
ORDER BY rank
LIMIT 100;
```

**Additional Filtering** (post-query, from video database):
```cpp
std::vector<HybridSearchResult> ApplyExtendedFilters(
    const std::vector<HybridSearchResult>& results,
    const HybridSearchOptions& options)
{
    std::vector<HybridSearchResult> filtered;

    for (const auto& result : results) {
        // Get media metadata from video database
        auto metadata = videoDb.GetMediaMetadata(result.chunk.mediaId,
                                                   result.chunk.mediaType);

        // Apply genre filter
        if (!options.genres.empty()) {
            bool hasMatchingGenre = false;
            for (const auto& genre : options.genres) {
                if (metadata.genres.count(genre)) {
                    hasMatchingGenre = true;
                    break;
                }
            }
            if (!hasMatchingGenre) continue;
        }

        // Apply year filter
        if (options.minYear > 0 && metadata.year < options.minYear) continue;
        if (options.maxYear > 0 && metadata.year > options.maxYear) continue;

        // Apply rating filter
        if (!options.mpaaRating.empty() &&
            metadata.mpaaRating != options.mpaaRating) continue;

        // Apply duration filter
        if (options.minDurationMinutes > 0 &&
            metadata.durationMinutes < options.minDurationMinutes) continue;
        if (options.maxDurationMinutes > 0 &&
            metadata.durationMinutes > options.maxDurationMinutes) continue;

        filtered.push_back(result);
    }

    return filtered;
}
```

## Configuration

**advancedsettings.xml**:
```xml
<semantic>
  <filters>
    <enablepresets>true</enablepresets>
    <defaultpreset>Recent Releases</defaultpreset>
    <rememberlastfilter>true</rememberlastfilter>
  </filters>
</semantic>
```

**Storage**:
- Presets: `special://masterprofile/semantic/filter_presets.xml`
- Last used filter: Per-profile setting

## Filter Persistence

### Preset Storage (XML)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<filter_presets>
  <preset>
    <name>Recent Action Movies</name>
    <media_type>movies</media_type>
    <genres>
      <genre>Action</genre>
    </genres>
    <min_year>2020</min_year>
    <mpaa_rating>PG-13</mpaa_rating>
  </preset>
  <preset>
    <name>Classic Sci-Fi</name>
    <media_type>movies</media_type>
    <genres>
      <genre>Science Fiction</genre>
    </genres>
    <max_year>2000</max_year>
  </preset>
</filter_presets>
```

### Serialization

```cpp
std::string CSearchFilters::Serialize() const {
  rapidjson::Document doc;
  doc.SetObject();

  // Media type
  doc.AddMember("media_type", GetMediaTypeString(), doc.GetAllocator());

  // Genres
  rapidjson::Value genresArray(rapidjson::kArrayType);
  for (const auto& genre : m_genres) {
    rapidjson::Value genreVal(genre.c_str(), doc.GetAllocator());
    genresArray.PushBack(genreVal, doc.GetAllocator());
  }
  doc.AddMember("genres", genresArray, doc.GetAllocator());

  // Year range
  if (m_yearRange.IsActive()) {
    doc.AddMember("min_year", m_yearRange.minYear, doc.GetAllocator());
    doc.AddMember("max_year", m_yearRange.maxYear, doc.GetAllocator());
  }

  // ... (serialize other filters)

  return JsonToString(doc);
}
```

## UI Integration

### Filter Controls (GUI)

**Control IDs** (in CGUIDialogSemanticSearch):
- 13: Filter panel group
- 14: Genre filter list
- 15: Year range slider (min)
- 16: Year range slider (max)
- 17: Rating filter button
- 18: Duration filter button
- 19: Source filter group
- 20: Clear filters button

**Visual Indicators**:
```
Active Filters: [Movies] [Action][Thriller] [2020-2024] [PG-13] [×]
                   ×         ×        ×          ×         ×      └─ Clear all
```

### Filter Badges

```cpp
std::vector<std::string> CSearchFilters::GetActiveFilterBadges() const {
  std::vector<std::string> badges;

  if (m_mediaType != MediaTypeFilter::All) {
    badges.push_back(GetMediaTypeString());
  }

  for (const auto& genre : m_genres) {
    badges.push_back(genre);
  }

  if (m_yearRange.IsActive()) {
    badges.push_back(m_yearRange.ToString());
  }

  // ... (other filters)

  return badges;
}
```

## Testing

**Unit Tests**: `xbmc/semantic/test/TestSearchFilters.cpp`

**Test Cases**:
- Individual filter application
- Multiple filter combinations (AND logic)
- Multi-select OR logic (genres)
- Range boundary conditions
- Preset save/load
- Serialization/deserialization

**Example Test**:
```cpp
TEST(SearchFilters, GenreFilterORLogic) {
  CSearchFilters filters;
  filters.AddGenre("Action");
  filters.AddGenre("Thriller");

  // Should match movies with Action OR Thriller
  EXPECT_TRUE(filters.MatchesGenre({"Action"}));
  EXPECT_TRUE(filters.MatchesGenre({"Thriller"}));
  EXPECT_TRUE(filters.MatchesGenre({"Action", "Comedy"}));
  EXPECT_FALSE(filters.MatchesGenre({"Comedy"}));
}
```

## Performance Considerations

### Filter Order Optimization

**Apply cheap filters first** (reduce dataset early):
```cpp
// Good order:
1. Media type (narrows to specific table/index)
2. Source type (cheap, indexed)
3. Year range (cheap, indexed)
4. Genres (requires video DB lookup)
5. Rating (requires video DB lookup)
6. Duration (requires video DB lookup)
```

### Caching Metadata

**Cache video DB metadata** to avoid repeated lookups:
```cpp
std::unordered_map<int, MediaMetadata> m_metadataCache;

MediaMetadata GetMetadata(int mediaId, const std::string& mediaType) {
  auto key = std::to_string(mediaId) + "_" + mediaType;
  if (m_metadataCache.count(key)) {
    return m_metadataCache[key];
  }

  auto metadata = videoDb.GetMediaMetadata(mediaId, mediaType);
  m_metadataCache[key] = metadata;
  return metadata;
}
```

## Future Enhancements

- **Smart filters** (e.g., "Oscar winners", "High rated")
- **Collection filters** (MCU, Star Wars, etc.)
- **Actor/director filters** (all movies with specific person)
- **Language filters** (audio/subtitle languages)
- **Quality filters** (4K, HDR, etc.)
- **Location filters** (media path/source)

## See Also

- [GUIGuide.md](../docs/GUIGuide.md) - Filter usage in UI
- [APIReference.md](../docs/APIReference.md) - C++ API reference
- [TechnicalDesign.md](../docs/TechnicalDesign.md) - System architecture

---

*Last Updated: 2025-11-25*
