# Kodi Semantic Search - Phase 1: Content Text Index

Full-text search for your Kodi video library. Search for dialogue, plot points, and metadata instead of just titles.

## Features

### Content Indexing
- **Subtitle parsing** - SRT, ASS/SSA, VTT formats with timestamp preservation
- **Metadata indexing** - Plot summaries, tags, genres from video database
- **Audio transcription** - Groq Whisper API for content without subtitles
- **Chunk processing** - Smart text segmentation for optimal search

### Full-Text Search
- **SQLite FTS5** - Industry-standard full-text search engine
- **BM25 ranking** - Relevance scoring for search results
- **Timestamp search** - Find exact moments in videos
- **Context display** - Show surrounding text for matches

### Background Indexing
- **Manual mode** - Index on demand
- **Idle mode** - Index when Kodi is idle
- **Background mode** - Continuous background indexing
- **Progress tracking** - Monitor indexing status

### Transcription
- **Groq Whisper API** - Fast, accurate audio transcription
- **Budget management** - Monthly cost tracking and limits
- **Audio extraction** - FFmpeg-based audio processing

### JSON-RPC API
```json
// Start indexing
{"jsonrpc":"2.0","method":"Semantic.StartIndexing","id":1}

// Search
{"jsonrpc":"2.0","method":"Semantic.Search",
 "params":{"query":"robot uprising","limit":20},"id":2}

// Get index status
{"jsonrpc":"2.0","method":"Semantic.GetIndexState","id":3}
```

## Architecture

```
┌─────────────────────────────────────────┐
│        JSON-RPC / Internal API          │
└──────────────────┬──────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────┐
│          CSemanticSearch                │
│     (Full-text search interface)        │
└──────────────────┬──────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────┐
│        CSemanticDatabase                │
│      (SQLite FTS5 storage)              │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────┴──────────────────────┐
│       CSemanticIndexService             │
│      (Background indexing)              │
└───┬─────────┬─────────┬─────────────────┘
    │         │         │
    ▼         ▼         ▼
┌───────┐ ┌───────┐ ┌────────────────┐
│Subtitle│ │Metadata│ │ Transcription  │
│Parser  │ │Parser  │ │   Provider     │
└───────┘ └───────┘ └────────────────┘
```

## Configuration

Settings are in `Settings > Services > Semantic Search`:

| Setting | Description | Default |
|---------|-------------|---------|
| Enable indexing | Turn on/off | Off |
| Index mode | Manual/Idle/Background | Idle |
| Groq API key | For transcription | Empty |
| Monthly budget | Transcription limit | $5.00 |

## Building

The semantic module is built as part of Kodi:

```bash
cmake -B build -DENABLE_INTERNAL_GTEST=ON
cmake --build build
```

## Testing

```bash
ctest --test-dir build -R semantic
```

## Documentation

- [Phase 1 Technical Docs](docs/Phase1-ContentTextIndex.md)
- [User Guide](docs/UserGuide.md)
- [Developer Guide](docs/DeveloperGuide.md)

## Future: Phase 2 (Semantic Intelligence)

Phase 2 will add vector embeddings for true semantic search:
- ONNX Runtime for sentence embeddings
- Hybrid search (FTS5 + vectors)
- Cross-encoder reranking
- Voice input
- GUI dialog

See the `feature/semantic-embeddings` branch for Phase 2 development.
