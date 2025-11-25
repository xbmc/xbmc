# Performance Tuning Guide

## Overview

This guide provides comprehensive information on optimizing semantic search performance for your specific hardware and usage patterns.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Memory Configuration](#memory-configuration)
3. [Model Loading Strategies](#model-loading-strategies)
4. [Cache Configuration](#cache-configuration)
5. [Search Optimization](#search-optimization)
6. [Indexing Performance](#indexing-performance)
7. [Database Tuning](#database-tuning)
8. [Monitoring and Metrics](#monitoring-and-metrics)
9. [Platform-Specific Tips](#platform-specific-tips)
10. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Recommended Configurations by Hardware

#### Low-End (2GB RAM, Slow CPU)

**advancedsettings.xml**:
```xml
<semantic>
  <embeddingmodel>
    <lazyload>true</lazyload>
    <idletimeout>120</idletimeout>  <!-- Unload quickly -->
  </embeddingmodel>

  <cache>
    <enabled>true</enabled>
    <maxsizemb>20</maxsizemb>  <!-- Small cache -->
    <resultttl>180</resultttl>
    <embeddingttl>600</embeddingttl>
  </cache>

  <hybridsearch>
    <keywordweight>0.6</keywordweight>  <!-- Prefer fast FTS5 -->
    <vectorweight>0.4</vectorweight>
    <keywordtopk>50</keywordtopk>  <!-- Fewer candidates -->
    <vectortopk>50</vectortopk>
  </hybridsearch>
</semantic>
```

**Recommendations**:
- Use keyword-only mode for simple queries
- Limit max results to 10-15
- Close other applications during search
- Consider disabling voice search

---

#### Mid-Range (4-8GB RAM, Modern CPU)

**advancedsettings.xml**:
```xml
<semantic>
  <embeddingmodel>
    <lazyload>true</lazyload>
    <idletimeout>300</idletimeout>  <!-- Default -->
  </embeddingmodel>

  <cache>
    <enabled>true</enabled>
    <maxsizemb>50</maxsizemb>  <!-- Balanced -->
    <resultttl>300</resultttl>
    <embeddingttl>3600</embeddingttl>
  </cache>

  <hybridsearch>
    <keywordweight>0.4</keywordweight>  <!-- Balanced -->
    <vectorweight>0.6</vectorweight>
    <keywordtopk>100</keywordtopk>
    <vectortopk>100</vectortopk>
  </hybridsearch>
</semantic>
```

**Recommendations**:
- Default hybrid mode works well
- Enable performance monitoring
- Standard cache settings

---

#### High-End (16GB+ RAM, Fast CPU/GPU)

**advancedsettings.xml**:
```xml
<semantic>
  <embeddingmodel>
    <lazyload>false</lazyload>  <!-- Keep loaded -->
    <idletimeout>0</idletimeout>
  </embeddingmodel>

  <cache>
    <enabled>true</enabled>
    <maxsizemb>200</maxsizemb>  <!-- Large cache -->
    <resultttl>600</resultttl>
    <embeddingttl>7200</embeddingttl>
  </cache>

  <hybridsearch>
    <keywordweight>0.3</keywordweight>  <!-- Prefer semantic -->
    <vectorweight>0.7</vectorweight>
    <keywordtopk>200</keywordtopk>  <!-- More candidates -->
    <vectortopk>200</vectortopk>
  </hybridsearch>

  <performance>
    <monitoring>true</monitoring>
    <detailedtimings>true</detailedtimings>
  </performance>
</semantic>
```

**Recommendations**:
- Always use hybrid/semantic mode
- Enable detailed performance metrics
- Consider GPU acceleration (future)

---

#### Always-On Server / HTPC

**advancedsettings.xml**:
```xml
<semantic>
  <embeddingmodel>
    <lazyload>false</lazyload>  <!-- Always loaded -->
    <idletimeout>0</idletimeout>
  </embeddingmodel>

  <cache>
    <enabled>true</enabled>
    <maxsizemb>100</maxsizemb>
    <resultttl>3600</resultttl>  <!-- Long TTL -->
    <embeddingttl>14400</embeddingttl>  <!-- 4 hours -->
  </cache>

  <indexing>
    <autoindex>true</autoindex>
    <backgroundpriority>low</backgroundpriority>
  </indexing>
</semantic>
```

**Recommendations**:
- Pre-load model at startup
- Large cache for better responsiveness
- Enable auto-indexing for new media

---

## Memory Configuration

### Memory Components

| Component | Default | Range | Description |
|-----------|---------|-------|-------------|
| ONNX Model | 100-150 MB | - | Fixed when loaded |
| Vector Index | ~1.5 KB/vector | - | Grows with content |
| FTS5 Index | ~0.5 KB/chunk | - | Grows with content |
| Query Cache | 50 MB | 10-500 MB | Configurable |
| Runtime Overhead | 50-100 MB | - | C++ objects, buffers |

### Total Memory Estimation

```
Total = Model + (VectorCount × 1.5KB) + (ChunkCount × 0.5KB) + Cache + 100MB

Example (10K chunks):
= 125 MB + (10K × 1.5 KB) + (10K × 0.5 KB) + 50 MB + 100 MB
= 125 + 15 + 5 + 50 + 100
= 295 MB
```

### Memory Limits

**Set in advancedsettings.xml**:
```xml
<semantic>
  <memory>
    <modellimit>200</modellimit>  <!-- MB -->
    <cachelimit>100</cachelimit>  <!-- MB -->
    <totalwarnlimit>500</totalwarnlimit>  <!-- MB, log warning -->
    <totalhardlimit>1000</totalhardlimit>  <!-- MB, unload model -->
  </memory>
</semantic>
```

**C++ API**:
```cpp
#include "semantic/perf/MemoryManager.h"

CMemoryManager& mem = CMemoryManager::GetInstance();
mem.SetModelMemoryLimit(200 * 1024 * 1024);  // 200 MB
mem.SetCacheMemoryLimit(100 * 1024 * 1024);  // 100 MB

// Check current usage
auto usage = mem.GetMemoryUsage();
if (usage.totalMemoryMB > 500) {
    mem.UnloadModel();  // Free ~150 MB
}
```

### Memory Monitoring

**Enable monitoring**:
```xml
<semantic>
  <performance>
    <monitoring>true</monitoring>
    <memorysampling>true</memorysampling>
    <samplinginterval>60</samplinginterval>  <!-- seconds -->
  </performance>
</semantic>
```

**View metrics**:
```cpp
auto& perf = CPerformanceMonitor::GetInstance();
auto metrics = perf.GetMetrics();

std::cout << "Current: " << metrics.currentMemoryUsage / 1024 / 1024 << " MB\n";
std::cout << "Peak: " << metrics.peakMemoryUsage / 1024 / 1024 << " MB\n";
std::cout << "Model: " << metrics.modelMemoryUsage / 1024 / 1024 << " MB\n";
std::cout << "Cache: " << metrics.cacheMemoryUsage / 1024 / 1024 << " MB\n";
```

**JSON-RPC**:
```bash
curl -X POST http://localhost:8080/jsonrpc \
  -d '{"jsonrpc":"2.0","method":"Semantic.GetStats","id":1}' | \
  jq '.result.memory_usage_mb'
```

---

## Model Loading Strategies

### Lazy Loading (Default)

**When to use**:
- Limited RAM (<4GB)
- Infrequent searches
- Shared system (other apps need memory)

**Configuration**:
```xml
<embeddingmodel>
  <lazyload>true</lazyload>
  <idletimeout>300</idletimeout>  <!-- 5 minutes -->
</embeddingmodel>
```

**Behavior**:
- Model loads on first search (2-5 second delay)
- Subsequent searches are fast
- Automatically unloads after idle timeout
- Reloads when needed

**Performance**:
- **First query**: 2000-5000ms (load) + query time
- **Cached queries**: 10-50ms
- **After unload**: Back to first-query delay

---

### Eager Loading

**When to use**:
- Dedicated HTPC (always running Kodi)
- Sufficient RAM (>8GB)
- Frequent searches
- Consistent latency required

**Configuration**:
```xml
<embeddingmodel>
  <lazyload>false</lazyload>
  <idletimeout>0</idletimeout>  <!-- Never unload -->
</embeddingmodel>
```

**Behavior**:
- Model loads at Kodi startup (one-time delay)
- Always ready for queries
- Never unloads

**Performance**:
- **All queries**: 20-80ms (no load delay)
- **Startup time**: +3-7 seconds

---

### Adaptive Loading (Experimental)

**When to use**:
- Variable usage patterns
- Want best of both worlds

**Configuration**:
```xml
<embeddingmodel>
  <lazyload>true</lazyload>
  <idletimeout>600</idletimeout>  <!-- 10 min (longer) -->
  <adaptiveunload>true</adaptiveunload>
  <memorythreshold>80</memorythreshold>  <!-- Unload if >80% system RAM -->
</embeddingmodel>
```

**Behavior**:
- Loads on first use
- Longer idle timeout
- Monitors system memory pressure
- Unloads if other apps need memory

---

## Cache Configuration

### Cache Types

**1. Query Result Cache**

Stores search results for repeated queries.

**Settings**:
```xml
<cache>
  <enabled>true</enabled>
  <resultttl>300</resultttl>  <!-- 5 minutes -->
  <resultcapacity>100</resultcapacity>  <!-- entries -->
</cache>
```

**Hit Rate**: 40-70% for typical usage

**Memory**: ~10 KB per cached result

---

**2. Embedding Cache**

Stores generated embeddings for query texts.

**Settings**:
```xml
<cache>
  <embeddingttl>3600</embeddingttl>  <!-- 1 hour -->
  <embeddingcapacity>1000</embeddingcapacity>
</cache>
```

**Hit Rate**: 50-80% (many query variations)

**Memory**: ~1.5 KB per cached embedding

---

**3. Database Query Cache** (SQLite-level)

SQLite's built-in query plan cache.

**Settings**:
```xml
<database>
  <pagesize>4096</pagesize>  <!-- bytes -->
  <cachesize>-2000</cachesize>  <!-- -KB or page count -->
</database>
```

`cachesize = -2000` means 2 MB cache

---

### Cache Tuning by Usage

**Infrequent Searches** (few queries, varied):
```xml
<cache>
  <maxsizemb>20</maxsizemb>
  <resultttl>60</resultttl>  <!-- Short TTL -->
  <embeddingttl>300</embeddingttl>
</cache>
```

**Frequent Repeated Searches**:
```xml
<cache>
  <maxsizemb>100</maxsizemb>
  <resultttl>600</resultttl>  <!-- Long TTL -->
  <embeddingttl>7200</embeddingttl>
</cache>
```

**Exploratory Browsing** (many similar queries):
```xml
<cache>
  <maxsizemb>75</maxsizemb>
  <resultttl>300</resultttl>
  <embeddingttl>1800</embeddingttl>  <!-- High embedding TTL -->
</cache>
```

---

### Cache Invalidation

**Manual clearing** (after reindexing):
```cpp
searchEngine.ClearCache();
```

**Automatic invalidation** (on database changes):
```xml
<cache>
  <autoinvalidate>true</autoinvalidate>  <!-- Clear on DB write -->
</cache>
```

**Selective clearing**:
```cpp
cache.ClearSearchCache();    // Keep embeddings
cache.ClearEmbeddingCache(); // Keep results
```

---

## Search Optimization

### Query Complexity

**Simple queries** → Keyword mode:
```cpp
HybridSearchOptions opts;
opts.mode = SearchMode::KeywordOnly;  // Fastest
auto results = engine.Search("batman", opts);
```

**Performance**: 10-20ms (no embedding overhead)

---

**Complex queries** → Hybrid mode:
```cpp
opts.mode = SearchMode::Hybrid;  // Best relevance
auto results = engine.Search("detective solving complicated mystery", opts);
```

**Performance**: 40-80ms (embedding + vector + fusion)

---

**Exploratory queries** → Semantic mode:
```cpp
opts.mode = SearchMode::SemanticOnly;  // Conceptual
auto results = engine.Search("emotional goodbye scene", opts);
```

**Performance**: 30-60ms (embedding + vector, no FTS5)

---

### TopK Tuning

**topK** controls candidate count before final ranking.

**Small topK** (faster, may miss results):
```cpp
opts.keywordTopK = 50;
opts.vectorTopK = 50;
opts.maxResults = 10;
```

**Performance**: 25-40ms

---

**Large topK** (slower, better recall):
```cpp
opts.keywordTopK = 200;
opts.vectorTopK = 200;
opts.maxResults = 20;
```

**Performance**: 60-100ms

---

**Balanced (default)**:
```cpp
opts.keywordTopK = 100;
opts.vectorTopK = 100;
opts.maxResults = 20;
```

**Performance**: 40-70ms

---

### Weight Tuning

**Favor speed** (more keyword weight):
```cpp
opts.keywordWeight = 0.6f;
opts.vectorWeight = 0.4f;
```

- Faster (FTS5 is quick)
- Better for exact matches
- Lower semantic quality

---

**Favor quality** (more vector weight):
```cpp
opts.keywordWeight = 0.3f;
opts.vectorWeight = 0.7f;
```

- Slower (vector search overhead)
- Better conceptual matching
- Higher relevance

---

**Balanced** (default):
```cpp
opts.keywordWeight = 0.4f;
opts.vectorWeight = 0.6f;
```

- Good compromise
- Recommended for most use cases

---

### Batch Operations

**Inefficient** (one at a time):
```cpp
for (const auto& text : texts) {
    auto emb = embedder.Embed(text);
    vectorSearcher.InsertVector(chunkId++, emb);
}
```

**Performance**: ~30ms per item

---

**Efficient** (batching):
```cpp
// Batch embedding
auto embeddings = embedder.EmbedBatch(texts);

// Batch insertion
std::vector<std::pair<int64_t, Embedding>> batch;
for (size_t i = 0; i < texts.size(); ++i) {
    batch.push_back({chunkIds[i], embeddings[i]});
}
vectorSearcher.InsertVectorBatch(batch);
```

**Performance**: ~10ms per item (3x faster)

---

## Indexing Performance

### Parallel Indexing

**Sequential indexing**:
```cpp
for (const auto& movie : movies) {
    indexService.IndexMedia(movie.id, "movie");
}
```

**Time**: 10K movies × 30s = 83 hours

---

**Parallel indexing**:
```cpp
ThreadPool pool(4);  // 4 worker threads

for (const auto& movie : movies) {
    pool.Enqueue([&]() {
        indexService.IndexMedia(movie.id, "movie");
    });
}
pool.Wait();
```

**Time**: 10K movies × 30s / 4 = 21 hours

**Configuration**:
```xml
<indexing>
  <workerThreads>4</workerThreads>
  <maxParallelJobs>4</maxParallelJobs>
</indexing>
```

---

### Transcription Optimization

**Slow** (sequential with default provider):
```xml
<transcription>
  <provider>local</provider>  <!-- 0.2x realtime -->
</transcription>
```

**Time**: 2-hour movie → 10 hours transcription

---

**Fast** (cloud provider):
```xml
<transcription>
  <provider>groq</provider>  <!-- 10x realtime -->
  <maxparalleljobs>3</maxparalleljobs>
</transcription>
```

**Time**: 2-hour movie → 12 minutes transcription

**Cost**: ~$0.01-0.02 per movie

---

### Incremental Indexing

**Full reindex** (slow):
```cpp
indexService.ReindexAll();  // Hours for large library
```

---

**Incremental** (fast):
```cpp
// Only index new/modified media
auto newMovies = videoDb.GetMoviesModifiedSince(lastIndexTime);
for (const auto& movie : newMovies) {
    indexService.QueueMediaIndex(movie.id, "movie");
}
```

**Configuration**:
```xml
<indexing>
  <autoindex>true</autoindex>
  <scaninterval>3600</scaninterval>  <!-- Check hourly -->
</indexing>
```

---

### Embedding Model Optimization

**Future: Quantized Models**

INT8 quantization reduces:
- Model size: 23 MB → 8 MB
- Memory usage: 150 MB → 60 MB
- Inference time: 25ms → 15ms
- Accuracy: ~1% degradation

```xml
<embeddingmodel>
  <modeltype>quantized</modeltype>  <!-- Not yet available -->
  <quantization>int8</quantization>
</embeddingmodel>
```

---

## Database Tuning

### SQLite Configuration

**Page Size**:
```sql
PRAGMA page_size = 4096;  -- Default
-- Larger pages (8192/16384) may help with large BLOBs
```

**Cache Size**:
```sql
PRAGMA cache_size = -2000;  -- 2 MB
-- Increase for better performance: -10000 = 10 MB
```

**Journal Mode**:
```sql
PRAGMA journal_mode = WAL;  -- Write-Ahead Log (recommended)
-- Better concurrency, faster writes
```

**Synchronous**:
```sql
PRAGMA synchronous = NORMAL;  -- Default
-- FULL = safest but slower
-- OFF = fastest but risky (corruption on crash)
```

**advancedsettings.xml**:
```xml
<database>
  <pagesize>4096</pagesize>
  <cachesize>-10000</cachesize>  <!-- 10 MB -->
  <journalmode>WAL</journalmode>
  <synchronous>NORMAL</synchronous>
</database>
```

---

### Index Optimization

**FTS5 Index**:
```sql
-- Rebuild FTS5 index (after bulk inserts)
INSERT INTO semantic_chunks_fts(semantic_chunks_fts) VALUES('rebuild');

-- Optimize index (reduce fragmentation)
INSERT INTO semantic_chunks_fts(semantic_chunks_fts) VALUES('optimize');
```

**Vector Index**:
```sql
-- Rebuild vector index
-- (handled automatically by sqlite-vec)
```

**C++ API**:
```cpp
// Rebuild FTS5 index
database.RebuildFTSIndex();

// Rebuild vector index
vectorSearcher.RebuildIndex();
```

**When to rebuild**:
- After bulk inserts/deletes
- Index fragmentation (slower queries)
- Database corruption

---

### Query Planning

**Analyze statistics** (helps query optimizer):
```sql
ANALYZE;
```

**Schedule**:
```xml
<database>
  <autoanalyze>true</autoanalyze>
  <analyzeinterval>86400</analyzeinterval>  <!-- Daily -->
</database>
```

---

## Monitoring and Metrics

### Performance Monitoring

**Enable monitoring**:
```cpp
CPerformanceMonitor& perf = CPerformanceMonitor::GetInstance();
perf.Initialize(/*enableLogging=*/true,
                /*logPath=*/"semantic_perf.log");
```

**Configuration**:
```xml
<performance>
  <monitoring>true</monitoring>
  <logging>true</logging>
  <logpath>special://temp/semantic_perf.log</logpath>
  <detailedtimings>false</detailedtimings>  <!-- Overhead ~5% -->
</performance>
```

---

### Metrics Collection

**Automatic timing**:
```cpp
{
    PERF_TIMER("embedding");
    auto emb = embedder.Embed(query);
}  // Automatically recorded
```

**Manual recording**:
```cpp
auto start = std::chrono::steady_clock::now();
// ... operation ...
auto end = std::chrono::steady_clock::now();

double ms = std::chrono::duration<double, std::milli>(end - start).count();
perf.RecordEmbedding(ms, 1);
```

---

### Metrics Analysis

**Get current metrics**:
```cpp
auto metrics = perf.GetMetrics();

std::cout << "Avg embedding: " << metrics.avgEmbeddingTime << "ms\n";
std::cout << "Avg search: " << metrics.avgSearchTime << "ms\n";
std::cout << "Cache hit rate: " << metrics.cacheHitRate * 100 << "%\n";
std::cout << "Total searches: " << metrics.totalSearches << "\n";
```

**JSON export**:
```cpp
std::string json = perf.GetMetricsJSON();
// Export to file or send to analytics service
```

**JSON-RPC**:
```bash
curl -X POST http://localhost:8080/jsonrpc \
  -d '{"jsonrpc":"2.0","method":"Semantic.GetPerformanceMetrics","id":1}'
```

---

### Bottleneck Identification

**Slow embeddings**:
- Check CPU usage (ONNX inference is CPU-bound)
- Consider lazy loading with longer timeout
- Future: GPU acceleration

**Slow FTS5 search**:
- Rebuild FTS5 index
- Reduce topK
- Increase database cache size

**Slow vector search**:
- Reduce topK
- Check index size (>100K vectors?)
- Consider filtering candidates first

**High memory usage**:
- Reduce cache size
- Enable lazy loading
- Shorten idle timeout
- Check for memory leaks (monitor peak usage)

**Low cache hit rate**:
- Increase cache size
- Increase TTL values
- Check query variety (too diverse?)

---

## Platform-Specific Tips

### Windows

**Optimize ONNX Runtime**:
```xml
<embeddingmodel>
  <onnxthreads>4</onnxthreads>  <!-- CPU cores for inference -->
  <onnxoptimizationlevel>2</onnxoptimizationlevel>  <!-- 0-3 -->
</embeddingmodel>
```

**Power settings**:
- Use "High Performance" power plan for best speed
- "Balanced" acceptable for HTPC

---

### Linux

**CPU Affinity** (dedicate cores):
```bash
taskset -c 0-3 kodi  # Use first 4 cores
```

**Nice Priority** (lower priority for indexing):
```bash
nice -n 10 kodi  # Run with lower priority
```

**I/O Scheduler**:
```bash
# Check current scheduler
cat /sys/block/sda/queue/scheduler

# Set to deadline (better for databases)
echo deadline > /sys/block/sda/queue/scheduler
```

---

### macOS / iOS

**Metal Acceleration** (future):
```xml
<embeddingmodel>
  <accelerator>metal</accelerator>
</embeddingmodel>
```

**Power efficiency**:
- macOS automatically manages CPU/GPU
- Ensure "Prevent App Nap" is enabled for Kodi

---

### Android / Embedded

**Reduce memory footprint**:
```xml
<cache>
  <maxsizemb>10</maxsizemb>  <!-- Very small cache -->
</cache>

<embeddingmodel>
  <lazyload>true</lazyload>
  <idletimeout>60</idletimeout>  <!-- Aggressive unload -->
</embeddingmodel>
```

**Use keyword mode by default**:
```xml
<hybridsearch>
  <defaultmode>keyword</defaultmode>
</hybridsearch>
```

---

## Troubleshooting

### Slow Queries

**Symptom**: Queries take >500ms consistently

**Diagnosis**:
```cpp
auto metrics = perf.GetMetrics();

if (metrics.avgEmbeddingTime > 100) {
    // Embedding is the bottleneck
} else if (metrics.avgKeywordSearchTime > 100) {
    // FTS5 is the bottleneck
} else if (metrics.avgVectorSearchTime > 100) {
    // Vector search is the bottleneck
}
```

**Solutions**:
- **Embedding**: Enable caching, use keyword mode for simple queries
- **FTS5**: Rebuild index, reduce topK, increase DB cache
- **Vector**: Reduce topK, consider filtering candidates first

---

### High Memory Usage

**Symptom**: Kodi using >1 GB RAM with semantic search

**Check**:
```cpp
auto usage = CMemoryManager::GetInstance().GetMemoryUsage();
std::cout << "Model: " << usage.modelMemoryMB << " MB\n";
std::cout << "Cache: " << usage.cacheMemoryMB << " MB\n";
std::cout << "Index: " << usage.indexMemoryMB << " MB\n";
```

**Solutions**:
- Reduce cache size
- Enable lazy loading
- Shorten idle timeout
- Check for large vector index (consider pruning old media)

---

### Poor Search Quality

**Symptom**: Irrelevant results, low relevance

**Check**:
```cpp
for (const auto& result : results) {
    std::cout << "Combined: " << result.combinedScore
              << ", Keyword: " << result.keywordScore
              << ", Vector: " << result.vectorScore << "\n";
}
```

**If keyword score high, vector score low**:
- Increase vector weight
- Check if embeddings are stale (reindex)
- Try semantic-only mode

**If vector score high, keyword score low**:
- Increase keyword weight
- Refine query (more specific)

**If both scores low**:
- Media may not contain relevant content
- Check indexing status
- Verify source filters

---

### Cache Not Working

**Symptom**: Low hit rate (<20%)

**Check**:
```cpp
auto cacheStats = cache.GetStats();
std::cout << "Hits: " << cacheStats.hits << "\n";
std::cout << "Misses: " << cacheStats.misses << "\n";
std::cout << "Hit rate: " << cacheStats.hitRate * 100 << "%\n";
```

**Solutions**:
- Increase TTL (queries timing out)
- Increase cache size (eviction too aggressive)
- Check query normalization (slight variations bypassing cache)

---

### Database Locked Errors

**Symptom**: "database is locked" errors

**Cause**: Concurrent writes in WAL mode

**Solutions**:
```sql
-- Increase busy timeout
PRAGMA busy_timeout = 5000;  -- 5 seconds
```

```cpp
database.SetBusyTimeout(5000);
```

**Or serialize writes**:
```cpp
std::mutex g_dbWriteMutex;

void ThreadSafeInsert() {
    std::lock_guard<std::mutex> lock(g_dbWriteMutex);
    database.InsertChunk(...);
}
```

---

## Best Practices Summary

### Do's

✅ Enable caching for better responsiveness
✅ Use lazy loading on resource-constrained devices
✅ Batch operations when inserting/updating multiple items
✅ Monitor performance metrics to identify bottlenecks
✅ Tune topK values based on result quality needs
✅ Rebuild indexes periodically after bulk changes
✅ Use appropriate search mode for query complexity
✅ Enable WAL journal mode for better concurrency
✅ Set reasonable memory limits to prevent OOM

### Don'ts

❌ Don't disable caching without good reason
❌ Don't use semantic mode for simple keyword queries
❌ Don't set topK > 500 (diminishing returns)
❌ Don't run indexing on main thread (blocks UI)
❌ Don't reindex frequently (expensive operation)
❌ Don't use synchronous=OFF in production (data loss risk)
❌ Don't ignore memory monitoring (can cause crashes)
❌ Don't forget to rebuild indexes after bulk operations
❌ Don't use huge cache sizes on low-RAM devices

---

## Benchmarking Tools

### Built-in Benchmark

**Run benchmark**:
```cpp
#include "semantic/test/PerformanceBenchmark.h"

CPerformanceBenchmark bench;
bench.RunFullBenchmark();
bench.PrintResults();
```

**Output**:
```
=== Semantic Search Performance Benchmark ===

Embedding Performance:
  Single embedding: 23.4ms
  Batch (10): 156.2ms (15.6ms per item)
  Batch (100): 1342.1ms (13.4ms per item)

Search Performance:
  Keyword-only: 12.3ms
  Semantic-only: 38.7ms
  Hybrid: 52.1ms

Vector Search:
  1K index: 5.2ms
  10K index: 18.6ms
  100K index: 34.9ms

Cache Performance:
  Hit: 0.8ms
  Miss: 52.1ms
  Hit rate: 68.3%
```

---

### Custom Benchmark

```cpp
// Benchmark your specific workload

std::vector<std::string> testQueries = {
    "action scene",
    "romantic conversation",
    "detective mystery"
};

auto start = std::chrono::steady_clock::now();

for (int i = 0; i < 100; ++i) {
    for (const auto& query : testQueries) {
        auto results = searchEngine.Search(query);
    }
}

auto end = std::chrono::steady_clock::now();
auto totalMs = std::chrono::duration<double, std::milli>(end - start).count();

std::cout << "300 queries in " << totalMs << "ms\n";
std::cout << "Avg: " << totalMs / 300 << "ms per query\n";
```

---

*For API details, see [APIReference.md](APIReference.md)*
*For architecture overview, see [TechnicalDesign.md](TechnicalDesign.md)*

---

*Last Updated: 2025-11-25*
*Phase 2 Performance Tuning Complete*
