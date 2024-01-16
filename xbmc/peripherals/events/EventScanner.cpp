/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventScanner.h"

#include "peripherals/events/interfaces/IEventScannerCallback.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>

using namespace PERIPHERALS;

// Default event scan rate when no polling handles are held
#define DEFAULT_SCAN_RATE_HZ 60

// Timeout when a polling handle is held but doesn't trigger scan. This reduces
// input latency when the game is running at < 1/4 speed.
#define WATCHDOG_TIMEOUT_MS 80

CEventScanner::CEventScanner(IEventScannerCallback& callback)
  : CThread("PeripEventScan"), m_callback(callback)
{
}

void CEventScanner::Start()
{
  Create();
}

void CEventScanner::Stop()
{
  StopThread(false);
  m_scanEvent.Set();
  StopThread(true);
}

EventPollHandlePtr CEventScanner::RegisterPollHandle()
{
  EventPollHandlePtr handle(new CEventPollHandle(*this));

  {
    std::unique_lock<CCriticalSection> lock(m_handleMutex);
    m_activeHandles.insert(handle.get());
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle registered");

  return handle;
}

void CEventScanner::Activate(CEventPollHandle& handle)
{
  {
    std::unique_lock<CCriticalSection> lock(m_handleMutex);
    m_activeHandles.insert(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle activated");
}

void CEventScanner::Deactivate(CEventPollHandle& handle)
{
  {
    std::unique_lock<CCriticalSection> lock(m_handleMutex);
    m_activeHandles.erase(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle deactivated");
}

void CEventScanner::HandleEvents(bool bWait)
{
  if (bWait)
  {
    std::unique_lock<CCriticalSection> lock(m_pollMutex);

    m_scanFinishedEvent.Reset();
    m_scanEvent.Set();
    m_scanFinishedEvent.Wait();
  }
  else
  {
    m_scanEvent.Set();
  }
}

void CEventScanner::Release(CEventPollHandle& handle)
{
  {
    std::unique_lock<CCriticalSection> lock(m_handleMutex);
    m_activeHandles.erase(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle released");
}

EventLockHandlePtr CEventScanner::RegisterLock()
{
  EventLockHandlePtr handle(new CEventLockHandle(*this));

  {
    std::unique_lock<CCriticalSection> lock(m_lockMutex);
    m_activeLocks.insert(handle.get());
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event lock handle registered");

  return handle;
}

void CEventScanner::ReleaseLock(CEventLockHandle& handle)
{
  {
    std::unique_lock<CCriticalSection> lock(m_lockMutex);
    m_activeLocks.erase(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event lock handle released");
}

void CEventScanner::Process()
{
  auto nextScan = std::chrono::steady_clock::now();

  while (!m_bStop)
  {
    {
      std::unique_lock<CCriticalSection> lock(m_lockMutex);
      if (m_activeLocks.empty())
        m_callback.ProcessEvents();
    }

    m_scanFinishedEvent.Set();

    auto now = std::chrono::steady_clock::now();
    auto scanIntervalMs = GetScanIntervalMs();

    // Handle wrap-around
    if (now < nextScan)
      nextScan = now;

    while (nextScan <= now)
      nextScan += scanIntervalMs;

    auto waitTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(nextScan - now);

    if (!m_bStop && waitTimeMs.count() > 0)
      m_scanEvent.Wait(waitTimeMs);
  }
}

std::chrono::milliseconds CEventScanner::GetScanIntervalMs() const
{
  bool bHasActiveHandle;

  {
    std::unique_lock<CCriticalSection> lock(m_handleMutex);
    bHasActiveHandle = !m_activeHandles.empty();
  }

  if (!bHasActiveHandle)
  {
    // this truncates to 16 (from 16.666) should it round up to 17 using std::nearbyint or should we use nanoseconds?
    return std::chrono::milliseconds(static_cast<uint32_t>(1000.0 / DEFAULT_SCAN_RATE_HZ));
  }
  else
    return std::chrono::milliseconds(WATCHDOG_TIMEOUT_MS);
}
