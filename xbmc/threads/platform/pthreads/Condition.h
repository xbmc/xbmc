/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "threads/SingleLock.h"
#include "threads/Helpers.h"

#include <pthread.h>

#ifdef TARGET_DARWIN
  #include <sys/time.h> //for gettimeofday
#else
  #include <time.h> //for clock_gettime
#endif

namespace XbmcThreads
{

  /**
   * This is a thin wrapper around boost::condition_variable. It is subject
   *  to "spurious returns" as it is built on boost which is built on posix
   *  on many of our platforms.
   */
  class ConditionVariable : public NonCopyable
  {
  private:
    pthread_cond_t cond;

  public:
    inline ConditionVariable() 
    { 
      pthread_cond_init(&cond,NULL); 
    }

    inline ~ConditionVariable() 
    { 
      pthread_cond_destroy(&cond);
    }

    inline void wait(CCriticalSection& lock) 
    { 
      pthread_cond_wait(&cond,&lock.get_underlying().mutex);
    }

    inline bool wait(CCriticalSection& lock, unsigned long milliseconds) 
    { 
      struct timespec ts;

#ifdef TARGET_DARWIN
      struct timeval tv;
      gettimeofday(&tv, NULL);
      ts.tv_nsec = tv.tv_usec * 1000;
      ts.tv_sec  = tv.tv_sec;
#else
      clock_gettime(CLOCK_REALTIME, &ts);
#endif

      ts.tv_nsec += milliseconds % 1000 * 1000000;
      ts.tv_sec  += milliseconds / 1000 + ts.tv_nsec / 1000000000;
      ts.tv_nsec %= 1000000000;

      return (pthread_cond_timedwait(&cond,&lock.get_underlying().mutex,&ts) == 0);
    }

    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }

    inline void notifyAll() 
    { 
      pthread_cond_broadcast(&cond);
    }

    inline void notify() 
    { 
      pthread_cond_signal(&cond);
    }
  };

}
