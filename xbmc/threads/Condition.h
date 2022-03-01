/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>

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

    inline void wait(CCriticalSection& lock, std::function<bool()> predicate)
    {
      int count = lock.count;
      lock.count = 0;
      cond.wait(lock.get_underlying(), std::move(predicate));
      lock.count = count;
    }

    inline void wait(CCriticalSection& lock)
    {
      int count  = lock.count;
      lock.count = 0;
      cond.wait(lock.get_underlying());
      lock.count = count;
    }

    template<typename Rep, typename Period>
    inline bool wait(CCriticalSection& lock,
                     std::chrono::duration<Rep, Period> duration,
                     std::function<bool()> predicate)
    {
      int count = lock.count;
      lock.count = 0;
      bool ret = cond.wait_for(lock.get_underlying(), duration, predicate);
      lock.count = count;
      return ret;
    }

    template<typename Rep, typename Period>
    inline bool wait(CCriticalSection& lock, std::chrono::duration<Rep, Period> duration)
    {
      int count  = lock.count;
      lock.count = 0;
      std::cv_status res = cond.wait_for(lock.get_underlying(), duration);
      lock.count = count;
      return res == std::cv_status::no_timeout;
    }

    inline void wait(std::unique_lock<CCriticalSection>& lock, std::function<bool()> predicate)
    {
      cond.wait(*lock.mutex(), std::move(predicate));
    }

    inline void wait(std::unique_lock<CCriticalSection>& lock) { wait(*lock.mutex()); }

    template<typename Rep, typename Period>
    inline bool wait(std::unique_lock<CCriticalSection>& lock,
                     std::chrono::duration<Rep, Period> duration,
                     std::function<bool()> predicate)
    {
      return wait(*lock.mutex(), duration, predicate);
    }

    template<typename Rep, typename Period>
    inline bool wait(std::unique_lock<CCriticalSection>& lock,
                     std::chrono::duration<Rep, Period> duration)
    {
      return wait(*lock.mutex(), duration);
    }

    inline void notifyAll()
    {
      cond.notify_all();
    }

    inline void notify()
    {
      cond.notify_one();
    }
  };

}

