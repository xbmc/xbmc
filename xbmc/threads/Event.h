/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <vector>

#include "threads/Condition.h"
#include "threads/Interruptible.h"

// forward declare the CEventGroup
namespace XbmcThreads
{
  class CEventGroup;
}


/**
 * This is an Event class built from a ConditionVariable. The Event adds the state
 * that the condition is gating as well as the mutex/lock.
 *
 * This Event can be 'interruptible' (even though there is only a single place
 * in the code that uses this behavior).
 *
 * This class manages 'spurious returns' from the condition variable.
 */
class CEvent : public XbmcThreads::IInterruptible
{
  bool manualReset;
  bool signaled;
  bool interrupted;
  bool interruptible;

  unsigned int numWaits;

  std::vector<XbmcThreads::CEventGroup*> * groups;

  XbmcThreads::ConditionVariable condVar;
  CCriticalSection mutex;

  // block the ability to copy
  inline CEvent& operator=(const CEvent& src) { return *this; }
  inline CEvent(const CEvent& other) {}

  friend class XbmcThreads::CEventGroup;

  void groupSet();
  void addGroup(XbmcThreads::CEventGroup* group);
  void removeGroup(XbmcThreads::CEventGroup* group);
public:

  inline CEvent(bool manual = false, bool interruptible_ = false) : 
    manualReset(manual), signaled(false), interrupted(false), 
    interruptible(interruptible_), numWaits(0), groups(NULL) {}
  inline void Reset() { CSingleLock lock(mutex); signaled = false; }
  inline void Set() { CSingleLock lock(mutex); signaled = true; condVar.notifyAll(); groupSet(); }

  virtual void Interrupt();
  inline bool wasInterrupted() { CSingleLock lock(mutex); return interrupted; }

  /**
   * This will wait up to 'milliSeconds' milliseconds for the Event
   *  to be triggered. The method will return 'true' if the Event
   *  was triggered. If it was either interrupted, or it timed out
   *  it will return false. To determine if it was interrupted you can
   *  use 'wasInterrupted()' call prior to any further call to a 
   *  Wait* method.
   */
  bool WaitMSec(unsigned int milliSeconds);

  /**
   * This will wait for the Event to be triggered. The method will return 
   * 'true' if the Event was triggered. If it was either interrupted
   * it will return false. To determine if it was interrupted you can
   * use 'wasInterrupted()' call prior to any further call to a Wait* method.
   */
  bool Wait();
};

namespace XbmcThreads
{
  /**
   * CEventGroup is a means of grouping CEvents to wait on them together.
   * It is equivalent to WaitOnMultipleObject with that returns when "any"
   * in the group signaled.
   */
  class CEventGroup
  {
    std::vector<CEvent*> events;
    XbmcThreads::ConditionVariable condVar;
    CCriticalSection mutex;
    CEvent* signaled;

    unsigned int numWaits;
    void Set(CEvent* child);

    friend class ::CEvent;
  public:

    /**
     * Create a CEventGroup from a number of CEvents. num is the number
     *  of Events that follow. E.g.:
     *
     *  CEventGroup g(3, event1, event2, event3);
     */
    CEventGroup(int num, CEvent* v1, ...);

    /**
     * Create a CEventGroup from a number of CEvents. The parameters
     *  should form a NULL terminated list of CEvent*'s
     *
     *  CEventGroup g(event1, event2, event3, NULL);
     */
    CEventGroup(CEvent* v1, ...);
    ~CEventGroup();

    /**
     * This will block until any one of the CEvents in the group are
     * signaled at which point a pointer to that CEvents will be 
     * returned.
     */
    CEvent* wait();

    /**
     * This will block until any one of the CEvents in the group are
     * signaled or the timeout is reachec. If an event is signaled then
     * it will return a pointer to that CEvent, otherwise it will return
     * NULL.
     */
    CEvent* wait(unsigned int milliseconds);
  };
}
