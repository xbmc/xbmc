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

#include <unittest++/UnitTest++.h>

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
  template <class F> class FunctorRunnable : public IRunnable
  {
    F f;
  public:
    inline explicit FunctorRunnable(F f_) : f(f_) { }
    inline virtual ~FunctorRunnable(){ }
    inline virtual void Run() { f (); }
  };

  IRunnable* f;
  CThread* cthread;

//  inline thread(const thread& other) { }
public:
  template <class F> inline explicit thread(F functor) : 
    f(new FunctorRunnable<F>(functor)), 
    cthread(new CThread(f, "dumb thread"))
  {
    cthread->Create();
  }

  inline thread() : f(NULL), cthread(NULL) {}

  inline thread(thread& other) : f(other.f), cthread(other.cthread) { other.f = NULL; other.cthread = NULL; }
  inline thread& operator=(const thread& other) { f = other.f; ((thread&)other).f = NULL; cthread = other.cthread; ((thread&)other).cthread = NULL; return *this; }

  virtual ~thread()
  {
//    if (cthread && cthread->IsRunning())
//      cthread->StopThread();

    if (f)
      delete f;
  }

  void join()
  {
    cthread->WaitForThreadExit((unsigned int)-1);
  }

  bool timed_join(unsigned int millis)
  {
    return cthread->WaitForThreadExit(millis);
  }
};

template <class F> class FunctorReference
{
  F& f;
 public:
  inline FunctorReference(F& f_) : f(f_) {}
  inline FunctorReference(const FunctorReference<F>& fr) : f(fr.f) {}

  inline void operator() ()
  {
    f ();
  }
};

template<class F> inline FunctorReference<F> ref(F& f) { return FunctorReference<F>(f); }
