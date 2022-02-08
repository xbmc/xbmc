/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Condition.h"

#include <mutex>
#include <shared_mutex>

/**
 * A CSharedSection is a mutex that satisfies the Shared Lockable concept (see Lockables.h).
 */
class CSharedSection
{
  CCriticalSection sec;
  XbmcThreads::ConditionVariable actualCv;

  unsigned int sharedCount = 0;

public:
  inline CSharedSection() = default;

  inline void lock()
  {
    std::unique_lock<CCriticalSection> l(sec);
    while (sharedCount)
      actualCv.wait(l, [this]() { return sharedCount == 0; });
    sec.lock();
  }
  inline bool try_lock() { return (sec.try_lock() ? ((sharedCount == 0) ? true : (sec.unlock(), false)) : false); }
  inline void unlock() { sec.unlock(); }

  inline void lock_shared()
  {
    std::unique_lock<CCriticalSection> l(sec);
    sharedCount++;
  }
  inline bool try_lock_shared() { return (sec.try_lock() ? sharedCount++, sec.unlock(), true : false); }
  inline void unlock_shared()
  {
    std::unique_lock<CCriticalSection> l(sec);
    sharedCount--;
    if (!sharedCount)
    {
      actualCv.notifyAll();
    }
  }
};
