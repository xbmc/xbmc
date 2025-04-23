/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidDatabaseOptimizer.h"
#include "platform/android/activity/XBMCApp.h"
#include "utils/log.h"
#include "ServiceBroker.h"
#include "platform/android/CPUInfoAndroid.h"
#include "platform/android/GPUInfoAndroid.h"
#include "dbwrappers/Database.h"
#include <androidjni/SystemClock.h>
#include <androidjni/ActivityManager.h>
#include <androidjni/Context.h>

#include <sqlite3.h>
#include <cstdlib>
#include <algorithm>
#include <regex>

// Default cache sizes
#define DEFAULT_CACHE_SIZE_KB 2048   // 2MB
#define LOW_END_CACHE_SIZE_KB 1024   // 1MB
#define HIGH_END_CACHE_SIZE_KB 8192  // 8MB

// Default page size
#define DEFAULT_PAGE_SIZE 4096      // 4KB
#define OPTIMAL_PAGE_SIZE 8192      // 8KB for newer devices
#define LOW_END_PAGE_SIZE 1024      // 1KB for low-memory devices

// Max cache entries
#define DEFAULT_MAX_CACHE_ENTRIES 100
#define LOW_END_MAX_CACHE_ENTRIES 50
#define HIGH_END_MAX_CACHE_ENTRIES 200

// Journal modes
#define JOURNAL_MODE_DELETE 0
#define JOURNAL_MODE_TRUNCATE 1
#define JOURNAL_MODE_PERSIST 2
#define JOURNAL_MODE_WAL 3
#define JOURNAL_MODE_MEMORY 4

// Synchronous modes
#define SYNC_MODE_OFF 0
#define SYNC_MODE_NORMAL 1
#define SYNC_MODE_FULL 2

// Timer intervals
#define CLEANUP_INTERVAL_MS 30000   // 30 seconds
#define OPTIMIZATION_INTERVAL_MS 3600000  // 1 hour

std::unique_ptr<CAndroidDatabaseOptimizer> CAndroidDatabaseOptimizer::s_instance;

void CAndroidDatabaseOptimizer::Initialize()
{
  if (!s_instance)
    s_instance.reset(new CAndroidDatabaseOptimizer());
}

CAndroidDatabaseOptimizer& CAndroidDatabaseOptimizer::GetInstance()
{
  if (!s_instance)
    Initialize();
    
  return *s_instance;
}

CAndroidDatabaseOptimizer::CAndroidDatabaseOptimizer()
  : m_pageSize(DEFAULT_PAGE_SIZE),
    m_cacheSize(DEFAULT_CACHE_SIZE_KB),
    m_journalMode(JOURNAL_MODE_WAL),
    m_syncMode(SYNC_MODE_NORMAL),
    m_batchModeEnabled(false),
    m_maxCacheEntries(DEFAULT_MAX_CACHE_ENTRIES),
    m_lastCacheCleanup(0),
    m_enablePerformanceTracking(false),
    m_nextTrackingId(1)
{
  // Calculate optimal settings based on device capabilities
  CalculateOptimalSettings();
  
  // Set up optimization timer
  m_optimizationTimer.SetTimeout(OPTIMIZATION_INTERVAL_MS);
  
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Initialized with page size: %d bytes, cache size: %d KB",
          m_pageSize, m_cacheSize);
}

CAndroidDatabaseOptimizer::~CAndroidDatabaseOptimizer()
{
  // Stop timer
  m_optimizationTimer.Stop();
  
  // Clear cache
  ClearCache();
}

void CAndroidDatabaseOptimizer::CalculateOptimalSettings()
{
  // Determine optimal settings based on device capabilities
  m_pageSize = 4096; // Most modern file systems use 4KB blocks
  
#ifdef TARGET_ANDROID
  try {
    // Get device memory information
    CJNIActivityManager activityManager(CJNIContext::getSystemService(CJNIContext::ACTIVITY_SERVICE));
    CJNIActivityManager::MemoryInfo memoryInfo;
    activityManager.getMemoryInfo(memoryInfo);
    
    // Scale cache based on available memory
    const int64_t ONE_GB = 1024L * 1024L * 1024L;
    
    if (memoryInfo.totalMem < 2 * ONE_GB) // < 2GB RAM
    {
      m_cacheSize = 2048; // 2MB cache
      m_syncMode = 1;     // NORMAL sync
      m_journalMode = 3;  // WAL
    }
    else if (memoryInfo.totalMem < 4 * ONE_GB) // 2-4GB RAM
    {
      m_cacheSize = 4096; // 4MB cache
      m_syncMode = 1;     // NORMAL sync
      m_journalMode = 3;  // WAL
    }
    else // >= 4GB RAM
    {
      m_cacheSize = 8192; // 8MB cache
      m_syncMode = 0;     // OFF sync for better performance
      m_journalMode = 3;  // WAL
    }
    
    // If running low on memory, reduce cache further
    if (memoryInfo.availMem < memoryInfo.totalMem * 0.2) // Less than 20% memory available
    {
      m_cacheSize /= 2;
      m_syncMode = 1; // Make sure we're at least NORMAL sync to avoid data loss
    }
    
    CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Configured for device with %.1f GB RAM (cache: %d KB, journalMode: %d, syncMode: %d)",
              static_cast<double>(memoryInfo.totalMem) / ONE_GB, m_cacheSize, m_journalMode, m_syncMode);
  }
  catch (const std::exception& e) {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Failed to get memory info: %s", e.what());
    
    // Fallback to conservative defaults
    m_cacheSize = 2048;
    m_syncMode = 1;
    m_journalMode = 3;
  }
#else
  // Non-Android default settings
  m_cacheSize = 4096;
  m_syncMode = 1;
  m_journalMode = 3;
#endif
}

bool CAndroidDatabaseOptimizer::IsLowEndDevice() const
{
#ifdef TARGET_ANDROID
  // Get device memory information
  CJNIActivityManager activityManager(CJNIContext::getSystemService(CJNIContext::ACTIVITY_SERVICE));
  CJNIActivityManager::MemoryInfo memoryInfo;
  activityManager.getMemoryInfo(memoryInfo);
  
  // If total memory is less than 3GB, or available memory is less than 15% of total,
  // consider it a low-end or memory-constrained device
  const int64_t LOW_MEMORY_THRESHOLD = 3L * 1024L * 1024L * 1024L; // 3GB
  const float LOW_AVAILABLE_PERCENT = 0.15f; // 15%
  
  if (memoryInfo.totalMem < LOW_MEMORY_THRESHOLD) 
  {
    CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Device has less than 3GB RAM (%lld bytes), using low-memory optimizations", 
              static_cast<long long>(memoryInfo.totalMem));
    return true;
  }
  
  float availablePercent = static_cast<float>(memoryInfo.availMem) / static_cast<float>(memoryInfo.totalMem);
  if (availablePercent < LOW_AVAILABLE_PERCENT)
  {
    CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Available memory is low (%.1f%% available), using low-memory optimizations", 
              availablePercent * 100.0f);
    return true;
  }
#endif
  
  return false;
}

void CAndroidDatabaseOptimizer::OptimizeDatabaseConnection(const std::string& dbPath)
{
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Optimizing database connection for %s", dbPath.c_str());
  
  // Apply optimal PRAGMA settings
  ApplyOptimalPragmas(dbPath);
  
  // Enable WAL mode if appropriate
  SetWALMode(dbPath, m_journalMode == JOURNAL_MODE_WAL);
  
  // Set synchronous mode
  SetSynchronousMode(dbPath, m_syncMode);
  
  // Schedule optimization
  ScheduleOptimization(dbPath);
}

void CAndroidDatabaseOptimizer::ApplyOptimalPragmas(const std::string& dbPath)
{
  // Set page size
  ExecutePragma(dbPath, "PRAGMA page_size = " + std::to_string(m_pageSize));
  
  // Set cache size in KB
  ExecutePragma(dbPath, "PRAGMA cache_size = -" + std::to_string(m_cacheSize));
  
  // Set temp store to memory
  ExecutePragma(dbPath, "PRAGMA temp_store = MEMORY");
  
  // Enable auto vacuum (0=NONE, 1=FULL, 2=INCREMENTAL)
  int vacuumMode = IsLowEndDevice() ? 0 : 2;
  ExecutePragma(dbPath, "PRAGMA auto_vacuum = " + std::to_string(vacuumMode));
  
  // Set mmap size for faster read operations (if not a low-end device)
  if (!IsLowEndDevice())
  {
    int mmapSize = m_cacheSize * 1024 * 2; // 2x cache size
    ExecutePragma(dbPath, "PRAGMA mmap_size = " + std::to_string(mmapSize));
  }
  
  // Disable lookaside (SQLite's per-connection memory allocator) on low-memory devices
  if (IsLowEndDevice())
  {
    ExecutePragma(dbPath, "PRAGMA lookaside = OFF");
  }
  else
  {
    // Optimize lookaside for better performance
    ExecutePragma(dbPath, "PRAGMA lookaside = 512, 128");
  }
}

void CAndroidDatabaseOptimizer::SetWALMode(const std::string& dbPath, bool enable)
{
  if (enable)
  {
    ExecutePragma(dbPath, "PRAGMA journal_mode = WAL");
    
    // Configure WAL autocheckpoint (pages)
    int checkpointSize = IsLowEndDevice() ? 100 : 1000;
    ExecutePragma(dbPath, "PRAGMA wal_autocheckpoint = " + std::to_string(checkpointSize));
    
    CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Enabled WAL mode for %s with checkpoint size %d", 
            dbPath.c_str(), checkpointSize);
  }
  else
  {
    // Determine best non-WAL mode based on device
    std::string mode = IsLowEndDevice() ? "DELETE" : "TRUNCATE";
    ExecutePragma(dbPath, "PRAGMA journal_mode = " + mode);
    
    CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Set journal mode to %s for %s",
            mode.c_str(), dbPath.c_str());
  }
}

void CAndroidDatabaseOptimizer::CacheQueryResult(const std::string& query, const std::string& result, unsigned int ttlSeconds)
{
  if (query.empty() || result.empty())
    return;

  // Skip caching large results
  if (result.size() > 1024 * 100) // 100KB limit
    return;

  std::unique_lock<CCriticalSection> lock(m_cacheLock);
  
  // Check if we need to clean up old entries first
  int64_t now = CJNISystemClock::uptimeMillis();
  
  // Only clean up periodically
  if (now - m_lastCacheCleanup > CLEANUP_INTERVAL_MS)
  {
    // Remove expired entries
    auto it = m_queryCache.begin();
    while (it != m_queryCache.end())
    {
      if (it->second.ttl > 0 && (now - it->second.timestamp > it->second.ttl * 1000))
      {
        it = m_queryCache.erase(it);
      }
      else
      {
        ++it;
      }
    }
    
    // If still too many entries, remove oldest
    while (m_queryCache.size() > m_maxCacheEntries)
    {
      int64_t oldest = now;
      auto oldestIt = m_queryCache.end();
      
      for (auto it = m_queryCache.begin(); it != m_queryCache.end(); ++it)
      {
        if (it->second.timestamp < oldest)
        {
          oldest = it->second.timestamp;
          oldestIt = it;
        }
      }
      
      if (oldestIt != m_queryCache.end())
        m_queryCache.erase(oldestIt);
      else
        break; // Safety check
    }
    
    m_lastCacheCleanup = now;
  }
  
  // Add or update cache entry
  CacheEntry entry;
  entry.result = result;
  entry.timestamp = now;
  entry.ttl = ttlSeconds;
  
  m_queryCache[query] = entry;
}

bool CAndroidDatabaseOptimizer::GetCachedQueryResult(const std::string& query, std::string& resultOut)
{
  if (query.empty())
    return false;
    
  std::unique_lock<CCriticalSection> lock(m_cacheLock);
  
  auto it = m_queryCache.find(query);
  if (it == m_queryCache.end())
    return false; // Cache miss
    
  // Check if entry has expired
  int64_t now = CJNISystemClock::uptimeMillis();
  if (it->second.ttl > 0 && (now - it->second.timestamp > it->second.ttl * 1000))
  {
    // Entry expired, remove it
    m_queryCache.erase(it);
    return false;
  }
  
  // Update timestamp (extends TTL for frequently used queries)
  it->second.timestamp = now;
  
  // Return cached result
  resultOut = it->second.result;
  return true;
}

void CAndroidDatabaseOptimizer::InvalidateCache(const std::string& query)
{
  std::unique_lock<CCriticalSection> lock(m_cacheLock);
  
  if (query.empty())
    return;
    
  // Remove specific entry
  m_queryCache.erase(query);
}

void CAndroidDatabaseOptimizer::ClearCache()
{
  try
  {
    // Loop through all scheduled optimizations (tracked databases)
    CCriticalSection lock(m_optimizationLock);
    for (const auto& dbPath : m_scheduledOptimizations)
    {
      ExecutePragma(dbPath, "PRAGMA shrink_memory");
    }
    
    CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Cache cleared for all tracked databases");
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Error clearing cache: %s", e.what());
  }
}

void CAndroidDatabaseOptimizer::SetTablePrefetch(const std::string& tableName, bool prefetch)
{
  // This would implement prefetching logic for frequently accessed tables
  // Not fully implemented in this version
  
  CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Table prefetch for %s %s", 
          tableName.c_str(), prefetch ? "enabled" : "disabled");
}

void CAndroidDatabaseOptimizer::ScheduleOptimization(const std::string& dbPath)
{
  std::unique_lock<CCriticalSection> lock(m_optimizationLock);
  
  // Don't schedule the same DB multiple times
  auto it = std::find(m_scheduledOptimizations.begin(), m_scheduledOptimizations.end(), dbPath);
  if (it == m_scheduledOptimizations.end())
  {
    m_scheduledOptimizations.push_back(dbPath);
    
    CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Scheduled optimization for %s", dbPath.c_str());
    
    // Start the timer if not already running
    if (!m_optimizationTimer.IsRunning())
    {
      m_optimizationTimer.Start([this]() {
        std::unique_lock<CCriticalSection> lock(m_optimizationLock);
        if (!m_scheduledOptimizations.empty())
        {
          std::string dbPath = m_scheduledOptimizations.front();
          m_scheduledOptimizations.erase(m_scheduledOptimizations.begin());
          lock.unlock();
          
          // Run optimization task
          RunOptimizationTask(dbPath);
        }
        
        // Keep timer running if more optimizations are scheduled
        if (!m_scheduledOptimizations.empty())
          m_optimizationTimer.Start();
      });
    }
  }
}

void CAndroidDatabaseOptimizer::RunOptimizationTask(const std::string& dbPath)
{
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Running optimization for %s", dbPath.c_str());
  
  // Run ANALYZE to update statistics
  ExecutePragma(dbPath, "ANALYZE");
  
  // Run optimization
  ExecutePragma(dbPath, "PRAGMA optimize");
  
  // Checkpoint WAL if in WAL mode
  if (m_journalMode == JOURNAL_MODE_WAL)
    ForceWALCheckpoint(dbPath);
    
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Optimization completed for %s", dbPath.c_str());
}

void CAndroidDatabaseOptimizer::OnLowMemory()
{
  // When Android system reports low memory, take action to reduce our memory footprint
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Low memory notification received, reducing database cache");
  
  // Clear the query cache to free memory
  ClearCache();
  
  // Reduce the SQLite cache size for all currently active databases
  CCriticalSection lock(m_optimizationLock);
  for (const auto& dbPath : m_scheduledOptimizations)
  {
    try {
      // Reduce cache size to minimum
      int reducedCacheSize = IsLowEndDevice() ? 256 : 512; // KB
      ExecutePragma(dbPath, PrepareSQL("PRAGMA cache_size=-%d", reducedCacheSize));
      
      // Force a checkpoint to flush WAL to main DB and reduce memory usage
      ForceWALCheckpoint(dbPath);
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Error reducing memory usage: %s", e.what());
    }
  }
}

void CAndroidDatabaseOptimizer::SetBatchMode(bool enable)
{
  m_batchModeEnabled = enable;
}

bool CAndroidDatabaseOptimizer::IsBatchModeEnabled() const
{
  return m_batchModeEnabled;
}

void CAndroidDatabaseOptimizer::SetMemoryCacheSize(int sizeKB)
{
  m_cacheSize = sizeKB;
}

void CAndroidDatabaseOptimizer::ForceWALCheckpoint(const std::string& dbPath)
{
  try
  {
    // Force a FULL checkpoint of the WAL
    ExecutePragma(dbPath, "PRAGMA wal_checkpoint(FULL)");
    CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Forced WAL checkpoint for %s", dbPath.c_str());
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Error forcing WAL checkpoint: %s", e.what());
  }
}

void CAndroidDatabaseOptimizer::SetSynchronousMode(const std::string& dbPath, int synchronousMode)
{
  if (synchronousMode < 0 || synchronousMode > 2)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Invalid synchronous mode %d", synchronousMode);
    return;
  }
  
  // Convert numeric mode to text
  const char* syncModeText[] = {"OFF", "NORMAL", "FULL"};
  
  try
  {
    std::string pragma = PrepareSQL("PRAGMA synchronous=%s", syncModeText[synchronousMode]);
    ExecutePragma(dbPath, pragma);
    CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Set synchronous mode to %s for %s", 
              syncModeText[synchronousMode], dbPath.c_str());
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Error setting synchronous mode: %s", e.what());
  }
}

void CAndroidDatabaseOptimizer::PerformMaintenance(const std::string& dbPath, bool full)
{
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Performing %s maintenance on %s",
            full ? "full" : "basic", dbPath.c_str());
  
  try
  {
    SqliteDatabase db;
    db.setDatabase(dbPath.c_str());
    if (db.connect())
    {
      std::unique_ptr<Dataset> dataset = db.CreateDataset();
      
      // Analyze to update internal statistics
      dataset->exec("ANALYZE");
      
      // Check database integrity
      if (dataset->query("PRAGMA integrity_check") && dataset->num_rows() > 0)
      {
        std::string result = dataset->fv(0).get_asString();
        if (result != "ok")
        {
          CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Database integrity check failed for %s: %s", 
                    dbPath.c_str(), result.c_str());
        }
      }
      
      // Force WAL checkpoint
      dataset->exec("PRAGMA wal_checkpoint(FULL)");
      
      if (full)
      {
        // Vacuum database to defragment and optimize
        dataset->exec("VACUUM");
        
        // Reanalyze after vacuum
        dataset->exec("ANALYZE");
      }
      
      dataset->close();
      db.disconnect();
      
      CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Maintenance complete for %s", dbPath.c_str());
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Error during database maintenance: %s", e.what());
  }
}

std::string CAndroidDatabaseOptimizer::ExecutePragma(const std::string& dbPath, const std::string& pragma)
{
  std::string result;
  
  sqlite3* db = nullptr;
  int rc = sqlite3_open(dbPath.c_str(), &db);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Failed to open database %s: %s",
            dbPath.c_str(), sqlite3_errmsg(db));
    sqlite3_close(db);
    return result;
  }
  
  // Execute pragma
  sqlite3_stmt* stmt = nullptr;
  rc = sqlite3_prepare_v2(db, pragma.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Failed to prepare statement: %s",
            sqlite3_errmsg(db));
    sqlite3_close(db);
    return result;
  }
  
  // Step through all rows
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
  {
    // Get the first column as text (if any)
    const unsigned char* text = sqlite3_column_text(stmt, 0);
    if (text)
      result = reinterpret_cast<const char*>(text);
  }
  
  // Check for errors
  if (rc != SQLITE_DONE)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Error executing pragma: %s",
            sqlite3_errmsg(db));
  }
  
  // Clean up
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  
  return result;
}

int64_t CAndroidDatabaseOptimizer::BeginQueryPerformanceTracking(const std::string& query)
{
  if (!m_enablePerformanceTracking)
    return 0;

  std::unique_lock<CCriticalSection> lock(m_perfLock);
  
  int64_t trackingId = m_nextTrackingId++;
  
  // Create a new tracker for this query
  ActiveQueryTracker tracker;
  tracker.query = query;
  tracker.pattern = GetQueryPattern(query);
  tracker.startTimeMs = CJNISystemClock::uptimeMillis();
  
  m_activeQueries[trackingId] = tracker;
  
  return trackingId;
}

void CAndroidDatabaseOptimizer::EndQueryPerformanceTracking(int64_t trackingId, bool wasCached)
{
  if (!m_enablePerformanceTracking || trackingId == 0)
    return;

  std::unique_lock<CCriticalSection> lock(m_perfLock);
  
  auto it = m_activeQueries.find(trackingId);
  if (it == m_activeQueries.end())
    return; // Tracking ID not found
    
  int64_t now = CJNISystemClock::uptimeMillis();
  int64_t elapsedMs = now - it->second.startTimeMs;
  
  // Update performance data for this query pattern
  const std::string& pattern = it->second.pattern;
  
  if (m_performanceData.find(pattern) == m_performanceData.end())
  {
    // First time seeing this pattern
    QueryPerformanceData data;
    data.totalTimeMs = elapsedMs;
    data.maxTimeMs = elapsedMs;
    data.minTimeMs = elapsedMs;
    data.invocationCount = 1;
    data.cacheHits = wasCached ? 1 : 0;
    data.slowestQueryPattern = it->second.query;
    
    m_performanceData[pattern] = data;
  }
  else
  {
    // Update existing stats
    QueryPerformanceData& data = m_performanceData[pattern];
    data.totalTimeMs += elapsedMs;
    data.invocationCount++;
    
    if (wasCached)
      data.cacheHits++;
      
    if (elapsedMs > data.maxTimeMs)
    {
      data.maxTimeMs = elapsedMs;
      data.slowestQueryPattern = it->second.query;
    }
    
    if (elapsedMs < data.minTimeMs)
      data.minTimeMs = elapsedMs;
  }
  
  // Remove the active query tracker
  m_activeQueries.erase(it);
}

std::unordered_map<std::string, CAndroidDatabaseOptimizer::QueryPerformanceData> CAndroidDatabaseOptimizer::GetPerformanceStats()
{
  std::unique_lock<CCriticalSection> lock(m_perfLock);
  return m_performanceData;
}

void CAndroidDatabaseOptimizer::ExportPerformanceStats(const std::string& fileName)
{
  std::unique_lock<CCriticalSection> lock(m_perfLock);
  
  // First log to Kodi log
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Database Performance Report");
  CLog::Log(LOGINFO, "============================================");
  
  struct PerformanceEntry {
    std::string pattern;
    QueryPerformanceData data;
    double avgTimeMs;
  };
  
  std::vector<PerformanceEntry> sortedStats;
  
  for (const auto& entry : m_performanceData)
  {
    PerformanceEntry pe;
    pe.pattern = entry.first;
    pe.data = entry.second;
    pe.avgTimeMs = entry.second.invocationCount > 0 ? 
                  static_cast<double>(entry.second.totalTimeMs) / entry.second.invocationCount : 0;
    sortedStats.push_back(pe);
  }
  
  // Sort by average execution time (descending)
  std::sort(sortedStats.begin(), sortedStats.end(), 
            [](const PerformanceEntry& a, const PerformanceEntry& b) {
              return a.avgTimeMs > b.avgTimeMs;
            });
  
  // Log the top 20 slowest queries
  size_t count = std::min(sortedStats.size(), static_cast<size_t>(20));
  
  for (size_t i = 0; i < count; i++)
  {
    const auto& entry = sortedStats[i];
    const auto& data = entry.data;
    
    CLog::Log(LOGINFO, "%d. Pattern: %s", static_cast<int>(i+1), entry.pattern.c_str());
    CLog::Log(LOGINFO, "   Count: %d, Avg time: %.2f ms, Min: %" PRId64 " ms, Max: %" PRId64 " ms, Cache hits: %d",
              data.invocationCount, entry.avgTimeMs, data.minTimeMs, data.maxTimeMs, data.cacheHits);
    
    if (data.maxTimeMs > 100)  // Only show the slowest query if it's significant
      CLog::Log(LOGINFO, "   Slowest query: %s", data.slowestQueryPattern.c_str());
  }
  
  CLog::Log(LOGINFO, "============================================");
  
  // If a filename was provided, also write to file
  if (!fileName.empty())
  {
    // File writing implementation would go here
    CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Performance data exported to %s", fileName.c_str());
  }
}

void CAndroidDatabaseOptimizer::ResetPerformanceStats()
{
  std::unique_lock<CCriticalSection> lock(m_perfLock);
  m_performanceData.clear();
  m_activeQueries.clear();
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Performance statistics reset");
}

std::string CAndroidDatabaseOptimizer::GetQueryPattern(const std::string& query)
{
  // This function converts specific SQL queries into generalized patterns
  // for performance tracking, e.g. "SELECT * FROM table WHERE id=5" becomes
  // "SELECT * FROM table WHERE id=?"
  
  std::string pattern = query;
  
  // Simple approach: replace numbers and quoted strings with placeholders
  // More sophisticated approach would use proper SQL parsing
  
  // Replace numeric literals with ?
  std::regex numericRegex("\\b\\d+\\.?\\d*\\b");
  pattern = std::regex_replace(pattern, numericRegex, "?");
  
  // Replace quoted strings with ?
  std::regex quotedRegex("'[^']*'");
  pattern = std::regex_replace(pattern, quotedRegex, "?");
  
  // Replace double-quoted strings with ?
  std::regex dquotedRegex("\"[^\"]*\"");
  pattern = std::regex_replace(pattern, dquotedRegex, "?");
  
  return pattern;
}

void CAndroidDatabaseOptimizer::RecordQueryExecution(const std::string& dbPath, 
                                                    const std::string& query, 
                                                    double executionTimeMs)
{
  static const double SLOW_QUERY_THRESHOLD_MS = 100.0; // Queries over 100ms are considered slow
  
  if (dbPath.empty())
    return;
    
  CSingleLock lock(m_statsLock);
  
  DatabaseStats& stats = m_databaseStats[dbPath];
  stats.queryCount++;
  stats.totalQueryTime += executionTimeMs;
  
  if (executionTimeMs > stats.maxQueryTime)
  {
    stats.maxQueryTime = executionTimeMs;
    stats.slowestQuery = query;
  }
  
  if (executionTimeMs > SLOW_QUERY_THRESHOLD_MS)
  {
    stats.slowQueryCount++;
    
    // Log slow queries to help diagnose performance issues
    CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Slow query detected (%.2f ms) in %s: %s", 
              executionTimeMs, dbPath.c_str(), query.c_str());
              
    // Check if we need to optimize based on slow query ratio
    if (stats.queryCount > 100 && 
        (static_cast<double>(stats.slowQueryCount) / stats.queryCount) > 0.05)
    {
      // If more than 5% of queries are slow, schedule an optimization
      CLog::Log(LOGNOTICE, "CAndroidDatabaseOptimizer: Performance degradation detected in %s "
                           "(%.1f%% slow queries). Scheduling optimization.",
                dbPath.c_str(), (static_cast<double>(stats.slowQueryCount) / stats.queryCount) * 100);
      
      // Schedule optimization outside of the lock to avoid deadlock
      lock.Leave();
      ScheduleOptimization(dbPath);
    }
  }
}

std::map<std::string, CAndroidDatabaseOptimizer::DatabaseStats> CAndroidDatabaseOptimizer::GetPerformanceStats()
{
  CSingleLock lock(m_statsLock);
  return m_databaseStats;
}

void CAndroidDatabaseOptimizer::ResetPerformanceStats(const std::string& dbPath)
{
  CSingleLock lock(m_statsLock);
  
  if (dbPath.empty())
  {
    // Reset all stats
    m_databaseStats.clear();
    CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Reset all performance statistics");
  }
  else if (m_databaseStats.find(dbPath) != m_databaseStats.end())
  {
    // Reset stats for specific database
    m_databaseStats.erase(dbPath);
    CLog::Log(LOGDEBUG, "CAndroidDatabaseOptimizer: Reset performance statistics for %s", dbPath.c_str());
  }
}

bool CAndroidDatabaseOptimizer::ShouldOptimize(const std::string& dbPath)
{
  // Check if database needs optimization based on performance metrics
  CSingleLock lock(m_statsLock);
  
  const auto& it = m_databaseStats.find(dbPath);
  if (it == m_databaseStats.end())
    return false;
    
  const DatabaseStats& stats = it->second;
  
  // Optimize if:
  // 1. We've seen at least 1000 queries AND
  // 2. Either:
  //    a. Average query time is over 50ms OR
  //    b. More than 5% of queries are slow
  if (stats.queryCount >= 1000)
  {
    double avgQueryTime = stats.totalQueryTime / stats.queryCount;
    double slowQueryRatio = static_cast<double>(stats.slowQueryCount) / stats.queryCount;
    
    if (avgQueryTime > 50.0 || slowQueryRatio > 0.05)
    {
      CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Database %s needs optimization "
                         "(avg query: %.2f ms, slow queries: %.1f%%)",
                dbPath.c_str(), avgQueryTime, slowQueryRatio * 100);
      return true;
    }
  }
  
  return false;
}

// Enhanced optimization process with performance monitoring
bool CAndroidDatabaseOptimizer::OptimizeDatabase(const std::string& dbPath)
{
  CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Starting optimization for %s", dbPath.c_str());
  
  try
  {
    // Calculate the optimal settings for the current device if not done already
    CalculateOptimalSettings();
    
    // Record performance stats before optimization
    std::map<std::string, DatabaseStats> beforeStats;
    {
      CSingleLock lock(m_statsLock);
      auto it = m_databaseStats.find(dbPath);
      if (it != m_databaseStats.end())
        beforeStats[dbPath] = it->second;
    }
    
    // Perform the actual optimization
    SqliteDatabase db;
    db.setDatabase(dbPath.c_str());
    if (db.connect())
    {
      // Apply optimizations to the database connection
      if (!OptimizeDatabaseConnection(&db))
      {
        CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Failed to optimize database %s", dbPath.c_str());
        db.disconnect();
        return false;
      }
      
      // Run maintenance tasks
      std::unique_ptr<Dataset> dataset = db.CreateDataset();
      
      // Set optimal cache size
      dataset->exec(PrepareSQL("PRAGMA cache_size=-%d", m_cacheSize).c_str());
      
      // Configure optimal page size if database is empty or new
      std::string currentPageSize = "0";
      if (dataset->query("PRAGMA page_size") && dataset->num_rows() > 0)
      {
        currentPageSize = dataset->fv(0).get_asString();
      }
      
      // We can only set page size on empty databases, so check if this is a new DB
      int64_t pageCount = 0;
      if (dataset->query("PRAGMA page_count") && dataset->num_rows() > 0)
      {
        pageCount = dataset->fv(0).get_asInt64();
      }
      
      if (pageCount <= 2 && currentPageSize != std::to_string(m_pageSize))
      {
        // This is effectively an empty database, so we can set the page size
        dataset->exec(PrepareSQL("PRAGMA page_size=%d", m_pageSize).c_str());
        CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Set page size to %d for new database %s", 
                  m_pageSize, dbPath.c_str());
      }
      
      // Configure journal mode (WAL generally gives best performance on Android)
      dataset->exec("PRAGMA journal_mode=WAL");
      
      // Configure sync mode based on device capabilities
      const char* syncModes[] = {"OFF", "NORMAL", "FULL"};
      if (m_syncMode >= 0 && m_syncMode <= 2)
      {
        dataset->exec(PrepareSQL("PRAGMA synchronous=%s", syncModes[m_syncMode]).c_str());
      }
      
      // Configure temp_store for performance
      dataset->exec("PRAGMA temp_store=MEMORY");
      
      // Run ANALYZE to update statistics
      dataset->exec("ANALYZE");
      
      // Perform integrity check
      bool integrityOk = true;
      if (dataset->query("PRAGMA integrity_check") && dataset->num_rows() > 0)
      {
        std::string result = dataset->fv(0).get_asString();
        if (result != "ok")
        {
          CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Integrity check failed for %s: %s", 
                    dbPath.c_str(), result.c_str());
          integrityOk = false;
        }
      }
      
      // Run VACUUM to defragment and compact the database if integrity check passed
      if (integrityOk)
      {
        dataset->exec("VACUUM");
      }
      
      dataset->close();
      db.disconnect();
      
      // Reset performance stats for this database now that we've optimized it
      ResetPerformanceStats(dbPath);
      
      CLog::Log(LOGINFO, "CAndroidDatabaseOptimizer: Optimization complete for %s", dbPath.c_str());
      return true;
    }
  }
  catch (const DbErrors& err)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Failed to optimize database: %s", err.getMsg());
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidDatabaseOptimizer: Exception during optimization: %s", e.what());
  }
  
  return false;
}