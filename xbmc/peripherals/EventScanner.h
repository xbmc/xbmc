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

#pragma once

#include <set>

#include "EventLockHandle.h"
#include "EventPollHandle.h"
#include "PeripheralTypes.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"

namespace PERIPHERALS
{
  class IEventScannerCallback;

  /*!
   * \brief Class to scan for peripheral events
   *
   * By default, a rate of 60 Hz is used. A client can obtain control over when
   * input is handled by registering for a polling handle.
   */
  class CEventScanner : public IEventPollCallback,
                        public IEventLockCallback,
                        protected CThread
  {
  public:
    explicit CEventScanner(IEventScannerCallback &callback);

    ~CEventScanner() override = default;

    void Start();
    void Stop();

    EventPollHandlePtr RegisterPollHandle();

    /*!
     * \brief Acquire a lock that prevents event processing while held
     */
    EventLockHandlePtr RegisterLock();

    // implementation of IEventPollCallback
    void Activate(CEventPollHandle &handle) override;
    void Deactivate(CEventPollHandle &handle) override;
    void HandleEvents(bool bWait) override;
    void Release(CEventPollHandle &handle) override;

    // implementation of IEventLockCallback
    void ReleaseLock(CEventLockHandle &handle) override;

  protected:
    // implementation of CThread
    void Process() override;

  private:
    double GetScanIntervalMs() const;

    // Construction parameters
    IEventScannerCallback &m_callback;

    // Event parameters
    std::set<void*> m_activeHandles;
    std::set<void*> m_activeLocks;
    CEvent m_scanEvent;
    CEvent m_scanFinishedEvent;
    CCriticalSection m_handleMutex;
    CCriticalSection m_lockMutex;
    CCriticalSection m_pollMutex; // Prevent two poll handles from polling at once
  };
}
