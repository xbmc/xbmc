/*
 *      Copyright (C) 2005-2012 Team XBMC
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
  class ConditionVariable;
  
  namespace intern
  {
    /**
     * ConditionVariableXp is effectively a condition variable implementation
     *  assuming we're on Windows XP or earlier. This means we don't have 
     *  access to InitializeConditionVariable and that the structure 
     *  CONDITION_VARIABLE doesnt actually exist.
     *
     * This code is basically copied from SDL_syscond.c but structured to use 
     * native windows threading primitives rather than other SDL primitives.
     */
    class ConditionVariableXp : public NonCopyable
    {
      friend class ConditionVariable;
      CCriticalSection lock;
      int waiting;
      int signals;

      class Semaphore
      {
        friend class ConditionVariableXp;
        HANDLE sem;
        volatile LONG count;

        inline Semaphore() : count(0L), sem(NULL) {  }
        inline ~Semaphore() { if (sem) CloseHandle(sem); }

        inline void Init() { sem = CreateSemaphore(NULL,0,32*1024,NULL); }

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

      inline ConditionVariableXp() : waiting(0), signals(0) { }
      inline ~ConditionVariableXp() {}

      inline void Init() { wait_sem.Init(); wait_done.Init(); }

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

    /**
     * This is condition variable implementation that uses the Vista (or later)
     * windows api but it is safe to compile, link, and load on Xp.
     */
    class ConditionVariableVista : public NonCopyable
    {
      friend class ConditionVariable;

      CONDITION_VARIABLE cond;

      typedef VOID (WINAPI *TakesCV)(PCONDITION_VARIABLE);
      typedef BOOL (WINAPI *SleepCVCS)(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD);

      static bool setConditionVarFuncs();
      static TakesCV InitializeConditionVariableProc;
      static SleepCVCS SleepConditionVariableCSProc;
      static TakesCV WakeConditionVariableProc;
      static TakesCV WakeAllConditionVariableProc;

      inline ConditionVariableVista() { }

    inline void Init() { (*InitializeConditionVariableProc)(&cond); }

      // apparently, windows condition variables do not need to be deleted
      inline ~ConditionVariableVista() { }

      inline void wait(CCriticalSection& lock) 
      { 
        // even the windows implementation is capable of spontaneous wakes
        (*SleepConditionVariableCSProc)(&cond,&lock.get_underlying().mutex,INFINITE);
      }

      inline bool wait(CCriticalSection& lock, unsigned long milliseconds) 
      { 
        return (*SleepConditionVariableCSProc)(&cond,&lock.get_underlying().mutex,milliseconds) ? true : false;
      }

      inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
      inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }
      inline void notifyAll() { (*WakeAllConditionVariableProc)(&cond); }
      inline void notify() { (*WakeConditionVariableProc)(&cond); }
    };
  }

#define XBMC_CV(func) if (isVista) vistaimpl. func ;else xpimpl. func 
#define XBMC_RCV(func) (isVista ? vistaimpl. func : xpimpl. func )
  class ConditionVariable : public NonCopyable
  {
    // stupid hack for statics
    static bool isIsVistaSet;
    static bool isVista;
    static bool getIsVista();
    intern::ConditionVariableXp xpimpl;
    intern::ConditionVariableVista vistaimpl;
  public:
    inline ConditionVariable()
    {
      if (isIsVistaSet ? isVista : getIsVista())
        vistaimpl.Init();
      else
        xpimpl.Init();
    }
    inline void wait(CCriticalSection& lock) { XBMC_CV(wait(lock)); }
    inline bool wait(CCriticalSection& lock, unsigned long milliseconds) { return XBMC_RCV(wait(lock, milliseconds)); }

    inline void wait(CSingleLock& lock) { XBMC_CV(wait(lock)); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return XBMC_RCV(wait(lock, milliseconds)); }
    inline void notifyAll() { XBMC_CV(notifyAll()); }
    inline void notify() { XBMC_CV(notify()); }
  };
}
