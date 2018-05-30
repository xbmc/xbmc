/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include <mutex>

#if (defined TARGET_POSIX)
#include <pthread.h>
namespace XbmcThreads
{
  // forward declare in preparation for the friend declaration
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

