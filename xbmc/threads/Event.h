/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Condition.h"
#include "threads/SingleLock.h"

#include <initializer_list>
#include <memory>
#include <vector>

// forward declare the CEventGroup
namespace XbmcThreads
{
class CEventGroup;
}

/**
 * @brief This is an Event class built from a ConditionVariable. The Event adds the state
 *        that the condition is gating as well as the mutex/lock.
 *
 *        This Event can be 'interruptible' (even though there is only a single place
 *        in the code that uses this behavior).
 *
 *        This class manages 'spurious returns' from the condition variable.
 *
 */

class CEvent
{
  bool manualReset;
  volatile bool signaled;
  unsigned int numWaits = 0;

  CCriticalSection groupListMutex; // lock for the groups list
  std::unique_ptr<std::vector<XbmcThreads::CEventGroup*>> groups;

  /**
   * To satisfy the TightConditionVariable requirements and allow the
   *  predicate being monitored to include both the signaled and interrupted
   *  states.
   */
  XbmcThreads::ConditionVariable actualCv;
  XbmcThreads::TightConditionVariable<volatile bool&> condVar;
  CCriticalSection mutex;

  friend class XbmcThreads::CEventGroup;

  void addGroup(XbmcThreads::CEventGroup* group);
  void removeGroup(XbmcThreads::CEventGroup* group);

  // helper for the two wait methods
  inline bool prepReturn()
  {
    bool ret = signaled;
    if (!manualReset && numWaits == 0)
      signaled = false;
    return ret;
  }

  CEvent(const CEvent&) = delete;
  CEvent& operator=(const CEvent&) = delete;

public:
  inline CEvent(bool manual = false, bool signaled_ = false)
    : manualReset(manual), signaled(signaled_), condVar(actualCv, signaled)
  {
  }

  inline void Reset()
  {
    CSingleLock lock(mutex);
    signaled = false;
  }
  void Set();

  /**
   * @brief Returns true if Event has been triggered and not reset, false otherwise.
   *
   */
  inline bool Signaled()
  {
    CSingleLock lock(mutex);
    return signaled;
  }

  /**
   * @brief This will wait up to 'duration' for the Event to be
   *        triggered. The method will return 'true' if the Event
   *        was triggered. Otherwise it will return false.
   *
   */
  template<typename Rep, typename Period>
  inline bool Wait(std::chrono::duration<Rep, Period> duration)
  {
    CSingleLock lock(mutex);
    numWaits++;
    condVar.wait(mutex, duration);
    numWaits--;
    return prepReturn();
  }

  /**
   * @brief This will wait for the Event to be triggered. The method will return
   *        'true' if the Event was triggered. If it was either interrupted
   *        it will return false. Otherwise it will return false.
   *
   */
  inline bool Wait()
  {
    CSingleLock lock(mutex);
    numWaits++;
    condVar.wait(mutex);
    numWaits--;
    return prepReturn();
  }

  /**
   * @brief This is mostly for testing. It allows a thread to make sure there are
   *        the right amount of other threads waiting.
   *
   */
  inline int getNumWaits()
  {
    CSingleLock lock(mutex);
    return numWaits;
  }
};

namespace XbmcThreads
{
/**
 * @brief  CEventGroup is a means of grouping CEvents to wait on them together.
 *         It is equivalent to WaitOnMultipleObject that returns when "any" Event
 *         in the group signaled.
 *
 */
class CEventGroup
{
  std::vector<CEvent*> events;
  CEvent* signaled{};
  XbmcThreads::ConditionVariable actualCv;
  XbmcThreads::TightConditionVariable<CEvent*&> condVar{actualCv, signaled};
  CCriticalSection mutex;

  unsigned int numWaits{0};

  // This is ONLY called from CEvent::Set.
  inline void Set(CEvent* child)
  {
    CSingleLock l(mutex);
    signaled = child;
    condVar.notifyAll();
  }

  friend class ::CEvent;

  CEventGroup(const CEventGroup&) = delete;
  CEventGroup& operator=(const CEventGroup&) = delete;

public:
  /**
   * @brief Create a CEventGroup from a number of CEvents.
   *
   */
  CEventGroup(std::initializer_list<CEvent*> events);

  ~CEventGroup();

  /**
   * @brief This will block until any one of the CEvents in the group are
   *        signaled at which point a pointer to that CEvents will be
   *        returned.
   *
   */
  CEvent* wait();

  /**
   * @brief locking is ALWAYS done in this order:
   *        CEvent::groupListMutex -> CEventGroup::mutex -> CEvent::mutex
   *
   *        Notice that this method doesn't grab the CEvent::groupListMutex at all. This
   *        is fine. It just grabs the CEventGroup::mutex and THEN the individual
   *
   */
  template<typename Rep, typename Period>
  CEvent* wait(std::chrono::duration<Rep, Period> duration)
  {
    CSingleLock lock(mutex); // grab CEventGroup::mutex
    numWaits++;

    // ==================================================
    // This block checks to see if any child events are
    // signaled and sets 'signaled' to the first one it
    // finds.
    // ==================================================
    signaled = nullptr;
    for (auto* cur : events)
    {
      CSingleLock lock2(cur->mutex);
      if (cur->signaled)
        signaled = cur;
    }
    // ==================================================

    if (!signaled)
    {
      // both of these release the CEventGroup::mutex
      if (duration == std::chrono::duration<Rep, Period>::max())
        condVar.wait(mutex);
      else
        condVar.wait(mutex, duration);
    } // at this point the CEventGroup::mutex is reacquired
    numWaits--;

    // signaled should have been set by a call to CEventGroup::Set
    CEvent* ret = signaled;
    if (numWaits == 0)
    {
      if (signaled)
        // This acquires and releases the CEvent::mutex. This is fine since the
        //  CEventGroup::mutex is already being held
        signaled->Wait(std::chrono::duration<Rep, Period>::zero()); // reset the event if needed
      signaled = nullptr; // clear the signaled if all the waiters are gone
    }
    return ret;
  }

  /**
   * @brief This is mostly for testing. It allows a thread to make sure there are
   *        the right amount of other threads waiting.
   *
   */
  inline int getNumWaits()
  {
    CSingleLock lock(mutex);
    return numWaits;
  }
};
} // namespace XbmcThreads
