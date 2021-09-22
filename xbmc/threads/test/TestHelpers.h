/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

#include <memory>

#include <gtest/gtest.h>

template<class E>
inline static bool waitForWaiters(E& event, int numWaiters, std::chrono::milliseconds duration)
{
  for (auto i = std::chrono::milliseconds::zero(); i < duration; i++)
  {
    if (event.getNumWaits() == numWaiters)
      return true;

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return false;
}

inline static bool waitForThread(std::atomic<long>& mutex,
                                 int numWaiters,
                                 std::chrono::milliseconds duration)
{
  CCriticalSection sec;
  for (auto i = std::chrono::milliseconds::zero(); i < duration; i++)
  {
    if (mutex == (long)numWaiters)
      return true;

    {
      CSingleLock tmplock(sec); // kick any memory syncs
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
  std::unique_ptr<CThread> cthread;

public:
  inline explicit thread(IRunnable& runnable)
    : cthread(std::make_unique<CThread>(&runnable, "DumbThread"))
  {
    cthread->Create();
  }

  void join() { cthread->Join(std::chrono::milliseconds::max()); }

  bool timed_join(std::chrono::milliseconds duration) { return cthread->Join(duration); }
};

