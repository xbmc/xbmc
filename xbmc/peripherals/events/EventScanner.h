/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "EventLockHandle.h"
#include "EventPollHandle.h"
#include "peripherals/PeripheralTypes.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <chrono>
#include <set>

namespace PERIPHERALS
{
class IEventScannerCallback;

/*!
 * \ingroup peripherals
 *
 * \brief Class to scan for peripheral events
 *
 * By default, a rate of 60 Hz is used. A client can obtain control over when
 * input is handled by registering for a polling handle.
 */
class CEventScanner : public IEventPollCallback, public IEventLockCallback, protected CThread
{
public:
  explicit CEventScanner(IEventScannerCallback& callback);

  ~CEventScanner() override = default;

  void Start();
  void Stop();

  EventPollHandlePtr RegisterPollHandle();

  /*!
   * \brief Acquire a lock that prevents event processing while held
   */
  EventLockHandlePtr RegisterLock();

  // implementation of IEventPollCallback
  void Activate(CEventPollHandle& handle) override;
  void Deactivate(CEventPollHandle& handle) override;
  void HandleEvents(bool bWait) override;
  void Release(CEventPollHandle& handle) override;

  // implementation of IEventLockCallback
  void ReleaseLock(CEventLockHandle& handle) override;

protected:
  // implementation of CThread
  void Process() override;

private:
  std::chrono::milliseconds GetScanIntervalMs() const;

  // Construction parameters
  IEventScannerCallback& m_callback;

  // Event parameters
  std::set<void*> m_activeHandles;
  std::set<void*> m_activeLocks;
  CEvent m_scanEvent;
  CEvent m_scanFinishedEvent;
  mutable CCriticalSection m_handleMutex;
  CCriticalSection m_lockMutex;
  CCriticalSection m_pollMutex; // Prevent two poll handles from polling at once
};
} // namespace PERIPHERALS
