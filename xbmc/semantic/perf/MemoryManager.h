/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief Memory pressure levels
 */
enum class MemoryPressure
{
  Low,     //!< Normal operation, plenty of memory available
  Medium,  //!< Approaching memory limits, consider cleanup
  High,    //!< Critical memory usage, aggressive cleanup needed
  Critical //!< Out of memory condition, emergency measures
};

/*!
 * @brief Memory pressure callback function
 *
 * Called when memory pressure increases. The callback should:
 * - Free unnecessary memory
 * - Clear caches
 * - Unload unused resources
 *
 * @param pressure Current memory pressure level
 * @return Amount of memory freed in bytes
 */
using MemoryPressureCallback = std::function<size_t(MemoryPressure pressure)>;

/*!
 * @brief Memory statistics
 */
struct MemoryStats
{
  size_t totalAllocated{0};   //!< Total memory allocated (bytes)
  size_t modelMemory{0};      //!< Memory used by embedding model
  size_t cacheMemory{0};      //!< Memory used by caches
  size_t indexMemory{0};      //!< Memory used by vector index
  size_t otherMemory{0};      //!< Other allocations
  size_t systemAvailable{0};  //!< Available system memory (if available)
  MemoryPressure pressure{MemoryPressure::Low};
};

/*!
 * @brief Memory manager for semantic search system
 *
 * Provides centralized memory management with:
 * - Memory usage tracking
 * - Memory pressure detection
 * - Automatic cleanup callbacks
 * - Memory limits enforcement
 *
 * Singleton pattern for global access.
 */
class CMemoryManager
{
public:
  /*!
   * @brief Get the singleton instance
   * @return Reference to the memory manager
   */
  static CMemoryManager& GetInstance();

  /*!
   * @brief Initialize the memory manager
   * @param maxMemoryMB Maximum memory usage in megabytes (0 = no limit)
   * @return true if initialization succeeded
   */
  bool Initialize(size_t maxMemoryMB = 200);

  /*!
   * @brief Shutdown the memory manager
   */
  void Shutdown();

  /*!
   * @brief Check if manager is initialized
   * @return true if initialized
   */
  bool IsInitialized() const { return m_initialized.load(); }

  // ===== Memory Tracking =====

  /*!
   * @brief Register memory allocation
   * @param bytes Number of bytes allocated
   * @param component Component name (model, cache, index, etc.)
   */
  void RegisterAllocation(size_t bytes, const std::string& component);

  /*!
   * @brief Register memory deallocation
   * @param bytes Number of bytes freed
   * @param component Component name
   */
  void RegisterDeallocation(size_t bytes, const std::string& component);

  /*!
   * @brief Update component memory usage (replaces previous value)
   * @param bytes Current memory usage
   * @param component Component name
   */
  void UpdateComponentMemory(size_t bytes, const std::string& component);

  /*!
   * @brief Get total memory usage
   * @return Current memory usage in bytes
   */
  size_t GetTotalMemoryUsage() const;

  /*!
   * @brief Get component memory usage
   * @param component Component name
   * @return Memory usage in bytes
   */
  size_t GetComponentMemory(const std::string& component) const;

  /*!
   * @brief Get memory statistics
   * @return Current memory statistics
   */
  MemoryStats GetStats() const;

  // ===== Memory Pressure Management =====

  /*!
   * @brief Register a memory pressure callback
   * @param callback Function to call when memory pressure increases
   * @param component Component name (for logging)
   * @return Callback ID for later removal
   */
  int RegisterPressureCallback(MemoryPressureCallback callback, const std::string& component);

  /*!
   * @brief Unregister a memory pressure callback
   * @param callbackId ID returned from RegisterPressureCallback
   */
  void UnregisterPressureCallback(int callbackId);

  /*!
   * @brief Get current memory pressure level
   * @return Current pressure level
   */
  MemoryPressure GetMemoryPressure() const;

  /*!
   * @brief Manually trigger memory pressure callbacks
   * @param pressure Pressure level to simulate
   * @return Total amount of memory freed in bytes
   */
  size_t TriggerPressureCallbacks(MemoryPressure pressure);

  // ===== Memory Limits =====

  /*!
   * @brief Set maximum memory limit
   * @param maxBytes Maximum memory in bytes (0 = no limit)
   */
  void SetMemoryLimit(size_t maxBytes);

  /*!
   * @brief Get current memory limit
   * @return Memory limit in bytes (0 = no limit)
   */
  size_t GetMemoryLimit() const { return m_maxMemoryBytes.load(); }

  /*!
   * @brief Check if memory usage exceeds limit
   * @return true if over limit
   */
  bool IsOverLimit() const;

  /*!
   * @brief Check if allocation would exceed limit
   * @param bytes Size of proposed allocation
   * @return true if allocation would fit within limit
   */
  bool CanAllocate(size_t bytes) const;

  // ===== Automatic Memory Management =====

  /*!
   * @brief Request memory cleanup
   * @param bytesNeeded Minimum bytes to free (0 = as much as possible)
   * @return Actual bytes freed
   */
  size_t RequestCleanup(size_t bytesNeeded = 0);

  /*!
   * @brief Enable automatic memory management
   * @param enabled true to enable automatic cleanup
   */
  void SetAutoCleanup(bool enabled) { m_autoCleanup.store(enabled); }

  /*!
   * @brief Check if automatic cleanup is enabled
   * @return true if enabled
   */
  bool IsAutoCleanupEnabled() const { return m_autoCleanup.load(); }

  // ===== Utility Functions =====

  /*!
   * @brief Get system memory information (if available)
   * @param totalMemory Output: Total system memory in bytes
   * @param availableMemory Output: Available system memory in bytes
   * @return true if information is available
   */
  static bool GetSystemMemoryInfo(size_t& totalMemory, size_t& availableMemory);

  /*!
   * @brief Format bytes as human-readable string
   * @param bytes Number of bytes
   * @return Formatted string (e.g., "150.5 MB")
   */
  static std::string FormatMemorySize(size_t bytes);

private:
  CMemoryManager() = default;
  ~CMemoryManager() = default;

  // Prevent copying
  CMemoryManager(const CMemoryManager&) = delete;
  CMemoryManager& operator=(const CMemoryManager&) = delete;

  // Internal helpers
  void UpdateMemoryPressure();
  MemoryPressure CalculatePressureLevel() const;

  // State
  std::atomic<bool> m_initialized{false};
  std::atomic<bool> m_autoCleanup{true};
  std::atomic<size_t> m_maxMemoryBytes{0};

  // Memory tracking (protected by mutex)
  mutable std::mutex m_mutex;
  std::map<std::string, size_t> m_componentMemory;
  size_t m_totalMemory{0};
  size_t m_peakMemory{0};
  MemoryPressure m_currentPressure{MemoryPressure::Low};

  // Pressure callbacks
  struct PressureCallbackEntry
  {
    int id;
    std::string component;
    MemoryPressureCallback callback;
  };

  std::vector<PressureCallbackEntry> m_pressureCallbacks;
  std::atomic<int> m_nextCallbackId{1};
};

/*!
 * @brief RAII helper for memory allocation tracking
 *
 * Automatically registers allocation on construction and
 * deallocation on destruction.
 */
class CMemoryTracker
{
public:
  CMemoryTracker(size_t bytes, const std::string& component)
    : m_bytes(bytes), m_component(component)
  {
    CMemoryManager::GetInstance().RegisterAllocation(m_bytes, m_component);
  }

  ~CMemoryTracker()
  {
    CMemoryManager::GetInstance().RegisterDeallocation(m_bytes, m_component);
  }

  // Prevent copying
  CMemoryTracker(const CMemoryTracker&) = delete;
  CMemoryTracker& operator=(const CMemoryTracker&) = delete;

private:
  size_t m_bytes;
  std::string m_component;
};

} // namespace SEMANTIC
} // namespace KODI
