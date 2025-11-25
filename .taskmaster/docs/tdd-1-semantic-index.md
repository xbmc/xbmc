# Technical Design Document: Kodi Content Text Index

**PRD Reference**: `prd-1.md`
**Status**: Draft
**Last Updated**: 2025-01-24

---

## 1. Overview

This document contains the detailed technical implementation for the Content Text Index feature. For requirements and goals, see the PRD.

---

## 2. Database Schema

### 2.1 Core Tables

```sql
-- Schema version tracking
CREATE TABLE schema_version (
    version INTEGER PRIMARY KEY,
    applied_at TEXT DEFAULT (datetime('now'))
);

-- Main content chunks table
CREATE TABLE semantic_chunks (
    chunk_id INTEGER PRIMARY KEY AUTOINCREMENT,
    media_id INTEGER NOT NULL,
    media_type TEXT NOT NULL CHECK(media_type IN ('movie', 'episode', 'musicvideo')),
    source_type TEXT NOT NULL CHECK(source_type IN ('subtitle', 'transcription', 'metadata')),
    source_path TEXT,
    start_ms INTEGER,
    end_ms INTEGER,
    text TEXT NOT NULL,
    language TEXT,
    confidence REAL DEFAULT 1.0,
    created_at TEXT DEFAULT (datetime('now')),

    UNIQUE(media_id, media_type, source_type, start_ms)
);

CREATE INDEX idx_chunks_media ON semantic_chunks(media_id, media_type);
CREATE INDEX idx_chunks_source ON semantic_chunks(source_type);
CREATE INDEX idx_chunks_time ON semantic_chunks(start_ms, end_ms);

-- FTS5 virtual table for full-text search
CREATE VIRTUAL TABLE semantic_fts USING fts5(
    text,
    content='semantic_chunks',
    content_rowid='chunk_id',
    tokenize='porter unicode61 remove_diacritics 1'
);

-- Triggers to keep FTS in sync
CREATE TRIGGER chunks_ai AFTER INSERT ON semantic_chunks BEGIN
    INSERT INTO semantic_fts(rowid, text) VALUES (new.chunk_id, new.text);
END;

CREATE TRIGGER chunks_ad AFTER DELETE ON semantic_chunks BEGIN
    INSERT INTO semantic_fts(semantic_fts, rowid, text) VALUES('delete', old.chunk_id, old.text);
END;

CREATE TRIGGER chunks_au AFTER UPDATE ON semantic_chunks BEGIN
    INSERT INTO semantic_fts(semantic_fts, rowid, text) VALUES('delete', old.chunk_id, old.text);
    INSERT INTO semantic_fts(rowid, text) VALUES (new.chunk_id, new.text);
END;

-- Index state tracking per media item
CREATE TABLE semantic_index_state (
    state_id INTEGER PRIMARY KEY AUTOINCREMENT,
    media_id INTEGER NOT NULL,
    media_type TEXT NOT NULL,
    media_path TEXT NOT NULL,

    -- Subtitle indexing status
    subtitle_status TEXT DEFAULT 'pending'
        CHECK(subtitle_status IN ('pending', 'processing', 'complete', 'failed', 'no_source')),
    subtitle_error TEXT,

    -- Transcription status
    transcription_status TEXT DEFAULT 'pending'
        CHECK(transcription_status IN ('pending', 'processing', 'complete', 'failed', 'skipped')),
    transcription_provider TEXT,
    transcription_progress REAL DEFAULT 0.0,
    transcription_error TEXT,

    -- Metadata status
    metadata_status TEXT DEFAULT 'pending'
        CHECK(metadata_status IN ('pending', 'processing', 'complete', 'failed')),

    priority INTEGER DEFAULT 0,
    chunk_count INTEGER DEFAULT 0,
    created_at TEXT DEFAULT (datetime('now')),
    updated_at TEXT DEFAULT (datetime('now')),

    UNIQUE(media_id, media_type)
);

CREATE INDEX idx_state_status ON semantic_index_state(subtitle_status, transcription_status);
CREATE INDEX idx_state_priority ON semantic_index_state(priority DESC);

-- Provider configuration and usage tracking
CREATE TABLE semantic_providers (
    provider_id TEXT PRIMARY KEY,
    display_name TEXT NOT NULL,
    is_enabled INTEGER DEFAULT 1,
    api_key_set INTEGER DEFAULT 0,
    total_minutes_used REAL DEFAULT 0.0,
    total_cost_usd REAL DEFAULT 0.0,
    last_used_at TEXT,
    monthly_minutes_used REAL DEFAULT 0.0,
    monthly_reset_date TEXT
);

-- Initialize default provider
INSERT INTO semantic_providers (provider_id, display_name)
VALUES ('groq', 'Groq Whisper');
```

### 2.2 Schema Migrations

```cpp
// SemanticDatabase.cpp - Migration handling

void CSemanticDatabase::Migrate()
{
    int currentVersion = GetSchemaVersion();

    if (currentVersion < 1)
    {
        // Initial schema - run full creation
        ExecuteSchemaSQL();
        SetSchemaVersion(1);
    }

    // Future migrations go here:
    // if (currentVersion < 2) { ... }
}

int CSemanticDatabase::GetSchemaVersion()
{
    try
    {
        auto result = m_db.execAndGet("SELECT MAX(version) FROM schema_version");
        return result.isNull() ? 0 : result.getInt();
    }
    catch (...)
    {
        return 0;  // Table doesn't exist yet
    }
}
```

---

## 3. C++ Component Design

### 3.1 Namespace and File Structure

```
xbmc/semantic/
├── SemanticDatabase.h/.cpp       # Database operations
├── SemanticSearch.h/.cpp         # Search implementation
├── SemanticIndexService.h/.cpp   # Background service orchestrator
├── ingest/
│   ├── SubtitleParser.h/.cpp     # SRT/ASS/VTT parsing
│   ├── MetadataParser.h/.cpp     # NFO/plot extraction
│   └── ChunkProcessor.h/.cpp     # Text chunking utilities
└── transcription/
    ├── ITranscriptionProvider.h  # Provider interface
    ├── TranscriptionProviderManager.h/.cpp
    └── GroqProvider.h/.cpp       # Groq Whisper implementation
```

### 3.2 Core Data Types

```cpp
// SemanticTypes.h

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace KODI::SEMANTIC
{

enum class MediaType
{
    Movie,
    Episode,
    MusicVideo
};

enum class SourceType
{
    Subtitle,
    Transcription,
    Metadata
};

enum class IndexStatus
{
    Pending,
    Processing,
    Complete,
    Failed,
    NoSource,
    Skipped
};

struct SemanticChunk
{
    int64_t chunkId{0};
    int mediaId{0};
    MediaType mediaType{MediaType::Movie};
    SourceType sourceType{SourceType::Subtitle};
    std::string sourcePath;
    int64_t startMs{0};
    int64_t endMs{0};
    std::string text;
    std::string language;
    float confidence{1.0f};
};

struct SearchResult
{
    int mediaId;
    MediaType mediaType;
    std::string mediaTitle;
    std::string matchedText;
    int64_t startMs;
    int64_t endMs;
    float score;
    SourceType sourceType;
};

struct SearchOptions
{
    std::vector<MediaType> mediaTypes;  // Empty = all types
    std::vector<SourceType> sourceTypes; // Empty = all sources
    int limit{50};
    int offset{0};
    float minConfidence{0.0f};
};

struct IndexState
{
    int64_t stateId;
    int mediaId;
    MediaType mediaType;
    std::string mediaPath;
    IndexStatus subtitleStatus;
    IndexStatus transcriptionStatus;
    IndexStatus metadataStatus;
    std::string transcriptionProvider;
    float transcriptionProgress;
    int priority;
    int chunkCount;
};

} // namespace KODI::SEMANTIC
```

### 3.3 Subtitle Parser

```cpp
// ingest/SubtitleParser.h

#pragma once

#include "semantic/SemanticTypes.h"
#include <string>
#include <vector>

namespace KODI::SEMANTIC
{

struct SubtitleEntry
{
    int64_t startMs;
    int64_t endMs;
    std::string text;
    std::string speaker;  // Optional, from ASS styles
};

class CSubtitleParser
{
public:
    CSubtitleParser() = default;
    ~CSubtitleParser() = default;

    // Main entry point - auto-detects format
    std::vector<SubtitleEntry> Parse(const std::string& path);

    // Format-specific parsers
    std::vector<SubtitleEntry> ParseSRT(const std::string& path);
    std::vector<SubtitleEntry> ParseASS(const std::string& path);
    std::vector<SubtitleEntry> ParseVTT(const std::string& path);

    // Find subtitle file for media
    std::string FindSubtitleForMedia(const std::string& mediaPath);

private:
    // SRT timestamp: "00:01:23,456" -> milliseconds
    int64_t ParseSRTTimestamp(const std::string& ts);

    // ASS timestamp: "0:01:23.45" -> milliseconds
    int64_t ParseASSTimestamp(const std::string& ts);

    // VTT timestamp: "00:01:23.456" -> milliseconds
    int64_t ParseVTTTimestamp(const std::string& ts);

    // Clean subtitle text
    std::string StripHTMLTags(const std::string& text);
    std::string StripASSCodes(const std::string& text);
    bool IsNonDialogue(const std::string& text);  // Filter [music], ♪, etc.

    // Character encoding
    std::string DetectAndConvertCharset(const std::string& content);
};

} // namespace KODI::SEMANTIC
```

```cpp
// ingest/SubtitleParser.cpp

#include "SubtitleParser.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"

#include <regex>
#include <fstream>
#include <sstream>

namespace KODI::SEMANTIC
{

std::vector<SubtitleEntry> CSubtitleParser::Parse(const std::string& path)
{
    std::string ext = URIUtils::GetExtension(path);
    StringUtils::ToLower(ext);

    if (ext == ".srt")
        return ParseSRT(path);
    else if (ext == ".ass" || ext == ".ssa")
        return ParseASS(path);
    else if (ext == ".vtt")
        return ParseVTT(path);

    CLog::Log(LOGWARNING, "SubtitleParser: Unsupported format {}", ext);
    return {};
}

std::vector<SubtitleEntry> CSubtitleParser::ParseSRT(const std::string& path)
{
    std::vector<SubtitleEntry> entries;

    XFILE::CFile file;
    if (!file.Open(path))
    {
        CLog::Log(LOGERROR, "SubtitleParser: Failed to open {}", path);
        return entries;
    }

    // Read entire file
    std::vector<uint8_t> buffer(file.GetLength());
    file.Read(buffer.data(), buffer.size());
    file.Close();

    std::string content(buffer.begin(), buffer.end());
    content = DetectAndConvertCharset(content);

    // SRT format:
    // 1
    // 00:00:01,000 --> 00:00:04,000
    // Subtitle text
    // (blank line)

    std::regex entryPattern(
        R"((\d+)\s*\n(\d{2}:\d{2}:\d{2},\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2},\d{3})\s*\n([\s\S]*?)(?=\n\n|\n*$))",
        std::regex::ECMAScript
    );

    auto begin = std::sregex_iterator(content.begin(), content.end(), entryPattern);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it)
    {
        std::smatch match = *it;

        SubtitleEntry entry;
        entry.startMs = ParseSRTTimestamp(match[2].str());
        entry.endMs = ParseSRTTimestamp(match[3].str());
        entry.text = StripHTMLTags(match[4].str());

        // Clean up whitespace
        StringUtils::Trim(entry.text);
        StringUtils::Replace(entry.text, "\n", " ");

        // Filter non-dialogue
        if (!entry.text.empty() && !IsNonDialogue(entry.text))
        {
            entries.push_back(std::move(entry));
        }
    }

    CLog::Log(LOGDEBUG, "SubtitleParser: Parsed {} entries from {}", entries.size(), path);
    return entries;
}

int64_t CSubtitleParser::ParseSRTTimestamp(const std::string& ts)
{
    // Format: "00:01:23,456"
    int hours, minutes, seconds, millis;
    if (sscanf(ts.c_str(), "%d:%d:%d,%d", &hours, &minutes, &seconds, &millis) != 4)
        return 0;

    return (hours * 3600000LL) + (minutes * 60000LL) + (seconds * 1000LL) + millis;
}

std::string CSubtitleParser::StripHTMLTags(const std::string& text)
{
    static std::regex htmlPattern(R"(<[^>]*>)", std::regex::ECMAScript);
    return std::regex_replace(text, htmlPattern, "");
}

bool CSubtitleParser::IsNonDialogue(const std::string& text)
{
    // Filter music notes, sound effects, etc.
    if (text.find("\u266A") != std::string::npos ||  // ♪
        text.find("\u266B") != std::string::npos)    // ♫
        return true;

    // Filter bracketed sound effects: [music], [applause], etc.
    static std::regex bracketPattern(R"(^\s*[\[\(][^\]\)]+[\]\)]\s*$)");
    if (std::regex_match(text, bracketPattern))
        return true;

    return false;
}

std::string CSubtitleParser::FindSubtitleForMedia(const std::string& mediaPath)
{
    // Check for subtitle file with same base name
    std::string basePath = URIUtils::ReplaceExtension(mediaPath, "");

    std::vector<std::string> extensions = {".srt", ".ass", ".ssa", ".vtt"};

    for (const auto& ext : extensions)
    {
        std::string subPath = basePath + ext;
        if (XFILE::CFile::Exists(subPath))
            return subPath;
    }

    // Also check common subtitle subdirectories
    std::string parentPath = URIUtils::GetParentPath(mediaPath);
    std::string filename = URIUtils::GetFileName(mediaPath);
    std::string baseName = URIUtils::ReplaceExtension(filename, "");

    std::vector<std::string> subDirs = {"Subs", "subs", "Subtitles", "subtitles"};

    for (const auto& subDir : subDirs)
    {
        for (const auto& ext : extensions)
        {
            std::string subPath = URIUtils::AddFileToFolder(parentPath, subDir, baseName + ext);
            if (XFILE::CFile::Exists(subPath))
                return subPath;
        }
    }

    return "";  // No subtitle found
}

std::string CSubtitleParser::DetectAndConvertCharset(const std::string& content)
{
    // Check for BOM
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF)
    {
        // UTF-8 with BOM - strip BOM and return
        return content.substr(3);
    }

    // Check if valid UTF-8
    if (CCharsetConverter::IsValidUtf8(content))
        return content;

    // Assume Latin-1 and convert
    std::string utf8;
    g_charsetConverter.ToUtf8("ISO-8859-1", content, utf8);
    return utf8;
}

} // namespace KODI::SEMANTIC
```

### 3.4 Transcription Provider Interface

```cpp
// transcription/ITranscriptionProvider.h

#pragma once

#include <string>
#include <functional>
#include <cstdint>

namespace KODI::SEMANTIC
{

struct TranscriptSegment
{
    int64_t startMs;
    int64_t endMs;
    std::string text;
    std::string language;
    float confidence;
};

struct TranscriptionOptions
{
    std::string language{"auto"};  // "auto" for detection, or ISO code
    int64_t startMs{0};            // Start offset for chunked processing
    int64_t endMs{0};              // End offset (0 = until end)
};

using SegmentCallback = std::function<void(const TranscriptSegment&)>;
using ProgressCallback = std::function<void(float)>;  // 0.0 to 1.0
using ErrorCallback = std::function<void(const std::string&)>;

class ITranscriptionProvider
{
public:
    virtual ~ITranscriptionProvider() = default;

    virtual std::string GetName() const = 0;
    virtual std::string GetId() const = 0;

    virtual bool IsConfigured() const = 0;
    virtual bool IsAvailable() const = 0;

    virtual bool Transcribe(
        const std::string& mediaPath,
        const TranscriptionOptions& options,
        SegmentCallback onSegment,
        ProgressCallback onProgress,
        ErrorCallback onError) = 0;

    virtual void Cancel() = 0;

    // Cost estimation (in USD)
    virtual float EstimateCost(int64_t durationMs) const = 0;

    // Rate limit info
    virtual int GetRemainingRequests() const { return -1; }  // -1 = unknown
};

} // namespace KODI::SEMANTIC
```

### 3.5 Groq Provider Implementation

```cpp
// transcription/GroqProvider.h

#pragma once

#include "ITranscriptionProvider.h"
#include <atomic>

namespace KODI::SEMANTIC
{

class CGroqProvider : public ITranscriptionProvider
{
public:
    CGroqProvider();
    ~CGroqProvider() override = default;

    std::string GetName() const override { return "Groq Whisper"; }
    std::string GetId() const override { return "groq"; }

    bool IsConfigured() const override;
    bool IsAvailable() const override;

    bool Transcribe(
        const std::string& mediaPath,
        const TranscriptionOptions& options,
        SegmentCallback onSegment,
        ProgressCallback onProgress,
        ErrorCallback onError) override;

    void Cancel() override;

    float EstimateCost(int64_t durationMs) const override;

private:
    bool TranscribeSingleFile(
        const std::string& audioPath,
        const TranscriptionOptions& options,
        SegmentCallback onSegment,
        ProgressCallback onProgress,
        ErrorCallback onError);

    bool TranscribeLargeFile(
        const std::string& mediaPath,
        const TranscriptionOptions& options,
        SegmentCallback onSegment,
        ProgressCallback onProgress,
        ErrorCallback onError);

    std::string GetApiKey() const;
    std::string ExtractAudioChunk(const std::string& mediaPath,
                                   int64_t startMs, int64_t endMs);

    std::atomic<bool> m_cancelled{false};

    static constexpr const char* API_ENDPOINT =
        "https://api.groq.com/openai/v1/audio/transcriptions";
    static constexpr int64_t MAX_FILE_SIZE = 25 * 1024 * 1024;  // 25MB
    static constexpr float COST_PER_MINUTE = 0.0033f;  // $0.20/hour
};

} // namespace KODI::SEMANTIC
```

```cpp
// transcription/GroqProvider.cpp

#include "GroqProvider.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "ServiceBroker.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "utils/JSONVariantParser.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <fmt/format.h>

namespace KODI::SEMANTIC
{

CGroqProvider::CGroqProvider() = default;

bool CGroqProvider::IsConfigured() const
{
    return !GetApiKey().empty();
}

bool CGroqProvider::IsAvailable() const
{
    return IsConfigured();
}

std::string CGroqProvider::GetApiKey() const
{
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    return settings->GetString(CSettings::SETTING_SEMANTIC_GROQ_APIKEY);
}

float CGroqProvider::EstimateCost(int64_t durationMs) const
{
    float minutes = durationMs / 60000.0f;
    return minutes * COST_PER_MINUTE;
}

bool CGroqProvider::Transcribe(
    const std::string& mediaPath,
    const TranscriptionOptions& options,
    SegmentCallback onSegment,
    ProgressCallback onProgress,
    ErrorCallback onError)
{
    m_cancelled = false;

    if (!IsConfigured())
    {
        if (onError) onError("Groq API key not configured");
        return false;
    }

    // Check file size - Groq has 25MB limit
    XFILE::CFile file;
    if (!file.Open(mediaPath))
    {
        if (onError) onError("Could not open media file");
        return false;
    }

    int64_t fileSize = file.GetLength();
    file.Close();

    if (fileSize > MAX_FILE_SIZE)
    {
        // Need to chunk the file
        return TranscribeLargeFile(mediaPath, options, onSegment, onProgress, onError);
    }

    return TranscribeSingleFile(mediaPath, options, onSegment, onProgress, onError);
}

bool CGroqProvider::TranscribeSingleFile(
    const std::string& audioPath,
    const TranscriptionOptions& options,
    SegmentCallback onSegment,
    ProgressCallback onProgress,
    ErrorCallback onError)
{
    XFILE::CCurlFile curl;

    // Set up headers
    curl.SetRequestHeader("Authorization", fmt::format("Bearer {}", GetApiKey()));

    // Build multipart form data
    std::string boundary = "----KodiSemanticBoundary";
    curl.SetRequestHeader("Content-Type",
        fmt::format("multipart/form-data; boundary={}", boundary));

    // Read audio file
    XFILE::CFile audioFile;
    if (!audioFile.Open(audioPath))
    {
        if (onError) onError("Could not open audio file for upload");
        return false;
    }

    std::vector<uint8_t> audioData(audioFile.GetLength());
    audioFile.Read(audioData.data(), audioData.size());
    audioFile.Close();

    // Build request body
    std::string body;

    // File part
    body += fmt::format("--{}\r\n", boundary);
    body += fmt::format("Content-Disposition: form-data; name=\"file\"; filename=\"{}\"\r\n",
        URIUtils::GetFileName(audioPath));
    body += "Content-Type: audio/mpeg\r\n\r\n";
    body.append(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    body += "\r\n";

    // Model part
    body += fmt::format("--{}\r\n", boundary);
    body += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
    body += "whisper-large-v3-turbo\r\n";

    // Response format
    body += fmt::format("--{}\r\n", boundary);
    body += "Content-Disposition: form-data; name=\"response_format\"\r\n\r\n";
    body += "verbose_json\r\n";

    // Language (if specified)
    if (options.language != "auto" && !options.language.empty())
    {
        body += fmt::format("--{}\r\n", boundary);
        body += "Content-Disposition: form-data; name=\"language\"\r\n\r\n";
        body += options.language + "\r\n";
    }

    // Timestamp granularity
    body += fmt::format("--{}\r\n", boundary);
    body += "Content-Disposition: form-data; name=\"timestamp_granularities[]\"\r\n\r\n";
    body += "segment\r\n";

    body += fmt::format("--{}--\r\n", boundary);

    // Make request
    std::string response;
    if (!curl.Post(API_ENDPOINT, body, response))
    {
        if (onError) onError(fmt::format("API request failed: {}", curl.GetLastError()));
        return false;
    }

    // Parse response
    CVariant json;
    if (!CJSONVariantParser::Parse(response, json))
    {
        if (onError) onError("Failed to parse API response");
        return false;
    }

    // Check for error
    if (json.isMember("error"))
    {
        std::string errorMsg = json["error"]["message"].asString();
        if (onError) onError(errorMsg);
        return false;
    }

    // Extract segments
    std::string detectedLanguage = json["language"].asString();
    const auto& segments = json["segments"];

    int totalSegments = segments.size();
    int processedSegments = 0;

    for (const auto& seg : segments)
    {
        if (m_cancelled) break;

        TranscriptSegment segment;
        segment.startMs = static_cast<int64_t>(seg["start"].asDouble() * 1000) + options.startMs;
        segment.endMs = static_cast<int64_t>(seg["end"].asDouble() * 1000) + options.startMs;
        segment.text = seg["text"].asString();
        segment.language = detectedLanguage;
        segment.confidence = seg.isMember("confidence") ?
            seg["confidence"].asFloat() : 0.9f;

        // Trim whitespace
        StringUtils::Trim(segment.text);

        if (!segment.text.empty() && onSegment)
            onSegment(segment);

        processedSegments++;
        if (onProgress)
            onProgress(static_cast<float>(processedSegments) / totalSegments);
    }

    return !m_cancelled;
}

bool CGroqProvider::TranscribeLargeFile(
    const std::string& mediaPath,
    const TranscriptionOptions& options,
    SegmentCallback onSegment,
    ProgressCallback onProgress,
    ErrorCallback onError)
{
    // For files > 25MB, process in ~45-minute chunks
    // At 64kbps MP3: 25MB = ~52 minutes, use 45 min to be safe
    const int64_t chunkDurationMs = 45 * 60 * 1000;

    // Get total duration (would need media info from Kodi)
    // For now, estimate from file size assuming 128kbps
    XFILE::CFile file;
    if (!file.Open(mediaPath))
    {
        if (onError) onError("Could not open media file");
        return false;
    }

    int64_t fileSize = file.GetLength();
    file.Close();

    // Rough estimate: 128kbps = 16KB/sec
    int64_t estimatedDurationMs = (fileSize / 16) * 1000 / 1024;

    int64_t startMs = options.startMs;
    int64_t endMs = (options.endMs > 0) ? options.endMs : estimatedDurationMs;

    int totalChunks = static_cast<int>((endMs - startMs + chunkDurationMs - 1) / chunkDurationMs);
    int currentChunk = 0;

    while (startMs < endMs && !m_cancelled)
    {
        int64_t chunkEnd = std::min(startMs + chunkDurationMs, endMs);

        // Extract audio chunk to temp file
        std::string chunkPath = ExtractAudioChunk(mediaPath, startMs, chunkEnd);
        if (chunkPath.empty())
        {
            if (onError) onError(fmt::format("Failed to extract chunk at {}ms", startMs));
            return false;
        }

        // Create options with timing offset
        TranscriptionOptions chunkOptions = options;
        chunkOptions.startMs = startMs;  // Used for timestamp offset

        // Progress adjusted for chunk position
        auto chunkProgress = [&](float p) {
            if (onProgress)
            {
                float overallProgress = (currentChunk + p) / totalChunks;
                onProgress(overallProgress);
            }
        };

        bool success = TranscribeSingleFile(
            chunkPath, chunkOptions, onSegment, chunkProgress, onError);

        // Clean up temp file
        XFILE::CFile::Delete(chunkPath);

        if (!success)
            return false;

        startMs = chunkEnd;
        currentChunk++;
    }

    return !m_cancelled;
}

std::string CGroqProvider::ExtractAudioChunk(
    const std::string& mediaPath, int64_t startMs, int64_t endMs)
{
    // This would use FFmpeg or Kodi's internal audio extraction
    // to create a temporary audio file
    // Implementation depends on how Kodi handles audio extraction

    // Placeholder - actual implementation would:
    // 1. Use FFmpeg to extract audio segment
    // 2. Convert to MP3 at 16kHz mono (optimal for Whisper)
    // 3. Save to temp file and return path

    CLog::Log(LOGWARNING, "GroqProvider: Audio chunk extraction not implemented");
    return "";
}

void CGroqProvider::Cancel()
{
    m_cancelled = true;
}

} // namespace KODI::SEMANTIC
```

### 3.6 Background Index Service

```cpp
// SemanticIndexService.h

#pragma once

#include "SemanticTypes.h"
#include "threads/Thread.h"
#include "interfaces/AnnouncementManager.h"

#include <memory>
#include <mutex>
#include <functional>
#include <optional>

namespace KODI::SEMANTIC
{

class CSemanticDatabase;
class CSubtitleParser;
class CMetadataParser;
class CChunkProcessor;
class CTranscriptionProviderManager;

using ProgressCallbackFn = std::function<void(int mediaId, float progress, const std::string& status)>;

class CSemanticIndexService : public CThread,
                               public ANNOUNCEMENT::IAnnouncer
{
public:
    CSemanticIndexService();
    ~CSemanticIndexService() override;

    bool Start();
    void Stop();

    // Queue management
    std::optional<int64_t> QueueIndex(int mediaId, MediaType mediaType, int priority = 0);
    std::optional<int64_t> QueueTranscription(int mediaId, MediaType mediaType,
                                               const std::string& provider = "");
    int QueueAllSubtitles();

    // Status
    std::optional<IndexState> GetIndexState(int mediaId, MediaType mediaType);
    float EstimateBulkCost(const std::string& providerName, int maxItems = 0);

    // Callbacks
    void SetProgressCallback(ProgressCallbackFn callback);

    // IAnnouncer
    void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const CVariant& data) override;

protected:
    void Process() override;

private:
    bool ShouldProcess();

    bool ProcessNextSubtitle();
    bool ProcessNextTranscription();
    bool ProcessNextMetadata();

    void QueueNewItems();
    void RemoveIndex(int mediaId, MediaType mediaType);
    void CancelAllJobs();

    void NotifyProgress(int mediaId, float progress, const std::string& status);

    SourceType DetectSubtitleType(const std::string& path);
    SourceType ProviderToSourceType(const std::string& provider);

    std::unique_ptr<CSemanticDatabase> m_database;
    std::unique_ptr<CSubtitleParser> m_subtitleParser;
    std::unique_ptr<CMetadataParser> m_metadataParser;
    std::unique_ptr<CChunkProcessor> m_chunkProcessor;
    std::unique_ptr<CTranscriptionProviderManager> m_providerManager;

    ProgressCallbackFn m_progressCallback;
    std::mutex m_callbackMutex;
};

} // namespace KODI::SEMANTIC
```

### 3.7 Search Implementation

```cpp
// SemanticSearch.h

#pragma once

#include "SemanticTypes.h"
#include <vector>
#include <string>

namespace KODI::SEMANTIC
{

class CSemanticDatabase;

class CSemanticSearch
{
public:
    explicit CSemanticSearch(CSemanticDatabase* db = nullptr);
    ~CSemanticSearch() = default;

    std::vector<SearchResult> Search(const std::string& query,
                                      const SearchOptions& options = {});

    // Get total indexed stats
    int GetTotalChunks() const;
    int GetIndexedMediaCount() const;

private:
    std::string BuildFTSQuery(const std::string& userQuery);
    std::string EscapeFTSQuery(const std::string& query);

    CSemanticDatabase* m_database;
};

} // namespace KODI::SEMANTIC
```

```cpp
// SemanticSearch.cpp

#include "SemanticSearch.h"
#include "SemanticDatabase.h"
#include "video/VideoDatabase.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

namespace KODI::SEMANTIC
{

CSemanticSearch::CSemanticSearch(CSemanticDatabase* db)
    : m_database(db)
{
}

std::vector<SearchResult> CSemanticSearch::Search(
    const std::string& query, const SearchOptions& options)
{
    std::vector<SearchResult> results;

    if (query.empty() || !m_database)
        return results;

    std::string ftsQuery = BuildFTSQuery(query);

    // Build SQL with FTS5 MATCH
    std::string sql = R"(
        SELECT
            c.chunk_id,
            c.media_id,
            c.media_type,
            c.source_type,
            c.start_ms,
            c.end_ms,
            c.text,
            c.confidence,
            bm25(semantic_fts) as score
        FROM semantic_fts f
        JOIN semantic_chunks c ON f.rowid = c.chunk_id
        WHERE semantic_fts MATCH ?
    )";

    // Add filters
    if (!options.mediaTypes.empty())
    {
        std::vector<std::string> types;
        for (auto type : options.mediaTypes)
        {
            types.push_back(MediaTypeToString(type));
        }
        sql += " AND c.media_type IN ('" + StringUtils::Join(types, "','") + "')";
    }

    if (!options.sourceTypes.empty())
    {
        std::vector<std::string> sources;
        for (auto source : options.sourceTypes)
        {
            sources.push_back(SourceTypeToString(source));
        }
        sql += " AND c.source_type IN ('" + StringUtils::Join(sources, "','") + "')";
    }

    if (options.minConfidence > 0)
    {
        sql += fmt::format(" AND c.confidence >= {}", options.minConfidence);
    }

    sql += " ORDER BY score LIMIT ? OFFSET ?";

    // Execute query
    auto stmt = m_database->Prepare(sql);
    stmt.bind(1, ftsQuery);
    stmt.bind(2, options.limit);
    stmt.bind(3, options.offset);

    // Fetch media titles for results
    CVideoDatabase videoDB;
    videoDB.Open();

    while (stmt.executeStep())
    {
        SearchResult result;
        result.mediaId = stmt.getColumn(1).getInt();
        result.mediaType = StringToMediaType(stmt.getColumn(2).getString());
        result.sourceType = StringToSourceType(stmt.getColumn(3).getString());
        result.startMs = stmt.getColumn(4).getInt64();
        result.endMs = stmt.getColumn(5).getInt64();
        result.matchedText = stmt.getColumn(6).getString();
        result.score = static_cast<float>(-stmt.getColumn(8).getDouble());  // BM25 returns negative

        // Get media title
        if (result.mediaType == MediaType::Movie)
        {
            CVideoInfoTag tag;
            videoDB.GetMovieInfo("", tag, result.mediaId);
            result.mediaTitle = tag.m_strTitle;
        }
        else if (result.mediaType == MediaType::Episode)
        {
            CVideoInfoTag tag;
            videoDB.GetEpisodeInfo("", tag, result.mediaId);
            result.mediaTitle = fmt::format("{} - S{:02d}E{:02d} - {}",
                tag.m_strShowTitle, tag.m_iSeason, tag.m_iEpisode, tag.m_strTitle);
        }

        results.push_back(std::move(result));
    }

    videoDB.Close();

    CLog::Log(LOGDEBUG, "SemanticSearch: Query '{}' returned {} results",
              query, results.size());

    return results;
}

std::string CSemanticSearch::BuildFTSQuery(const std::string& userQuery)
{
    // Escape special FTS5 characters and build query
    std::string escaped = EscapeFTSQuery(userQuery);

    // Use phrase matching for multi-word queries
    if (userQuery.find(' ') != std::string::npos)
    {
        // Try phrase match first, then individual terms
        return fmt::format("\"{}\" OR {}", escaped, escaped);
    }

    return escaped;
}

std::string CSemanticSearch::EscapeFTSQuery(const std::string& query)
{
    std::string result = query;

    // Escape FTS5 special characters: " * - ^
    StringUtils::Replace(result, "\"", "\"\"");
    StringUtils::Replace(result, "*", "");
    StringUtils::Replace(result, "^", "");

    return result;
}

} // namespace KODI::SEMANTIC
```

---

## 4. JSON-RPC API

### 4.1 Method Definitions

```json
{
  "Semantic.Search": {
    "description": "Search indexed media content",
    "params": [
      {"name": "query", "type": "string", "required": true},
      {"name": "options", "type": "object", "required": false, "properties": {
        "mediaTypes": {"type": "array", "items": {"type": "string", "enum": ["movie", "episode", "musicvideo"]}},
        "sourceTypes": {"type": "array", "items": {"type": "string", "enum": ["subtitle", "transcription", "metadata"]}},
        "limit": {"type": "integer", "default": 50, "maximum": 200},
        "offset": {"type": "integer", "default": 0}
      }}
    ],
    "returns": {
      "type": "object",
      "properties": {
        "results": {"type": "array", "items": {"$ref": "#/definitions/SearchResult"}},
        "total": {"type": "integer"}
      }
    }
  },

  "Semantic.GetIndexStatus": {
    "description": "Get indexing status for a media item",
    "params": [
      {"name": "mediaId", "type": "integer", "required": true},
      {"name": "mediaType", "type": "string", "required": true, "enum": ["movie", "episode"]}
    ],
    "returns": {"$ref": "#/definitions/IndexState"}
  },

  "Semantic.QueueIndex": {
    "description": "Queue a media item for indexing",
    "params": [
      {"name": "mediaId", "type": "integer", "required": true},
      {"name": "mediaType", "type": "string", "required": true},
      {"name": "priority", "type": "integer", "default": 0}
    ],
    "returns": {"type": "integer", "description": "Job ID"}
  },

  "Semantic.QueueTranscription": {
    "description": "Queue a media item for transcription",
    "params": [
      {"name": "mediaId", "type": "integer", "required": true},
      {"name": "mediaType", "type": "string", "required": true},
      {"name": "provider", "type": "string", "default": "groq"}
    ],
    "returns": {"type": "integer", "description": "Job ID"}
  },

  "Semantic.GetStats": {
    "description": "Get overall index statistics",
    "params": [],
    "returns": {
      "type": "object",
      "properties": {
        "totalChunks": {"type": "integer"},
        "indexedMedia": {"type": "integer"},
        "pendingSubtitles": {"type": "integer"},
        "pendingTranscriptions": {"type": "integer"}
      }
    }
  },

  "Semantic.EstimateCost": {
    "description": "Estimate transcription cost for pending items",
    "params": [
      {"name": "provider", "type": "string", "default": "groq"},
      {"name": "maxItems", "type": "integer", "default": 0, "description": "0 = all pending"}
    ],
    "returns": {
      "type": "object",
      "properties": {
        "estimatedCostUSD": {"type": "number"},
        "estimatedMinutes": {"type": "number"},
        "itemCount": {"type": "integer"}
      }
    }
  }
}
```

### 4.2 Type Definitions

```json
{
  "definitions": {
    "SearchResult": {
      "type": "object",
      "properties": {
        "mediaId": {"type": "integer"},
        "mediaType": {"type": "string"},
        "mediaTitle": {"type": "string"},
        "matchedText": {"type": "string"},
        "startMs": {"type": "integer"},
        "endMs": {"type": "integer"},
        "score": {"type": "number"},
        "sourceType": {"type": "string"}
      }
    },
    "IndexState": {
      "type": "object",
      "properties": {
        "mediaId": {"type": "integer"},
        "mediaType": {"type": "string"},
        "subtitleStatus": {"type": "string", "enum": ["pending", "processing", "complete", "failed", "no_source"]},
        "transcriptionStatus": {"type": "string", "enum": ["pending", "processing", "complete", "failed", "skipped"]},
        "transcriptionProvider": {"type": "string"},
        "transcriptionProgress": {"type": "number"},
        "chunkCount": {"type": "integer"}
      }
    }
  }
}
```

---

## 5. Testing Reference

### 5.1 Unit Test Examples

```cpp
// test/TestSubtitleParser.cpp

#include <gtest/gtest.h>
#include "semantic/ingest/SubtitleParser.h"

using namespace KODI::SEMANTIC;

class SubtitleParserTest : public ::testing::Test
{
protected:
    CSubtitleParser parser;
};

TEST_F(SubtitleParserTest, ParseSRT_ValidFile_ReturnsEntries)
{
    std::string srtContent =
        "1\n"
        "00:00:01,000 --> 00:00:04,000\n"
        "Hello, world!\n"
        "\n"
        "2\n"
        "00:00:05,500 --> 00:00:08,200\n"
        "This is a test.\n"
        "\n";

    std::string tempPath = CreateTempFile(srtContent, ".srt");
    auto entries = parser.ParseSRT(tempPath);

    ASSERT_EQ(entries.size(), 2);
    EXPECT_EQ(entries[0].startMs, 1000);
    EXPECT_EQ(entries[0].endMs, 4000);
    EXPECT_EQ(entries[0].text, "Hello, world!");

    DeleteTempFile(tempPath);
}

TEST_F(SubtitleParserTest, ParseSRT_HTMLTags_Stripped)
{
    std::string srtContent =
        "1\n"
        "00:00:01,000 --> 00:00:04,000\n"
        "<i>Italic text</i> and <b>bold</b>\n"
        "\n";

    std::string tempPath = CreateTempFile(srtContent, ".srt");
    auto entries = parser.ParseSRT(tempPath);

    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].text, "Italic text and bold");

    DeleteTempFile(tempPath);
}

TEST_F(SubtitleParserTest, ParseSRT_MusicNotes_Filtered)
{
    std::string srtContent =
        "1\n"
        "00:00:01,000 --> 00:00:04,000\n"
        "\u266A Some song lyrics \u266A\n"
        "\n"
        "2\n"
        "00:00:05,000 --> 00:00:08,000\n"
        "Actual dialogue\n"
        "\n";

    std::string tempPath = CreateTempFile(srtContent, ".srt");
    auto entries = parser.ParseSRT(tempPath);

    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].text, "Actual dialogue");

    DeleteTempFile(tempPath);
}
```

### 5.2 Performance Benchmarks

```cpp
TEST(SemanticPerformance, Search_10000Chunks_Under100ms)
{
    CSemanticDatabase db;
    db.OpenInMemory();

    // Insert 10000 test chunks
    for (int i = 0; i < 10000; ++i)
    {
        db.InsertChunk({
            .mediaId = i % 100,
            .mediaType = MediaType::Movie,
            .text = GenerateRandomDialogue(),
            .startMs = i * 1000,
            .endMs = i * 1000 + 900
        });
    }

    CSemanticSearch search(&db);

    auto start = std::chrono::high_resolution_clock::now();
    auto results = search.Search("test query phrase");
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100);
}
```

---

## 6. Audio Extraction Notes

Audio extraction for transcription requires converting media to a format suitable for the Whisper API:

**Target format:**
- MP3 or WAV
- 16kHz sample rate (Whisper optimal)
- Mono channel
- Under 25MB per chunk

**Options for implementation:**
1. Use FFmpeg via system call (simplest, requires FFmpeg installed)
2. Use Kodi's internal audio decoding + libmp3lame encoding
3. Extract to PCM and send as WAV

**Recommended approach:** Start with FFmpeg system call for v1, consider native implementation if FFmpeg dependency is problematic.

```bash
# Example FFmpeg command for audio extraction
ffmpeg -i input.mkv -vn -ar 16000 -ac 1 -b:a 64k -ss 0 -t 2700 output.mp3
```
