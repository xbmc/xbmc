# Kodi Semantic Search - User Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Configuration](#configuration)
4. [Using Semantic Search](#using-semantic-search)
5. [JSON-RPC API](#json-rpc-api)
6. [Troubleshooting](#troubleshooting)
7. [FAQ](#faq)

---

## Introduction

Semantic Search for Kodi enables you to find content in your video library using natural language queries. Instead of browsing by title or genre, you can search for specific scenes, dialogue, or plot points.

### What Can You Search?

- **Dialogue from subtitles:** "find the scene where they talk about robots"
- **Audio transcriptions:** Search videos without subtitles (requires API key)
- **Plot summaries:** "movies about time travel"
- **Metadata:** Genre, tags, descriptions

### Example Searches

- "explosion in the warehouse"
- "love confession scene"
- "they discuss the plan to escape"
- "robot uprising"
- "funny moment with the dog"

---

## Getting Started

### Prerequisites

1. **Kodi 22 (Piers) or later**
2. **Subtitle files** for your videos (SRT, ASS, or VTT format)
3. **(Optional) Groq API key** for transcribing videos without subtitles

### First-Time Setup

#### Step 1: Enable Semantic Search

1. Open Kodi Settings
2. Navigate to: **System > Services > Semantic Search**
3. Enable **"Enable Semantic Search"**
4. Choose processing mode:
   - **Manual:** Index only when you request it
   - **Idle:** Index automatically when Kodi is idle
   - **Background:** Index continuously

#### Step 2: Configure Indexing Options

Choose what to index:
- ☑ **Index Subtitles** (Recommended: ON)
- ☑ **Index Metadata** (Recommended: ON)
- ☐ **Auto-transcribe videos without subtitles** (Optional, requires API key)

#### Step 3: (Optional) Set Up Transcription

If you want to transcribe videos without subtitles:

1. Create a free account at [Groq Console](https://console.groq.com)
2. Generate an API key
3. In Kodi settings, enter your API key in:
   **Semantic Search > Groq API Key**
4. Set monthly budget limit (default: $10 USD)

**Note:** Transcription costs ~$0.05-0.15 per hour of video.

#### Step 4: Start Indexing

**Option A: Automatic (Idle/Background mode)**
- Wait for Kodi to automatically start indexing your library
- Progress shown in System Info

**Option B: Manual**
- Use JSON-RPC to trigger indexing (see API section)
- Or use a compatible skin/add-on

---

## Configuration

### Settings Overview

Access via: **Settings > System > Services > Semantic Search**

#### Basic Settings

| Setting | Options | Description |
|---------|---------|-------------|
| Enable Semantic Search | ON/OFF | Master switch for the system |
| Processing Mode | Manual/Idle/Background | When to index content |
| Index Subtitles | ON/OFF | Extract text from subtitle files |
| Index Metadata | ON/OFF | Extract plot summaries, tags, etc. |
| Auto-transcribe | ON/OFF | Automatically transcribe videos without subtitles |

#### Advanced Settings (Level 3)

| Setting | Type | Description |
|---------|------|-------------|
| Groq API Key | String | API key for Groq Whisper transcription |
| Monthly Budget | Number | Max transcription cost per month (USD) |
| Chunk Size | Number | Words per indexed chunk (default: 50) |
| Minimum Chunk Size | Number | Minimum chunk size (default: 10) |

### Processing Modes Explained

#### Manual Mode
- **Best for:** Users who want full control
- **Behavior:** Nothing happens automatically
- **Trigger:** Use JSON-RPC API or compatible skin
- **Resource usage:** None until triggered

#### Idle Mode (Recommended)
- **Best for:** Most users
- **Behavior:** Indexes when Kodi is idle (no playback, no user input for 5 min)
- **Trigger:** Automatic during idle periods
- **Resource usage:** Low impact on system

#### Background Mode
- **Best for:** Power users with fast systems
- **Behavior:** Indexes continuously at low priority
- **Trigger:** Automatic and continuous
- **Resource usage:** Constant low background load

### Budget Management

When auto-transcription is enabled:

- Set a monthly budget to avoid unexpected costs
- Kodi tracks usage and warns when approaching limit
- Budget resets on the 1st of each month
- View usage: **System Info > Semantic Search**

**Example Costs:**
- 90-minute movie: ~$0.08-0.12
- 45-minute TV episode: ~$0.04-0.06
- 22-minute sitcom: ~$0.02-0.03

---

## Using Semantic Search

### From Kodi UI

**Note:** UI integration depends on your skin. Check your skin's documentation for semantic search support.

Typical workflow:
1. Open search interface
2. Type natural language query
3. View results with timestamps
4. Click to jump to exact scene

### From Add-ons

Several add-ons support semantic search:

- **Voice Search Assistant:** Voice-activated search
- **Smart Search:** Enhanced search interface
- **Library Tools:** Batch operations and management

Check add-on documentation for specific usage instructions.

### Search Tips

#### Good Queries
- ✓ "robot uprising" - Simple and specific
- ✓ "love confession" - Common phrases
- ✓ "explosion warehouse" - Multiple keywords
- ✓ "plan to escape" - Natural phrases

#### Queries to Avoid
- ✗ "find me the scene where..." - Too conversational
- ✗ Single letters or very short words
- ✗ Complex boolean logic (use multiple searches)

#### Advanced Search Techniques

**Phrase search:** Use quotes (in JSON-RPC)
```json
"query": "\"robot uprising\""
```

**Multiple terms:** Searches for all terms
```json
"query": "robot explosion city"
```

**Filter by media type:**
```json
"options": {"media_type": "movie"}
```

**Filter by source:**
```json
"options": {"source_type": "subtitle"}
```

---

## JSON-RPC API

Semantic search can be controlled via JSON-RPC, enabling integration with external applications, web interfaces, and custom scripts.

### Connection Setup

**WebSocket (Recommended):**
```javascript
const ws = new WebSocket('ws://kodi-ip:9090/jsonrpc');

ws.onmessage = (event) => {
  const response = JSON.parse(event.data);
  console.log(response);
};
```

**HTTP:**
```bash
curl -X POST http://kodi-ip:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"Semantic.Search","params":{"query":"robot"},"id":1}'
```

### API Methods

#### Semantic.Search

Search across all indexed content.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.Search",
  "params": {
    "query": "robot uprising",
    "options": {
      "limit": 20,
      "media_type": "movie",
      "min_confidence": 0.8
    }
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "results": [
      {
        "chunk_id": 12345,
        "media_id": 456,
        "media_type": "movie",
        "text": "The robots are taking over the city...",
        "snippet": "The <b>robots</b> are taking over...",
        "start_ms": 1234000,
        "end_ms": 1240000,
        "timestamp": "00:20:34.000",
        "source": "subtitle",
        "confidence": 1.0,
        "score": 15.3
      }
    ],
    "total_results": 5,
    "query_time_ms": 23
  },
  "id": 1
}
```

**Parameters:**
- `query` (required): Search query string
- `options` (optional):
  - `limit`: Max results (default: 50)
  - `media_type`: Filter by "movie", "episode", or "musicvideo"
  - `media_id`: Search within specific media item
  - `source_type`: Filter by "subtitle", "transcription", or "metadata"
  - `min_confidence`: Minimum confidence score (0.0-1.0)

---

#### Semantic.GetContext

Get surrounding dialogue/text around a timestamp.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetContext",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "timestamp_ms": 1234000,
    "window_ms": 60000
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "chunks": [
      {
        "chunk_id": 123,
        "text": "What are you doing?",
        "start_ms": 1210000,
        "end_ms": 1213000,
        "timestamp": "00:20:10.000",
        "source": "subtitle",
        "confidence": 1.0
      },
      {
        "chunk_id": 124,
        "text": "I'm trying to stop them!",
        "start_ms": 1213000,
        "end_ms": 1217000,
        "timestamp": "00:20:13.000",
        "source": "subtitle",
        "confidence": 1.0
      }
    ],
    "media_id": 456,
    "media_type": "movie",
    "center_timestamp_ms": 1234000,
    "window_ms": 60000
  },
  "id": 1
}
```

**Use case:** Show context around a search result.

---

#### Semantic.GetIndexState

Check if a media item has been indexed.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetIndexState",
  "params": {
    "media_id": 456,
    "media_type": "movie"
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "media_id": 456,
    "media_type": "movie",
    "path": "/media/movies/Example.mkv",
    "subtitle_status": "completed",
    "transcription_status": "pending",
    "transcription_progress": 0.0,
    "metadata_status": "completed",
    "embedding_status": "pending",
    "is_searchable": true,
    "chunk_count": 234,
    "embeddings_count": 0,
    "priority": 0
  },
  "id": 1
}
```

**Status values:**
- `pending`: Not started
- `in_progress`: Currently processing
- `completed`: Finished successfully
- `failed`: Error occurred

---

#### Semantic.QueueIndex

Manually trigger indexing for a media item.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.QueueIndex",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "priority": 10
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "queued": true,
    "media_id": 456,
    "media_type": "movie"
  },
  "id": 1
}
```

**Priority:** Higher values process sooner (default: 0)

---

#### Semantic.QueueTranscription

Manually request transcription for a media item.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.QueueTranscription",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "provider_id": "groq"
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "queued": true,
    "media_id": 456,
    "media_type": "movie",
    "provider_id": "groq"
  },
  "id": 1
}
```

---

#### Semantic.GetStats

Get overall indexing statistics.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetStats",
  "params": {},
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "total_media": 1250,
    "indexed_media": 980,
    "total_chunks": 145678,
    "total_words": 2456789,
    "queued_jobs": 12,
    "total_cost_usd": 3.45,
    "monthly_cost_usd": 1.20,
    "budget_exceeded": false,
    "remaining_budget_usd": 8.80,
    "embedding_count": 0
  },
  "id": 1
}
```

---

#### Semantic.GetProviders

List available transcription providers.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetProviders",
  "params": {},
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "providers": [
      {
        "id": "groq",
        "name": "Groq Whisper",
        "is_configured": true,
        "is_available": true,
        "is_local": false,
        "cost_per_minute": 0.05
      }
    ],
    "default_provider": "groq"
  },
  "id": 1
}
```

---

#### Semantic.EstimateCost

Estimate transcription cost for a media item.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.EstimateCost",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "provider_id": "groq"
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "media_id": 456,
    "media_type": "movie",
    "provider_id": "groq",
    "duration_seconds": 5400,
    "duration_minutes": 90.0,
    "cost_per_minute": 0.05,
    "estimated_cost_usd": 4.50,
    "is_local": false
  },
  "id": 1
}
```

---

## Troubleshooting

### Search Returns No Results

**Possible causes:**

1. **Media not indexed yet**
   - Check: `Semantic.GetIndexState` for the media
   - Solution: Wait for indexing or use `Semantic.QueueIndex`

2. **No subtitles or transcription**
   - Check: Does the video have subtitle files?
   - Check: Is metadata indexing enabled?
   - Solution: Add subtitles or enable transcription

3. **Query too specific**
   - Try: Broader search terms
   - Try: Different phrasing

4. **Subtitle quality issues**
   - Check: Open subtitle file manually
   - Solution: Use better quality subtitles

### Transcription Fails

**Error: "API key not set"**
- Solution: Add Groq API key in settings

**Error: "Budget exceeded"**
- Solution: Increase monthly budget or wait for new month

**Error: "Network error"**
- Check: Internet connection
- Check: Firewall settings
- Try: Again later (provider may be down)

**Error: "File too large"**
- Cause: Video exceeds provider limits
- Solution: Provider automatically chunks large files

### Indexing is Slow

**In Idle mode:**
- Normal: Indexing only happens during idle periods
- Solution: Switch to Background mode for faster indexing

**In Background mode:**
- Check: System resources (CPU, disk I/O)
- Check: Large library = takes time
- Monitor: `Semantic.GetStats` to see progress

### Database Errors

**Error: "Database locked"**
- Cause: Multiple operations accessing database
- Solution: Wait and retry, or restart Kodi

**Error: "Disk full"**
- Check: Available disk space
- Solution: Free up space, database grows with library

### High Resource Usage

**CPU usage during indexing:**
- Normal: Parsing and processing is CPU-intensive
- Solution: Use Idle mode to minimize impact

**Memory usage:**
- Normal: ~50-100MB during active indexing
- Issue: If higher, check for memory leaks (report bug)

**Disk I/O:**
- Normal: Database writes during indexing
- Solution: Use SSD for better performance

---

## FAQ

### General Questions

**Q: How much disk space does semantic search use?**

A: Approximately 300 bytes per chunk. For a typical library:
- 100 movies (avg 2hrs each) = ~50MB
- 500 TV episodes (avg 45min) = ~40MB
- Total: ~90MB for 600 items

**Q: Can I search in languages other than English?**

A: Yes, but search quality varies:
- **Good:** Spanish, French, German, Italian
- **Fair:** Russian, Japanese, Chinese
- **Limited:** Languages with non-Latin scripts

**Q: Does this work offline?**

A: Partially:
- Search: Works completely offline
- Indexing subtitles/metadata: Works offline
- Transcription: Requires internet (unless using local Whisper in future)

**Q: Can I use this on a Raspberry Pi?**

A: Yes, but:
- Use Idle or Manual mode (Background mode may be slow)
- Indexing takes longer
- Search performance is good

### Privacy and Security

**Q: What data is sent to cloud providers?**

A: Only when transcription is requested:
- Audio extracted from video files
- No video data, no file names, no personal data

**Q: Is my search history stored?**

A: Search history tracking is planned but not yet implemented. Currently, searches are not logged.

**Q: Can I use a local transcription service?**

A: Not yet. Local Whisper.cpp integration is planned for a future release.

### Advanced Usage

**Q: Can I export search results?**

A: Use JSON-RPC API and save results to a file:
```bash
curl -X POST http://kodi:8080/jsonrpc \
  -d '{"jsonrpc":"2.0","method":"Semantic.Search","params":{"query":"robot"},"id":1}' \
  > results.json
```

**Q: Can I bulk-index my entire library?**

A: Yes, use a script with JSON-RPC:
```python
import requests

# Get all movies
movies = requests.post('http://kodi:8080/jsonrpc', json={
  'jsonrpc': '2.0',
  'method': 'VideoLibrary.GetMovies',
  'params': {},
  'id': 1
}).json()['result']['movies']

# Queue each for indexing
for movie in movies:
  requests.post('http://kodi:8080/jsonrpc', json={
    'jsonrpc': '2.0',
    'method': 'Semantic.QueueIndex',
    'params': {
      'media_id': movie['movieid'],
      'media_type': 'movie',
      'priority': 1
    },
    'id': 1
  })
```

**Q: Can I delete indexed data for a specific media item?**

A: Not yet via API. For now:
```bash
sqlite3 ~/.kodi/userdata/Database/SemanticIndex.db
DELETE FROM semantic_chunks WHERE media_id = 456 AND media_type = 'movie';
```

**Q: Can I customize chunk sizes?**

A: Currently requires code modification. Settings UI is planned.

---

## Getting Help

### Community Support

- **Kodi Forum:** [forum.kodi.tv](https://forum.kodi.tv)
- **GitHub Issues:** Report bugs and feature requests
- **Discord:** Join #semantic-search channel

### Reporting Bugs

Include:
1. Kodi version
2. Operating system
3. Semantic search settings
4. Steps to reproduce
5. Error logs (enable debug logging)

### Feature Requests

Submit to GitHub with:
1. Clear description of desired feature
2. Use case / benefit
3. Example of how it would work

---

## Appendix

### Supported Subtitle Formats

| Format | Extension | Timing | Formatting | Notes |
|--------|-----------|--------|------------|-------|
| SubRip | .srt | ✓ | Basic | Most common |
| ASS/SSA | .ass, .ssa | ✓ | Advanced | Speaker identification |
| WebVTT | .vtt | ✓ | Basic | Web standard |

### Supported Metadata Formats

| Format | Extension | Fields Indexed |
|--------|-----------|----------------|
| NFO | .nfo | plot, tagline, outline, genre, tags |
| VideoInfoTag | (memory) | Same as NFO |

### System Requirements

**Minimum:**
- Kodi 22 (Piers)
- 512MB RAM
- 100MB disk space
- SQLite 3.35+

**Recommended:**
- Kodi 22 (Piers) or later
- 1GB+ RAM
- 500MB+ disk space (for large libraries)
- SSD storage
- Multi-core CPU

---

**Document Version:** 1.0
**Last Updated:** 2025-11-25
**For Kodi Version:** 22 (Piers) and later
