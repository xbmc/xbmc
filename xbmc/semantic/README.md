# Kodi Semantic Search

Natural language search for your Kodi video library. Find content by searching for dialogue, plot points, scenes, and themes instead of just titles and metadata.

## What is Semantic Search?

Semantic Search enables you to search your media library using natural language queries. Instead of browsing by title, you can search for specific moments:

- **"robot uprising"** - Find scenes about robots taking over
- **"love confession scene"** - Jump to romantic moments
- **"explosion in warehouse"** - Find action sequences
- **"they discuss the plan"** - Locate plot-critical dialogue

## Features

### Phase 1: Content Text Index (✅ Complete)

- **Full-text search** with SQLite FTS5 and BM25 ranking
- **Subtitle parsing** (SRT, ASS/SSA, VTT formats)
- **Metadata indexing** (plot summaries, tags, genres)
- **Audio transcription** via Groq Whisper API
- **JSON-RPC API** for external control
- **Background indexing** with configurable modes (manual/idle/background)
- **Budget management** for transcription costs

### Phase 2: Vector Embeddings (🚧 Planned)

- Sentence embeddings for semantic similarity
- "Find similar" functionality
- Cross-lingual search

### Phase 3: Hybrid Search (🚧 Planned)

- Combined keyword + semantic search
- Advanced ranking algorithms
- Query understanding and expansion

### Phase 4: UI & Voice (🚧 Planned)

- Voice-activated search
- Search UI components
- Result previews with video thumbnails

## Quick Start

### For Users

See the **[User Guide](docs/UserGuide.md)** for:
- Installation and setup
- Configuring settings
- Using the JSON-RPC API
- Troubleshooting

### For Developers

See the **[Developer Guide](docs/DeveloperGuide.md)** for:
- Adding new content parsers
- Implementing transcription providers
- Extending the indexing pipeline
- Testing strategies

### For Contributors

See the **[PR Checklist](docs/PRChecklist.md)** before submitting pull requests.

## Documentation

### User Documentation
- **[User Guide](docs/UserGuide.md)** - Setup, configuration, and usage
- **[API Reference](docs/Phase1-ContentTextIndex.md#json-rpc-api)** - JSON-RPC API documentation

### Developer Documentation
- **[Phase 1 Technical Docs](docs/Phase1-ContentTextIndex.md)** - Architecture, database schema, API reference
- **[Developer Guide](docs/DeveloperGuide.md)** - Extending and contributing
- **[PR Checklist](docs/PRChecklist.md)** - Code review requirements

### Component Documentation
- **[Subtitle Parser](ingest/SUBTITLE_PARSER_README.md)** - SRT/ASS/VTT parsing
- **[Chunk Processor](ingest/CHUNK_PROCESSOR_README.md)** - Text segmentation
- **[Search Integration](search/INTEGRATION.md)** - Search system overview
- **[Tokenizer](embedding/TOKENIZER_README.md)** - Text tokenization

## Architecture

```
┌────────────────────────────────────────────┐
│         Client Applications                 │
│    (JSON-RPC, UI, Voice, External)         │
└──────────────────┬─────────────────────────┘
                   │
                   ▼
┌────────────────────────────────────────────┐
│          CSemanticSearch                   │
│     (High-level Search Interface)          │
└──────────────────┬─────────────────────────┘
                   │
                   ▼
┌────────────────────────────────────────────┐
│        CSemanticDatabase                   │
│    (SQLite FTS5 + Vector Storage)          │
└────────────────────────────────────────────┘
                   ▲
                   │
┌──────────────────┴─────────────────────────┐
│       CSemanticIndexService                │
│         (Background Indexing)              │
└──┬──────────┬──────────┬──────────────┬───┘
   │          │          │              │
   ▼          ▼          ▼              ▼
┌──────┐  ┌──────┐  ┌────────┐  ┌────────────┐
│Sub.  │  │Meta. │  │Audio   │  │Transcribe  │
│Parser│  │Parser│  │Extract.│  │Manager     │
└──────┘  └──────┘  └────────┘  └────────────┘
```

### Core Components

| Component | Description | Location |
|-----------|-------------|----------|
| **CSemanticDatabase** | SQLite database with FTS5 index | `SemanticDatabase.h/cpp` |
| **CSemanticIndexService** | Background indexing orchestrator | `SemanticIndexService.h/cpp` |
| **CSemanticSearch** | High-level search API | `search/SemanticSearch.h/cpp` |
| **Content Parsers** | Extract text from various sources | `ingest/` |
| **Transcription** | Audio-to-text conversion | `transcription/` |
| **ChunkProcessor** | Text segmentation and optimization | `ingest/ChunkProcessor.h/cpp` |

## Database Schema

### Key Tables

- **`semantic_chunks`**: Indexed content with timing info
- **`semantic_chunks_fts`**: FTS5 full-text search index
- **`semantic_index_state`**: Per-media indexing status
- **`semantic_providers`**: Transcription provider config
- **`semantic_embeddings`**: Vector embeddings (Phase 2)

See [Phase 1 Technical Docs](docs/Phase1-ContentTextIndex.md#database-schema) for complete schema.

## Usage Examples

### Search via JSON-RPC

```json
{
  "jsonrpc": "2.0",
  "method": "Semantic.Search",
  "params": {
    "query": "robot uprising",
    "options": {
      "limit": 20,
      "media_type": "movie"
    }
  },
  "id": 1
}
```

### C++ API

```cpp
#include "semantic/search/SemanticSearch.h"

CSemanticDatabase db;
db.Open();

CSemanticSearch search;
search.Initialize(&db);

SearchOptions options;
options.maxResults = 20;

auto results = search.Search("robot uprising", options);

for (const auto& result : results) {
  // Process result with timestamp, snippet, score
}
```

See [Usage Examples](docs/Phase1-ContentTextIndex.md#usage-examples) for more.

## Configuration

### Settings Path
**Settings > System > Services > Semantic Search**

### Key Settings

| Setting | Default | Description |
|---------|---------|-------------|
| Enable Semantic Search | OFF | Master switch |
| Processing Mode | Idle | When to index: manual/idle/background |
| Index Subtitles | ON | Parse subtitle files |
| Index Metadata | ON | Extract plot summaries |
| Auto-transcribe | OFF | Transcribe videos without subtitles |
| Groq API Key | (empty) | API key for transcription |
| Monthly Budget | $10 | Max transcription cost per month |

## Performance

### Indexing Speed
- **Subtitle parsing:** ~1000 entries/second
- **Chunk processing:** ~500-1000 chunks/second
- **Database insertion:** ~2000 chunks/second (batch)
- **Transcription:** 10-30x real-time (provider-dependent)

### Search Performance
- **Simple queries:** 1-5ms (100K chunks)
- **Complex queries:** 10-50ms (100K chunks)
- **Context queries:** 1-3ms

### Resource Usage
- **Database size:** ~300 bytes per chunk
- **Memory:** ~50-100MB during indexing
- **CPU:** Background priority, minimal impact

See [Performance Characteristics](docs/Phase1-ContentTextIndex.md#performance-characteristics).

## Testing

### Run Unit Tests

```bash
cd build
make test

# Or run specific tests
./kodi-test --gtest_filter=Semantic*
```

### Run Integration Tests

```bash
./kodi-test --gtest_filter=SemanticIntegration*
```

### Test Coverage

```bash
cmake .. -DENABLE_COVERAGE=ON
make
make test
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

Target: **>80% coverage** for new code

## Dependencies

### Required
- **SQLite 3.35+** with FTS5 support
- **C++17** compiler
- **Kodi 22 (Piers)** or later

### Optional
- **FFmpeg** - For audio extraction (transcription)
- **Groq API key** - For cloud transcription
- **sqlite-vec extension** - For vector search (Phase 2)

## Contributing

We welcome contributions! Please see:

1. **[Developer Guide](docs/DeveloperGuide.md)** - Development setup and guidelines
2. **[PR Checklist](docs/PRChecklist.md)** - Requirements before submitting
3. **[Kodi Development Guide](https://kodi.wiki/view/Development)** - General Kodi contribution guidelines

### Areas for Contribution

- **New parsers:** LRC lyrics, ComicInfo.xml, etc.
- **Transcription providers:** OpenAI Whisper, local Whisper.cpp
- **Performance optimizations:** Caching, parallel processing
- **UI components:** Search interface, results display
- **Documentation:** Tutorials, examples, translations

## Roadmap

### Phase 1: Content Text Index ✅
- [x] FTS5 database schema
- [x] Subtitle parsing (SRT/ASS/VTT)
- [x] Metadata extraction
- [x] Audio transcription (Groq)
- [x] Background indexing
- [x] JSON-RPC API

### Phase 2: Vector Embeddings 🚧
- [ ] Sentence embedding generation
- [ ] Vector similarity search
- [ ] Hybrid search (FTS5 + vectors)
- [ ] Model management

### Phase 3: Advanced Search 🚧
- [ ] Query understanding
- [ ] Synonym expansion
- [ ] Cross-lingual search
- [ ] Result ranking improvements

### Phase 4: UI & Voice 🚧
- [ ] Search UI components
- [ ] Voice-activated search
- [ ] Result previews
- [ ] Skin integration

## Known Limitations (Phase 1)

- **English-optimized:** Stemming is English-specific
- **Keyword-only:** No semantic similarity (requires Phase 2)
- **No query understanding:** Literal matching only
- **Limited providers:** Only Groq Whisper currently
- **No incremental updates:** Subtitle changes require full re-index

See [Known Limitations](docs/Phase1-ContentTextIndex.md#known-limitations) for details.

## Troubleshooting

### Common Issues

**Search returns no results:**
- Check if media is indexed: `Semantic.GetIndexState`
- Verify subtitles exist or transcription completed
- Try broader search terms

**Transcription fails:**
- Verify API key is set and valid
- Check monthly budget not exceeded
- Ensure FFmpeg is installed

**Slow indexing:**
- Use Idle mode instead of Background mode
- Check system resources (CPU, disk I/O)
- Large libraries take time - monitor with `Semantic.GetStats`

See [Troubleshooting](docs/UserGuide.md#troubleshooting) for more.

## FAQ

**Q: How much disk space does this use?**
A: ~300 bytes per chunk. Typical library of 500 items: ~90MB

**Q: Does this work offline?**
A: Search works offline. Transcription requires internet.

**Q: Can I use this on Raspberry Pi?**
A: Yes, but use Idle or Manual mode for better performance.

**Q: Is my data private?**
A: Only audio is sent to transcription providers. No video, file names, or metadata.

See [FAQ](docs/UserGuide.md#faq) for more.

## License

This code is part of Kodi and is licensed under **GPL-2.0-or-later**.

See [LICENSES/README.md](../../LICENSES/README.md) for details.

## Support

- **Forum:** [forum.kodi.tv](https://forum.kodi.tv)
- **GitHub Issues:** Report bugs and request features
- **Discord:** #semantic-search channel
- **Wiki:** [kodi.wiki](https://kodi.wiki)

## Acknowledgments

- Kodi development team
- SQLite FTS5 team
- Groq for Whisper API access
- FFmpeg project
- All contributors

---

**Status:** Phase 1 Complete ✅
**Version:** 1.0
**Last Updated:** 2025-11-25
**Kodi Version:** 22 (Piers) and later
