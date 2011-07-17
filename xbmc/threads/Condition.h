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

#include <boost/thread/condition_variable.hpp>

#include "threads/SingleLock.h"

namespace XbmcThreads
{
  /**
   * This is a thin wrapper around boost::condition_variable. It is subject
   *  to "spurious returns" as it is built on boost which is built on posix
   *  on many of our platforms.
   */
  class ConditionVariable
  {
  private:
    boost::condition_variable_any impl;

    // explicitly deny copying
    inline ConditionVariable(const ConditionVariable& other) {}
    inline ConditionVariable& operator=(ConditionVariable& other) { return *this; }
  public:
    inline ConditionVariable() {}

    enum TimedWaitResponse { TW_OK = 0, TW_TIMEDOUT = 1, TW_INTERRUPTED=-1, TW_ERROR=-2 };

    template<typename L> inline void wait(L& lock) { impl.wait(lock); }

    template<typename L> inline TimedWaitResponse wait(L& lock, int milliseconds)
    {
      ConditionVariable::TimedWaitResponse ret = TW_OK;
      try { ret = (impl.timed_wait(lock, boost::posix_time::milliseconds(milliseconds))) ? TW_OK : TW_TIMEDOUT; }
      catch (boost::thread_interrupted ) { ret = TW_INTERRUPTED; }
      catch (...) { ret = TW_ERROR; }
      return ret;
    }

    inline void notifyAll() { impl.notify_all(); }
    inline void notify() { impl.notify_one(); }
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
    ConditionVariable cond;
    P predicate;
  public:
    inline TightConditionVariable(P predicate_) : predicate(predicate_) {}
    template <typename L> inline void wait(L& lock) { while(!predicate) cond.wait(lock); }

    template <typename L> inline ConditionVariable::TimedWaitResponse wait(L& lock, int milliseconds)
    {
      ConditionVariable::TimedWaitResponse ret = ConditionVariable::TW_OK;
      boost::system_time const timeout=boost::get_system_time() + boost::posix_time::milliseconds(milliseconds);
      while ((!predicate) && ret != ConditionVariable::TW_TIMEDOUT)
      {
        cond.wait(lock,milliseconds);
        if (milliseconds)
        {
          if ((!predicate) && boost::get_system_time() > timeout)
            ret = ConditionVariable::TW_TIMEDOUT;
        }
        else
          ret = (!predicate) ? ConditionVariable::TW_TIMEDOUT : ConditionVariable::TW_OK;
      }
      return ret;
    }

    inline void notifyAll() { cond.notifyAll(); }
    inline void notify() { cond.notify(); }
  };
}

