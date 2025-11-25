/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Performance metrics for semantic search operations
 */
struct PerformanceMetrics
{
  // Timing metrics (milliseconds)
  double avgEmbeddingTime{0.0};
  double avgSearchTime{0.0};
  double avgKeywordSearchTime{0.0};
  double avgVectorSearchTime{0.0};
  double avgHybridSearchTime{0.0};
  double maxEmbeddingTime{0.0};
  double maxSearchTime{0.0};

  // Memory metrics (bytes)
  size_t currentMemoryUsage{0};
  size_t peakMemoryUsage{0};
  size_t modelMemoryUsage{0};
  size_t cacheMemoryUsage{0};
  size_t indexMemoryUsage{0};

  // Throughput counters
  uint64_t totalEmbeddings{0};
  uint64_t totalSearches{0};
  uint64_t totalCacheHits{0};
  uint64_t totalCacheMisses{0};
  uint64_t batchedEmbeddings{0};

  // Operation counts
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

  // Timestamp
  std::chrono::system_clock::time_point startTime;
  std::chrono::system_clock::time_point lastReset;
};

/*!
 * @brief Performance monitor for semantic search operations
 *
 * This singleton class provides comprehensive performance monitoring for all
 * semantic search operations including:
 * - Timing metrics for embeddings and searches
 * - Memory usage tracking
 * - Throughput and cache statistics
 * - JSON export for analysis
 *
 * Thread-safe for concurrent access from multiple threads.
 */
class CPerformanceMonitor
{
public:
  /*!
   * @brief Get the singleton instance
   * @return Reference to the performance monitor instance
   */
  static CPerformanceMonitor& GetInstance();

  /*!
   * @brief Initialize the performance monitor
   * @param enableLogging Enable file logging of metrics
   * @param logPath Path to log file (optional)
   * @return true if initialization succeeded
   */
  bool Initialize(bool enableLogging = false, const std::string& logPath = "");

  /*!
   * @brief Shutdown and cleanup the performance monitor
   */
  void Shutdown();

  /*!
   * @brief Check if monitoring is enabled
   * @return true if monitoring is active
   */
  bool IsEnabled() const { return m_enabled.load(); }

  /*!
   * @brief Enable or disable monitoring
   * @param enabled true to enable, false to disable
   */
  void SetEnabled(bool enabled) { m_enabled.store(enabled); }

  // ===== Timing Measurements =====

  /*!
   * @brief RAII timing helper
   */
  class Timer
  {
  public:
    Timer(const std::string& operation);
    ~Timer();

    double ElapsedMs() const;

  private:
    std::string m_operation;
    std::chrono::steady_clock::time_point m_start;
  };

  /*!
   * @brief Record embedding operation timing
   * @param durationMs Duration in milliseconds
   * @param batchSize Number of texts embedded (1 for single embedding)
   */
  void RecordEmbedding(double durationMs, size_t batchSize = 1);

  /*!
   * @brief Record search operation timing
   * @param durationMs Duration in milliseconds
   * @param searchType Type of search (keyword, semantic, hybrid)
   */
  void RecordSearch(double durationMs, const std::string& searchType);

  /*!
   * @brief Record model load operation
   * @param durationMs Duration in milliseconds
   */
  void RecordModelLoad(double durationMs);

  /*!
   * @brief Record model unload operation
   */
  void RecordModelUnload();

  // ===== Memory Tracking =====

  /*!
   * @brief Update current memory usage
   * @param bytes Current memory usage in bytes
   * @param component Component name (model, cache, index, etc.)
   */
  void UpdateMemoryUsage(size_t bytes, const std::string& component);

  /*!
   * @brief Get current total memory usage
   * @return Current memory usage in bytes
   */
  size_t GetCurrentMemoryUsage() const;

  /*!
   * @brief Get peak memory usage
   * @return Peak memory usage in bytes
   */
  size_t GetPeakMemoryUsage() const;

  // ===== Cache Statistics =====

  /*!
   * @brief Record cache hit
   */
  void RecordCacheHit();

  /*!
   * @brief Record cache miss
   */
  void RecordCacheMiss();

  /*!
   * @brief Update cache size
   * @param size Current cache size (number of entries)
   * @param capacity Cache capacity (max entries)
   */
  void UpdateCacheStats(size_t size, size_t capacity);

  // ===== Metrics Retrieval =====

  /*!
   * @brief Get current performance metrics
   * @return Snapshot of all performance metrics
   */
  PerformanceMetrics GetMetrics() const;

  /*!
   * @brief Get metrics as JSON string
   * @return JSON-formatted metrics string
   */
  std::string GetMetricsJSON() const;

  /*!
   * @brief Log current metrics to file (if enabled)
   */
  void LogMetrics();

  /*!
   * @brief Reset all metrics to zero
   */
  void ResetMetrics();

  // ===== Detailed Operation Tracking =====

  /*!
   * @brief Get detailed timing breakdown
   * @return Map of operation names to timing statistics
   */
  std::map<std::string, std::vector<double>> GetDetailedTimings() const;

  /*!
   * @brief Get throughput statistics
   * @return Map of throughput metrics (embeddings/sec, searches/sec, etc.)
   */
  std::map<std::string, double> GetThroughputStats() const;

private:
  CPerformanceMonitor() = default;
  ~CPerformanceMonitor() = default;

  // Prevent copying
  CPerformanceMonitor(const CPerformanceMonitor&) = delete;
  CPerformanceMonitor& operator=(const CPerformanceMonitor&) = delete;

  // Helper functions
  void UpdateAverageAndMax(double& avg, double& max, uint64_t& count, double newValue);
  std::string FormatBytes(size_t bytes) const;
  double GetElapsedSeconds() const;

  // State
  std::atomic<bool> m_enabled{false};
  std::atomic<bool> m_loggingEnabled{false};
  std::string m_logPath;

  // Metrics (protected by mutex)
  mutable std::mutex m_mutex;
  PerformanceMetrics m_metrics;

  // Component memory breakdown
  std::map<std::string, size_t> m_componentMemory;

  // Detailed timing data (optional, for analysis)
  std::map<std::string, std::vector<double>> m_detailedTimings;
  bool m_collectDetailedTimings{false};
};

/*!
 * @brief RAII timer for automatic performance measurement
 *
 * Usage:
 * {
 *   PERF_TIMER("embedding");
 *   // ... code to measure ...
 * } // Timer automatically records on destruction
 */
#define PERF_TIMER(operation) \
  KODI::SEMANTIC::CPerformanceMonitor::Timer _perfTimer##__LINE__(operation)

/*!
 * @brief Conditional performance timer (only if monitoring is enabled)
 */
#define PERF_TIMER_COND(operation)                                              \
  std::unique_ptr<KODI::SEMANTIC::CPerformanceMonitor::Timer> _perfTimer##__LINE__; \
  if (KODI::SEMANTIC::CPerformanceMonitor::GetInstance().IsEnabled())          \
  {                                                                             \
    _perfTimer##__LINE__ =                                                     \
        std::make_unique<KODI::SEMANTIC::CPerformanceMonitor::Timer>(operation); \
  }

} // namespace SEMANTIC
} // namespace KODI
