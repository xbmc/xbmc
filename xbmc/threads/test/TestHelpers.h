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

#include "gtest/gtest.h"

#include "threads/Thread.h"
#include "threads/Atomics.h"

#define MILLIS(x) x

inline static void SleepMillis(unsigned int millis) { XbmcThreads::ThreadSleep(millis); }

template<class E> inline static bool waitForWaiters(E& event, int numWaiters, int milliseconds)
{
  for( int i = 0; i < milliseconds; i++)
  {
    if (event.getNumWaits() == numWaiters)
      return true;
    SleepMillis(1);
  }
  return false;
}
  
inline static bool waitForThread(volatile long& mutex, int numWaiters, int milliseconds)
{
  CCriticalSection sec;
  for( int i = 0; i < milliseconds; i++)
  {
    if (mutex == (long)numWaiters)
      return true;

    {
      CSingleLock tmplock(sec); // kick any memory syncs
    }
    SleepMillis(1);
  }
  return false;
}

class AtomicGuard
{
  volatile long* val;
public:
  inline AtomicGuard(volatile long* val_) : val(val_) { if (val) AtomicIncrement(val); }
  inline ~AtomicGuard() { if (val) AtomicDecrement(val); }
};

class thread
{
  IRunnable* f;
  CThread* cthread;

//  inline thread(const thread& other) { }
public:
  inline explicit thread(IRunnable& runnable) : 
    f(&runnable), cthread(new CThread(f, "dumb thread"))
  {
    cthread->Create();
  }

  inline thread() : f(NULL), cthread(NULL) {}

  /**
   * Gcc-4.2 requires this to be 'const' to find the right constructor.
   * It really shouldn't be since it modifies the parameter thread
   * to ensure only one thread instance has control of the
   * Runnable.a
   */
  inline thread(const thread& other) : f(other.f), cthread(other.cthread) { ((thread&)other).f = NULL; ((thread&)other).cthread = NULL; }
  inline thread& operator=(const thread& other) { f = other.f; ((thread&)other).f = NULL; cthread = other.cthread; ((thread&)other).cthread = NULL; return *this; }

  void join()
  {
    cthread->WaitForThreadExit((unsigned int)-1);
  }

  bool timed_join(unsigned int millis)
  {
    return cthread->WaitForThreadExit(millis);
  }
};

