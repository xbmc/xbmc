# Kodi Semantic Search Infrastructure Research

**Date**: 2025-11-24
**Purpose**: Preparatory research for implementing PRD-1 (Core Semantic Index Infrastructure) and PRD-2 (Semantic Vector Search & Smart Discovery)
**Scope**: Comprehensive analysis of Kodi codebase patterns and integration points

---

## Executive Summary

This research document synthesizes findings from 18 parallel investigations into the Kodi codebase, identifying the key patterns, APIs, and integration points needed to implement semantic search functionality. The findings provide a complete technical foundation for:

1. **PRD-1**: Core Semantic Index Infrastructure (subtitle parsing, cloud transcription, SQLite FTS5, background processing)
2. **PRD-2**: Semantic Vector Search & Smart Discovery (ONNX embeddings, sqlite-vec, hybrid ranking, GUI)

---

## 1. Database Layer Architecture

### Core Infrastructure

| Component | Location | Purpose |
|-----------|----------|---------|
| `CDatabase` | `xbmc/dbwrappers/Database.cpp/h` | Base class for all databases |
| `CVideoDatabase` | `xbmc/video/VideoDatabase.cpp/h` | Video library database (~22K lines) |
| `SqliteDataset` | `xbmc/dbwrappers/sqlitedataset.cpp/h` | SQLite wrapper implementation |

### Schema Management

- **Current Version**: Schema version 139 (defined in `VideoDatabase.cpp`)
- **Migration System**: `UpdateOldVersion()` method handles incremental migrations
- **Table Count**: 22+ tables including `movie`, `episode`, `tvshow`, `files`, `path`, `settings`

### Key Patterns for Semantic Index Table

```cpp
// Example from VideoDatabase.cpp - table creation pattern
void CVideoDatabase::CreateTables()
{
  // Use same pattern for semantic_index table
  m_pDS->exec("CREATE TABLE semantic_index ("
              "idSemanticIndex INTEGER PRIMARY KEY, "
              "idFile INTEGER, "
              "content TEXT, "
              "embedding BLOB, "
              "source TEXT, "
              "timestamp TEXT)");
  m_pDS->exec("CREATE INDEX ix_semantic_file ON semantic_index(idFile)");
}
```

### FTS5 Integration Points

SQLite FTS5 can be added via:
1. Extension loading in `SqliteDataset::open()`
2. Create virtual table: `CREATE VIRTUAL TABLE semantic_fts USING fts5(content, content=semantic_index)`
3. Use `MATCH` queries for full-text search

### sqlite-vec Integration (PRD-2)

- Load as SQLite extension similar to FTS5
- Store embeddings as BLOB in semantic_index table
- Use `vec_distance_cosine()` for similarity search

---

## 2. Background Job Architecture

### CJobManager Thread Pool

| Component | Location | Purpose |
|-----------|----------|---------|
| `CJobManager` | `xbmc/jobs/JobManager.cpp/h` | Global job queue manager |
| `CJob` | `xbmc/jobs/Job.h` | Base class for all jobs |
| `CJobQueue` | `xbmc/jobs/JobQueue.h` | Queue for related jobs |

### Job Implementation Pattern

```cpp
// Pattern for semantic indexing job
class CSemanticIndexJob : public CJob
{
public:
  bool DoWork() override
  {
    // 1. Extract subtitles/transcribe audio
    // 2. Generate embeddings
    // 3. Store in database
    return true;
  }

  const char* GetType() const override { return "SemanticIndex"; }
};

// Usage
CServiceBroker::GetJobManager().AddJob(
  new CSemanticIndexJob(videoPath),
  callback,
  CJob::PRIORITY_LOW_PAUSABLE
);
```

### Priority Levels

- `PRIORITY_LOW_PAUSABLE` - For background indexing (recommended)
- `PRIORITY_LOW` - Standard background
- `PRIORITY_NORMAL` - Interactive operations
- `PRIORITY_HIGH` - Time-critical

### Service Integration

Services are registered via `CServiceManager` (xbmc/ServiceManager.cpp):
- Singleton access: `CServiceBroker::GetXxx()`
- Init during startup in `CServiceManager::InitStageTwo()`

---

## 3. Subtitle Handling Infrastructure

### Parser Architecture

| Format | Parser Class | Location |
|--------|--------------|----------|
| SRT | `CDVDSubtitleParserSubrip` | `xbmc/cores/VideoPlayer/DVDSubtitles/` |
| VTT | Similar pattern | Same directory |
| ASS/SSA | `libass` integration | External dependency |

### Factory Pattern

```cpp
// CDVDFactorySubtitle.cpp - creates appropriate parser
CDVDSubtitleParser* CDVDFactorySubtitle::CreateParser(const std::string& strFile)
{
  // Detects format and returns parser instance
}
```

### Subtitle Stream Access

- External subtitles: File path based loading
- Embedded subtitles: `CDVDDemuxFFmpeg::GetStream()` with STREAM_SUBTITLE type
- Demux packets contain subtitle text/timing

### Integration Point for Semantic Index

```cpp
// Hook into subtitle loading to extract text
void ExtractSubtitleText(const std::string& videoPath)
{
  auto parser = CDVDFactorySubtitle::CreateParser(subtitlePath);
  while (parser->GetSubtitle(text, start, end))
  {
    // Concatenate for semantic indexing
  }
}
```

---

## 4. JSON-RPC API Structure

### API Architecture

| Component | Location | Purpose |
|-----------|----------|---------|
| `CVideoLibrary` | `xbmc/interfaces/json-rpc/VideoLibrary.cpp` | Video library methods |
| Schema files | `xbmc/interfaces/json-rpc/schema/` | Method/type definitions |
| `CJSONServiceDescription` | Same directory | Schema validation |

### Method Registration Pattern

```cpp
// In VideoLibrary.cpp
JSONRPC_STATUS CVideoLibrary::SemanticSearch(
  const std::string &method,
  ITransportLayer *transport,
  IClient *client,
  const CVariant &parameterObject,
  CVariant &result)
{
  std::string query = parameterObject["query"].asString();
  // Perform search, populate result
  return OK;
}
```

### Schema Definition (methods.json)

```json
{
  "VideoLibrary.SemanticSearch": {
    "type": "method",
    "description": "Search video library using semantic similarity",
    "params": [
      {"name": "query", "type": "string", "required": true},
      {"name": "limit", "type": "integer", "default": 25}
    ],
    "returns": {"type": "array", "items": {"$ref": "Video.Details.Base"}}
  }
}
```

---

## 5. GUI/Dialog Implementation

### Dialog Architecture

| Base Class | Location | Purpose |
|------------|----------|---------|
| `CGUIDialog` | `xbmc/guilib/GUIDialog.h` | All dialog base |
| `CGUIDialogSelect` | `xbmc/dialogs/GUIDialogSelect.cpp` | Selection list dialog |
| `CGUIDialogKeyboardGeneric` | Similar | Text input |

### Dialog Implementation Pattern

```cpp
class CGUIDialogSemanticSearch : public CGUIDialog
{
public:
  CGUIDialogSemanticSearch();
  bool OnMessage(CGUIMessage& message) override;
  void OnInitWindow() override;

private:
  void DoSearch(const std::string& query);
  void UpdateResults(const std::vector<SearchResult>& results);
};
```

### XML Skin Definition

```xml
<!-- In addons/skin.estuary/xml/DialogSemanticSearch.xml -->
<window type="dialog" id="WINDOW_DIALOG_SEMANTIC_SEARCH">
  <controls>
    <control type="edit" id="1"><!-- Search input --></control>
    <control type="list" id="50"><!-- Results list --></control>
  </controls>
</window>
```

### Window ID Registration

Add to `xbmc/guilib/WindowIDs.h`:
```cpp
#define WINDOW_DIALOG_SEMANTIC_SEARCH 10999
```

---

## 6. Settings System

### Architecture

| Component | Location | Purpose |
|-----------|----------|---------|
| `CSettings` | `xbmc/settings/Settings.cpp` | Settings manager |
| Settings XML | `system/settings/settings.xml` | Definition file |
| `CSettingsBase` | Base class | Hierarchical storage |

### Setting Definition Pattern

```xml
<!-- In settings.xml -->
<category id="semanticsearch" label="39500">
  <group id="1">
    <setting id="semanticsearch.enable" type="boolean" label="39501" default="true"/>
    <setting id="semanticsearch.transcriptionprovider" type="string" label="39502">
      <options>
        <option value="groq">Groq</option>
        <option value="openai">OpenAI</option>
      </options>
    </setting>
    <setting id="semanticsearch.apikey" type="string" label="39503" option="hidden"/>
  </group>
</category>
```

### Programmatic Access

```cpp
bool enabled = CServiceBroker::GetSettingsComponent()
  ->GetSettings()->GetBool(CSettings::SETTING_SEMANTICSEARCH_ENABLE);

std::string apiKey = CServiceBroker::GetSettingsComponent()
  ->GetSettings()->GetString(CSettings::SETTING_SEMANTICSEARCH_APIKEY);
```

---

## 7. FFmpeg Integration

### Demuxer Architecture

| Component | Location | Purpose |
|-----------|----------|---------|
| `CDVDDemuxFFmpeg` | `xbmc/cores/VideoPlayer/DVDDemuxers/` | Main demuxer |
| `CDVDAudioCodecFFmpeg` | `DVDCodecs/Audio/` | Audio decoding |

### Audio Stream Access for Transcription

```cpp
// Get audio stream from video file
CDVDDemuxFFmpeg demux;
demux.Open(videoPath);

CDemuxStream* audioStream = demux.GetStream(STREAM_AUDIO, 0);
// Can extract audio data for transcription API
```

### Audio Extraction Pattern

For cloud transcription, extract audio to temporary file:
```cpp
// Use FFmpeg to extract audio
ffmpeg -i video.mkv -vn -acodec pcm_s16le -ar 16000 -ac 1 audio.wav
```

---

## 8. Build System (CMake)

### Directory Structure

```
CMakeLists.txt              # Root build file
cmake/
  scripts/common/
    Macros.cmake            # core_add_library(), etc.
  treedata/
    common/                 # Source file lists
    video/                  # Video module sources
```

### Adding New Module Pattern

1. Create source list in `cmake/treedata/common/`:
```cmake
# semanticsearch.txt
xbmc/semanticsearch/SemanticSearchService.cpp
xbmc/semanticsearch/SemanticIndexJob.cpp
xbmc/semanticsearch/EmbeddingGenerator.cpp
```

2. Include in main CMakeLists.txt:
```cmake
core_add_library(semanticsearch)
```

### External Dependencies

Add ONNX Runtime dependency:
```cmake
# In cmake/modules/FindONNXRuntime.cmake
find_package(ONNXRuntime REQUIRED)
target_link_libraries(${APP_NAME_LC} ONNXRuntime::ONNXRuntime)
```

---

## 9. Threading Patterns

### Thread Base Classes

| Class | Location | Purpose |
|-------|----------|---------|
| `CThread` | `xbmc/threads/Thread.cpp/h` | Base thread class |
| `CJobQueue` | `xbmc/jobs/JobQueue.cpp/h` | Serial job execution |
| `CEvent` | `xbmc/threads/Event.h` | Thread synchronization |

### Thread-Safe Patterns

```cpp
class CSemanticIndexer : public CThread
{
protected:
  void Process() override
  {
    while (!m_bStop)
    {
      // Process indexing queue
      m_event.Wait(1000); // Wait with timeout
    }
  }

private:
  CCriticalSection m_critSection;
  CEvent m_event;
};
```

### Locking Pattern

```cpp
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  // Thread-safe operations
}
```

---

## 10. HTTP Client Infrastructure

### Core Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `CCurlFile` | `xbmc/filesystem/CurlFile.cpp/h` | HTTP client |
| `CVariant` | `xbmc/utils/Variant.h` | JSON data structure |
| `CJSONVariantParser` | `xbmc/utils/JSONVariantParser.cpp` | JSON parsing |

### HTTP Request Pattern for Cloud APIs

```cpp
// Pattern for Groq/OpenAI API calls
bool TranscribeAudio(const std::string& audioPath, std::string& result)
{
  CCurlFile http;
  http.SetRequestHeader("Authorization", "Bearer " + apiKey);
  http.SetRequestHeader("Content-Type", "multipart/form-data");

  std::string response;
  if (http.Post(GROQ_API_URL, postData, response))
  {
    CVariant json;
    CJSONVariantParser::Parse(response, json);
    result = json["text"].asString();
    return true;
  }
  return false;
}
```

### Async HTTP Pattern

```cpp
// Use CJob for async requests
class CTranscriptionJob : public CJob
{
  bool DoWork() override
  {
    CCurlFile http;
    // Make API call
    return http.Post(url, data, response);
  }
};
```

---

## 11. Library Scanning Hooks

### Scanner Architecture

| Component | Location | Purpose |
|-----------|----------|---------|
| `CVideoInfoScanner` | `xbmc/video/VideoInfoScanner.cpp` | Main scanner |
| `CVideoLibraryQueue` | `xbmc/video/VideoLibraryQueue.cpp` | Queue manager |

### Hook Points for Semantic Indexing

1. **Post-scan hook** - After video is added to library:
```cpp
// In CVideoInfoScanner::OnProcessSeriesFolder() or similar
void NotifySemanticIndexer(const CVideoInfoTag& tag, int idFile)
{
  CServiceBroker::GetJobManager().AddJob(
    new CSemanticIndexJob(idFile),
    nullptr,
    CJob::PRIORITY_LOW_PAUSABLE
  );
}
```

2. **Announcement system** - Subscribe to library updates:
```cpp
// Listen for VideoLibrary.OnUpdate announcements
class CSemanticIndexListener : public ANNOUNCEMENT::IAnnouncer
{
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override
  {
    if (message == "OnUpdate" && sender == "VideoLibrary")
    {
      // Queue item for indexing
    }
  }
};
```

---

## 12. Event/Notification System

### Announcement Manager

| Component | Location | Purpose |
|-----------|----------|---------|
| `CAnnouncementManager` | `xbmc/interfaces/AnnouncementManager.cpp` | Global announcements |
| `CApplicationMessenger` | `xbmc/messaging/ApplicationMessenger.cpp` | Inter-thread messaging |

### Custom Announcements for Semantic Search

```cpp
// Announce indexing progress
CServiceBroker::GetAnnouncementManager()->Announce(
  ANNOUNCEMENT::Other,
  "SemanticIndex",
  "OnIndexProgress",
  CVariant{{"progress", 50}, {"file", currentFile}}
);
```

### GUI Message Pattern

```cpp
// Send message to update search dialog
CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
```

---

## 13. Video Metadata (VideoInfoTag)

### Core Structure

| Component | Location | Purpose |
|-----------|----------|---------|
| `CVideoInfoTag` | `xbmc/video/VideoInfoTag.cpp/h` | All video metadata |
| `VideoDbContentType` | Same location | Content type enum |

### Key Fields for Semantic Search

```cpp
// Relevant CVideoInfoTag fields
std::string m_strTitle;
std::string m_strPlot;        // Synopsis - useful for semantic matching
std::string m_strTagLine;
std::string m_strGenre;
std::vector<std::string> m_tags;  // User tags
int m_iDbId;                  // Database ID for linking
```

### Database Field Mapping

VideoInfoTag uses offset-based field mapping defined in `VideoDatabase.cpp`:
- Fields map to specific column indices
- Can add custom fields for semantic index references

---

## 14. Search Functionality

### Current Search Implementation

| Component | Location | Purpose |
|-----------|----------|---------|
| `CDatabaseQueryRule` | `xbmc/dbwrappers/DatabaseQuery.cpp` | Query building |
| `CSmartPlaylist` | `xbmc/playlists/SmartPlayList.cpp` | Smart playlist rules |

### SQL Pattern

```cpp
// Current search uses LIKE
strSQL = PrepareSQL("SELECT * FROM movie WHERE c00 LIKE '%%%s%%'", searchTerm);
```

### Hybrid Search Integration (PRD-2)

```cpp
// Combine FTS5 + vector search with RRF
std::string hybridQuery = R"(
  WITH fts_results AS (
    SELECT idFile, bm25(semantic_fts) as fts_score
    FROM semantic_fts WHERE content MATCH ?
  ),
  vec_results AS (
    SELECT idFile, vec_distance_cosine(embedding, ?) as vec_score
    FROM semantic_index
  )
  SELECT f.idFile,
         (1.0/(60+fts_rank) + 1.0/(60+vec_rank)) as rrf_score
  FROM fts_results f
  JOIN vec_results v ON f.idFile = v.idFile
  ORDER BY rrf_score DESC
  LIMIT 25
)";
```

---

## 15. Add-on Architecture

### Add-on Types

| Type | Base Class | Location |
|------|------------|----------|
| Binary (C++) | `CAddonDll` | `xbmc/addons/binary-addons/` |
| Python | `CAddonPythonInvoker` | `xbmc/interfaces/python/` |
| Service | `CService` | `xbmc/addons/Service.cpp` |

### Service Add-on Pattern

For optional semantic search as add-on:
```xml
<!-- addon.xml -->
<addon id="service.semanticsearch" version="1.0.0">
  <extension point="xbmc.service" library="service.py" start="startup"/>
  <extension point="xbmc.python.pluginsource" library="default.py"/>
</addon>
```

### Binary Add-on Integration (for ONNX)

```cpp
// Can create binary add-on for ONNX embedding generation
class CSemanticAddon : public CAddonDll
{
  // Expose embedding generation to Python/other add-ons
};
```

---

## 16. Localization System

### String Infrastructure

| Component | Location | Purpose |
|-----------|----------|---------|
| `CLocalizeStrings` | `xbmc/guilib/LocalizeStrings.cpp` | String manager |
| PO files | `addons/resource.language.*/resources/strings.po` | Translations |

### Adding Semantic Search Strings

```po
# In strings.po
msgctxt "#39500"
msgid "Semantic Search"
msgstr ""

msgctxt "#39501"
msgid "Enable semantic search"
msgstr ""

msgctxt "#39502"
msgid "Transcription provider"
msgstr ""

msgctxt "#39503"
msgid "API Key"
msgstr ""

msgctxt "#39504"
msgid "Search your library using natural language"
msgstr ""
```

### Programmatic Access

```cpp
std::string label = g_localizeStrings.Get(39500); // "Semantic Search"
```

---

## 17. File Handling (VFS)

### Virtual File System

| Component | Location | Purpose |
|-----------|----------|---------|
| `CFile` | `xbmc/filesystem/File.cpp` | File operations |
| `CDirectory` | `xbmc/filesystem/Directory.cpp` | Directory ops |
| `CFileFactory` | `xbmc/filesystem/FileFactory.cpp` | Protocol handlers |

### Protocol Support

- `file://` - Local files
- `smb://` - Network shares
- `nfs://` - NFS mounts
- `special://` - Kodi special paths

### File Access Pattern

```cpp
// Read subtitle file
XFILE::CFile file;
if (file.Open(subtitlePath))
{
  std::string content;
  char buffer[4096];
  while (file.Read(buffer, sizeof(buffer)) > 0)
  {
    content += buffer;
  }
  file.Close();
}
```

### Special Paths

```cpp
// Useful special paths
"special://temp/"        // Temporary files (for audio extraction)
"special://profile/"     // User profile
"special://database/"    // Database directory
```

---

## 18. Logging Infrastructure

### Logging System

| Component | Location | Purpose |
|-----------|----------|---------|
| `CLog` | `xbmc/utils/log.cpp/h` | Main logger |
| spdlog | External | Backend implementation |

### Log Levels

- `LOGDEBUG` - Verbose debugging
- `LOGINFO` - Informational
- `LOGWARNING` - Warnings
- `LOGERROR` - Errors
- `LOGFATAL` - Fatal errors

### Logging Pattern

```cpp
// Add semantic search logging
CLog::Log(LOGINFO, "SemanticSearch: Indexing file {}", videoPath);
CLog::Log(LOGDEBUG, "SemanticSearch: Generated {} embeddings", count);
CLog::Log(LOGERROR, "SemanticSearch: Transcription failed - {}", error);
```

### Component Logging

```cpp
// Can define component-specific logger
static Logger logger = CServiceBroker::GetLogging().GetLogger("SemanticSearch");
logger->info("Starting indexing...");
```

---

## Implementation Recommendations

### PRD-1 Implementation Order

1. **Database Schema** - Add `semantic_index` table with FTS5 virtual table
2. **Settings** - Add configuration for transcription provider and API key
3. **HTTP Client** - Implement Groq/OpenAI API integration
4. **Subtitle Parser** - Hook into existing subtitle infrastructure
5. **Background Jobs** - Create `CSemanticIndexJob` using CJob pattern
6. **Library Hooks** - Subscribe to VideoLibrary announcements
7. **JSON-RPC API** - Add `VideoLibrary.SemanticSearch` method

### PRD-2 Implementation Order

1. **ONNX Integration** - Add as CMake dependency, create embedding generator
2. **sqlite-vec** - Load extension, add embedding column to schema
3. **Hybrid Search** - Implement RRF ranking combining FTS5 + vector scores
4. **GUI Dialog** - Create `CGUIDialogSemanticSearch` with XML skin

### Key Files to Modify

| Feature | Files to Modify |
|---------|-----------------|
| Database | `VideoDatabase.cpp/h`, `sqlitedataset.cpp` |
| Jobs | New `SemanticIndexJob.cpp/h` |
| Settings | `settings.xml`, `Settings.cpp` |
| HTTP | New wrapper around `CurlFile` for API calls |
| Subtitles | Hook in `VideoInfoScanner.cpp` |
| JSON-RPC | `VideoLibrary.cpp`, `methods.json` |
| GUI | New dialog files + XML skin |
| Build | `CMakeLists.txt`, new treedata file |
| Localization | `strings.po` files |

### Thread Safety Considerations

- Use `CCriticalSection` for shared data structures
- Use `CJobManager` for background processing (already thread-safe)
- Database operations are thread-safe via SQLite
- Settings access is thread-safe via `CSettingsComponent`

---

## Appendix: File Reference

### Primary Source Locations

```
xbmc/video/VideoDatabase.cpp          # Main video database
xbmc/jobs/JobManager.cpp              # Job queue system
xbmc/cores/VideoPlayer/DVDSubtitles/  # Subtitle parsers
xbmc/interfaces/json-rpc/             # JSON-RPC API
xbmc/guilib/                          # GUI framework
xbmc/settings/                        # Settings system
xbmc/filesystem/CurlFile.cpp          # HTTP client
xbmc/threads/                         # Threading primitives
xbmc/interfaces/AnnouncementManager.cpp # Event system
```

### Build Configuration

```
CMakeLists.txt                        # Main build file
cmake/treedata/common/                # Source lists
cmake/modules/                        # Find modules
```

---

*Research conducted: 2025-11-24*
*Target: Kodi v22 (Piers) codebase*
*PRD References: prd-1.md (Core Infrastructure), prd-2.md (Vector Search & Discovery)*
