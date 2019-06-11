/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <mutex>

#if (defined TARGET_POSIX)
#include <pthread.h>
namespace XbmcThreads
{
  class CRecursiveMutex
  {
    pthread_mutex_t m_mutex;

    // implementation is in threads/platform/pthreads/ThreadImpl.cpp
    static pthread_mutexattr_t* getRecursiveAttr();

  public:

    CRecursiveMutex(const CRecursiveMutex&) = delete;
    CRecursiveMutex& operator=(const CRecursiveMutex&) = delete;

    inline CRecursiveMutex() { pthread_mutex_init(&m_mutex,getRecursiveAttr()); }

    inline ~CRecursiveMutex() { pthread_mutex_destroy(&m_mutex); }

    inline void lock() { pthread_mutex_lock(&m_mutex); }

    inline void unlock() { pthread_mutex_unlock(&m_mutex); }

    inline bool try_lock() { return (pthread_mutex_trylock(&m_mutex) == 0); }

    inline std::recursive_mutex::native_handle_type  native_handle()
    {
      return &m_mutex;
    }
  };
}
#elif (defined TARGET_WINDOWS)
namespace XbmcThreads
{
  typedef std::recursive_mutex CRecursiveMutex;
}
#endif

