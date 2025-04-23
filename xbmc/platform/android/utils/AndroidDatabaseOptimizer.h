/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DBUtils.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <string>
#include <atomic>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>

class CAndroidDatabaseOptimizer : public CThread
{
public:
  static CAndroidDatabaseOptimizer& GetInstance();
  
  void ScheduleOptimization(const std::string& dbPath);
  void ClearScheduledOptimizations();

  bool OptimizeDatabase(const std::string& dbPath);
  bool OptimizeDatabaseConnection(SqliteDatabase* db);
  
  void OnLowMemory();
  bool IsLowEndDevice() const;
  
  // Configuration methods
  void SetMemoryCacheSize(int sizeKB);
  void CalculateOptimalSettings();
  
  // Database maintenance methods
  void PerformMaintenance(const std::string& dbPath, bool full = false);
  void ForceWALCheckpoint(const std::string& dbPath);
  void SetSynchronousMode(const std::string& dbPath, int synchronousMode);
  
  // Memory management
  void ClearCache();
  
  // Query execution
  std::string ExecutePragma(const std::string& dbPath, const std::string& pragma);
  
  // Performance tracking methods
  struct DatabaseStats
  {
    DatabaseStats() : queryCount(0), slowQueryCount(0), totalQueryTime(0.0), maxQueryTime(0.0) {}
    
    int64_t queryCount;       // Total number of queries executed
    int64_t slowQueryCount;   // Number of queries that exceeded threshold
    double totalQueryTime;    // Total time spent executing queries (ms)
    double maxQueryTime;      // Duration of the slowest query (ms)
    std::string slowestQuery; // The text of the slowest query
  };
  
  void RecordQueryExecution(const std::string& dbPath, const std::string& query, double executionTimeMs);
  std::map<std::string, DatabaseStats> GetPerformanceStats();
  void ResetPerformanceStats(const std::string& dbPath = "");
  bool ShouldOptimize(const std::string& dbPath);

protected:
  CAndroidDatabaseOptimizer();
  ~CAndroidDatabaseOptimizer() override;
  
  // Thread implementation
  void Process() override;
  
private:
  static constexpr unsigned int DEFAULT_OPTIMIZE_INTERVAL_MS = 30 * 60 * 1000; // 30 minutes
  
  // Device-dependent optimization settings
  int m_pageSize;
  int m_cacheSize;
  int m_journalMode;  // 0=DELETE, 1=TRUNCATE, 2=PERSIST, 3=WAL, 4=MEMORY
  int m_syncMode;     // 0=OFF, 1=NORMAL, 2=FULL
  int m_tempStore;    // 0=DEFAULT, 1=FILE, 2=MEMORY
  
  CCriticalSection m_optimizationLock;
  std::vector<std::string> m_scheduledOptimizations;
  
  // Performance metrics
  std::map<std::string, DatabaseStats> m_databaseStats;
  CCriticalSection m_statsLock;
};