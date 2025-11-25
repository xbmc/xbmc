# PRD: Kodi Content Text Index

## Document Information

| Field | Value |
|-------|-------|
| **Feature Name** | Content Text Index (Foundation for Semantic Search) |
| **Version** | 1.1 |
| **Status** | Draft |
| **Target Release** | Kodi 23 "Q*" |
| **Author** | [Contributor Name] |
| **PR Scope** | Foundation layer: FTS5 keyword search, subtitle indexing, cloud transcription |
| **Technical Design** | See `tdd-1-semantic-index.md` for implementation details and code samples |

> **Note**: This PR delivers keyword search via SQLite FTS5. True semantic search (vector embeddings, meaning-based matching) is planned for PR #2.

---

## 1. Executive Summary

This PRD defines the foundational infrastructure for semantic search in Kodi—a system that enables users to search their media libraries using natural language queries and find specific moments within content. This first PR establishes the core indexing pipeline, database schema, subtitle ingestion, cloud-based transcription, and keyword search via SQLite FTS5.

### 1.1 The Vision

```
User types: "the restaurant scene"
System returns: Heat (1995) @ 1:23:45 - "So you never wanted a regular type life?"
               Reservoir Dogs (1992) @ 0:04:12 - "Let me tell you what Like a Virgin is about"
               
User clicks result → Video opens at exact timestamp
```

### 1.2 Why This Matters

- **Library Discovery**: Users with large libraries (1000+ items) struggle to find specific content
- **Accessibility**: Natural language search is more intuitive than filename/metadata browsing
- **Competitive Parity**: Plex, Jellyfin, and streaming services offer increasingly sophisticated search
- **Unique Value**: Timestamp-accurate results go beyond what competitors offer

---

## 2. Problem Statement

### 2.1 Current State

Kodi's existing search capabilities are limited to:
- Filename matching
- Metadata fields (title, plot, genre, cast, director)
- Manual smart playlists with predefined rules

Users cannot:
- Search for dialogue or spoken content within videos
- Find specific scenes or moments
- Use natural language queries
- Discover content based on what happens in the media itself

### 2.2 User Pain Points

| Pain Point | Frequency | Impact |
|------------|-----------|--------|
| "I know I have a movie where they say X, but can't find it" | High | Frustration, abandoned searches |
| "I want to show someone a specific scene but can't locate it" | Medium | Time wasted scrubbing through content |
| "My library is too big to browse effectively" | High | Underutilized media collection |
| "Search only matches titles, not what's actually in the video" | High | Poor discovery experience |

### 2.3 Opportunity

By indexing the textual content of media (via subtitles or transcription), Kodi can offer:
- Full-text search across all dialogue/narration
- Timestamp-precise navigation to search results
- Foundation for future semantic/AI-powered search (PR #2)

---

## 3. Goals and Non-Goals

### 3.1 Goals (This PR)

| ID | Goal | Success Metric |
|----|------|----------------|
| G1 | Create extensible indexing infrastructure | Schema supports subtitles, transcripts, metadata, future embeddings |
| G2 | Ingest existing subtitles (SRT, ASS, VTT, SUB) | 95%+ of common subtitle formats parsed correctly |
| G3 | Enable cloud-based transcription | Groq Whisper integration functional (OpenAI deferred to v1.1) |
| G4 | Provide fast keyword search | <100ms query response for libraries up to 10,000 items |
| G5 | Background processing with minimal UX impact | Indexing uses <10% CPU when user is active |
| G6 | Settings integration | Users can configure providers, API keys, processing preferences |
| G7 | JSON-RPC API for external tools | Full CRUD + search operations exposed |

### 3.2 Non-Goals (This PR)

| ID | Non-Goal | Rationale |
|----|----------|-----------|
| NG1 | Semantic/vector search | Deferred to PR #2 |
| NG2 | Local whisper.cpp integration | Build complexity; deferred to addon or PR #3 |
| NG3 | Music library support | Focus on video first; audio indexing has different requirements |
| NG4 | Real-time transcription during playback | Performance concerns; batch processing only |
| NG5 | GUI search dialog | Minimal GUI in this PR; full dialog in PR #2 |
| NG6 | Multi-language transcription in single file | Edge case; handle in future iteration |

### 3.3 Future Considerations

- Vector embeddings and semantic search (PR #2)
- Local whisper.cpp for privacy-focused users
- Music lyrics indexing
- Scene detection and chapter generation
- Cross-device index sync

---

## 4. User Stories

### 4.1 Primary User Stories

#### US1: Subtitle-Based Search
```
AS A user with subtitle files for my media
I WANT to search for dialogue across my library
SO THAT I can find movies/shows by what characters say

ACCEPTANCE CRITERIA:
- System automatically detects and indexes .srt/.ass/.vtt files
- Search returns results with timestamps
- Results show matched text snippet with context
- Clicking result opens media at timestamp
```

#### US2: Cloud Transcription
```
AS A user without subtitle files
I WANT to transcribe my media using cloud services
SO THAT I can search content that lacks subtitles

ACCEPTANCE CRITERIA:
- Can configure Groq API key in settings (OpenAI support in v1.1)
- Transcription runs in background
- Progress is visible in UI
- Can cancel/pause transcription queue
- Transcripts are stored and searchable
```

> **Note on Subtitles**: Kodi already handles embedded subtitles (MKV internal tracks, etc). This feature indexes whatever subtitles Kodi can access - both external files (.srt, .ass) and embedded tracks. Transcription via Whisper is only needed for media with no subtitles at all.

#### US3: Selective Indexing
```
AS A user with limited API budget
I WANT to choose which media to transcribe
SO THAT I can control costs while indexing important content

ACCEPTANCE CRITERIA:
- Can trigger transcription for individual items via context menu
- Can set rules for auto-transcription (e.g., only movies, only unwatched)
- Cost estimate shown before batch transcription
- Can exclude folders/sources from indexing
```

#### US4: Index Status Visibility
```
AS A user
I WANT to see indexing status for my library
SO THAT I know what's searchable and what's pending

ACCEPTANCE CRITERIA:
- Library items show index status indicator
- Can view overall index statistics
- Can see queue of pending transcriptions
- Error states are visible with retry option
```

### 4.2 Secondary User Stories

#### US5: External Tool Integration
```
AS A developer building Kodi tools
I WANT JSON-RPC access to the semantic index
SO THAT I can build custom search interfaces

ACCEPTANCE CRITERIA:
- Full CRUD operations via JSON-RPC
- Search endpoint with filtering options
- Bulk operations for efficiency
- Proper error responses
```

#### US6: Privacy-Conscious Usage
```
AS A privacy-conscious user
I WANT to understand what data leaves my device
SO THAT I can make informed decisions about cloud transcription

ACCEPTANCE CRITERIA:
- Clear documentation of what data is sent to APIs
- Option to use subtitles only (no cloud)
- Local provider option visible (even if not implemented)
- No telemetry or analytics
```

---

## 5. Technical Architecture

### 5.1 System Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              KODI CORE                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                    CSemanticIndexService                          │   │
│  │  - Orchestrates all indexing operations                           │   │
│  │  - Manages background processing thread                           │   │
│  │  - Handles library change notifications                           │   │
│  │  - Coordinates provider selection                                 │   │
│  └──────────────────────────┬───────────────────────────────────────┘   │
│                             │                                            │
│         ┌───────────────────┼───────────────────┐                       │
│         ▼                   ▼                   ▼                        │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                  │
│  │ Subtitle    │    │Transcription│    │  Metadata   │                  │
│  │ Parser      │    │ Provider    │    │  Parser     │                  │
│  │             │    │ Manager     │    │             │                  │
│  │ - SRT       │    │             │    │ - NFO       │                  │
│  │ - ASS/SSA   │    │ - Groq (v1) │    │ - Plot      │                  │
│  │ - VTT       │    │ - OpenAI*   │    │ - Tags      │                  │
│  │ - Embedded  │    │ - Local*    │    │             │                  │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘                  │
│         │                  │                  │                          │
│         └──────────────────┼──────────────────┘                          │
│                            ▼                                             │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                    CSemanticDatabase                              │   │
│  │  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐      │   │
│  │  │ semantic_chunks│  │ chunk_fts5     │  │ index_state    │      │   │
│  │  │ (content)      │  │ (FTS5 search)  │  │ (progress)     │      │   │
│  │  └────────────────┘  └────────────────┘  └────────────────┘      │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                            │                                             │
│                            ▼                                             │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                    CSemanticSearch                                │   │
│  │  - Query parsing                                                  │   │
│  │  - FTS5 search execution                                         │   │
│  │  - Result ranking and formatting                                  │   │
│  │  - Context retrieval                                              │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                           EXTERNAL SERVICES                              │
│  ┌─────────────────────┐              ┌─────────────────────┐           │
│  │    Groq API (v1)    │              │    OpenAI API (v1.1)│           │
│  │ whisper-large-v3    │              │    whisper-1        │           │
│  │ ~180x realtime      │              │    ~10x realtime    │           │
│  │ $0.20/hr audio      │              │    $0.36/hr audio   │           │
│  └─────────────────────┘              └─────────────────────┘           │
│                                                                          │
│  * = Deferred to v1.1                                                    │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 Directory Structure

```
xbmc/
├── semantic/                              # NEW MODULE
│   ├── CMakeLists.txt
│   ├── SemanticServices.h                 # Service broker registration
│   │
│   ├── SemanticIndexService.h
│   ├── SemanticIndexService.cpp           # Main orchestrator (~800 lines)
│   │
│   ├── SemanticDatabase.h
│   ├── SemanticDatabase.cpp               # DB operations (~600 lines)
│   │
│   ├── SemanticSearch.h
│   ├── SemanticSearch.cpp                 # Search engine (~400 lines)
│   │
│   ├── ingest/
│   │   ├── IContentParser.h               # Parser interface
│   │   ├── SubtitleParser.h
│   │   ├── SubtitleParser.cpp             # Multi-format subtitle parsing (~500 lines)
│   │   ├── MetadataParser.h
│   │   ├── MetadataParser.cpp             # NFO/description parsing (~200 lines)
│   │   ├── ChunkProcessor.h
│   │   └── ChunkProcessor.cpp             # Text segmentation (~300 lines)
│   │
│   ├── transcription/
│   │   ├── ITranscriptionProvider.h       # Provider interface
│   │   ├── TranscriptionProviderManager.h
│   │   ├── TranscriptionProviderManager.cpp (~200 lines)
│   │   ├── GroqProvider.h
│   │   ├── GroqProvider.cpp               # Groq API integration (~400 lines)
│   │   ├── OpenAIProvider.h
│   │   ├── OpenAIProvider.cpp             # OpenAI API integration (~400 lines)
│   │   ├── AudioExtractor.h
│   │   └── AudioExtractor.cpp             # FFmpeg audio extraction (~300 lines)
│   │
│   └── utils/
│       ├── SemanticUtils.h
│       └── SemanticUtils.cpp              # Helpers (~200 lines)
│
├── interfaces/json-rpc/
│   ├── SemanticOperations.h
│   └── SemanticOperations.cpp             # JSON-RPC handlers (~500 lines)
│
├── settings/
│   └── semantic/
│       └── settings.xml                   # Settings definitions
│
└── guilib/
    └── guiinfo/
        └── SemanticGUIInfo.cpp            # GUI info labels (~100 lines)

addons/
└── skin.estuary/
    └── xml/
        ├── DialogContextMenu.xml          # Context menu additions
        └── Includes_SemanticInfo.xml      # Index status indicators

system/
└── settings/
    └── settings.xml                       # Settings category registration

Total new code: ~5,000 lines
```

### 5.3 Module Interactions

```
┌─────────────────┐     Library Update      ┌──────────────────────┐
│  VideoDatabase  │ ────────────────────►   │ SemanticIndexService │
└─────────────────┘     (Observer)          └──────────┬───────────┘
                                                       │
        ┌──────────────────────────────────────────────┼──────────────┐
        │                                              │              │
        ▼                                              ▼              ▼
┌───────────────┐                           ┌──────────────┐  ┌──────────────┐
│SubtitleParser │                           │ AudioExtract │  │MetadataParser│
│               │                           │              │  │              │
│ Parse .srt    │                           │ FFmpeg       │  │ Parse .nfo   │
│ Parse .ass    │                           │ Extract      │  │ Parse plot   │
│ Parse .vtt    │                           │ 16kHz mono   │  │              │
└───────┬───────┘                           └──────┬───────┘  └──────┬───────┘
        │                                          │                 │
        │                                          ▼                 │
        │                                  ┌──────────────┐          │
        │                                  │ Transcription│          │
        │                                  │ Provider     │          │
        │                                  │              │          │
        │                                  │ Groq/OpenAI  │          │
        │                                  │ API call     │          │
        │                                  └──────┬───────┘          │
        │                                         │                  │
        └─────────────────────┬───────────────────┴──────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  ChunkProcessor  │
                    │                  │
                    │ - Sentence split │
                    │ - Timestamp align│
                    │ - Deduplication  │
                    └────────┬─────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │ SemanticDatabase │
                    │                  │
                    │ INSERT chunks    │
                    │ UPDATE FTS5      │
                    │ UPDATE state     │
                    └──────────────────┘
```

---

## 6. Database Schema

### 6.1 Core Tables

```sql
-- ============================================================================
-- SEMANTIC INDEX DATABASE SCHEMA
-- Location: special://database/SemanticIndex.db
-- ============================================================================

-- Version tracking for migrations
CREATE TABLE schema_version (
    version INTEGER PRIMARY KEY,
    applied_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    description TEXT
);

INSERT INTO schema_version (version, description) VALUES (1, 'Initial schema');

-- ============================================================================
-- CONTENT STORAGE
-- ============================================================================

-- Main content chunks table
-- Each row represents a searchable segment of content with timing info
CREATE TABLE semantic_chunks (
    chunk_id INTEGER PRIMARY KEY AUTOINCREMENT,
    
    -- Media reference (links to Kodi's video/music databases)
    media_id INTEGER NOT NULL,
    media_type TEXT NOT NULL CHECK (media_type IN ('movie', 'episode', 'musicvideo')),
    
    -- Content source (simplified to 3 types; parser/provider stored separately)
    source_type TEXT NOT NULL CHECK (source_type IN (
        'subtitle',      -- Any subtitle format (SRT, ASS, VTT, SUB - parser auto-detects)
        'transcription', -- Cloud transcription (provider stored in index_state)
        'metadata'       -- Plot summaries, descriptions, taglines
    )),
    source_path TEXT,                    -- Path to source file (for subtitles)
    
    -- Timing (NULL for non-temporal content like plot summaries)
    start_ms INTEGER,
    end_ms INTEGER,
    
    -- Content
    text TEXT NOT NULL,
    text_normalized TEXT,                -- Lowercase, stripped for matching
    language TEXT DEFAULT 'en',          -- ISO 639-1 code
    
    -- Quality/confidence metrics
    confidence REAL DEFAULT 1.0,         -- Transcription confidence (0-1)
    word_count INTEGER,
    
    -- Deduplication
    content_hash TEXT NOT NULL,          -- SHA256 of normalized text + timing
    
    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    
    -- Constraints
    UNIQUE (media_id, media_type, source_type, content_hash)
);

-- Indexes for common queries
CREATE INDEX idx_chunks_media ON semantic_chunks(media_id, media_type);
CREATE INDEX idx_chunks_source ON semantic_chunks(source_type);
CREATE INDEX idx_chunks_timing ON semantic_chunks(media_id, start_ms) 
    WHERE start_ms IS NOT NULL;
CREATE INDEX idx_chunks_language ON semantic_chunks(language);

-- ============================================================================
-- FULL-TEXT SEARCH (FTS5)
-- ============================================================================

-- FTS5 virtual table for fast text search
-- Uses porter stemmer and unicode tokenization
CREATE VIRTUAL TABLE semantic_fts USING fts5(
    text,
    content='semantic_chunks',
    content_rowid='chunk_id',
    tokenize='porter unicode61 remove_diacritics 2'
);

-- Triggers to keep FTS index synchronized
CREATE TRIGGER semantic_chunks_ai AFTER INSERT ON semantic_chunks BEGIN
    INSERT INTO semantic_fts(rowid, text) VALUES (new.chunk_id, new.text);
END;

CREATE TRIGGER semantic_chunks_ad AFTER DELETE ON semantic_chunks BEGIN
    INSERT INTO semantic_fts(semantic_fts, rowid, text) 
        VALUES ('delete', old.chunk_id, old.text);
END;

CREATE TRIGGER semantic_chunks_au AFTER UPDATE OF text ON semantic_chunks BEGIN
    INSERT INTO semantic_fts(semantic_fts, rowid, text) 
        VALUES ('delete', old.chunk_id, old.text);
    INSERT INTO semantic_fts(rowid, text) VALUES (new.chunk_id, new.text);
END;

-- ============================================================================
-- INDEXING STATE MANAGEMENT
-- ============================================================================

-- Tracks indexing status per media item
CREATE TABLE semantic_index_state (
    media_id INTEGER NOT NULL,
    media_type TEXT NOT NULL CHECK (media_type IN ('movie', 'episode', 'musicvideo')),
    
    -- File tracking for change detection
    media_path TEXT NOT NULL,
    file_modified INTEGER,               -- File mtime for change detection
    file_size INTEGER,
    duration_ms INTEGER,                 -- Media duration for progress calculation
    
    -- Processing status per source
    subtitle_status TEXT DEFAULT 'pending' CHECK (subtitle_status IN (
        'pending', 'processing', 'complete', 'skipped', 'failed', 'no_source'
    )),
    subtitle_path TEXT,
    subtitle_error TEXT,
    subtitle_processed_at INTEGER,
    
    transcription_status TEXT DEFAULT 'pending' CHECK (transcription_status IN (
        'pending', 'processing', 'complete', 'skipped', 'failed', 'queued'
    )),
    transcription_provider TEXT,
    transcription_progress REAL DEFAULT 0,  -- 0-1 progress indicator
    transcription_error TEXT,
    transcription_processed_at INTEGER,
    transcription_cost REAL,              -- Estimated/actual cost in USD
    
    metadata_status TEXT DEFAULT 'pending' CHECK (metadata_status IN (
        'pending', 'processing', 'complete', 'skipped', 'failed'
    )),
    metadata_error TEXT,
    metadata_processed_at INTEGER,
    
    -- Overall state
    is_searchable INTEGER DEFAULT 0,      -- 1 if any content indexed
    chunk_count INTEGER DEFAULT 0,
    total_words INTEGER DEFAULT 0,
    
    -- User preferences
    auto_transcribe INTEGER DEFAULT 0,    -- User opted in for this item
    priority INTEGER DEFAULT 0,           -- Queue priority (higher = sooner)
    
    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    
    PRIMARY KEY (media_id, media_type)
);

-- Index for queue management
CREATE INDEX idx_state_queue ON semantic_index_state(
    transcription_status, priority DESC, created_at ASC
) WHERE transcription_status IN ('pending', 'queued');

CREATE INDEX idx_state_searchable ON semantic_index_state(is_searchable)
    WHERE is_searchable = 1;

-- ============================================================================
-- PROCESSING QUEUE
-- ============================================================================

-- Detailed job queue for transcription tasks
CREATE TABLE semantic_job_queue (
    job_id INTEGER PRIMARY KEY AUTOINCREMENT,
    
    media_id INTEGER NOT NULL,
    media_type TEXT NOT NULL,
    
    job_type TEXT NOT NULL CHECK (job_type IN (
        'transcribe', 'parse_subtitle', 'parse_metadata', 'reindex'
    )),
    
    status TEXT DEFAULT 'queued' CHECK (status IN (
        'queued', 'processing', 'complete', 'failed', 'cancelled'
    )),
    
    priority INTEGER DEFAULT 0,
    
    -- Provider selection (for transcription)
    provider TEXT,
    
    -- Progress tracking
    progress REAL DEFAULT 0,
    progress_message TEXT,
    
    -- Timing
    queued_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    started_at INTEGER,
    completed_at INTEGER,
    
    -- Results
    chunks_created INTEGER DEFAULT 0,
    error_message TEXT,
    retry_count INTEGER DEFAULT 0,
    
    -- Configuration
    options TEXT                          -- JSON blob for job-specific options
);

CREATE INDEX idx_queue_status ON semantic_job_queue(status, priority DESC, queued_at ASC)
    WHERE status = 'queued';

-- ============================================================================
-- PROVIDER CONFIGURATION
-- ============================================================================

-- Stores provider settings and usage stats
CREATE TABLE semantic_providers (
    provider_id TEXT PRIMARY KEY,         -- 'groq', 'openai', 'local'
    
    display_name TEXT NOT NULL,
    is_enabled INTEGER DEFAULT 0,
    is_local INTEGER DEFAULT 0,
    
    -- API configuration (encrypted in practice)
    api_key TEXT,
    api_endpoint TEXT,
    
    -- Model configuration
    model_name TEXT,
    model_path TEXT,                      -- For local providers
    
    -- Usage tracking
    total_minutes_processed REAL DEFAULT 0,
    total_cost REAL DEFAULT 0,
    total_jobs INTEGER DEFAULT 0,
    last_used_at INTEGER,
    
    -- Rate limiting
    rate_limit_requests INTEGER,
    rate_limit_window_seconds INTEGER,
    current_window_requests INTEGER DEFAULT 0,
    current_window_start INTEGER,
    
    -- Status
    last_error TEXT,
    last_error_at INTEGER,
    consecutive_errors INTEGER DEFAULT 0
);

-- Default provider entries
INSERT INTO semantic_providers (provider_id, display_name, is_local, api_endpoint, model_name)
VALUES 
    ('groq', 'Groq Whisper', 0, 'https://api.groq.com/openai/v1/audio/transcriptions', 'whisper-large-v3'),
    ('openai', 'OpenAI Whisper', 0, 'https://api.openai.com/v1/audio/transcriptions', 'whisper-1'),
    ('local', 'Local Whisper', 1, NULL, NULL);

-- ============================================================================
-- SEARCH HISTORY (Optional - for analytics/suggestions)
-- ============================================================================

CREATE TABLE semantic_search_history (
    search_id INTEGER PRIMARY KEY AUTOINCREMENT,
    query TEXT NOT NULL,
    query_normalized TEXT NOT NULL,
    result_count INTEGER,
    selected_chunk_id INTEGER,           -- Which result user clicked
    search_time_ms INTEGER,
    searched_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

CREATE INDEX idx_search_history_time ON semantic_search_history(searched_at DESC);

-- ============================================================================
-- STATISTICS VIEW
-- ============================================================================

CREATE VIEW semantic_stats AS
SELECT
    (SELECT COUNT(*) FROM semantic_index_state) AS total_media,
    (SELECT COUNT(*) FROM semantic_index_state WHERE is_searchable = 1) AS indexed_media,
    (SELECT COUNT(*) FROM semantic_chunks) AS total_chunks,
    (SELECT SUM(word_count) FROM semantic_chunks) AS total_words,
    (SELECT COUNT(*) FROM semantic_chunks WHERE source_type LIKE 'subtitle_%') AS subtitle_chunks,
    (SELECT COUNT(*) FROM semantic_chunks WHERE source_type LIKE 'transcription_%') AS transcription_chunks,
    (SELECT COUNT(*) FROM semantic_job_queue WHERE status = 'queued') AS queued_jobs,
    (SELECT COUNT(*) FROM semantic_job_queue WHERE status = 'processing') AS active_jobs,
    (SELECT SUM(total_cost) FROM semantic_providers) AS total_cost;
```

### 6.2 Query Examples

```sql
-- Basic FTS5 search
SELECT 
    c.chunk_id,
    c.media_id,
    c.media_type,
    c.text,
    c.start_ms,
    c.end_ms,
    c.source_type,
    bm25(semantic_fts) AS relevance
FROM semantic_fts f
JOIN semantic_chunks c ON f.rowid = c.chunk_id
WHERE semantic_fts MATCH 'joker burns money'
ORDER BY relevance
LIMIT 20;

-- Search with media info join (actual implementation)
SELECT 
    c.chunk_id,
    c.media_id,
    c.media_type,
    c.text,
    c.start_ms,
    c.end_ms,
    c.source_type,
    c.confidence,
    bm25(semantic_fts, 1.0) AS score,
    -- Snippet with highlighting
    snippet(semantic_fts, 0, '<b>', '</b>', '...', 10) AS snippet
FROM semantic_fts f
JOIN semantic_chunks c ON f.rowid = c.chunk_id
WHERE semantic_fts MATCH ?
ORDER BY score
LIMIT ?;

-- Get context around a timestamp
SELECT *
FROM semantic_chunks
WHERE media_id = ? 
  AND media_type = ?
  AND start_ms BETWEEN ? - 30000 AND ? + 30000
ORDER BY start_ms;

-- Queue next transcription job
SELECT 
    s.media_id,
    s.media_type,
    s.media_path,
    s.duration_ms
FROM semantic_index_state s
WHERE s.transcription_status = 'queued'
  AND s.subtitle_status IN ('complete', 'no_source', 'skipped')
ORDER BY s.priority DESC, s.created_at ASC
LIMIT 1;
```

---

## 7. API Design

### 7.1 Internal C++ API

```cpp
// ============================================================================
// SemanticSearch.h - Primary search interface
// ============================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace KODI
{
namespace SEMANTIC
{

// Forward declarations
class CSemanticDatabase;

// ============================================================================
// Data Types
// ============================================================================

enum class MediaType
{
    Movie,
    Episode,
    MusicVideo
};

enum class SourceType
{
    SubtitleSRT,
    SubtitleASS,
    SubtitleVTT,
    SubtitleSUB,
    TranscriptionGroq,
    TranscriptionOpenAI,
    TranscriptionLocal,
    MetadataNFO,
    MetadataPlot,
    MetadataTagline
};

enum class IndexStatus
{
    Pending,
    Processing,
    Complete,
    Skipped,
    Failed,
    NoSource
};

struct SearchResult
{
    int64_t chunkId;
    int mediaId;
    MediaType mediaType;
    std::string mediaTitle;          // Joined from video database
    std::string mediaPath;
    
    std::string matchedText;
    std::string snippet;             // Highlighted snippet
    
    std::optional<int64_t> startMs;
    std::optional<int64_t> endMs;
    
    SourceType sourceType;
    float confidence;
    float score;                     // Search relevance score
};

struct SearchOptions
{
    int maxResults = 20;
    float minScore = 0.0f;
    
    // Filters
    std::vector<MediaType> mediaTypes;   // Empty = all types
    std::vector<int> mediaIds;           // Empty = all media
    std::vector<SourceType> sourceTypes; // Empty = all sources
    std::string language;                // Empty = all languages
    
    // Result options
    bool includeSnippet = true;
    int snippetLength = 50;              // Words around match
    bool highlightMatches = true;
    std::string highlightPrefix = "<b>";
    std::string highlightSuffix = "</b>";
};

struct IndexState
{
    int mediaId;
    MediaType mediaType;
    std::string mediaPath;
    
    IndexStatus subtitleStatus;
    IndexStatus transcriptionStatus;
    IndexStatus metadataStatus;
    
    float transcriptionProgress;      // 0-1
    std::string currentError;
    
    bool isSearchable;
    int chunkCount;
    int totalWords;
};

struct IndexStats
{
    int totalMedia;
    int indexedMedia;
    int totalChunks;
    int totalWords;
    int queuedJobs;
    int activeJobs;
    float totalCostUSD;
};

// ============================================================================
// Main Search Interface
// ============================================================================

class CSemanticSearch
{
public:
    CSemanticSearch();
    ~CSemanticSearch();
    
    // ========================================================================
    // Search Operations
    // ========================================================================
    
    /// Perform full-text search across indexed content
    /// @param query Search query (supports FTS5 syntax)
    /// @param options Search configuration
    /// @return Vector of search results, ordered by relevance
    std::vector<SearchResult> Search(
        const std::string& query,
        const SearchOptions& options = {});
    
    /// Get content chunks around a specific timestamp
    /// @param mediaId Media item ID
    /// @param mediaType Type of media
    /// @param timestampMs Center timestamp in milliseconds
    /// @param windowMs Time window around timestamp (default 60 seconds)
    /// @return Vector of chunks within the time window
    std::vector<SearchResult> GetContext(
        int mediaId,
        MediaType mediaType,
        int64_t timestampMs,
        int64_t windowMs = 60000);
    
    /// Get all indexed chunks for a media item
    std::vector<SearchResult> GetMediaChunks(
        int mediaId,
        MediaType mediaType);
    
    // ========================================================================
    // Index Status
    // ========================================================================
    
    /// Check if a media item is indexed and searchable
    bool IsIndexed(int mediaId, MediaType mediaType);
    
    /// Get detailed index state for a media item
    std::optional<IndexState> GetIndexState(int mediaId, MediaType mediaType);
    
    /// Get overall index statistics
    IndexStats GetStats();
    
    // ========================================================================
    // Search Suggestions (Future)
    // ========================================================================
    
    /// Get autocomplete suggestions based on indexed content
    std::vector<std::string> GetSuggestions(
        const std::string& prefix,
        int maxSuggestions = 10);

private:
    std::unique_ptr<CSemanticDatabase> m_database;
    
    // Query processing helpers
    std::string NormalizeQuery(const std::string& query);
    std::string BuildFTSQuery(const std::string& query);
};

// ============================================================================
// Index Service Interface
// ============================================================================

class ISemanticIndexService
{
public:
    virtual ~ISemanticIndexService() = default;
    
    // ========================================================================
    // Service Lifecycle
    // ========================================================================
    
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;
    
    // ========================================================================
    // Indexing Control
    // ========================================================================
    
    /// Queue a media item for indexing
    /// @param mediaId Media item ID
    /// @param mediaType Type of media
    /// @param priority Queue priority (higher = sooner)
    /// @return Job ID if queued successfully
    virtual std::optional<int64_t> QueueIndex(
        int mediaId,
        MediaType mediaType,
        int priority = 0) = 0;
    
    /// Queue transcription for a media item
    /// @param mediaId Media item ID
    /// @param mediaType Type of media
    /// @param provider Provider to use (empty = default)
    /// @return Job ID if queued successfully
    virtual std::optional<int64_t> QueueTranscription(
        int mediaId,
        MediaType mediaType,
        const std::string& provider = "") = 0;
    
    /// Cancel a queued or running job
    virtual bool CancelJob(int64_t jobId) = 0;
    
    /// Cancel all jobs for a media item
    virtual void CancelMediaJobs(int mediaId, MediaType mediaType) = 0;
    
    // ========================================================================
    // Bulk Operations
    // ========================================================================
    
    /// Queue all unindexed media for subtitle parsing
    virtual int QueueAllSubtitles() = 0;
    
    /// Queue all media without transcripts for transcription
    /// @param provider Provider to use
    /// @param maxItems Maximum items to queue (0 = unlimited)
    /// @return Number of items queued
    virtual int QueueBulkTranscription(
        const std::string& provider,
        int maxItems = 0) = 0;
    
    /// Estimate cost for bulk transcription
    virtual float EstimateBulkCost(
        const std::string& provider,
        int maxItems = 0) = 0;
    
    // ========================================================================
    // Index Maintenance
    // ========================================================================
    
    /// Remove index for a media item
    virtual void RemoveIndex(int mediaId, MediaType mediaType) = 0;
    
    /// Rebuild FTS index (for maintenance)
    virtual void RebuildFTSIndex() = 0;
    
    /// Clean up orphaned index entries
    virtual int CleanupOrphans() = 0;
    
    // ========================================================================
    // Progress Callbacks
    // ========================================================================
    
    using ProgressCallback = std::function<void(int mediaId, float progress, const std::string& status)>;
    
    virtual void SetProgressCallback(ProgressCallback callback) = 0;
};

} // namespace SEMANTIC
} // namespace KODI
```

### 7.2 JSON-RPC API

```json
// ============================================================================
// Semantic.Search - Search indexed content
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.Search",
    "params": {
        "query": "joker burns money",        // Required: search query
        "options": {                          // Optional: search options
            "limit": 20,
            "min_score": 0.0,
            "media_types": ["movie"],         // Filter by type
            "media_ids": [],                  // Filter by specific items
            "include_snippet": true,
            "snippet_length": 50
        }
    },
    "id": 1
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "results": [
            {
                "chunk_id": 12345,
                "media_id": 789,
                "media_type": "movie",
                "title": "The Dark Knight",
                "path": "/movies/The Dark Knight (2008)/movie.mkv",
                "text": "All you care about is money. This town deserves a better class of criminal.",
                "snippet": "...All you care about is <b>money</b>. This town deserves...",
                "start_ms": 5765000,
                "end_ms": 5772000,
                "timestamp": "1:36:05",
                "source": "subtitle_srt",
                "confidence": 1.0,
                "score": 0.89
            }
        ],
        "total_results": 1,
        "query_time_ms": 12
    },
    "id": 1
}

// ============================================================================
// Semantic.GetIndexState - Get index status for media
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.GetIndexState",
    "params": {
        "media_id": 789,
        "media_type": "movie"
    },
    "id": 2
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "media_id": 789,
        "media_type": "movie",
        "path": "/movies/The Dark Knight (2008)/movie.mkv",
        "subtitle_status": "complete",
        "transcription_status": "pending",
        "metadata_status": "complete",
        "is_searchable": true,
        "chunk_count": 1847,
        "total_words": 12350
    },
    "id": 2
}

// ============================================================================
// Semantic.QueueTranscription - Queue media for transcription
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.QueueTranscription",
    "params": {
        "media_id": 790,
        "media_type": "movie",
        "provider": "groq",              // Optional: groq, openai, local
        "priority": 10                   // Optional: queue priority
    },
    "id": 3
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "job_id": 456,
        "estimated_cost": 0.72,
        "estimated_time_seconds": 120,
        "queue_position": 3
    },
    "id": 3
}

// ============================================================================
// Semantic.GetStats - Get overall index statistics
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.GetStats",
    "params": {},
    "id": 4
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "total_media": 1250,
        "indexed_media": 890,
        "total_chunks": 245000,
        "total_words": 3400000,
        "subtitle_chunks": 220000,
        "transcription_chunks": 25000,
        "queued_jobs": 15,
        "active_jobs": 2,
        "total_cost_usd": 45.67
    },
    "id": 4
}

// ============================================================================
// Semantic.GetContext - Get content around timestamp
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.GetContext",
    "params": {
        "media_id": 789,
        "media_type": "movie",
        "timestamp_ms": 5765000,
        "window_ms": 60000
    },
    "id": 5
}

// ============================================================================
// Semantic.CancelJob - Cancel queued/running job
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.CancelJob",
    "params": {
        "job_id": 456
    },
    "id": 6
}

// ============================================================================
// Semantic.GetProviders - List available transcription providers
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.GetProviders",
    "params": {},
    "id": 7
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "providers": [
            {
                "id": "groq",
                "name": "Groq Whisper",
                "is_available": true,
                "is_local": false,
                "estimated_speed": 180.0,
                "cost_per_minute": 0.0066
            },
            {
                "id": "openai",
                "name": "OpenAI Whisper",
                "is_available": true,
                "is_local": false,
                "estimated_speed": 10.0,
                "cost_per_minute": 0.006
            },
            {
                "id": "local",
                "name": "Local Whisper",
                "is_available": false,
                "is_local": true,
                "estimated_speed": 5.0,
                "cost_per_minute": 0
            }
        ],
        "default_provider": "groq"
    },
    "id": 7
}

// ============================================================================
// Semantic.EstimateCost - Estimate transcription cost
// ============================================================================
{
    "jsonrpc": "2.0",
    "method": "Semantic.EstimateCost",
    "params": {
        "provider": "groq",
        "media_ids": [790, 791, 792],     // Specific items
        // OR
        "all_pending": true               // All items needing transcription
    },
    "id": 8
}

// Response
{
    "jsonrpc": "2.0",
    "result": {
        "media_count": 3,
        "total_duration_minutes": 360,
        "estimated_cost_usd": 2.38,
        "estimated_time_seconds": 720,
        "provider": "groq"
    },
    "id": 8
}
```

---

## 8. Implementation Details

> **Code samples and detailed implementation specifications have been moved to the Technical Design Document: `tdd-1-semantic-index.md`**

This section provides a high-level overview. See the TDD for:
- Complete database schema with migrations
- C++ class interfaces and implementations
- Subtitle parser implementation (SRT, ASS, VTT)
- Groq provider API integration
- Background service orchestration
- Search query implementation

### 8.1 Component Overview

| Component | Responsibility | Key Files |
|-----------|---------------|-----------|
| **SubtitleParser** | Parse SRT/ASS/VTT/SUB formats, extract timed entries | `ingest/SubtitleParser.cpp` |
| **GroqProvider** | Integrate with Groq Whisper API, handle chunked uploads | `transcription/GroqProvider.cpp` |
| **SemanticIndexService** | Background processing orchestration, queue management | `SemanticIndexService.cpp` |
| **SemanticDatabase** | SQLite operations, FTS5 sync, migrations | `SemanticDatabase.cpp` |
| **SemanticSearch** | FTS5 query building, result ranking | `SemanticSearch.cpp` |

### 8.2 Key Design Decisions

1. **FTS5 with Porter Stemmer**: Enables "burn" to match "burning", "burned", etc.
2. **Chunk-based Storage**: Each subtitle entry or transcription segment is a separate row for timestamp-precise search.
3. **Groq First**: Single provider for v1 reduces complexity. OpenAI added in v1.1.
4. **Background Processing**: Uses Kodi's CThread with idle detection to minimize UI impact.
5. **Embedded Subtitle Support**: Leverages Kodi's existing subtitle extraction - no additional implementation needed.

### 8.3 Audio Extraction for Transcription

For media without subtitles, audio must be extracted before sending to Groq:

- **Format**: MP3, 16kHz, mono (optimal for Whisper)
- **Chunking**: Files >25MB split into ~45-minute segments
- **Implementation**: FFmpeg via system call (simplest for v1)

```bash
# Example extraction command
ffmpeg -i input.mkv -vn -ar 16000 -ac 1 -b:a 64k output.mp3
```

---

## 9. Settings & Configuration

> **Note**: Night-only processing settings have been deferred to v1.1 to reduce initial complexity.

### 9.1 Settings Schema (Simplified for v1)

> **Full settings XML is in the TDD.** Below is a summary of v1 settings.

| Setting ID | Type | Default | Description |
|------------|------|---------|-------------|
| `semantic.enabled` | boolean | false | Master enable for content indexing |
| `semantic.processmode` | string | "idle" | When to process: "idle", "background", "manual" |
| `semantic.groq.apikey` | string | "" | Groq API key (stored securely) |
| `semantic.autotranscribe` | boolean | false | Auto-transcribe media without subtitles |
| `semantic.autotranscribe.maxcost` | number | 10.00 | Monthly budget cap (USD) |
| `semantic.index.subtitles` | boolean | true | Index subtitle files |
| `semantic.index.metadata` | boolean | true | Index plot summaries |

### 9.2 Deferred Settings (v1.1)

The following settings are planned for v1.1:
- `semantic.nightonly` - Process during night hours only
- `semantic.nightstart` / `semantic.nightend` - Night window hours
- `semantic.openai.apikey` - OpenAI Whisper support

---

## 10. Testing Strategy

> **Full test implementations are in the TDD.** Summary below.

### 10.1 Test Categories

| Category | Focus | Example |
|----------|-------|---------|
| **Unit Tests** | Individual parsers, database ops | SRT parsing, FTS queries |
| **Integration Tests** | End-to-end indexing flow | Queue item → indexed → searchable |
| **Performance Tests** | Latency and throughput | 10K chunks search < 100ms |

### 10.2 Key Test Scenarios

1. **Subtitle Parsing**
   - Valid SRT/ASS/VTT files return correct timestamps and text
   - HTML tags stripped (`<i>italic</i>` → `italic`)
   - Music notes and sound effects filtered (`[music]`, `♪`)
   - Character encoding detection (UTF-8, Latin-1)

2. **FTS5 Search**
   - Exact phrase matching
   - Stemming ("burn" matches "burning")
   - Special character handling
   - Media type filtering

3. **Background Service**
   - Idle detection respects user activity
   - Queue processing in priority order
   - Graceful cancellation

4. **Transcription**
   - API key validation
   - Large file chunking (>25MB)
   - Progress reporting
   - Error recovery

---

## 11. Rollout Plan

### 11.1 Implementation Phases

> **Note**: Time estimates removed - focus on deliverables, not timelines.

| Phase | Deliverables |
|-------|--------------|
| **Phase 1** | Database schema, subtitle parser (SRT/ASS/VTT), FTS5 search |
| **Phase 2** | Groq transcription, audio extraction via FFmpeg |
| **Phase 3** | Background service, queue management, settings UI |
| **Phase 4** | JSON-RPC API, testing, documentation |

### 11.2 PR Checklist

- [ ] Code follows Kodi style guidelines
- [ ] Unit tests pass (>80% coverage)
- [ ] Integration tests pass
- [ ] No compiler warnings
- [ ] Settings properly categorized
- [ ] All strings internationalized
- [ ] JSON-RPC API documented
- [ ] Tested on: Linux, Windows, macOS

---

## 12. Success Metrics

| Metric | Target |
|--------|--------|
| Subtitle parsing success rate | >95% |
| Transcription success rate | >90% |
| Search latency (p95) | <100ms for 10K chunks |
| Memory overhead | <50MB with 100K chunks |
| Background CPU usage | <10% when user active |

---

## 13. Security Considerations

- **API Keys**: Stored using Kodi's secure credential storage
- **Data Privacy**: Audio sent to cloud is not stored server-side (per API terms)
- **Network**: All API calls use HTTPS with certificate validation
- **No Telemetry**: No analytics or tracking

---

## 14. Open Questions

| Question | Status |
|----------|--------|
| Music library support? | Deferred to future PR |
| Rate limiting strategy? | Token bucket - implement in v1.1 |
| Multi-audio track handling? | Default to first track |

---

## 15. Glossary

| Term | Definition |
|------|------------|
| **Chunk** | A searchable text segment with optional timestamps |
| **FTS5** | SQLite Full-Text Search version 5 |
| **Transcription** | Converting audio to text via Whisper |

---

## 16. References

- [SQLite FTS5 Documentation](https://www.sqlite.org/fts5.html)
- [Groq API Documentation](https://console.groq.com/docs/speech-text)
- [Kodi Contributing Guidelines](https://github.com/xbmc/xbmc/blob/master/docs/CONTRIBUTING.md)
