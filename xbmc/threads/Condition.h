/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/SingleLock.h"
#include "threads/SystemClock.h"

#include <chrono>
#include <condition_variable>

namespace XbmcThreads
{

  /**
   * This is a thin wrapper around std::condition_variable_any. It is subject
   *  to "spurious returns"
   */
  class ConditionVariable
  {
  private:
    std::condition_variable_any cond;
    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

  public:
    ConditionVariable() = default;

    inline void wait(CCriticalSection& lock)
    {
      int count  = lock.count;
      lock.count = 0;
      cond.wait(lock.get_underlying());
      lock.count = count;
    }

    inline bool wait(CCriticalSection& lock, unsigned long milliseconds)
    {
      int count  = lock.count;
      lock.count = 0;
      std::cv_status res = cond.wait_for(lock.get_underlying(), std::chrono::milliseconds(milliseconds));
      lock.count = count;
      return res == std::cv_status::no_timeout;
    }

    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }

    inline void notifyAll()
    {
      cond.notify_all();
    }

    inline void notify()
    {
      cond.notify_one();
    }
  };

  /**
   * This is a condition variable along with its predicate. This allows the use of a
   *  condition variable without the spurious returns since the state being monitored
   *  is also part of the condition.
   *
   * L should implement the Lockable concept
   *
   * The requirements on P are that it can act as a predicate (that is, I can use
   *  it in an 'while(!predicate){...}' where 'predicate' is of type 'P').
   */
  template <typename P> class TightConditionVariable
  {
    ConditionVariable& cond;
    P predicate;

  public:
    inline TightConditionVariable(ConditionVariable& cv, P predicate_) : cond(cv), predicate(predicate_) {}

    template <typename L> inline void wait(L& lock) { while(!predicate) cond.wait(lock); }
    template <typename L> inline bool wait(L& lock, unsigned long milliseconds)
    {
      bool ret = true;
      if (!predicate)
      {
        if (!milliseconds)
        {
          cond.wait(lock,milliseconds /* zero */);
          return !(!predicate); // eh? I only require the ! operation on P
        }
        else
        {
          EndTime endTime((unsigned int)milliseconds);
          for (bool notdone = true; notdone && ret == true;
               ret = (notdone = (!predicate)) ? ((milliseconds = endTime.MillisLeft()) != 0) : true)
            cond.wait(lock,milliseconds);
        }
      }
      return ret;
    }

    inline void notifyAll() { cond.notifyAll(); }
    inline void notify() { cond.notify(); }
  };
}

