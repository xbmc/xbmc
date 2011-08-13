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

#include "threads/SingleLock.h"
#include "threads/Helpers.h"

#include <windows.h>
#include <sys/time.h>

#ifndef TARGET_VISTAPLUS
#include "threads/platform/win/FairMonitor.h"
#endif

namespace XbmcThreads
{

// If you want to use a more efficent version of the ConditionVariable then
// you can define TARGET_VISTAPLUS. However, the executable will not run
// on windows xp or earlier.
#ifdef TARGET_VISTAPLUS
  /**
   * This is a thin wrapper around boost::condition_variable. It is subject
   *  to "spurious returns" as it is built on boost which is built on posix
   *  on many of our platforms.
   */
  class ConditionVariable : public NonCopyable
  {
  private:
    CONDITION_VARIABLE cond;

  public:
    inline ConditionVariable() 
    {
      InitializeConditionVariable(&cond);
    }

    inline ~ConditionVariable() 
    { 
      // apparently, windows condition variables do not need to be deleted
    }

    inline void wait(CCriticalSection& lock) 
    { 
      // even the windows implementation is capable of spontaneous wakes
      SleepConditionVariableCS(&cond,&lock.get_underlying().mutex,INFINITE);
    }

    inline bool wait(CCriticalSection& lock, unsigned long milliseconds) 
    { 
      return SleepConditionVariableCS(&cond,&lock.get_underlying().mutex,milliseconds) ? true : false;
    }

    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }

    inline void notifyAll() 
    { 
      WakeAllConditionVariable(&cond);
    }

    inline void notify() 
    { 
      WakeConditionVariable(&cond);
    }
  };

#else

  /**
   * ConditionVariableXp is effectively a condition variable implementation
   *  assuming we're on Windows XP or earlier. This means we don't have 
   *  access to InitializeConditionVariable and that the structure 
   *  CONDITION_VARIABLE doesnt actually exist.
   */
  class ConditionVariable : public NonCopyable
  {
  private:
    class FairMonitorLockable : public FairMonitor
    {
    public:
      inline void lock() { BeginSynchronized(); }
      inline void unlock() { EndSynchronized(); }

      // doesn't support try_lock, but it doesn't need to
    };

	class FairMonitorLock : public XbmcThreads::UniqueLock<FairMonitorLockable> 
	{
	public:
		inline FairMonitorLock(FairMonitorLockable& l) : XbmcThreads::UniqueLock<FairMonitorLockable>(l) {}
	};

    FairMonitorLockable impl;

  public:

    inline void wait(CCriticalSection& lock) 
    { 
      // intertwined unlock.
      FairMonitorLock fairMonitorGuard(impl);
      CSingleExit exiter(lock);

      impl.Wait();
    }

    inline bool wait(CCriticalSection& lock, unsigned long milliseconds) 
    { 
      // intertwined unlock.
      FairMonitorLock fairMonitorGuard(impl);
      CSingleExit exiter(lock);

      return (impl.Wait((DWORD)milliseconds) == WAIT_OBJECT_0);
    }

    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }

    inline void notifyAll() { impl.NotifyAll(); }

    inline void notify() { impl.Notify(); }
  };
#endif
}
