# Kodi Semantic Search - Developer Guide

## Table of Contents

1. [Getting Started](#getting-started)
2. [Adding New Content Parsers](#adding-new-content-parsers)
3. [Adding New Transcription Providers](#adding-new-transcription-providers)
4. [Extending the Indexing Pipeline](#extending-the-indexing-pipeline)
5. [Code Contribution Guidelines](#code-contribution-guidelines)
6. [Testing Strategy](#testing-strategy)
7. [Debugging Tips](#debugging-tips)

---

## Getting Started

### Development Environment Setup

**Prerequisites:**
- Kodi development environment (see main Kodi build docs)
- SQLite 3.35+ with FTS5 support
- FFmpeg (for audio extraction testing)
- Google Test (for unit tests)

**Building with Semantic Search:**

```bash
cd kodi
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

The semantic search module is built automatically with Kodi.

### Project Structure

```
xbmc/semantic/
├── SemanticDatabase.h/cpp          # Core database layer
├── SemanticIndexService.h/cpp      # Orchestrator service
├── SemanticTypes.h                 # Common data structures
├── ingest/                         # Content parsing
│   ├── IContentParser.h            # Parser interface
│   ├── ContentParserBase.h/cpp     # Base implementation
│   ├── SubtitleParser.h/cpp        # Subtitle parsing
│   ├── MetadataParser.h/cpp        # Metadata extraction
│   ├── ChunkProcessor.h/cpp        # Text segmentation
│   └── test/                       # Parser tests
├── transcription/                  # Audio transcription
│   ├── ITranscriptionProvider.h    # Provider interface
│   ├── TranscriptionProviderManager.h/cpp
│   ├── GroqProvider.h/cpp          # Groq Whisper implementation
│   ├── AudioExtractor.h/cpp        # FFmpeg wrapper
│   └── test/                       # Transcription tests
├── search/                         # Search functionality
│   ├── SemanticSearch.h/cpp        # High-level search API
│   └── test/                       # Search tests
├── docs/                           # Documentation
└── test/                           # Integration tests
```

### Key Concepts

**SemanticChunk:** The fundamental unit of indexed content. Contains:
- Text content (10-50 words typically)
- Timing information (start/end timestamps)
- Source information (subtitle, transcription, metadata)
- Media association (media ID + type)

**ParsedEntry:** Intermediate representation from parsers. Contains:
- Raw text extracted from source
- Timing information (if available)
- Confidence score
- Language code

**Index State:** Per-media tracking of indexing progress:
- Subtitle status (pending/in-progress/completed/failed)
- Transcription status + progress
- Metadata status
- Chunk count

---

## Adding New Content Parsers

Content parsers extract searchable text from various file formats. Follow this guide to add support for new formats.

### Step 1: Define Parser Interface

All parsers must implement `IContentParser`:

```cpp
// xbmc/semantic/ingest/IContentParser.h
namespace KODI::SEMANTIC {

class IContentParser
{
public:
  virtual ~IContentParser() = default;

  // Parse file and return entries
  virtual std::vector<ParsedEntry> Parse(const std::string& path) = 0;

  // Check if this parser supports the file
  virtual bool CanParse(const std::string& path) const = 0;

  // Get supported extensions
  virtual std::vector<std::string> GetSupportedExtensions() const = 0;
};

} // namespace KODI::SEMANTIC
```

### Step 2: Create Parser Implementation

**Example: Adding a LRC (Lyrics) Parser**

```cpp
// xbmc/semantic/ingest/LyricsParser.h
#pragma once

#include "ContentParserBase.h"

namespace KODI::SEMANTIC {

/*!
 * \brief Parser for LRC (Lyrics) files
 *
 * Parses synchronized lyrics files with timestamps.
 * Format: [MM:SS.xx]Lyric text
 */
class CLyricsParser : public CContentParserBase
{
public:
  std::vector<ParsedEntry> Parse(const std::string& path) override;
  bool CanParse(const std::string& path) const override;
  std::vector<std::string> GetSupportedExtensions() const override;

private:
  /*!
   * \brief Parse LRC timestamp [MM:SS.xx]
   * \return Milliseconds, or -1 if invalid
   */
  static int64_t ParseLRCTimestamp(const std::string& timestamp);
};

} // namespace KODI::SEMANTIC
```

```cpp
// xbmc/semantic/ingest/LyricsParser.cpp
#include "LyricsParser.h"
#include "utils/StringUtils.h"
#include <fstream>
#include <regex>

using namespace KODI::SEMANTIC;

std::vector<ParsedEntry> CLyricsParser::Parse(const std::string& path)
{
  std::vector<ParsedEntry> entries;

  // Read file
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("Cannot open LRC file: " + path);

  // Regex for LRC format: [MM:SS.xx]Text
  std::regex lrcPattern(R"(\[(\d{2}):(\d{2})\.(\d{2})\](.+))");
  std::string line;

  while (std::getline(file, line))
  {
    std::smatch match;
    if (std::regex_match(line, match, lrcPattern))
    {
      int minutes = std::stoi(match[1]);
      int seconds = std::stoi(match[2]);
      int centiseconds = std::stoi(match[3]);

      ParsedEntry entry;
      entry.startMs = (minutes * 60 + seconds) * 1000 + centiseconds * 10;
      entry.endMs = entry.startMs + 3000; // Assume 3s duration
      entry.text = StringUtils::Trim(match[4]);
      entry.language = ""; // Could be detected
      entry.confidence = 1.0f;

      if (!entry.text.empty())
        entries.push_back(entry);
    }
  }

  return entries;
}

bool CLyricsParser::CanParse(const std::string& path) const
{
  return StringUtils::EndsWithNoCase(path, ".lrc");
}

std::vector<std::string> CLyricsParser::GetSupportedExtensions() const
{
  return {"lrc"};
}
```

### Step 3: Register Parser

Register your parser in `CSemanticIndexService` or create a factory:

```cpp
// In SemanticIndexService.cpp constructor
m_parsers.push_back(std::make_unique<CSubtitleParser>());
m_parsers.push_back(std::make_unique<CMetadataParser>());
m_parsers.push_back(std::make_unique<CLyricsParser>()); // NEW
```

### Step 4: Add Unit Tests

```cpp
// xbmc/semantic/ingest/test/TestLyricsParser.cpp
#include "ingest/LyricsParser.h"
#include <gtest/gtest.h>

using namespace KODI::SEMANTIC;

TEST(LyricsParser, ParsesSimpleLRC)
{
  // Create test LRC file
  std::string testFile = "/tmp/test.lrc";
  std::ofstream out(testFile);
  out << "[00:12.00]First line\n";
  out << "[00:17.50]Second line\n";
  out.close();

  CLyricsParser parser;
  auto entries = parser.Parse(testFile);

  ASSERT_EQ(entries.size(), 2);
  EXPECT_EQ(entries[0].text, "First line");
  EXPECT_EQ(entries[0].startMs, 12000);
  EXPECT_EQ(entries[1].text, "Second line");
  EXPECT_EQ(entries[1].startMs, 17500);

  std::remove(testFile.c_str());
}

TEST(LyricsParser, CanParseDetection)
{
  CLyricsParser parser;
  EXPECT_TRUE(parser.CanParse("song.lrc"));
  EXPECT_TRUE(parser.CanParse("song.LRC"));
  EXPECT_FALSE(parser.CanParse("song.txt"));
}
```

### Best Practices

1. **Inherit from ContentParserBase** when possible (provides common utilities)
2. **Handle encoding properly** (UTF-8, UTF-16, etc.)
3. **Validate input** and throw descriptive exceptions
4. **Extract timing** with millisecond precision when available
5. **Set confidence scores** based on source reliability
6. **Document format** in header comments
7. **Add comprehensive tests** for edge cases

---

## Adding New Transcription Providers

Transcription providers convert audio to text. Follow this guide to integrate new cloud or local providers.

### Step 1: Implement Provider Interface

```cpp
// xbmc/semantic/transcription/ITranscriptionProvider.h
namespace KODI::SEMANTIC {

class ITranscriptionProvider
{
public:
  virtual ~ITranscriptionProvider() = default;

  // Provider identification
  virtual std::string GetId() const = 0;
  virtual std::string GetName() const = 0;

  // Configuration
  virtual bool IsConfigured() const = 0;
  virtual bool IsAvailable() const = 0;
  virtual bool IsLocal() const = 0;

  // Cost estimation
  virtual float GetCostPerMinute() const = 0;

  // Transcription
  virtual bool TranscribeAudio(const std::string& audioPath,
                               std::vector<ParsedEntry>& entries,
                               const std::string& language = "en") = 0;

  // Progress callback (optional)
  virtual void SetProgressCallback(
    std::function<void(float)> callback) {}
};

} // namespace KODI::SEMANTIC
```

### Step 2: Create Provider Implementation

**Example: OpenAI Whisper Provider**

```cpp
// xbmc/semantic/transcription/OpenAIProvider.h
#pragma once

#include "ITranscriptionProvider.h"
#include <atomic>
#include <functional>

namespace KODI::SEMANTIC {

/*!
 * \brief OpenAI Whisper API transcription provider
 *
 * Supports Whisper v2 and v3 models via OpenAI API.
 */
class COpenAIProvider : public ITranscriptionProvider
{
public:
  COpenAIProvider();
  ~COpenAIProvider() override;

  // ITranscriptionProvider implementation
  std::string GetId() const override { return "openai"; }
  std::string GetName() const override { return "OpenAI Whisper"; }

  bool IsConfigured() const override;
  bool IsAvailable() const override;
  bool IsLocal() const override { return false; }

  float GetCostPerMinute() const override { return 0.006f; } // $0.006/min

  bool TranscribeAudio(const std::string& audioPath,
                       std::vector<ParsedEntry>& entries,
                       const std::string& language = "en") override;

  void SetProgressCallback(std::function<void(float)> callback) override
  {
    m_progressCallback = callback;
  }

  /*!
   * \brief Set OpenAI API key
   * \param apiKey The API key
   */
  void SetApiKey(const std::string& apiKey);

  /*!
   * \brief Set model to use
   * \param model Model name: "whisper-1"
   */
  void SetModel(const std::string& model) { m_model = model; }

private:
  std::string m_apiKey;
  std::string m_model{"whisper-1"};
  std::function<void(float)> m_progressCallback;
  std::atomic<bool> m_cancelled{false};

  /*!
   * \brief Upload audio and get transcription
   * \param audioPath Path to audio file
   * \param language Language code
   * \return JSON response string
   */
  std::string CallWhisperAPI(const std::string& audioPath,
                             const std::string& language);

  /*!
   * \brief Parse OpenAI response to entries
   */
  std::vector<ParsedEntry> ParseResponse(const std::string& jsonResponse);
};

} // namespace KODI::SEMANTIC
```

```cpp
// xbmc/semantic/transcription/OpenAIProvider.cpp
#include "OpenAIProvider.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "ServiceBroker.h"
#include "utils/JSONVariantParser.h"
#include "utils/log.h"
#include "network/HttpClient.h"

using namespace KODI::SEMANTIC;

COpenAIProvider::COpenAIProvider()
{
  // Load API key from settings
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_apiKey = settings->GetString("semanticsearch.openai.apikey");
}

COpenAIProvider::~COpenAIProvider() = default;

bool COpenAIProvider::IsConfigured() const
{
  return !m_apiKey.empty();
}

bool COpenAIProvider::IsAvailable() const
{
  // Check network connectivity and API status
  return IsConfigured() && CServiceBroker::GetNetwork().IsAvailable();
}

bool COpenAIProvider::TranscribeAudio(const std::string& audioPath,
                                      std::vector<ParsedEntry>& entries,
                                      const std::string& language)
{
  if (!IsAvailable())
  {
    CLog::LogF(LOGERROR, "Provider not available");
    return false;
  }

  try
  {
    // Call API
    if (m_progressCallback)
      m_progressCallback(0.1f);

    std::string response = CallWhisperAPI(audioPath, language);

    if (m_progressCallback)
      m_progressCallback(0.8f);

    // Parse response
    entries = ParseResponse(response);

    if (m_progressCallback)
      m_progressCallback(1.0f);

    return !entries.empty();
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "Transcription failed: {}", e.what());
    return false;
  }
}

std::string COpenAIProvider::CallWhisperAPI(const std::string& audioPath,
                                            const std::string& language)
{
  // Construct multipart/form-data request
  CHttpClient client;
  client.SetHeader("Authorization", "Bearer " + m_apiKey);

  std::map<std::string, std::string> formData;
  formData["model"] = m_model;
  formData["language"] = language;
  formData["response_format"] = "verbose_json"; // Get timestamps

  std::map<std::string, std::string> files;
  files["file"] = audioPath;

  std::string response;
  if (!client.PostMultipart("https://api.openai.com/v1/audio/transcriptions",
                            formData, files, response))
  {
    throw std::runtime_error("HTTP request failed");
  }

  return response;
}

std::vector<ParsedEntry> COpenAIProvider::ParseResponse(
  const std::string& jsonResponse)
{
  std::vector<ParsedEntry> entries;

  CVariant json;
  if (!CJSONVariantParser::Parse(jsonResponse, json))
    throw std::runtime_error("Invalid JSON response");

  // OpenAI verbose_json format includes segments with timestamps
  if (!json.isMember("segments"))
    throw std::runtime_error("Response missing segments");

  const CVariant& segments = json["segments"];
  for (unsigned int i = 0; i < segments.size(); i++)
  {
    const CVariant& segment = segments[i];

    ParsedEntry entry;
    entry.text = segment["text"].asString();
    entry.startMs = static_cast<int64_t>(segment["start"].asDouble() * 1000);
    entry.endMs = static_cast<int64_t>(segment["end"].asDouble() * 1000);
    entry.confidence = 1.0f; // OpenAI doesn't provide confidence
    entry.language = json["language"].asString();

    if (!entry.text.empty())
      entries.push_back(entry);
  }

  return entries;
}

void COpenAIProvider::SetApiKey(const std::string& apiKey)
{
  m_apiKey = apiKey;
}
```

### Step 3: Register Provider

```cpp
// In TranscriptionProviderManager::Initialize()
bool CTranscriptionProviderManager::Initialize()
{
  // Existing providers
  RegisterProvider(std::make_unique<CGroqProvider>());

  // Add new provider
  RegisterProvider(std::make_unique<COpenAIProvider>());

  LoadProviderSettings();
  return true;
}
```

### Step 4: Add Settings

```xml
<!-- In system/settings/settings.xml -->
<setting id="semanticsearch.openai.apikey" type="string" label="32051" help="32052">
  <level>3</level>
  <default></default>
  <control type="edit" format="string">
    <heading>32051</heading>
  </control>
</setting>
```

### Step 5: Add Tests

```cpp
// xbmc/semantic/transcription/test/TestOpenAIProvider.cpp
#include "transcription/OpenAIProvider.h"
#include <gtest/gtest.h>

TEST(OpenAIProvider, RequiresApiKey)
{
  COpenAIProvider provider;
  EXPECT_FALSE(provider.IsConfigured());

  provider.SetApiKey("sk-test-key");
  EXPECT_TRUE(provider.IsConfigured());
}

TEST(OpenAIProvider, IdentificationCorrect)
{
  COpenAIProvider provider;
  EXPECT_EQ(provider.GetId(), "openai");
  EXPECT_EQ(provider.GetName(), "OpenAI Whisper");
  EXPECT_FALSE(provider.IsLocal());
}

// Integration test (requires API key)
TEST(OpenAIProvider, DISABLED_TranscriptionIntegration)
{
  COpenAIProvider provider;
  provider.SetApiKey(std::getenv("OPENAI_API_KEY"));

  std::vector<ParsedEntry> entries;
  bool success = provider.TranscribeAudio("test_audio.mp3", entries, "en");

  ASSERT_TRUE(success);
  EXPECT_GT(entries.size(), 0);
}
```

### Best Practices

1. **Handle API errors gracefully** with detailed logging
2. **Implement retry logic** with exponential backoff
3. **Respect rate limits** (track requests per minute)
4. **Support cancellation** via atomic flag
5. **Provide progress callbacks** for long operations
6. **Validate API responses** thoroughly
7. **Use connection pooling** for multiple requests
8. **Cache results** when appropriate
9. **Document API requirements** (keys, quotas, etc.)

---

## Extending the Indexing Pipeline

### Adding Custom Processing Steps

You can extend the indexing pipeline by modifying `CSemanticIndexService::ProcessItem()`:

```cpp
void CSemanticIndexService::ProcessItem(const QueueItem& item)
{
  // Existing steps
  IndexSubtitles(item.mediaId, item.mediaType);
  IndexMetadata(item.mediaId, item.mediaType);

  if (item.transcribe || ShouldTranscribe(item.mediaId))
  {
    StartTranscription(item.mediaId, item.mediaType);
  }

  // ADD CUSTOM STEP: Index scene descriptions from AI
  if (IsSceneDetectionEnabled())
  {
    IndexSceneDescriptions(item.mediaId, item.mediaType);
  }
}
```

### Custom Chunk Processors

Create specialized chunk processors for domain-specific needs:

```cpp
class CDialogueChunkProcessor : public CChunkProcessor
{
public:
  // Override to detect speaker changes
  std::vector<SemanticChunk> Process(
    const std::vector<ParsedEntry>& entries,
    int mediaId,
    const std::string& mediaType,
    SourceType sourceType) override
  {
    auto chunks = CChunkProcessor::Process(entries, mediaId,
                                           mediaType, sourceType);

    // Post-process to add speaker metadata
    for (auto& chunk : chunks)
    {
      chunk.metadata["speaker"] = DetectSpeaker(chunk.text);
    }

    return chunks;
  }

private:
  std::string DetectSpeaker(const std::string& text)
  {
    // Speaker detection logic
    // e.g., from ASS format speaker field
    return "unknown";
  }
};
```

### Adding New Index State Fields

To track additional metadata, extend `SemanticIndexState`:

```cpp
// In SemanticTypes.h
struct SemanticIndexState
{
  // Existing fields...
  IndexStatus embeddingStatus{IndexStatus::PENDING};

  // NEW: Add custom fields
  IndexStatus sceneDetectionStatus{IndexStatus::PENDING};
  float sceneDetectionProgress{0.0f};
  int sceneCount{0};
};
```

Then update database schema:

```cpp
// In SemanticDatabase.cpp::CreateTables()
std::string sql = "ALTER TABLE semantic_index_state "
                  "ADD COLUMN scene_detection_status TEXT DEFAULT 'pending'";
ExecuteQuery(sql);
```

---

## Code Contribution Guidelines

### Coding Standards

Follow Kodi's coding standards:

1. **Naming Conventions:**
   - Classes: `CSemanticDatabase` (C prefix)
   - Interfaces: `IContentParser` (I prefix)
   - Member variables: `m_database` (m_ prefix)
   - Constants: `MAX_CHUNK_SIZE` (UPPER_SNAKE_CASE)

2. **Formatting:**
   - Use clang-format with Kodi's .clang-format
   - 2-space indentation
   - 100-character line limit

3. **Comments:**
   - Use Doxygen-style comments for public APIs
   - `\brief` for brief descriptions
   - `\param` for parameters
   - `\return` for return values

**Example:**

```cpp
/*!
 * \brief Process parsed entries into optimized chunks
 * \param entries Vector of parsed entries with timing information
 * \param mediaId The media item ID
 * \param mediaType The media type (e.g., "movie", "episode")
 * \param sourceType The source type (subtitle, transcription, metadata)
 * \return Vector of semantic chunks ready for indexing
 */
std::vector<SemanticChunk> Process(const std::vector<ParsedEntry>& entries,
                                   int mediaId,
                                   const std::string& mediaType,
                                   SourceType sourceType);
```

### Pull Request Process

1. **Create feature branch:** `git checkout -b feature/new-parser`
2. **Implement feature** with tests
3. **Run tests:** `make test`
4. **Format code:** `clang-format -i *.cpp *.h`
5. **Commit with clear message:**
   ```
   feat(semantic): Add LRC lyrics parser support

   - Implement CLyricsParser for .lrc files
   - Add timestamp parsing for synchronized lyrics
   - Include unit tests with 95% coverage
   ```
6. **Push and create PR** on GitHub
7. **Address review feedback**
8. **Squash commits** if requested

### Commit Message Format

Follow conventional commits:

- `feat(semantic):` New feature
- `fix(semantic):` Bug fix
- `docs(semantic):` Documentation only
- `test(semantic):` Add/update tests
- `refactor(semantic):` Code refactoring
- `perf(semantic):` Performance improvement

---

## Testing Strategy

### Unit Testing

Use Google Test framework:

```cpp
#include <gtest/gtest.h>
#include "semantic/SemanticDatabase.h"

class SemanticDatabaseTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_db = std::make_unique<CSemanticDatabase>();
    m_db->Open();
  }

  void TearDown() override
  {
    m_db->Close();
  }

  std::unique_ptr<CSemanticDatabase> m_db;
};

TEST_F(SemanticDatabaseTest, InsertChunkReturnsValidId)
{
  SemanticChunk chunk;
  chunk.mediaId = 123;
  chunk.mediaType = "movie";
  chunk.text = "Test content";
  chunk.sourceType = SourceType::SUBTITLE;

  int chunkId = m_db->InsertChunk(chunk);

  ASSERT_GT(chunkId, 0);
}

TEST_F(SemanticDatabaseTest, SearchFindsInsertedContent)
{
  // Insert test data
  SemanticChunk chunk;
  chunk.mediaId = 123;
  chunk.mediaType = "movie";
  chunk.text = "The robot uprising begins";
  chunk.sourceType = SourceType::SUBTITLE;

  m_db->InsertChunk(chunk);

  // Search
  SearchOptions options;
  auto results = m_db->SearchChunks("robot uprising", options);

  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].chunk.text, "The robot uprising begins");
}
```

### Integration Testing

Test complete workflows:

```cpp
TEST(SemanticIntegration, CompleteIndexingWorkflow)
{
  // Setup
  CSemanticDatabase db;
  db.Open();

  CSemanticIndexService service;
  service.Start();

  // Queue media for indexing
  service.QueueMedia(456, "movie", 10);

  // Wait for completion (with timeout)
  int timeout = 30; // seconds
  while (!service.IsMediaIndexed(456, "movie") && timeout-- > 0)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  ASSERT_TRUE(service.IsMediaIndexed(456, "movie"));

  // Verify chunks were created
  std::vector<SemanticChunk> chunks;
  ASSERT_TRUE(db.GetChunksForMedia(456, "movie", chunks));
  EXPECT_GT(chunks.size(), 0);

  // Cleanup
  service.Stop();
  db.Close();
}
```

### Performance Testing

Benchmark critical operations:

```cpp
TEST(SemanticPerformance, BulkInsertPerformance)
{
  CSemanticDatabase db;
  db.Open();

  // Generate test chunks
  std::vector<SemanticChunk> chunks;
  for (int i = 0; i < 10000; i++)
  {
    SemanticChunk chunk;
    chunk.mediaId = i;
    chunk.mediaType = "movie";
    chunk.text = "Test content " + std::to_string(i);
    chunk.sourceType = SourceType::SUBTITLE;
    chunks.push_back(chunk);
  }

  // Benchmark
  auto start = std::chrono::high_resolution_clock::now();
  ASSERT_TRUE(db.InsertChunks(chunks));
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end - start);

  // Should insert 10K chunks in < 5 seconds
  EXPECT_LT(duration.count(), 5000);

  CLog::Log(LOGINFO, "Inserted 10K chunks in {}ms", duration.count());
}
```

### Test Coverage

Run coverage analysis:

```bash
# Build with coverage
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
make

# Run tests
make test

# Generate report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

Target: **>80% code coverage** for new code

---

## Debugging Tips

### Enable Debug Logging

```cpp
// In your code
#include "utils/log.h"

CLog::Log(LOGDEBUG, "Semantic: Processing chunk {}", chunkId);
CLog::Log(LOGINFO, "Semantic: Indexed {} chunks for media {}",
          count, mediaId);
CLog::Log(LOGERROR, "Semantic: Failed to parse file: {}", path);
```

### Database Inspection

Use SQLite CLI to inspect database:

```bash
sqlite3 ~/.kodi/userdata/Database/SemanticIndex.db

# View chunks
SELECT * FROM semantic_chunks LIMIT 10;

# Check FTS index
SELECT * FROM semantic_chunks_fts WHERE text MATCH 'robot';

# View index state
SELECT * FROM semantic_index_state;
```

### Memory Profiling

Use valgrind for memory leak detection:

```bash
valgrind --leak-check=full --show-leak-kinds=all \
  ./kodi-test --gtest_filter=Semantic*
```

### Performance Profiling

Use gprof or perf:

```bash
# Build with profiling
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make

# Run with profiling
perf record -g ./kodi

# Analyze
perf report
```

### Common Issues

**Issue: Database locked**
- Cause: Multiple connections or long transaction
- Fix: Ensure proper transaction management, use single instance

**Issue: FTS5 not available**
- Cause: SQLite built without FTS5
- Fix: Rebuild SQLite with `--enable-fts5`

**Issue: Test failures on CI**
- Cause: Missing test data or dependencies
- Fix: Check test setup, mock external dependencies

---

## Additional Resources

- [Kodi Development Guide](https://kodi.wiki/view/Development)
- [SQLite FTS5](https://www.sqlite.org/fts5.html)
- [Google Test Documentation](https://google.github.io/googletest/)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)

---

**Document Version:** 1.0
**Last Updated:** 2025-11-25
**Maintainer:** Kodi Semantic Search Team
