/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "threads/SingleLock.h"
#include "threads/Helpers.h"

#include <windows.h>

namespace XbmcThreads
{
  /**
   * This is condition variable implementation that uses the underlying
   *  Windows mechanisms and assumes Vista (or later)
   */
  class ConditionVariable : public NonCopyable
  {
    CONDITION_VARIABLE cond;

    // SleepConditionVariableCS requires the condition variable be entered
    //  only once.
    struct AlmostExit 
    {
      unsigned int count;
      CCriticalSection& cc;
      inline AlmostExit(CCriticalSection& pcc) : count(pcc.exit(1)), cc(pcc) { cc.count = 0; }
      inline ~AlmostExit() { cc.count = 1; cc.restore(count); }
    };

  public:
    inline ConditionVariable() { InitializeConditionVariable(&cond); }

    // apparently, windows condition variables do not need to be deleted
    inline ~ConditionVariable() { }

    inline void wait(CCriticalSection& lock) 
    { 
      AlmostExit ae(lock);
      // even the windows implementation is capable of spontaneous wakes
      SleepConditionVariableCS(&cond,&lock.get_underlying().mutex,INFINITE);
    }

    inline bool wait(CCriticalSection& lock, unsigned long milliseconds) 
    { 
      AlmostExit ae(lock);
      return SleepConditionVariableCS(&cond,&lock.get_underlying().mutex,milliseconds) ? true : false;
    }

    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }
    inline void notifyAll() { WakeAllConditionVariable(&cond); }
    inline void notify() { WakeConditionVariable(&cond); }
  };
}
