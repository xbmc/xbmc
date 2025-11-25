# ChunkProcessor Implementation Guide

## Overview

The `CChunkProcessor` class is responsible for segmenting parsed content entries (subtitles, transcriptions, metadata) into optimally-sized chunks for efficient FTS5 (Full-Text Search) indexing. It provides intelligent merging and splitting strategies to maintain search performance while preserving context.

## Purpose

Text chunking is critical for semantic search because:

1. **Search Performance**: FTS5 works best with moderately-sized text chunks (10-50 words)
2. **Context Preservation**: Chunks must maintain enough context to be meaningful
3. **Deduplication**: Content hashing prevents duplicate chunks from being indexed
4. **Memory Efficiency**: Smaller chunks reduce memory overhead during search
5. **Result Relevance**: Well-chunked content produces more precise search results

## Architecture

### Files

- **ChunkProcessor.h**: Class declaration and configuration structures
- **ChunkProcessor.cpp**: Implementation of chunking algorithms
- **test/TestChunkProcessor.cpp**: Comprehensive unit tests

### Location

```
/home/user/xbmc/xbmc/semantic/ingest/
├── ChunkProcessor.h
├── ChunkProcessor.cpp
└── test/
    └── TestChunkProcessor.cpp
```

## Chunking Strategies

### 1. Merge Short Entries

**Purpose**: Combine adjacent short entries to create optimal chunk sizes.

**Use Case**: Subtitle files often have very short entries (2-5 words) that are too small for effective indexing.

**Algorithm**:
```cpp
for each entry:
    if can_merge_with_previous AND total_words <= maxChunkWords:
        merge with accumulated text
    else:
        flush accumulated chunk if >= minChunkWords
        start new accumulation
```

**Merge Conditions**:
- Time gap between entries ≤ `maxMergeGapMs` (default: 2000ms)
- Combined word count ≤ `maxChunkWords` (default: 50 words)
- Accumulated text ≥ `minChunkWords` before flushing (default: 10 words)

**Example**:
```
Input entries:
  [1000-2000ms] "Hello there."            (2 words)
  [2500-3500ms] "How are you?"            (3 words)
  [4000-5000ms] "I'm doing well."         (3 words)

Output chunk:
  [1000-5000ms] "Hello there. How are you? I'm doing well." (8 words merged)
```

**Benefits**:
- Creates meaningful chunks from fragmented subtitles
- Preserves temporal relationships
- Reduces total chunk count while maintaining searchability

### 2. Split Long Entries

**Purpose**: Break up oversized entries that exceed optimal chunk size.

**Use Case**: Audio transcription segments or long subtitle blocks.

**Algorithm**:
```cpp
split text into sentences
for each sentence:
    if current_chunk + sentence > maxChunkWords:
        flush current chunk
        start new chunk with overlap (if configured)
    add sentence to current chunk
```

**Split Characteristics**:
- Respects sentence boundaries (., !, ?)
- Estimates timing proportionally based on word progress
- Optional word overlap between chunks for context continuity
- Preserves confidence scores from original entry

**Example**:
```
Input entry:
  [1000-10000ms] "This is a very long transcription segment. It contains multiple
                  sentences. Each sentence provides context. The system should split
                  this intelligently." (50+ words)

Output chunks:
  [1000-5000ms] "This is a very long transcription segment. It contains multiple
                 sentences." (30 words)
  [5000-10000ms] "Each sentence provides context. The system should split this
                  intelligently." (25 words)
```

**Timing Estimation**:
- Distributes original time range proportionally across chunks
- Uses word count progress to estimate boundaries
- Maintains temporal accuracy for seeking

### 3. Single Text Processing

**Purpose**: Process unstructured text without inherent timing (metadata, plot summaries).

**Use Case**: Movie/TV show plot descriptions, actor biographies.

**Algorithm**:
```cpp
if text_words <= maxChunkWords:
    return single chunk
else:
    split into sentences
    build chunks respecting maxChunkWords
```

**Characteristics**:
- No timing information (startMs = 0, endMs = 0)
- Purely content-based chunking
- Maintains sentence coherence
- Full confidence (1.0)

**Example**:
```
Input text:
  "A thrilling adventure movie about explorers discovering ancient ruins.
   The team faces dangerous challenges and mysterious enemies. Will they
   survive to tell their story?"

Output chunks:
  Chunk 1: "A thrilling adventure movie about explorers discovering ancient ruins."
  Chunk 2: "The team faces dangerous challenges and mysterious enemies. Will they
            survive to tell their story?"
```

## Configuration Options

### ChunkConfig Structure

```cpp
struct ChunkConfig {
    int maxChunkWords{50};           // Target maximum words per chunk
    int minChunkWords{10};           // Minimum viable chunk size
    int overlapWords{5};             // Word overlap between split chunks
    bool mergeShortEntries{true};    // Enable/disable merging strategy
    int maxMergeGapMs{2000};         // Maximum time gap for merging (ms)
};
```

### Configuration Parameters

#### maxChunkWords (default: 50)

**Purpose**: Maximum target size for chunks.

**Tuning Guidelines**:
- **Smaller (20-30)**: More granular results, better precision, more chunks
- **Larger (70-100)**: More context per result, fewer chunks, broader matches
- **Optimal Range**: 40-60 words balances context and precision

**Impact**:
- FTS5 search performance
- Result relevance scores
- Memory usage during indexing

#### minChunkWords (default: 10)

**Purpose**: Minimum size threshold for chunk creation.

**Tuning Guidelines**:
- **Too Low (< 5)**: Creates noise in search index, poor relevance
- **Too High (> 20)**: May discard valuable short content
- **Optimal Range**: 8-15 words ensures meaningful chunks

**Impact**:
- Index quality
- Chunk rejection rate
- Content coverage

#### overlapWords (default: 5)

**Purpose**: Words to carry over between split chunks for context continuity.

**Tuning Guidelines**:
- **0**: No overlap, cleaner separation, potential context loss
- **5-10**: Good context bridge between chunks
- **> 15**: Excessive redundancy, inflated index size

**Impact**:
- Search result context quality
- Index size (more overlap = larger index)
- Cross-chunk phrase matching

#### mergeShortEntries (default: true)

**Purpose**: Enable/disable the merge strategy.

**When to Disable**:
- Processing pre-chunked content
- Maintaining strict timing boundaries
- Testing individual entry processing

**When to Enable**:
- Subtitle files with short entries
- Fragmented transcriptions
- Optimizing search performance

#### maxMergeGapMs (default: 2000)

**Purpose**: Maximum temporal gap to allow merging.

**Tuning Guidelines**:
- **Short (500-1000ms)**: Strict temporal continuity, more chunks
- **Medium (2000-3000ms)**: Balanced merging across typical pauses
- **Long (> 5000ms)**: Risk merging across scene changes

**Impact**:
- Temporal accuracy of chunks
- Merge effectiveness
- Context preservation across time gaps

## Usage Examples

### Basic Usage

```cpp
#include "semantic/ingest/ChunkProcessor.h"

using namespace KODI::SEMANTIC;

// Create processor with default config
CChunkProcessor processor;

// Process subtitle entries
std::vector<ParsedEntry> entries = /* from SubtitleParser */;
auto chunks = processor.Process(entries, mediaId, "movie", SourceType::SUBTITLE);

// Index chunks into database
for (const auto& chunk : chunks) {
    database->InsertChunk(chunk);
}
```

### Custom Configuration

```cpp
// Configure for long-form transcription
ChunkConfig config;
config.maxChunkWords = 70;      // Longer chunks for continuous speech
config.minChunkWords = 15;      // Higher minimum for quality
config.overlapWords = 10;       // More overlap for context
config.mergeShortEntries = true;
config.maxMergeGapMs = 3000;    // Allow longer pauses

CChunkProcessor processor(config);
```

### Processing Metadata

```cpp
// Process plot summary
std::string plot = videoInfo->GetPlot();
auto chunks = processor.ProcessText(plot, mediaId, "movie", SourceType::METADATA);
```

### Adaptive Configuration

```cpp
// Different configs for different source types
ChunkConfig subtitleConfig;
subtitleConfig.maxChunkWords = 40;
subtitleConfig.mergeShortEntries = true;

ChunkConfig transcriptionConfig;
transcriptionConfig.maxChunkWords = 60;
transcriptionConfig.overlapWords = 8;

// Use appropriate config
if (sourceType == SourceType::SUBTITLE) {
    processor.SetConfig(subtitleConfig);
} else if (sourceType == SourceType::TRANSCRIPTION) {
    processor.SetConfig(transcriptionConfig);
}
```

## Algorithm Details

### Word Counting

Simple whitespace-based counting using `std::istringstream`:

```cpp
int CChunkProcessor::CountWords(const std::string& text) const
{
    std::istringstream iss(text);
    std::string word;
    int count = 0;
    while (iss >> word) {
        ++count;
    }
    return count;
}
```

**Characteristics**:
- Treats consecutive whitespace as single separator
- Language-agnostic
- Fast performance
- Good-enough accuracy for chunking decisions

### Sentence Splitting

Regex-based sentence detection:

```cpp
std::regex sentenceRegex(R"([^.!?]+[.!?]+\s*)");
```

**Pattern Breakdown**:
- `[^.!?]+`: Capture non-punctuation characters
- `[.!?]+`: Require sentence-ending punctuation
- `\s*`: Optional trailing whitespace

**Handles**:
- Standard sentences with period, exclamation, question mark
- Multiple punctuation (e.g., "What?!")
- Sentences without trailing space

**Limitations**:
- Does not handle abbreviations (e.g., "Dr.", "etc.")
- No special handling for quotes or parentheses
- Simple heuristic suitable for media content

### Content Hashing

SHA256-based deduplication:

```cpp
std::string CChunkProcessor::GenerateContentHash(const std::string& text,
                                                 int64_t startMs) const
{
    std::string normalized = StringUtils::ToLower(text);
    StringUtils::RemoveDuplicatedSpacesAndTabs(normalized);
    StringUtils::Trim(normalized);

    std::string hashInput = normalized + "|" + std::to_string(startMs);
    return CDigest::Calculate(CDigest::Type::SHA256, hashInput);
}
```

**Normalization Steps**:
1. Convert to lowercase (case-insensitive comparison)
2. Remove duplicate spaces/tabs
3. Trim leading/trailing whitespace
4. Append start timestamp for uniqueness

**Benefits**:
- Detects exact duplicate content
- Timestamp prevents false positives from repeated dialogue
- SHA256 provides collision-resistant hashing
- Database can use hash as unique constraint

**Note**: Currently implemented but not automatically applied. Integration with database layer pending.

## Performance Characteristics

### Time Complexity

- **MergeShortEntries**: O(n) - Single pass through entries
- **SplitLongEntry**: O(m) - Where m = number of sentences
- **ProcessText**: O(m) - Where m = number of sentences
- **CountWords**: O(k) - Where k = text length
- **SplitIntoSentences**: O(k) - Regex processing

**Overall**: Linear time relative to input size.

### Space Complexity

- **Chunk Storage**: O(n) - Output chunks
- **Temporary Buffers**: O(k) - Largest single entry/text
- **Sentence Arrays**: O(m) - Number of sentences in longest entry

**Overall**: Linear space, bounded by input size.

### Optimization Opportunities

1. **Sentence Splitting**: Could use pre-compiled regex (static)
2. **Word Counting**: Could cache counts for repeated entries
3. **String Building**: Could use `std::string::reserve()` for accumulation
4. **Batch Processing**: Could process multiple media items in parallel

## Testing

### Test Coverage

The test suite (`TestChunkProcessor.cpp`) covers:

1. **Configuration**: Default and custom configs
2. **Empty Input**: Handles empty entries gracefully
3. **Single Entry**: Processes individual entries correctly
4. **Merging**: Short adjacent entries combine properly
5. **Time Gaps**: Respects maxMergeGapMs threshold
6. **Splitting**: Long entries split at sentence boundaries
7. **Text Processing**: Handles unstructured text
8. **Filtering**: Skips entries below minChunkWords
9. **Source Types**: Handles subtitle, transcription, metadata
10. **Confidence Tracking**: Preserves minimum confidence in merges

### Running Tests

```bash
# Build with tests enabled
cmake -DENABLE_INTERNAL_GTEST=ON ..
make

# Run chunk processor tests
./kodi-test --gtest_filter=ChunkProcessorTest.*
```

### Example Test Output

```
[----------] 13 tests from ChunkProcessorTest
[ RUN      ] ChunkProcessorTest.DefaultConfiguration
[       OK ] ChunkProcessorTest.DefaultConfiguration (0 ms)
[ RUN      ] ChunkProcessorTest.MergeShortAdjacentEntries
[       OK ] ChunkProcessorTest.MergeShortAdjacentEntries (1 ms)
[ RUN      ] ChunkProcessorTest.SplitLongEntry
[       OK ] ChunkProcessorTest.SplitLongEntry (2 ms)
...
[----------] 13 tests from ChunkProcessorTest (15 ms total)
```

## Integration Points

### Input Sources

ChunkProcessor receives parsed entries from:

1. **SubtitleParser**: SRT, VTT, SSA/ASS subtitle formats
2. **TranscriptionParser**: Whisper JSON, cloud transcription APIs
3. **MetadataParser**: Video info, plot summaries, actor bios

### Output Destination

Processed chunks are consumed by:

1. **SemanticIndexer**: Inserts chunks into FTS5 database
2. **ContentDeduplicator**: Uses content hashes to prevent duplicates
3. **EmbeddingGenerator**: Creates vector embeddings for semantic search

### Data Flow

```
ContentParser → ParsedEntry[] → ChunkProcessor → SemanticChunk[] → Database
```

## Best Practices

### 1. Choose Appropriate Config for Content Type

```cpp
// Subtitles: Enable merging, moderate chunk size
ChunkConfig subtitleConfig{50, 10, 5, true, 2000};

// Transcriptions: Larger chunks, more overlap
ChunkConfig transcriptionConfig{70, 15, 8, true, 3000};

// Metadata: Disable merging, large chunks
ChunkConfig metadataConfig{100, 20, 0, false, 0};
```

### 2. Monitor Chunk Statistics

```cpp
auto chunks = processor.Process(entries, mediaId, mediaType, sourceType);

int totalWords = 0;
for (const auto& chunk : chunks) {
    totalWords += CountWords(chunk.text);
}

CLog::Log(LOGINFO, "Generated {} chunks, {} total words, avg {:.1f} words/chunk",
          chunks.size(), totalWords, (float)totalWords / chunks.size());
```

### 3. Validate Chunk Quality

```cpp
for (const auto& chunk : chunks) {
    int words = CountWords(chunk.text);
    if (words < config.minChunkWords || words > config.maxChunkWords * 1.5) {
        CLog::Log(LOGWARNING, "Chunk size outside expected range: {} words", words);
    }

    if (chunk.startMs > chunk.endMs && chunk.endMs != 0) {
        CLog::Log(LOGERROR, "Invalid chunk timing: start={} end={}",
                  chunk.startMs, chunk.endMs);
    }
}
```

### 4. Handle Edge Cases

```cpp
// Empty content
if (text.empty() || entries.empty()) {
    return {}; // Early return
}

// Single word entries
if (CountWords(entry.text) < 2) {
    continue; // Skip or accumulate
}

// Missing timing
if (entry.startMs == entry.endMs) {
    CLog::Log(LOGDEBUG, "Entry has zero duration, using as-is");
}
```

## Future Enhancements

### Planned Improvements

1. **Language-Aware Sentence Splitting**
   - Handle abbreviations (Dr., Mrs., etc.)
   - Support multiple languages (CJK, RTL)
   - Improve punctuation handling

2. **Adaptive Chunking**
   - Analyze content distribution
   - Auto-tune config based on source characteristics
   - Learn optimal sizes from search patterns

3. **Semantic Boundaries**
   - Detect topic changes
   - Respect speaker changes in transcriptions
   - Scene boundary awareness for subtitles

4. **Content Hash Integration**
   - Automatic deduplication during insertion
   - Cross-media duplicate detection
   - Version tracking for updated content

5. **Overlap Strategy**
   - Smart overlap based on phrase boundaries
   - Preserve full sentences in overlap region
   - Context window optimization

### Research Opportunities

- **Machine Learning**: Train models to predict optimal chunk boundaries
- **Cross-Lingual**: Adapt chunking for different language characteristics
- **Quality Metrics**: Develop scoring system for chunk quality
- **A/B Testing**: Compare chunking strategies on search effectiveness

## References

### Kodi Patterns

- **StringUtils**: `/home/user/xbmc/xbmc/utils/StringUtils.h`
- **Digest**: `/home/user/xbmc/xbmc/utils/Digest.h`
- **Logging**: `/home/user/xbmc/xbmc/utils/log.h`

### Related Components

- **SemanticTypes**: `/home/user/xbmc/xbmc/semantic/SemanticTypes.h`
- **IContentParser**: `/home/user/xbmc/xbmc/semantic/ingest/IContentParser.h`
- **SubtitleParser**: `/home/user/xbmc/xbmc/semantic/ingest/SubtitleParser.h`

### FTS5 Documentation

- SQLite FTS5: https://www.sqlite.org/fts5.html
- BM25 Ranking: https://en.wikipedia.org/wiki/Okapi_BM25

## Conclusion

The ChunkProcessor provides intelligent, configurable text segmentation optimized for FTS5 search performance. By merging short entries and splitting long ones, it creates consistently-sized chunks that balance context preservation with search efficiency. The implementation follows Kodi coding standards and integrates seamlessly with the semantic search infrastructure.
