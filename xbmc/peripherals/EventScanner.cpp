/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "EventScanner.h"
#include "IEventScannerCallback.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

#include <algorithm>

using namespace PERIPHERALS;
using namespace XbmcThreads;

// Default event scan rate when no polling handles are held
#define DEFAULT_SCAN_RATE_HZ  60

// Timeout when a polling handle is held but doesn't trigger scan. This reduces
// input latency when the game is running at < 1/4 speed.
#define WATCHDOG_TIMEOUT_MS   80

CEventScanner::CEventScanner(IEventScannerCallback &callback) :
  CThread("PeripEventScanner"),
  m_callback(callback)
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
    CSingleLock lock(m_handleMutex);
    m_activeHandles.insert(handle.get());
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle registered");

  return handle;
}

void CEventScanner::Activate(CEventPollHandle &handle)
{
  {
    CSingleLock lock(m_handleMutex);
    m_activeHandles.insert(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle activated");
}

void CEventScanner::Deactivate(CEventPollHandle &handle)
{
  {
    CSingleLock lock(m_handleMutex);
    m_activeHandles.erase(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle deactivated");
}

void CEventScanner::HandleEvents(bool bWait)
{
  if (bWait)
  {
    CSingleLock lock(m_pollMutex);

    m_scanFinishedEvent.Reset();
    m_scanEvent.Set();
    m_scanFinishedEvent.Wait();
  }
  else
  {
    m_scanEvent.Set();
  }
}

void CEventScanner::Release(CEventPollHandle &handle)
{
  {
    CSingleLock lock(m_handleMutex);
    m_activeHandles.erase(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event poll handle released");
}

EventLockHandlePtr CEventScanner::RegisterLock()
{
  EventLockHandlePtr handle(new CEventLockHandle(*this));

  {
    CSingleLock lock(m_lockMutex);
    m_activeLocks.insert(handle.get());
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event lock handle registered");

  return handle;
}

void CEventScanner::ReleaseLock(CEventLockHandle &handle)
{
  {
    CSingleLock lock(m_lockMutex);
    m_activeLocks.erase(&handle);
  }

  CLog::Log(LOGDEBUG, "PERIPHERALS: Event lock handle released");
}

void CEventScanner::Process()
{
  double nextScanMs = static_cast<double>(SystemClockMillis());

  while (!m_bStop)
  {
    {
      CSingleLock lock(m_lockMutex);
      if (m_activeLocks.empty())
        m_callback.ProcessEvents();
    }

    m_scanFinishedEvent.Set();

    const double nowMs = static_cast<double>(SystemClockMillis());
    const double scanIntervalMs = GetScanIntervalMs();

    // Handle wrap-around
    if (nowMs < nextScanMs)
      nextScanMs = nowMs;

    while (nextScanMs <= nowMs)
      nextScanMs += scanIntervalMs;

    unsigned int waitTimeMs = static_cast<unsigned int>(nextScanMs - nowMs);

    if (!m_bStop && waitTimeMs > 0)
      m_scanEvent.WaitMSec(waitTimeMs);
  }
}

double CEventScanner::GetScanIntervalMs() const
{
  bool bHasActiveHandle;

  {
    CSingleLock lock(m_handleMutex);
    bHasActiveHandle = !m_activeHandles.empty();
  }

  if (!bHasActiveHandle)
    return 1000.0 / DEFAULT_SCAN_RATE_HZ;
  else
    return WATCHDOG_TIMEOUT_MS;
}
