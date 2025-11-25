/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PerformanceMonitor.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace KODI::SEMANTIC;

// Timer implementation

CPerformanceMonitor::Timer::Timer(const std::string& operation)
  : m_operation(operation), m_start(std::chrono::steady_clock::now())
{
}

CPerformanceMonitor::Timer::~Timer()
{
  double elapsedMs = ElapsedMs();

  auto& monitor = CPerformanceMonitor::GetInstance();
  if (!monitor.IsEnabled())
    return;

  // Route to appropriate recording method based on operation type
  if (m_operation == "embedding" || m_operation.find("embed") != std::string::npos)
  {
    monitor.RecordEmbedding(elapsedMs);
  }
  else if (m_operation.find("search") != std::string::npos)
  {
    monitor.RecordSearch(elapsedMs, m_operation);
  }

  CLog::Log(LOGDEBUG, "PerformanceMonitor: {} took {:.2f}ms", m_operation, elapsedMs);
}

double CPerformanceMonitor::Timer::ElapsedMs() const
{
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(end - m_start).count();
}

// CPerformanceMonitor implementation

CPerformanceMonitor& CPerformanceMonitor::GetInstance()
{
  static CPerformanceMonitor instance;
  return instance;
}

bool CPerformanceMonitor::Initialize(bool enableLogging, const std::string& logPath)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_enabled.store(true);
  m_loggingEnabled.store(enableLogging);
  m_logPath = logPath;

  // Initialize timestamps
  m_metrics.startTime = std::chrono::system_clock::now();
  m_metrics.lastReset = m_metrics.startTime;

  CLog::Log(LOGINFO, "PerformanceMonitor: Initialized (logging={})", enableLogging);
  return true;
}

void CPerformanceMonitor::Shutdown()
{
  if (m_loggingEnabled.load())
  {
    LogMetrics();
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  m_enabled.store(false);
  CLog::Log(LOGINFO, "PerformanceMonitor: Shutdown complete");
}

void CPerformanceMonitor::RecordEmbedding(double durationMs, size_t batchSize)
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  // Update timing statistics
  m_metrics.totalEmbeddings += batchSize;
  if (batchSize > 1)
  {
    m_metrics.batchedEmbeddings += batchSize;
  }

  UpdateAverageAndMax(m_metrics.avgEmbeddingTime, m_metrics.maxEmbeddingTime,
                      m_metrics.totalEmbeddings, durationMs);

  // Store detailed timing if enabled
  if (m_collectDetailedTimings)
  {
    m_detailedTimings["embedding"].push_back(durationMs);
  }
}

void CPerformanceMonitor::RecordSearch(double durationMs, const std::string& searchType)
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  // Update general search statistics
  m_metrics.totalSearches++;
  UpdateAverageAndMax(m_metrics.avgSearchTime, m_metrics.maxSearchTime, m_metrics.totalSearches,
                      durationMs);

  // Update type-specific statistics
  if (searchType.find("keyword") != std::string::npos)
  {
    m_metrics.keywordOnlySearches++;
    UpdateAverageAndMax(m_metrics.avgKeywordSearchTime, m_metrics.maxSearchTime,
                        m_metrics.keywordOnlySearches, durationMs);
  }
  else if (searchType.find("semantic") != std::string::npos ||
           searchType.find("vector") != std::string::npos)
  {
    m_metrics.semanticOnlySearches++;
    UpdateAverageAndMax(m_metrics.avgVectorSearchTime, m_metrics.maxSearchTime,
                        m_metrics.semanticOnlySearches, durationMs);
  }
  else if (searchType.find("hybrid") != std::string::npos)
  {
    m_metrics.hybridSearches++;
    UpdateAverageAndMax(m_metrics.avgHybridSearchTime, m_metrics.maxSearchTime,
                        m_metrics.hybridSearches, durationMs);
  }

  // Store detailed timing if enabled
  if (m_collectDetailedTimings)
  {
    m_detailedTimings[searchType].push_back(durationMs);
  }
}

void CPerformanceMonitor::RecordModelLoad(double durationMs)
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  m_metrics.modelLoadCount++;
  m_metrics.totalModelLoadTime += durationMs;

  CLog::Log(LOGINFO, "PerformanceMonitor: Model loaded in {:.2f}ms", durationMs);
}

void CPerformanceMonitor::RecordModelUnload()
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  m_metrics.modelUnloadCount++;
  CLog::Log(LOGINFO, "PerformanceMonitor: Model unloaded");
}

void CPerformanceMonitor::UpdateMemoryUsage(size_t bytes, const std::string& component)
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  // Update component memory
  m_componentMemory[component] = bytes;

  // Calculate total memory usage
  size_t totalMemory = 0;
  for (const auto& [comp, mem] : m_componentMemory)
  {
    totalMemory += mem;
  }

  m_metrics.currentMemoryUsage = totalMemory;

  // Update peak if necessary
  if (totalMemory > m_metrics.peakMemoryUsage)
  {
    m_metrics.peakMemoryUsage = totalMemory;
  }

  // Update component-specific metrics
  if (component == "model")
  {
    m_metrics.modelMemoryUsage = bytes;
  }
  else if (component == "cache")
  {
    m_metrics.cacheMemoryUsage = bytes;
  }
  else if (component == "index")
  {
    m_metrics.indexMemoryUsage = bytes;
  }
}

size_t CPerformanceMonitor::GetCurrentMemoryUsage() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_metrics.currentMemoryUsage;
}

size_t CPerformanceMonitor::GetPeakMemoryUsage() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_metrics.peakMemoryUsage;
}

void CPerformanceMonitor::RecordCacheHit()
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);
  m_metrics.totalCacheHits++;

  // Update cache hit rate
  uint64_t total = m_metrics.totalCacheHits + m_metrics.totalCacheMisses;
  if (total > 0)
  {
    m_metrics.cacheHitRate =
        static_cast<float>(m_metrics.totalCacheHits) / static_cast<float>(total);
  }
}

void CPerformanceMonitor::RecordCacheMiss()
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);
  m_metrics.totalCacheMisses++;

  // Update cache hit rate
  uint64_t total = m_metrics.totalCacheHits + m_metrics.totalCacheMisses;
  if (total > 0)
  {
    m_metrics.cacheHitRate =
        static_cast<float>(m_metrics.totalCacheHits) / static_cast<float>(total);
  }
}

void CPerformanceMonitor::UpdateCacheStats(size_t size, size_t capacity)
{
  if (!m_enabled.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);
  m_metrics.cacheSize = size;
  m_metrics.cacheCapacity = capacity;
}

PerformanceMetrics CPerformanceMonitor::GetMetrics() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_metrics;
}

std::string CPerformanceMonitor::GetMetricsJSON() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::ostringstream json;
  json << std::fixed << std::setprecision(2);

  json << "{\n";
  json << "  \"timing\": {\n";
  json << "    \"avgEmbeddingMs\": " << m_metrics.avgEmbeddingTime << ",\n";
  json << "    \"avgSearchMs\": " << m_metrics.avgSearchTime << ",\n";
  json << "    \"avgKeywordSearchMs\": " << m_metrics.avgKeywordSearchTime << ",\n";
  json << "    \"avgVectorSearchMs\": " << m_metrics.avgVectorSearchTime << ",\n";
  json << "    \"avgHybridSearchMs\": " << m_metrics.avgHybridSearchTime << ",\n";
  json << "    \"maxEmbeddingMs\": " << m_metrics.maxEmbeddingTime << ",\n";
  json << "    \"maxSearchMs\": " << m_metrics.maxSearchTime << "\n";
  json << "  },\n";

  json << "  \"memory\": {\n";
  json << "    \"currentBytes\": " << m_metrics.currentMemoryUsage << ",\n";
  json << "    \"currentMB\": " << (m_metrics.currentMemoryUsage / 1024.0 / 1024.0) << ",\n";
  json << "    \"peakBytes\": " << m_metrics.peakMemoryUsage << ",\n";
  json << "    \"peakMB\": " << (m_metrics.peakMemoryUsage / 1024.0 / 1024.0) << ",\n";
  json << "    \"modelMB\": " << (m_metrics.modelMemoryUsage / 1024.0 / 1024.0) << ",\n";
  json << "    \"cacheMB\": " << (m_metrics.cacheMemoryUsage / 1024.0 / 1024.0) << ",\n";
  json << "    \"indexMB\": " << (m_metrics.indexMemoryUsage / 1024.0 / 1024.0) << "\n";
  json << "  },\n";

  json << "  \"throughput\": {\n";
  json << "    \"totalEmbeddings\": " << m_metrics.totalEmbeddings << ",\n";
  json << "    \"totalSearches\": " << m_metrics.totalSearches << ",\n";
  json << "    \"batchedEmbeddings\": " << m_metrics.batchedEmbeddings << ",\n";
  json << "    \"embeddingsPerSec\": "
       << (m_metrics.totalEmbeddings / std::max(1.0, GetElapsedSeconds())) << ",\n";
  json << "    \"searchesPerSec\": "
       << (m_metrics.totalSearches / std::max(1.0, GetElapsedSeconds())) << "\n";
  json << "  },\n";

  json << "  \"cache\": {\n";
  json << "    \"hits\": " << m_metrics.totalCacheHits << ",\n";
  json << "    \"misses\": " << m_metrics.totalCacheMisses << ",\n";
  json << "    \"hitRate\": " << (m_metrics.cacheHitRate * 100.0f) << ",\n";
  json << "    \"size\": " << m_metrics.cacheSize << ",\n";
  json << "    \"capacity\": " << m_metrics.cacheCapacity << "\n";
  json << "  },\n";

  json << "  \"operations\": {\n";
  json << "    \"keywordSearches\": " << m_metrics.keywordOnlySearches << ",\n";
  json << "    \"semanticSearches\": " << m_metrics.semanticOnlySearches << ",\n";
  json << "    \"hybridSearches\": " << m_metrics.hybridSearches << ",\n";
  json << "    \"modelLoads\": " << m_metrics.modelLoadCount << ",\n";
  json << "    \"modelUnloads\": " << m_metrics.modelUnloadCount << "\n";
  json << "  },\n";

  json << "  \"uptime\": {\n";
  json << "    \"seconds\": " << GetElapsedSeconds() << "\n";
  json << "  }\n";
  json << "}\n";

  return json.str();
}

void CPerformanceMonitor::LogMetrics()
{
  if (!m_loggingEnabled.load() || m_logPath.empty())
    return;

  try
  {
    std::ofstream logFile(m_logPath, std::ios::app);
    if (logFile.is_open())
    {
      logFile << GetMetricsJSON() << "\n";
      logFile.close();
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "PerformanceMonitor: Failed to write log: {}", e.what());
  }
}

void CPerformanceMonitor::ResetMetrics()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_metrics = PerformanceMetrics();
  m_metrics.startTime = std::chrono::system_clock::now();
  m_metrics.lastReset = m_metrics.startTime;

  m_detailedTimings.clear();

  CLog::Log(LOGINFO, "PerformanceMonitor: Metrics reset");
}

std::map<std::string, std::vector<double>> CPerformanceMonitor::GetDetailedTimings() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_detailedTimings;
}

std::map<std::string, double> CPerformanceMonitor::GetThroughputStats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  double elapsedSec = GetElapsedSeconds();
  if (elapsedSec < 0.001)
    elapsedSec = 1.0;

  std::map<std::string, double> stats;
  stats["embeddingsPerSec"] = m_metrics.totalEmbeddings / elapsedSec;
  stats["searchesPerSec"] = m_metrics.totalSearches / elapsedSec;
  stats["cacheHitRate"] = m_metrics.cacheHitRate;
  stats["batchRatio"] = m_metrics.totalEmbeddings > 0
                            ? static_cast<double>(m_metrics.batchedEmbeddings) /
                                  static_cast<double>(m_metrics.totalEmbeddings)
                            : 0.0;

  return stats;
}

void CPerformanceMonitor::UpdateAverageAndMax(double& avg,
                                               double& max,
                                               uint64_t& count,
                                               double newValue)
{
  // Update running average: new_avg = old_avg + (new_value - old_avg) / count
  avg = avg + (newValue - avg) / static_cast<double>(count);

  // Update max
  if (newValue > max)
  {
    max = newValue;
  }
}

std::string CPerformanceMonitor::FormatBytes(size_t bytes) const
{
  const char* units[] = {"B", "KB", "MB", "GB"};
  int unitIndex = 0;
  double size = static_cast<double>(bytes);

  while (size >= 1024.0 && unitIndex < 3)
  {
    size /= 1024.0;
    unitIndex++;
  }

  return StringUtils::Format("{:.2f} {}", size, units[unitIndex]);
}

double CPerformanceMonitor::GetElapsedSeconds() const
{
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration<double>(now - m_metrics.startTime).count();
}
