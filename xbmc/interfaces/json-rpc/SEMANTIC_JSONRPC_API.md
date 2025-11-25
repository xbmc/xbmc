# Semantic Search JSON-RPC API Documentation

## Overview

This document describes the JSON-RPC API for Kodi's semantic search functionality. These methods expose semantic search, indexing, and transcription operations to external tools and the Kodi UI.

## Implementation Files

- **Header**: `/home/user/xbmc/xbmc/interfaces/json-rpc/SemanticOperations.h`
- **Implementation**: `/home/user/xbmc/xbmc/interfaces/json-rpc/SemanticOperations.cpp`
- **Method Registration**: `/home/user/xbmc/xbmc/interfaces/json-rpc/schema/methods.json`

## API Methods

### 1. Semantic.Search

**Description**: Search indexed content using full-text search

**Permission**: ReadData

**Parameters**:
- `query` (string, required): Search query string
- `options` (object, optional):
  - `limit` (integer, 1-500, default: 50): Maximum number of results
  - `media_type` (string): Filter by media type ("movie", "episode", "musicvideo")
  - `media_id` (integer): Search within specific media item
  - `source_type` (string): Filter by source ("subtitle", "transcription", "metadata")
  - `min_confidence` (number, 0.0-1.0): Minimum confidence threshold

**Returns**:
```json
{
  "results": [
    {
      "chunk_id": 123,
      "media_id": 456,
      "media_type": "movie",
      "text": "Full text of the chunk",
      "snippet": "Highlighted snippet with <b>query</b> terms",
      "start_ms": 125000,
      "end_ms": 130000,
      "timestamp": "00:02:05.000",
      "source": "subtitle",
      "confidence": 0.95,
      "score": 15.7
    }
  ],
  "total_results": 42,
  "query_time_ms": 15
}
```

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.Search",
  "params": {
    "query": "time travel",
    "options": {
      "limit": 10,
      "media_type": "movie"
    }
  },
  "id": 1
}
```

---

### 2. Semantic.GetContext

**Description**: Get context around a specific timestamp in a media item

**Permission**: ReadData

**Parameters**:
- `media_id` (integer, required): Media item ID
- `media_type` (string, required): Media type ("movie", "episode", "musicvideo")
- `timestamp_ms` (integer, required): Timestamp in milliseconds
- `window_ms` (integer, optional, default: 60000): Context window size in milliseconds

**Returns**:
```json
{
  "chunks": [
    {
      "chunk_id": 789,
      "text": "Dialog or transcription text",
      "start_ms": 118000,
      "end_ms": 122000,
      "timestamp": "00:01:58.000",
      "source": "subtitle",
      "confidence": 1.0
    }
  ],
  "media_id": 456,
  "media_type": "movie",
  "center_timestamp_ms": 120000,
  "window_ms": 60000
}
```

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetContext",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "timestamp_ms": 120000,
    "window_ms": 30000
  },
  "id": 2
}
```

---

### 3. Semantic.FindSimilar

**Description**: Find similar content using vector search (Wave 1 feature - not yet implemented)

**Permission**: ReadData

**Parameters**:
- `chunk_id` (integer, optional): Find content similar to this chunk
- `media_id` (integer, optional): Media item ID
- `media_type` (string, optional): Media type

**Returns**: Object (structure TBD when implemented)

**Status**: Currently returns MethodNotFound with explanation that this requires Wave 1 embedding generation.

---

### 4. Semantic.GetIndexState

**Description**: Get indexing status for a media item

**Permission**: ReadData

**Parameters**:
- `media_id` (integer, required): Media item ID
- `media_type` (string, required): Media type ("movie", "episode", "musicvideo")

**Returns**:
```json
{
  "media_id": 456,
  "media_type": "movie",
  "path": "/path/to/movie.mkv",
  "subtitle_status": "completed",
  "transcription_status": "pending",
  "transcription_provider": "groq",
  "transcription_progress": 0.0,
  "metadata_status": "completed",
  "embedding_status": "pending",
  "embedding_progress": 0.0,
  "is_searchable": true,
  "chunk_count": 234,
  "embeddings_count": 234,
  "priority": 0
}
```

**Status Values**: "pending", "in_progress", "completed", "failed"

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetIndexState",
  "params": {
    "media_id": 456,
    "media_type": "movie"
  },
  "id": 3
}
```

---

### 5. Semantic.QueueIndex

**Description**: Queue a media item for indexing

**Permission**: UpdateData

**Parameters**:
- `media_id` (integer, required): Media item ID
- `media_type` (string, required): Media type ("movie", "episode", "musicvideo")
- `priority` (integer, optional, default: 0): Queue priority (higher = sooner)

**Returns**:
```json
{
  "queued": true,
  "media_id": 456,
  "media_type": "movie"
}
```

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.QueueIndex",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "priority": 10
  },
  "id": 4
}
```

---

### 6. Semantic.QueueTranscription

**Description**: Queue a media item for transcription

**Permission**: UpdateData

**Parameters**:
- `media_id` (integer, required): Media item ID
- `media_type` (string, required): Media type ("movie", "episode", "musicvideo")
- `provider_id` (string, optional): Transcription provider ID (uses default if not specified)

**Returns**:
```json
{
  "queued": true,
  "media_id": 456,
  "media_type": "movie",
  "provider_id": "groq"
}
```

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.QueueTranscription",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "provider_id": "groq"
  },
  "id": 5
}
```

---

### 7. Semantic.GetStats

**Description**: Get overall semantic index statistics

**Permission**: ReadData

**Parameters**: None

**Returns**:
```json
{
  "total_media": 1543,
  "indexed_media": 892,
  "total_chunks": 125489,
  "total_words": 2345678,
  "queued_jobs": 12,
  "total_cost_usd": 45.67,
  "monthly_cost_usd": 12.34,
  "budget_exceeded": false,
  "remaining_budget_usd": 87.66,
  "embedding_count": 98234
}
```

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetStats",
  "params": {},
  "id": 6
}
```

---

### 8. Semantic.GetProviders

**Description**: Get list of available transcription providers

**Permission**: ReadData

**Parameters**: None

**Returns**:
```json
{
  "providers": [
    {
      "id": "groq",
      "name": "Groq Whisper",
      "is_configured": true,
      "is_available": true,
      "is_local": false,
      "cost_per_minute": 0.011
    },
    {
      "id": "openai",
      "name": "OpenAI Whisper",
      "is_configured": false,
      "is_available": false,
      "is_local": false,
      "cost_per_minute": 0.006
    }
  ],
  "default_provider": "groq"
}
```

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.GetProviders",
  "params": {},
  "id": 7
}
```

---

### 9. Semantic.EstimateCost

**Description**: Estimate transcription cost for a media item

**Permission**: ReadData

**Parameters**:
- `media_id` (integer, required): Media item ID
- `media_type` (string, required): Media type ("movie", "episode", "musicvideo")
- `provider_id` (string, optional): Provider ID (uses default if not specified)

**Returns**:
```json
{
  "media_id": 456,
  "media_type": "movie",
  "provider_id": "groq",
  "duration_seconds": 7200,
  "duration_minutes": 120.0,
  "cost_per_minute": 0.011,
  "estimated_cost_usd": 1.32,
  "is_local": false
}
```

**Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.EstimateCost",
  "params": {
    "media_id": 456,
    "media_type": "movie",
    "provider_id": "groq"
  },
  "id": 8
}
```

---

## Implementation Notes

### Service Access

The implementation uses static instances for service access:
- `GetSemanticDatabase()`: Access to semantic database
- `GetSemanticSearch()`: Access to search engine
- `GetProviderManager()`: Access to transcription providers

These will be integrated into `CServiceBroker` in future waves.

### Error Handling

All methods follow standard JSON-RPC error codes:
- `InvalidParams`: Missing or invalid parameters
- `FailedToExecute`: Database or service initialization failed
- `InternalError`: Database connection or other internal errors
- `MethodNotFound`: Feature not yet implemented (e.g., FindSimilar)

### Timestamp Formatting

Timestamps are formatted as `HH:MM:SS.mmm` for UI display while maintaining millisecond precision in `_ms` fields.

### Performance

- Search operations typically complete in 10-50ms
- Results include query timing information for performance monitoring
- All database queries use prepared statements for security and performance

## Integration Requirements

To enable these JSON-RPC methods in the build system:

1. Add `SemanticOperations.cpp` to CMakeLists.txt
2. Register method handlers in JSON-RPC dispatcher
3. Ensure semantic database is initialized before handling requests
4. Add appropriate permission checks for write operations

## Testing

Example curl command to test the API:

```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "Semantic.Search",
    "params": {
      "query": "test query",
      "options": {"limit": 5}
    },
    "id": 1
  }'
```

## Future Enhancements (Wave 1+)

- `Semantic.FindSimilar`: Vector-based similarity search
- `Semantic.GetRecommendations`: Content recommendations based on viewing history
- `Semantic.AnalyzeScene`: AI-powered scene analysis and tagging
- Webhook notifications for indexing completion
- Batch operations for multi-item indexing

---

**Wave 0 Status**: Core search and indexing methods implemented and ready for testing.
**Wave 1 Preview**: Vector search method defined but not yet functional (requires embedding generation).
