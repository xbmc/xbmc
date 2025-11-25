# Performance Component

## Overview

The performance component provides comprehensive monitoring, profiling, and optimization tools for semantic search operations, including timing metrics, memory tracking, and cache management.

## Architecture

```
CPerformanceMonitor (Singleton)
    │
    ├─ Timing Metrics (embedding, search, indexing)
    ├─ Memory Tracking (model, cache, index)
    ├─ Throughput Counters (searches/sec, embeddings/sec)
    └─ JSON Export (for analysis)

CMemoryManager (Singleton)
    │
    ├─ Memory Limits (model, cache, total)
    ├─ Usage Monitoring (current, peak)
    └─ Automatic Unloading (on pressure)

CQueryCache (LRU Cache)
    │
    ├─ Result Cache (search results, 5min TTL)
    ├─ Embedding Cache (query embeddings, 1hr TTL)
    └─ LRU Eviction (least recently used)
```

## Files

| File | Description |
|------|-------------|
| `PerformanceMonitor.h/cpp` | Performance metrics collection and reporting |
| `MemoryManager.h/cpp` | Memory usage tracking and management |
| `QueryCache.h` | Template-based LRU cache implementation |

## Key Classes

### CPerformanceMonitor

**Singleton** for global performance monitoring.

**Collected Metrics**:
- **Timing**: Avg/max embedding time, search time, indexing time
- **Memory**: Current/peak usage for model, cache, index, total
- **Throughput**: Total searches, embeddings, cache hits/misses
- **Operations**: Search mode breakdown, model load/unload count

**Usage**:
```cpp
CPerformanceMonitor& perf = CPerformanceMonitor::GetInstance();

// Initialize (optional)
perf.Initialize(/*enableLogging=*/true,
                /*logPath=*/"semantic_perf.log");

// Automatic timing with RAII
{
    PERF_TIMER("embedding");
    auto emb = embedder.Embed(query);
}  // Timing automatically recorded

// Manual timing
auto start = std::chrono::steady_clock::now();
// ... operation ...
auto end = std::chrono::steady_clock::now();
double ms = std::chrono::duration<double, std::milli>(end - start).count();
perf.RecordSearch(ms, "hybrid");

// Get metrics
PerformanceMetrics metrics = perf.GetMetrics();
std::cout << "Avg search time: " << metrics.avgSearchTime << "ms\n";
std::cout << "Cache hit rate: " << metrics.cacheHitRate * 100 << "%\n";

// Export to JSON
std::string json = perf.GetMetricsJSON();
// Save to file or send to analytics service
```

**Metrics Structure**:
```cpp
struct PerformanceMetrics {
    // Timing (milliseconds)
    double avgEmbeddingTime{0.0};
    double avgSearchTime{0.0};
    double avgKeywordSearchTime{0.0};
    double avgVectorSearchTime{0.0};
    double avgHybridSearchTime{0.0};
    double maxEmbeddingTime{0.0};
    double maxSearchTime{0.0};

    // Memory (bytes)
    size_t currentMemoryUsage{0};
    size_t peakMemoryUsage{0};
    size_t modelMemoryUsage{0};
    size_t cacheMemoryUsage{0};
    size_t indexMemoryUsage{0};

    // Throughput
    uint64_t totalEmbeddings{0};
    uint64_t totalSearches{0};
    uint64_t totalCacheHits{0};
    uint64_t totalCacheMisses{0};
    uint64_t batchedEmbeddings{0};

    // Operations
    uint64_t keywordOnlySearches{0};
    uint64_t semanticOnlySearches{0};
    uint64_t hybridSearches{0};

    // Model management
    uint32_t modelLoadCount{0};
    uint32_t modelUnloadCount{0};
    double totalModelLoadTime{0.0};

    // Cache statistics
    float cacheHitRate{0.0f};
    size_t cacheSize{0};
    size_t cacheCapacity{0};

    // Timestamps
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point lastReset;
};
```

---

### CMemoryManager

**Singleton** for memory limit enforcement and monitoring.

**Features**:
- Set memory limits (model, cache, total)
- Monitor current/peak usage
- Automatic model unloading on memory pressure
- Component-level tracking

**Usage**:
```cpp
CMemoryManager& mem = CMemoryManager::GetInstance();

// Set limits
mem.SetModelMemoryLimit(200 * 1024 * 1024);  // 200 MB
mem.SetCacheMemoryLimit(50 * 1024 * 1024);   // 50 MB
mem.SetTotalMemoryLimit(500 * 1024 * 1024);  // 500 MB

// Update usage
mem.UpdateMemoryUsage(modelSize, "model");
mem.UpdateMemoryUsage(cacheSize, "cache");

// Check usage
auto usage = mem.GetMemoryUsage();
if (usage.totalMemoryMB > 400) {
    // Approaching limit, take action
    mem.UnloadModel();  // Free ~150 MB
}

// Manual unload
if (mem.ShouldUnloadModel()) {
    embeddingEngine.UnloadModel();
}
```

**Memory Usage Structure**:
```cpp
struct MemoryUsage {
    size_t modelMemoryMB{0};
    size_t cacheMemoryMB{0};
    size_t indexMemoryMB{0};
    size_t totalMemoryMB{0};
    size_t peakMemoryMB{0};

    bool isModelLoaded{false};
    size_t availableSystemMemoryMB{0};
};
```

---

### CQueryCache (Template)

**Generic LRU cache** with TTL support.

**Features**:
- Template-based (works with any key/value types)
- LRU eviction policy
- Time-to-live (TTL) expiration
- Thread-safe (internal mutex)

**Usage**:
```cpp
// Create cache (max 100 entries, 300 sec TTL)
CLRUCache<std::string, SearchResults> cache(100, 300);

// Insert
SearchResults results = PerformSearch(query);
cache.Put(query, results);

// Retrieve
auto cached = cache.Get(query);
if (cached.has_value()) {
    // Cache hit
    return cached.value();
} else {
    // Cache miss
    auto results = PerformSearch(query);
    cache.Put(query, results);
    return results;
}

// Check existence
if (cache.Contains(query)) {
    // Key exists and not expired
}

// Remove specific entry
cache.Remove(query);

// Clear all
cache.Clear();

// Get stats
size_t size = cache.Size();
size_t capacity = cache.Capacity();
size_t memoryUsage = cache.GetMemoryUsage();
```

**Template Specializations**:
```cpp
// Search result cache
CLRUCache<std::string, std::vector<HybridSearchResult>> resultCache{100, 300};

// Embedding cache
CLRUCache<std::string, Embedding> embeddingCache{1000, 3600};

// Generic data cache
CLRUCache<int, MyDataType> dataCache{50, 600};
```

---

## Performance Macros

### PERF_TIMER

**RAII-based automatic timing**:
```cpp
{
    PERF_TIMER("operation_name");
    // ... code to measure ...
}  // Timing recorded automatically on scope exit
```

**Expands to**:
```cpp
KODI::SEMANTIC::CPerformanceMonitor::Timer _perfTimer##__LINE__("operation_name");
```

**Timer Class**:
```cpp
class CPerformanceMonitor::Timer {
public:
    Timer(const std::string& operation)
      : m_operation(operation),
        m_start(std::chrono::steady_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - m_start).count();

        // Record timing based on operation name
        if (m_operation == "embedding") {
            CPerformanceMonitor::GetInstance().RecordEmbedding(ms, 1);
        } else if (m_operation == "search") {
            CPerformanceMonitor::GetInstance().RecordSearch(ms, "unknown");
        }
    }

private:
    std::string m_operation;
    std::chrono::steady_clock::time_point m_start;
};
```

---

## Configuration

**advancedsettings.xml**:
```xml
<semantic>
  <performance>
    <monitoring>true</monitoring>
    <logging>false</logging>
    <logpath>special://temp/semantic_perf.log</logpath>
    <detailedtimings>false</detailedtimings>  <!-- Extra overhead -->
    <samplinginterval>60</samplinginterval>  <!-- seconds -->
  </performance>

  <memory>
    <modellimit>200</modellimit>  <!-- MB -->
    <cachelimit>50</cachelimit>   <!-- MB -->
    <totallimit>500</totallimit>  <!-- MB -->
    <autounload>true</autounload>  <!-- Unload model on pressure -->
  </memory>

  <cache>
    <enabled>true</enabled>
    <maxsizemb>50</maxsizemb>
    <resultttl>300</resultttl>      <!-- seconds -->
    <embeddingttl>3600</embeddingttl>  <!-- seconds -->
  </cache>
</semantic>
```

---

## Monitoring Workflow

### 1. Enable Monitoring

```cpp
CPerformanceMonitor& perf = CPerformanceMonitor::GetInstance();
perf.Initialize(/*enableLogging=*/true);
perf.SetEnabled(true);
```

### 2. Instrument Code

```cpp
// Automatic timing
void SearchOperation(const std::string& query) {
    PERF_TIMER("search");

    // Keyword search
    {
        PERF_TIMER("keyword_search");
        auto kwResults = ftsSearch.Search(query);
    }

    // Vector search
    {
        PERF_TIMER("vector_search");
        auto vecResults = vectorSearch.Search(queryEmb);
    }

    // Combine results
    {
        PERF_TIMER("rrf_fusion");
        auto merged = ranker.Combine(kwResults, vecResults);
    }
}
```

### 3. Collect Metrics

```cpp
// Periodically (every 60 seconds)
void PeriodicMetricsCollection() {
    auto metrics = perf.GetMetrics();

    // Log to file
    perf.LogMetrics();

    // Or export to JSON
    std::string json = perf.GetMetricsJSON();
    SendToAnalytics(json);
}
```

### 4. Analyze Results

**Check for bottlenecks**:
```cpp
auto metrics = perf.GetMetrics();

if (metrics.avgEmbeddingTime > 100) {
    LOG("Warning: Embedding is slow ({}ms avg)", metrics.avgEmbeddingTime);
    // Consider: caching, lazy loading, GPU acceleration
}

if (metrics.cacheHitRate < 0.3f) {
    LOG("Warning: Low cache hit rate ({:.1f}%)", metrics.cacheHitRate * 100);
    // Consider: increase cache size, increase TTL
}

if (metrics.peakMemoryUsage > 800 * 1024 * 1024) {
    LOG("Warning: High memory usage ({} MB)", metrics.peakMemoryUsage / 1024 / 1024);
    // Consider: reduce cache, enable lazy loading
}
```

---

## Memory Monitoring

### Component-Level Tracking

```cpp
void UpdateComponentMemory() {
    CMemoryManager& mem = CMemoryManager::GetInstance();

    // Model
    if (embedEngine.IsModelLoaded()) {
        mem.UpdateMemoryUsage(modelSize, "model");
    }

    // Cache
    size_t cacheSize = queryCache.GetMemoryUsage();
    mem.UpdateMemoryUsage(cacheSize, "cache");

    // Vector index
    size_t indexSize = vectorSearcher.GetIndexMemoryUsage();
    mem.UpdateMemoryUsage(indexSize, "index");
}
```

### Automatic Unloading

```cpp
void CheckMemoryPressure() {
    CMemoryManager& mem = CMemoryManager::GetInstance();
    auto usage = mem.GetMemoryUsage();

    // Check total limit
    if (usage.totalMemoryMB > mem.GetTotalMemoryLimit()) {
        LOG("Memory limit exceeded, unloading model");
        embedEngine.UnloadModel();
    }

    // Check system memory
    if (usage.availableSystemMemoryMB < 500) {
        LOG("Low system memory, unloading model");
        embedEngine.UnloadModel();
        queryCache.Clear();
    }
}
```

---

## Cache Management

### Multi-Level Caching

**Level 1: Embedding Cache**
```cpp
Embedding GetEmbedding(const std::string& text) {
    // Check cache
    auto cached = embeddingCache.Get(text);
    if (cached.has_value()) {
        perf.RecordCacheHit();
        return cached.value();
    }

    // Cache miss, generate
    perf.RecordCacheMiss();
    auto embedding = embedEngine.Embed(text);
    embeddingCache.Put(text, embedding);
    return embedding;
}
```

**Level 2: Result Cache**
```cpp
std::vector<HybridSearchResult> Search(const std::string& query) {
    std::string cacheKey = GenerateCacheKey(query, options);

    // Check cache
    auto cached = resultCache.Get(cacheKey);
    if (cached.has_value()) {
        perf.RecordCacheHit();
        return cached.value();
    }

    // Cache miss, search
    perf.RecordCacheMiss();
    auto results = PerformSearch(query, options);
    resultCache.Put(cacheKey, results);
    return results;
}
```

### Cache Invalidation

**On database changes**:
```cpp
void OnDatabaseWrite() {
    // Invalidate result cache (stale data)
    resultCache.Clear();

    // Keep embedding cache (embeddings don't change)
}
```

**Manual invalidation**:
```cpp
// After reindexing
indexService.ReindexAll();
queryCache.Clear();  // All caches
```

---

## Logging and Export

### Performance Log Format

```
[2024-11-25 14:32:15] PERF: Search completed in 42ms
  - Embedding: 23ms
  - Keyword search: 12ms
  - Vector search: 18ms
  - RRF fusion: 2ms
  - Result enrichment: 5ms
  Cache hit: false
  Memory usage: 245 MB
```

### JSON Export

```json
{
  "performance_metrics": {
    "timing": {
      "avg_embedding_time_ms": 23.4,
      "avg_search_time_ms": 42.1,
      "avg_keyword_search_time_ms": 12.3,
      "avg_vector_search_time_ms": 18.6,
      "avg_hybrid_search_time_ms": 52.1,
      "max_search_time_ms": 156.7
    },
    "memory": {
      "current_mb": 245,
      "peak_mb": 312,
      "model_mb": 125,
      "cache_mb": 42,
      "index_mb": 78
    },
    "throughput": {
      "total_searches": 5842,
      "total_embeddings": 6123,
      "cache_hits": 4201,
      "cache_misses": 1641,
      "cache_hit_rate": 0.719
    },
    "operations": {
      "keyword_only": 1234,
      "semantic_only": 892,
      "hybrid": 3716
    },
    "timestamp": "2024-11-25T14:32:15Z"
  }
}
```

---

## Testing

**Unit Tests**: `xbmc/semantic/test/TestPerformance.cpp`

**Test Cases**:
- Timing measurement accuracy
- Memory tracking
- Cache hit/miss counting
- LRU eviction
- TTL expiration
- Thread safety

**Benchmark Tests**: `xbmc/semantic/test/BenchmarkPerformance.cpp`

---

## Best Practices

### 1. Use RAII Timing

✅ **Good** (automatic):
```cpp
{
    PERF_TIMER("operation");
    DoOperation();
}  // Timing recorded
```

❌ **Bad** (manual, error-prone):
```cpp
auto start = std::chrono::steady_clock::now();
DoOperation();
auto end = std::chrono::steady_clock::now();
// Forgot to record timing!
```

### 2. Monitor Continuously

```cpp
// Periodic monitoring (every 60s)
void MonitoringLoop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(60));

        auto metrics = perf.GetMetrics();
        LogMetrics(metrics);

        CheckBottlenecks(metrics);
    }
}
```

### 3. Set Appropriate Limits

```cpp
// Based on system RAM
size_t systemRAM = GetSystemRAM();

if (systemRAM < 4 * 1024 * 1024 * 1024) {
    // < 4 GB RAM
    mem.SetTotalMemoryLimit(200 * 1024 * 1024);  // 200 MB
} else if (systemRAM < 8 * 1024 * 1024 * 1024) {
    // 4-8 GB RAM
    mem.SetTotalMemoryLimit(500 * 1024 * 1024);  // 500 MB
} else {
    // > 8 GB RAM
    mem.SetTotalMemoryLimit(1000 * 1024 * 1024);  // 1 GB
}
```

---

## Future Enhancements

- **Distributed tracing** (OpenTelemetry integration)
- **Real-time dashboards** (Grafana, Prometheus)
- **Anomaly detection** (ML-based alerts)
- **Profiling integration** (perf, VTune)
- **A/B testing framework** (compare algorithms)

## See Also

- [PerformanceTuning.md](../docs/PerformanceTuning.md) - Optimization guide
- [APIReference.md](../docs/APIReference.md) - C++ API reference
- [TechnicalDesign.md](../docs/TechnicalDesign.md) - System architecture

---

*Last Updated: 2025-11-25*
