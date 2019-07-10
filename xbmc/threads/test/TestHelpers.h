/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

#include <gtest/gtest.h>

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

inline static bool waitForThread(std::atomic<long>& mutex, int numWaiters, int milliseconds)
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
  std::atomic<long>* val;
public:
  inline AtomicGuard(std::atomic<long>* val_) : val(val_) { if (val) ++(*val); }
  inline ~AtomicGuard() { if (val) --(*val); }
};

class thread
{
  IRunnable* f;
  CThread* cthread;

//  inline thread(const thread& other) { }
public:
  inline explicit thread(IRunnable& runnable) :
    f(&runnable), cthread(new CThread(f, "DumbThread"))
  {
    cthread->Create();
  }

  inline thread() : f(NULL), cthread(NULL) {}
  ~thread()
  {
    delete cthread;
  }

  inline thread(thread& other) : f(other.f), cthread(other.cthread) { other.f = NULL; other.cthread = NULL; }
  inline thread& operator=(thread& other) { f = other.f; other.f = NULL; cthread = other.cthread; other.cthread = NULL; return *this; }

  void join()
  {
    cthread->Join(static_cast<unsigned int>(-1));
  }

  bool timed_join(unsigned int millis)
  {
    return cthread->Join(millis);
  }
};

