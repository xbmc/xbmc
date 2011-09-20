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
   *
   * This code is basically copied from SDL_syscond.c but structured to use 
   * native windows threading primitives rather than other SDL primitives.
   */
  class ConditionVariable : public NonCopyable
  {
  private:
    CCriticalSection lock;
    int waiting;
    int signals;

    class Semaphore
    {
      HANDLE sem;
      volatile LONG count;

    public:
      inline Semaphore() : count(0L), sem(CreateSemaphore(NULL,0,32*1024,NULL)) {}
      inline ~Semaphore() { CloseHandle(sem); }

      inline bool wait(DWORD dwMilliseconds)
      {
        return (WAIT_OBJECT_0 == WaitForSingleObject(sem, dwMilliseconds)) ?
          (InterlockedDecrement(&count), true) : false;
      }

      inline bool post()
      {
        /* Increase the counter in the first place, because
         * after a successful release the semaphore may
         * immediately get destroyed by another thread which
         * is waiting for this semaphore.
         */
        InterlockedIncrement(&count);
        return ReleaseSemaphore(sem, 1, NULL) ? true : (InterlockedDecrement(&count), false);
      }
    };

    Semaphore wait_sem;
    Semaphore wait_done;

  public:
    inline ConditionVariable() : waiting(0), signals(0) {}

    inline ~ConditionVariable() {}

    inline void wait(CCriticalSection& mutex) 
    {
      wait(mutex,(unsigned long)-1L);
    }

    inline bool wait(CCriticalSection& mutex, unsigned long milliseconds) 
    { 
      bool success = false;
      DWORD ms = ((unsigned long)-1L) == milliseconds ? INFINITE : (DWORD)milliseconds;

      {
        CSingleLock l(lock);
        waiting++;
      }

      {
        CSingleExit ex(mutex);
        success = wait_sem.wait(ms);

        {
          CSingleLock l(lock);
          if (signals > 0)
          {
            if (!success)
              wait_sem.wait(INFINITE);
            wait_done.post();
            --signals;
          }
          --waiting;
        }
      }

      return success;
    }


    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }

    inline void notifyAll()
    {
      /* If there are waiting threads not already signalled, then
         signal the condition and wait for the thread to respond.
      */
      CSingleLock l(lock);
      if ( waiting > signals ) 
      {
        int i, num_waiting;

        num_waiting = (waiting - signals);
        signals = waiting;
        for ( i=0; i<num_waiting; ++i )
          wait_sem.post();

        /* Now all released threads are blocked here, waiting for us.
           Collect them all (and win fabulous prizes!) :-)
        */
        l.Leave();
        for ( i=0; i<num_waiting; ++i )
          wait_done.wait(INFINITE);
      }
    }

    inline void notify()
    {
      /* If there are waiting threads not already signalled, then
         signal the condition and wait for the thread to respond.
      */
      CSingleLock l(lock);
      if ( waiting > signals ) 
      {
        ++signals;
        wait_sem.post();
        l.Leave();
        wait_done.wait(INFINITE);
      }
    }
  };
#endif
}
