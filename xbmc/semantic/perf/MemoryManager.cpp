/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MemoryManager.h"

#include "PerformanceMonitor.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>

#ifdef TARGET_POSIX
#include <sys/sysinfo.h>
#include <unistd.h>
#elif defined(TARGET_WINDOWS)
#include <windows.h>
#endif

using namespace KODI::SEMANTIC;

CMemoryManager& CMemoryManager::GetInstance()
{
  static CMemoryManager instance;
  return instance;
}

bool CMemoryManager::Initialize(size_t maxMemoryMB)
{
  if (m_initialized.load())
  {
    CLog::Log(LOGWARNING, "MemoryManager: Already initialized");
    return true;
  }

  m_maxMemoryBytes.store(maxMemoryMB * 1024 * 1024);
  m_initialized.store(true);

  // Log system memory info if available
  size_t totalMem = 0, availableMem = 0;
  if (GetSystemMemoryInfo(totalMem, availableMem))
  {
    CLog::Log(LOGINFO,
              "MemoryManager: Initialized with limit={}MB, system total={}, available={}",
              maxMemoryMB, FormatMemorySize(totalMem), FormatMemorySize(availableMem));
  }
  else
  {
    CLog::Log(LOGINFO, "MemoryManager: Initialized with limit={}MB", maxMemoryMB);
  }

  return true;
}

void CMemoryManager::Shutdown()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_pressureCallbacks.clear();
  m_componentMemory.clear();
  m_totalMemory = 0;
  m_initialized.store(false);

  CLog::Log(LOGINFO, "MemoryManager: Shutdown complete");
}

void CMemoryManager::RegisterAllocation(size_t bytes, const std::string& component)
{
  if (!m_initialized.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  m_componentMemory[component] += bytes;
  m_totalMemory += bytes;

  if (m_totalMemory > m_peakMemory)
  {
    m_peakMemory = m_totalMemory;
  }

  // Update performance monitor
  auto& perfMon = CPerformanceMonitor::GetInstance();
  if (perfMon.IsEnabled())
  {
    perfMon.UpdateMemoryUsage(m_componentMemory[component], component);
  }

  // Check memory pressure
  UpdateMemoryPressure();
}

void CMemoryManager::RegisterDeallocation(size_t bytes, const std::string& component)
{
  if (!m_initialized.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_componentMemory.find(component);
  if (it != m_componentMemory.end())
  {
    if (it->second >= bytes)
    {
      it->second -= bytes;
      m_totalMemory -= bytes;
    }
    else
    {
      // Deallocation exceeds tracked allocation (shouldn't happen)
      CLog::Log(LOGWARNING,
                "MemoryManager: Deallocation ({} bytes) exceeds tracked allocation for {}",
                bytes, component);
      m_totalMemory -= it->second;
      it->second = 0;
    }

    // Update performance monitor
    auto& perfMon = CPerformanceMonitor::GetInstance();
    if (perfMon.IsEnabled())
    {
      perfMon.UpdateMemoryUsage(it->second, component);
    }
  }

  // Check memory pressure
  UpdateMemoryPressure();
}

void CMemoryManager::UpdateComponentMemory(size_t bytes, const std::string& component)
{
  if (!m_initialized.load())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  // Calculate delta
  size_t oldBytes = m_componentMemory[component];
  m_componentMemory[component] = bytes;

  if (bytes > oldBytes)
  {
    m_totalMemory += (bytes - oldBytes);
  }
  else
  {
    m_totalMemory -= (oldBytes - bytes);
  }

  if (m_totalMemory > m_peakMemory)
  {
    m_peakMemory = m_totalMemory;
  }

  // Update performance monitor
  auto& perfMon = CPerformanceMonitor::GetInstance();
  if (perfMon.IsEnabled())
  {
    perfMon.UpdateMemoryUsage(bytes, component);
  }

  // Check memory pressure
  UpdateMemoryPressure();
}

size_t CMemoryManager::GetTotalMemoryUsage() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_totalMemory;
}

size_t CMemoryManager::GetComponentMemory(const std::string& component) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_componentMemory.find(component);
  return (it != m_componentMemory.end()) ? it->second : 0;
}

MemoryStats CMemoryManager::GetStats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  MemoryStats stats;
  stats.totalAllocated = m_totalMemory;
  stats.modelMemory = GetComponentMemory("model");
  stats.cacheMemory = GetComponentMemory("cache");
  stats.indexMemory = GetComponentMemory("index");
  stats.pressure = m_currentPressure;

  // Calculate other memory
  stats.otherMemory = stats.totalAllocated - stats.modelMemory - stats.cacheMemory - stats.indexMemory;

  // Get system available memory if possible
  size_t totalSys = 0, availableSys = 0;
  if (GetSystemMemoryInfo(totalSys, availableSys))
  {
    stats.systemAvailable = availableSys;
  }

  return stats;
}

int CMemoryManager::RegisterPressureCallback(MemoryPressureCallback callback,
                                              const std::string& component)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  int id = m_nextCallbackId.fetch_add(1);
  m_pressureCallbacks.push_back({id, component, callback});

  CLog::Log(LOGDEBUG, "MemoryManager: Registered pressure callback for {}", component);
  return id;
}

void CMemoryManager::UnregisterPressureCallback(int callbackId)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = std::remove_if(m_pressureCallbacks.begin(), m_pressureCallbacks.end(),
                           [callbackId](const PressureCallbackEntry& entry) {
                             return entry.id == callbackId;
                           });

  if (it != m_pressureCallbacks.end())
  {
    m_pressureCallbacks.erase(it, m_pressureCallbacks.end());
    CLog::Log(LOGDEBUG, "MemoryManager: Unregistered pressure callback {}", callbackId);
  }
}

MemoryPressure CMemoryManager::GetMemoryPressure() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_currentPressure;
}

size_t CMemoryManager::TriggerPressureCallbacks(MemoryPressure pressure)
{
  // Make a copy of callbacks to avoid holding lock during callbacks
  std::vector<PressureCallbackEntry> callbacks;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    callbacks = m_pressureCallbacks;
  }

  size_t totalFreed = 0;

  for (const auto& entry : callbacks)
  {
    try
    {
      size_t freed = entry.callback(pressure);
      totalFreed += freed;

      if (freed > 0)
      {
        CLog::Log(LOGINFO, "MemoryManager: {} freed {} on pressure level {}", entry.component,
                  FormatMemorySize(freed), static_cast<int>(pressure));
      }
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "MemoryManager: Callback for {} threw exception: {}", entry.component,
                e.what());
    }
  }

  if (totalFreed > 0)
  {
    CLog::Log(LOGINFO, "MemoryManager: Total freed from pressure callbacks: {}",
              FormatMemorySize(totalFreed));
  }

  return totalFreed;
}

void CMemoryManager::SetMemoryLimit(size_t maxBytes)
{
  m_maxMemoryBytes.store(maxBytes);
  CLog::Log(LOGINFO, "MemoryManager: Memory limit set to {}", FormatMemorySize(maxBytes));

  // Check if we need to trigger cleanup
  if (m_autoCleanup.load() && IsOverLimit())
  {
    UpdateMemoryPressure();
  }
}

bool CMemoryManager::IsOverLimit() const
{
  size_t limit = m_maxMemoryBytes.load();
  if (limit == 0)
    return false; // No limit set

  return GetTotalMemoryUsage() > limit;
}

bool CMemoryManager::CanAllocate(size_t bytes) const
{
  size_t limit = m_maxMemoryBytes.load();
  if (limit == 0)
    return true; // No limit set

  return (GetTotalMemoryUsage() + bytes) <= limit;
}

size_t CMemoryManager::RequestCleanup(size_t bytesNeeded)
{
  MemoryPressure pressure = MemoryPressure::Medium;

  if (IsOverLimit() || bytesNeeded > 0)
  {
    pressure = MemoryPressure::High;
  }

  CLog::Log(LOGINFO, "MemoryManager: Cleanup requested (need: {})", FormatMemorySize(bytesNeeded));

  return TriggerPressureCallbacks(pressure);
}

bool CMemoryManager::GetSystemMemoryInfo(size_t& totalMemory, size_t& availableMemory)
{
#ifdef TARGET_POSIX
  struct sysinfo info;
  if (sysinfo(&info) == 0)
  {
    totalMemory = info.totalram * info.mem_unit;
    availableMemory = info.freeram * info.mem_unit;
    return true;
  }
#elif defined(TARGET_WINDOWS)
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  if (GlobalMemoryStatusEx(&statex))
  {
    totalMemory = statex.ullTotalPhys;
    availableMemory = statex.ullAvailPhys;
    return true;
  }
#endif

  return false;
}

std::string CMemoryManager::FormatMemorySize(size_t bytes)
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

void CMemoryManager::UpdateMemoryPressure()
{
  MemoryPressure newPressure = CalculatePressureLevel();

  if (newPressure != m_currentPressure)
  {
    MemoryPressure oldPressure = m_currentPressure;
    m_currentPressure = newPressure;

    CLog::Log(LOGINFO, "MemoryManager: Pressure level changed from {} to {}",
              static_cast<int>(oldPressure), static_cast<int>(newPressure));

    // Trigger callbacks if pressure increased and auto-cleanup is enabled
    if (m_autoCleanup.load() && newPressure > oldPressure)
    {
      // Unlock before triggering callbacks to avoid deadlock
      m_mutex.unlock();
      TriggerPressureCallbacks(newPressure);
      m_mutex.lock();
    }
  }
}

MemoryPressure CMemoryManager::CalculatePressureLevel() const
{
  size_t limit = m_maxMemoryBytes.load();
  if (limit == 0)
    return MemoryPressure::Low; // No limit set

  float usage = static_cast<float>(m_totalMemory) / static_cast<float>(limit);

  if (usage >= 0.95f)
    return MemoryPressure::Critical;
  else if (usage >= 0.85f)
    return MemoryPressure::High;
  else if (usage >= 0.70f)
    return MemoryPressure::Medium;
  else
    return MemoryPressure::Low;
}
