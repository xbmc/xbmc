# PRD: Kodi Semantic Vector Search & Smart Discovery

## Document Information

| Field | Value |
|-------|-------|
| **Feature Name** | Semantic Vector Search & Smart Discovery |
| **Version** | 1.0 |
| **Status** | Draft |
| **Target Release** | Kodi 23 "Q*" |
| **Author** | [Contributor Name] |
| **PR Scope** | Vector embeddings, semantic search, hybrid ranking, GUI |
| **Depends On** | PR #1: Core Semantic Index Infrastructure |
| **Technical Design** | See `tdd-2-semantic-vector.md` for implementation details and code samples |

---

## 1. Executive Summary

This PRD defines the semantic search layer for Kodi—building on the foundation from PR #1 to enable natural language queries that understand meaning, not just keywords. Users can search for concepts, themes, and scene descriptions, finding relevant content even when their query words don't exactly match the indexed text.

### 1.1 The Vision

```
┌─────────────────────────────────────────────────────────────────────────┐
│  🔍 Search your library                                                  │
│  ┌─────────────────────────────────────────────────────────────────────┐│
│  │ the scene where they plan the heist at the restaurant               ││
│  └─────────────────────────────────────────────────────────────────────┘│
│                                                                          │
│  ═══════════════════════════════════════════════════════════════════════│
│                                                                          │
│  📽️ Heat (1995)                                            [▶ Play]     │
│  ┌────────┐  "Told you I'm never going back."                           │
│  │ thumb  │  → 1:23:45 - Restaurant meeting scene                       │
│  └────────┘  Semantic: 94%  |  Keyword: 67%                             │
│                                                                          │
│  📽️ The Italian Job (2003)                                 [▶ Play]     │
│  ┌────────┐  "I trust everyone. It's the devil inside..."               │
│  │ thumb  │  → 0:34:12 - Planning scene                                 │
│  └────────┘  Semantic: 87%  |  Keyword: 23%                             │
│                                                                          │
│  📽️ Reservoir Dogs (1992)                                  [▶ Play]     │
│  ┌────────┐  "Let me tell you what Like a Virgin..."                    │
│  │ thumb  │  → 0:04:15 - Diner scene                                    │
│  └────────┘  Semantic: 82%  |  Keyword: 45%                             │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Key Differentiators from PR #1

| PR #1 (Foundation) | PR #2 (This PR) |
|--------------------|-----------------|
| Keyword matching only | Meaning-based matching |
| Exact word matches | Conceptual similarity |
| FTS5 search | Hybrid FTS5 + Vector search |
| No GUI | Full search dialog |
| Basic API | Rich API with context |

### 1.3 Why Semantic Search Matters

**Keyword search fails when:**
- User says "robbery scene" but subtitle says "heist"
- User says "sad ending" but text doesn't contain those words
- User describes a visual ("the scene with all the money") not dialogue
- User remembers the concept but not exact wording

**Semantic search succeeds because:**
- Embeddings capture meaning, not just words
- "Robbery" and "heist" have similar vector representations
- Emotional concepts cluster together in embedding space
- Natural language descriptions map to content meaning

---

## 2. Problem Statement

### 2.1 Limitations of Keyword Search (PR #1)

While PR #1's FTS5 search is fast and effective for exact matches, it has fundamental limitations:

| Limitation | Example | Impact |
|------------|---------|--------|
| **Vocabulary mismatch** | Search "car chase" → misses "vehicle pursuit" | 40% of relevant results missed |
| **Concept search** | Search "funny scene" → no matches | Can't search for emotions/themes |
| **Paraphrasing** | Search "he dies" → misses "passed away" | Users must guess exact wording |
| **Context understanding** | Search "opening scene" → matches any "opening" | False positives |

### 2.2 User Research Insights

Based on how users describe content they're looking for:

- **60%** use conceptual descriptions ("the scary part", "when they meet")
- **25%** use partial quotes they remember incorrectly
- **15%** search for exact phrases

Keyword search only serves the last 15% well.

### 2.3 Opportunity

By adding vector embeddings:
- Match on semantic similarity, not just keywords
- Understand user intent, not just words
- Rank by conceptual relevance
- Enable discovery of thematically related content

---

## 3. Goals and Non-Goals

### 3.1 Goals (This PR)

| ID | Goal | Success Metric |
|----|------|----------------|
| G1 | Implement vector embedding generation | All indexed chunks have embeddings |
| G2 | Integrate sqlite-vec for similarity search | Sub-100ms vector queries on 100k chunks |
| G3 | Build hybrid search ranking | 30%+ improvement in search relevance |
| G4 | Create search GUI dialog | Intuitive, keyboard/remote navigable |
| G5 | Enable timestamp navigation from results | Click-to-play at exact moment |
| G6 | Show search context and previews | Users understand why results matched |
| G7 | Support voice search input | Accessibility and remote control UX |

### 3.2 Non-Goals (This PR)

| ID | Non-Goal | Rationale |
|----|----------|-----------|
| NG1 | Recommendation engine | Future feature, different problem space |
| NG2 | Cross-library search (multiple Kodi instances) | Requires sync infrastructure |
| NG3 | Image/video content analysis | Would require vision models |
| NG4 | Custom embedding fine-tuning | Complexity; general models work well |
| NG5 | Cloud-based embedding APIs | Privacy; local-only embeddings |

### 3.3 Future Considerations

- "More like this" recommendations based on embedding similarity
- Automatic playlist generation from semantic queries
- Scene similarity detection for skip-intro/recap
- Integration with external apps via extended JSON-RPC

---

## 4. User Stories

### 4.1 Primary User Stories

#### US1: Semantic Search
```
AS A user looking for a specific scene
I WANT to describe what happens in natural language
SO THAT I can find it even without remembering exact dialogue

ACCEPTANCE CRITERIA:
- Search understands conceptual queries ("the sad ending")
- Results ranked by semantic relevance, not just keyword matches
- Matching works across synonyms and paraphrases
- Response time under 500ms for typical queries
```

#### US2: Search Results with Context
```
AS A user reviewing search results
I WANT to see context around each match
SO THAT I can verify it's the right scene before playing

ACCEPTANCE CRITERIA:
- Results show 2-3 sentences around the match
- Matched terms highlighted
- Timestamp displayed in readable format
- Thumbnail preview if available
```

#### US3: Direct Playback from Search
```
AS A user who found the right scene
I WANT to play directly from the search result
SO THAT I can immediately watch the relevant moment

ACCEPTANCE CRITERIA:
- Single click/select plays media at timestamp
- Option to play from beginning
- Option to see in context (±30 seconds)
- Maintains current search for easy return
```

#### US4: Search from Remote Control
```
AS A user with a TV remote (no keyboard)
I WANT to search using voice or on-screen keyboard
SO THAT I can search from the couch

ACCEPTANCE CRITERIA:
- Voice input button triggers device speech-to-text
- On-screen keyboard works with directional remote
- Recent searches accessible
- Search suggestions based on library content
```

### 4.2 Secondary User Stories

#### US5: Search Filters
```
AS A user with a large library
I WANT to filter search results by media type, genre, year
SO THAT I can narrow down results efficiently

ACCEPTANCE CRITERIA:
- Filter by: Movies/TV/Music Videos
- Filter by genre tags
- Filter by year range
- Filters persist during session
```

#### US6: Search History
```
AS A frequent searcher
I WANT quick access to my recent searches
SO THAT I can repeat common queries easily

ACCEPTANCE CRITERIA:
- Last 20 searches stored locally
- Quick select from dropdown
- Clear history option
- No search history sent externally
```

---

## 5. Technical Architecture

### 5.1 System Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              SEARCH FLOW                                 │
└─────────────────────────────────────────────────────────────────────────┘

    User Query: "the scene where they plan the heist"
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         QUERY PROCESSOR                                  │
│  ┌─────────────────────────────────────────────────────────────────────┐│
│  │ 1. Normalize: lowercase, remove punctuation                         ││
│  │ 2. Generate query embedding via ONNX model                          ││
│  │ 3. Build FTS5 query from keywords                                   ││
│  └─────────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────┘
                                │
                    ┌───────────┴───────────┐
                    ▼                       ▼
┌───────────────────────────┐   ┌───────────────────────────┐
│     FTS5 KEYWORD SEARCH   │   │   VECTOR SIMILARITY SEARCH │
│  ┌───────────────────────┐│   │  ┌───────────────────────┐ │
│  │ SELECT ... FROM       ││   │  │ SELECT ... FROM       │ │
│  │   semantic_fts        ││   │  │   semantic_vectors    │ │
│  │ WHERE MATCH 'plan     ││   │  │ WHERE embedding MATCH │ │
│  │   heist scene'        ││   │  │   query_vec           │ │
│  │ ORDER BY bm25()       ││   │  │ ORDER BY distance     │ │
│  │ LIMIT 50              ││   │  │ LIMIT 50              │ │
│  └───────────────────────┘│   │  └───────────────────────┘ │
│                           │   │                            │
│  Returns: chunk_ids +     │   │  Returns: chunk_ids +      │
│           bm25 scores     │   │           cosine distances │
└───────────────────────────┘   └───────────────────────────┘
                    │                       │
                    └───────────┬───────────┘
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                       HYBRID RANKER (RRF)                                │
│  ┌─────────────────────────────────────────────────────────────────────┐│
│  │ Reciprocal Rank Fusion:                                             ││
│  │   score = Σ (weight / (k + rank))                                   ││
│  │                                                                      ││
│  │ FTS weight: 0.4  |  Vector weight: 0.6  |  k = 60                   ││
│  │                                                                      ││
│  │ Deduplicate by chunk_id                                             ││
│  │ Sort by combined score                                              ││
│  └─────────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                       RESULT ENRICHMENT                                  │
│  ┌─────────────────────────────────────────────────────────────────────┐│
│  │ For each result:                                                    ││
│  │   - Fetch chunk context (±2 chunks)                                 ││
│  │   - Join with video database for title, thumbnail                   ││
│  │   - Format timestamp (ms → "1:23:45")                               ││
│  │   - Generate snippet with highlighting                              ││
│  └─────────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
                        Final Results to GUI
```

### 5.2 Component Overview

> **Implementation Details**: See `tdd-2-semantic-vector.md` for complete architecture diagrams, class relationships, and directory structure.

**Embedding Engine**:
- Model: all-MiniLM-L6-v2 via ONNX Runtime
- 384-dimensional embeddings, ~5ms per embedding (CPU)
- ~90MB model file, Apache 2.0 license
- Batch processing for efficiency (32 chunks at a time)

**Vector Search (sqlite-vec)**:
- SQLite extension for similarity search
- Single C file bundled with Kodi (~50KB)
- Cosine distance metric for semantic matching
- Top-k retrieval with optional filtering

**Key Classes**:
- `CHybridSearchEngine` - Main search interface combining FTS5 + vector
- `CEmbeddingEngine` - ONNX Runtime wrapper for embedding generation
- `CVectorSearcher` - sqlite-vec query interface
- `CResultRanker` - Reciprocal Rank Fusion algorithm
- `CGUIDialogSemanticSearch` - Search dialog controller

---

## 6. Database Schema (Additions)

> **Full Schema**: See `tdd-2-semantic-vector.md` for complete SQL DDL and query examples.

### 6.1 New Tables

| Table | Purpose |
|-------|---------|
| `semantic_vectors` | sqlite-vec virtual table storing 384-dim embeddings per chunk |
| `semantic_embedding_models` | Tracks model versions, paths, and active state |
| `semantic_search_history` | Records queries for suggestions and analytics |
| `semantic_suggestions` | Precomputed autocomplete options |

### 6.2 Schema Extensions

Extends `semantic_index_state` from PR #1 with:
- `embedding_status` - pending/processing/complete/failed
- `embedding_progress` - 0.0 to 1.0 progress indicator
- `embedding_error` - error message if failed
- `embeddings_count` - number of embeddings generated

### 6.3 Key Design Decisions

- **Cosine distance** metric for vector similarity
- **Model versioning** support for future upgrades
- **Search history** enables personalized suggestions (stored locally only)

---

## 7. API Design

> **Full Implementation**: See `tdd-2-semantic-vector.md` for complete C++ header files and JSON-RPC specifications.

### 7.1 C++ API Overview

**Main Classes**:
| Class | Responsibility |
|-------|----------------|
| `CHybridSearchEngine` | Main search interface combining FTS5 + vector search |
| `HybridSearchOptions` | Configuration struct for search mode, weights, context |
| `EnrichedSearchResult` | Extended result with scores, context, and display info |

**Search Modes**:
- `Hybrid` - Combined FTS + vector (default, best relevance)
- `KeywordOnly` - FTS only (faster, exact matches)
- `SemanticOnly` - Vector only (conceptual matches)

**Key Methods**:
- `Search(query, options)` - Perform hybrid search
- `FindSimilar(chunkId)` - Find semantically similar content
- `GetSuggestions(prefix)` - Autocomplete based on history
- `EmbedQuery(text)` - Generate embedding for external use

### 7.2 JSON-RPC API

| Method | Purpose |
|--------|---------|
| `Semantic.HybridSearch` | Enhanced search with mode/weight options |
| `Semantic.FindSimilar` | Get similar chunks to a result |
| `Semantic.GetSuggestions` | Autocomplete suggestions |
| `Semantic.GetEmbeddingStatus` | Check embedding generation progress |

---

## 8. GUI Design

### 8.1 Search Dialog Layout

```
┌─────────────────────────────────────────────────────────────────────────┐
│  ╔═══════════════════════════════════════════════════════════════════╗  │
│  ║                    SEMANTIC SEARCH                                 ║  │
│  ╚═══════════════════════════════════════════════════════════════════╝  │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐│
│  │ 🔍 Search your library...                                      🎤  ││
│  └─────────────────────────────────────────────────────────────────────┘│
│                                                                          │
│  ┌─ Recent ───────────────────────────────────────────────────────────┐ │
│  │ • the scene where they kiss                                        │ │
│  │ • funny moments                                                    │ │
│  │ • car chase                                                        │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  ┌─ Filters ──────────────────────────────────────────────────────────┐ │
│  │ Type: [All ▾]    Genre: [All ▾]    Year: [Any ▾]                   │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  ═══════════════════════════════════════════════════════════════════════│
│                         SEARCH RESULTS                                   │
│  ═══════════════════════════════════════════════════════════════════════│
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │ ┌──────┐                                                           │ │
│  │ │      │  Heat (1995)                                    ▶ Play   │ │
│  │ │ thumb│  "I told you. I'm never going back."                     │ │
│  │ │      │  → 1:23:43  •  Semantic: 94%  Keyword: 67%               │ │
│  │ └──────┘                                                           │ │
│  ├────────────────────────────────────────────────────────────────────┤ │
│  │ ┌──────┐                                                           │ │
│  │ │      │  The Italian Job (2003)                         ▶ Play   │ │
│  │ │ thumb│  "I trust everyone. It's the devil inside..."            │ │
│  │ │      │  → 0:34:12  •  Semantic: 87%  Keyword: 23%               │ │
│  │ └──────┘                                                           │ │
│  ├────────────────────────────────────────────────────────────────────┤ │
│  │ ┌──────┐                                                           │ │
│  │ │      │  Reservoir Dogs (1992)                          ▶ Play   │ │
│  │ │ thumb│  "Let me tell you what Like a Virgin is about"           │ │
│  │ │      │  → 0:04:15  •  Semantic: 82%  Keyword: 45%               │ │
│  │ └──────┘                                                           │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  [More Results...]                              Showing 3 of 15 results  │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 8.2 Implementation Details

> **Full Implementation**: See `tdd-2-semantic-vector.md` for complete XML skin definition and C++ dialog controller.

**Dialog Controls** (Window ID 13500):

| Control ID | Type | Purpose |
|------------|------|---------|
| 9000 | edit | Search input box |
| 9001 | panel | Results list |
| 9005 | button | Voice search (platform-dependent) |
| 9011 | list | Recent searches |
| 9020-9022 | spincontrolex | Filter controls (type, genre, year) |
| 9099 | button | Close dialog |

**Result Item Properties**:
- `chunk_id`, `media_id` - Database identifiers
- `timestamp` - Formatted position (e.g., "1:23:45")
- `fts_score`, `vec_score` - Score breakdown display
- `thumb` - Video thumbnail path

### 8.3 Dialog Controller

**Key Methods in `CGUIDialogSemanticSearch`**:
- `OnSearch()` - Execute hybrid search and populate results
- `OnResultSelected()` - Handle result selection
- `OnPlaySelected()` - Start playback at timestamp
- `OnFindSimilar()` - Show similar scenes
- `OnVoiceSearch()` - Platform-specific voice input

**Context Menu Options**:
1. Play from timestamp
2. Play from beginning
3. Show in library
4. Find similar scenes
5. See more context

---

## 9. Testing Strategy

> **Test Code**: See `tdd-2-semantic-vector.md` for complete test implementations and benchmarks.

### 9.1 Unit Tests

| Test Category | Coverage |
|--------------|----------|
| Embedding Engine | Similar text similarity, batch consistency, edge cases |
| Hybrid Search | Semantic matching, exact matching, mode combinations |
| Result Ranker | RRF scoring, deduplication, edge cases |
| GUI Dialog | Search flow, result selection, playback integration |

### 9.2 Performance Benchmarks

| Benchmark | Target |
|-----------|--------|
| Single embedding | <10ms |
| Batch embedding (32) | <200ms |
| Hybrid search (100k chunks) | <200ms |

---

## 10. Success Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Search relevance (MRR) | >0.7 | Mean Reciprocal Rank on test queries |
| Keyword-only queries | Equivalent to PR #1 | A/B comparison |
| Conceptual queries | 50%+ improvement | User study with semantic queries |
| Search latency (p50) | <200ms | Instrumentation |
| Search latency (p99) | <500ms | Instrumentation |
| Embedding throughput | >100 chunks/sec | Batch processing benchmark |
| Memory usage | <200MB additional | Runtime profiling |
| Model load time | <3s | Cold start timing |
| GUI responsiveness | 60fps during scroll | Frame timing |

---

## 11. Rollout Plan

### 11.1 Implementation Phases

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| **Phase 1** | 2 weeks | Embedding engine, tokenizer, ONNX integration |
| **Phase 2** | 1 week | sqlite-vec integration, vector storage |
| **Phase 3** | 2 weeks | Hybrid search engine, RRF ranking |
| **Phase 4** | 2 weeks | GUI dialog, results display, playback |
| **Phase 5** | 1 week | Testing, optimization, documentation |

### 11.2 Dependencies

- PR #1 must be merged first
- ONNX Runtime library (~15MB)
- sqlite-vec source (bundled, ~50KB)
- all-MiniLM-L6-v2 model (~90MB, downloaded on first use)

### 11.3 Migration from PR #1

> **Migration Code**: See `tdd-2-semantic-vector.md` for complete implementation.

Migration runs automatically after PR #2 deployment:
1. Load embedding model
2. Process existing chunks in batches of 32
3. Generate and store embeddings for each chunk
4. Progress notification via `NotifyMigrationProgress()`

---

## 12. Security & Privacy

### 12.1 Data Privacy

- **All embeddings generated locally** - no text sent to cloud
- Model runs entirely on-device
- No telemetry or usage data collected
- Search history stored locally only

### 12.2 Model Security

- Model file integrity verified via SHA256 hash
- Download from trusted CDN only (Hugging Face official)
- No model fine-tuning or user data incorporation

### 12.3 Resource Limits

- Embedding generation rate-limited to prevent CPU exhaustion
- Memory-mapped model loading to limit RAM usage
- Configurable batch sizes for constrained devices

---

## 13. Open Questions

| Question | Status | Owner |
|----------|--------|-------|
| Should we support multiple embedding models? | Deferred | - |
| How to handle model updates/versioning? | Re-embed on update | - |
| GPU acceleration for embeddings? | Future PR | - |
| Voice search implementation per-platform? | Research needed | - |
| Thumbnail generation for results? | Use existing video thumbs | - |

---

## 14. Appendix

### 14.1 Model Comparison

| Model | Size | Dim | Speed | Quality |
|-------|------|-----|-------|---------|
| all-MiniLM-L6-v2 | 90MB | 384 | 5ms | Good |
| all-MiniLM-L12-v2 | 120MB | 384 | 10ms | Better |
| all-mpnet-base-v2 | 420MB | 768 | 25ms | Best |
| bge-small-en-v1.5 | 130MB | 384 | 6ms | Good |

**Selection: all-MiniLM-L6-v2**
- Best balance of size, speed, and quality
- Widely used and well-tested
- Apache 2.0 license

### 14.2 sqlite-vec vs Alternatives

| Solution | Pros | Cons |
|----------|------|------|
| **sqlite-vec** | Single file, easy embed | Newer, less tested |
| FAISS | Very fast, mature | Large dependency, complex |
| Annoy | Simple, fast | No updates, write-once |
| hnswlib | Fast, small | C++ only, no persistence |

**Selection: sqlite-vec**
- Integrates naturally with existing SQLite usage
- Single C file to vendor
- Active development
- Good enough performance for our scale

### 14.3 References

- [sqlite-vec GitHub](https://github.com/asg017/sqlite-vec)
- [Sentence Transformers](https://www.sbert.net/)
- [ONNX Runtime](https://onnxruntime.ai/)
- [Reciprocal Rank Fusion](https://plg.uwaterloo.ca/~gvcormac/cormacksigir09-rrf.pdf)
