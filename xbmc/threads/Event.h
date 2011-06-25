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
class CEvent
{
  bool manualReset;
  bool signaled;
  unsigned int numWaits;

  std::vector<XbmcThreads::CEventGroup*> * groups;

  /**
   * To satisfy the TightConditionVariable requirements and allow the 
   *  predicate being monitored to include both the signaled and interrupted
   *  states.
   */
  XbmcThreads::TightConditionVariable<bool&> condVar;
  CCriticalSection mutex;

  friend class XbmcThreads::CEventGroup;

  void groupSet();
  void addGroup(XbmcThreads::CEventGroup* group);
  void removeGroup(XbmcThreads::CEventGroup* group);

  // helper for the two wait methods
  inline bool prepReturn() { bool ret = signaled; if (!manualReset && numWaits == 0) signaled = false; return ret; }

  // block the ability to copy
  inline CEvent& operator=(const CEvent& src) { return *this; }
  inline CEvent(const CEvent& other): condVar(signaled) {}

public:
  inline CEvent(bool manual = false, bool signaled_ = false) : 
    manualReset(manual), signaled(signaled_), numWaits(0), groups(NULL), condVar(signaled) {}

  inline void Reset() { CSingleLock lock(mutex); signaled = false; }
  inline void Set() { CSingleLock lock(mutex); signaled = true; condVar.notifyAll(); groupSet(); }

  /**
   * This will wait up to 'milliSeconds' milliseconds for the Event
   *  to be triggered. The method will return 'true' if the Event
   *  was triggered. Otherwise it will return false.
   */
  inline bool WaitMSec(unsigned int milliSeconds) 
  { CSingleLock lock(mutex); numWaits++; condVar.wait(mutex,milliSeconds); numWaits--; return prepReturn(); }

  /**
   * This will wait for the Event to be triggered. The method will return 
   * 'true' if the Event was triggered. If it was either interrupted
   * it will return false. Otherwise it will return false.
   */
  inline bool Wait()
  { CSingleLock lock(mutex); numWaits++; condVar.wait(mutex); numWaits--; return prepReturn(); }

};

namespace XbmcThreads
{
  /**
   * CEventGroup is a means of grouping CEvents to wait on them together.
   * It is equivalent to WaitOnMultipleObject that returns when "any" Event
   * in the group signaled.
   */
  class CEventGroup
  {
    std::vector<CEvent*> events;
    CEvent* signaled;
    XbmcThreads::TightConditionVariable<CEvent*&> condVar;
    CCriticalSection mutex;

    unsigned int numWaits;

    inline void Set(CEvent* child) { CSingleLock lock(mutex); signaled = child; condVar.notifyAll(); }

    friend class ::CEvent;

    inline CEvent* prepReturn() { CEvent* ret = signaled; if (numWaits == 0) signaled = NULL; return ret; }
    CEvent* anyEventsSignaled();

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
    inline CEvent* wait() 
    { CSingleLock lock(mutex); 
      numWaits++; 
      signaled = anyEventsSignaled(); 
      if (!signaled) condVar.wait(mutex); 
      numWaits--; 
      return prepReturn(); 
    }

    /**
     * This will block until any one of the CEvents in the group are
     * signaled or the timeout is reachec. If an event is signaled then
     * it will return a pointer to that CEvent, otherwise it will return
     * NULL.
     */
    inline CEvent* wait(unsigned int milliseconds)  
    { CSingleLock lock(mutex);
      numWaits++; 
      signaled = anyEventsSignaled(); 
      if(!signaled) condVar.wait(mutex,milliseconds); 
      numWaits--; 
      return prepReturn(); 
    }
  };
}
