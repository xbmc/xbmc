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

#include "threads/Condition.h"
#include "threads/Interruptible.h"

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
  XbmcThreads::ConditionVariable condVar;
  CCriticalSection mutex;

  // block the ability to copy
  inline CEvent& operator=(const CEvent& src) { return *this; }
  inline CEvent(const CEvent& other) {}
public:

  inline CEvent(bool manual = false, bool interruptible_ = false) : 
    manualReset(manual), signaled(false), interrupted(false), interruptible(interruptible_) {}
  inline void Reset() { CSingleLock lock(mutex); signaled = false; }
  inline void Set() { CSingleLock lock(mutex); signaled = true; condVar.notifyAll(); }

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

