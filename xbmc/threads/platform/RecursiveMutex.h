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
  class RecursiveMutex
  {
    pthread_mutex_t mutex;

    // implementation is in threads/platform/pthreads/ThreadImpl.cpp
    static pthread_mutexattr_t* getRecursiveAttr();

  public:

    RecursiveMutex(const RecursiveMutex&) = delete;
    RecursiveMutex& operator=(const RecursiveMutex&) = delete;

    inline RecursiveMutex() { pthread_mutex_init(&mutex,getRecursiveAttr()); }

    inline ~RecursiveMutex() { pthread_mutex_destroy(&mutex); }

    inline void lock() { pthread_mutex_lock(&mutex); }

    inline void unlock() { pthread_mutex_unlock(&mutex); }

    inline bool try_lock() { return (pthread_mutex_trylock(&mutex) == 0); }

    inline std::recursive_mutex::native_handle_type  native_handle()
    {
      return &mutex;
    }
  };
}
#elif (defined TARGET_WINDOWS)
namespace XbmcThreads
{
  typedef std::recursive_mutex RecursiveMutex;
}
#endif

