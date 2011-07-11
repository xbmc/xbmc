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

#include "threads/SingleLock.h"
#include "threads/Helpers.h"

#include <pthread.h>
#include <sys/time.h>

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
      int pthread_cond_init_result = pthread_cond_init(&cond,NULL); 
      assert(pthread_cond_init_result == 0); 
    }

    inline ~ConditionVariable() 
    { 
      int pthread_cond_destroy_result = pthread_cond_destroy(&cond); 
      assert(pthread_cond_destroy_result == 0); 
    }

    inline void wait(CCriticalSection& lock) 
    { 
      int pthread_cond_wait_result = pthread_cond_wait(&cond,&lock.get_underlying().mutex);
      assert(pthread_cond_wait_result == 0);
    }

    inline bool wait(CCriticalSection& lock, unsigned long milliseconds) 
    { 
      struct timeval tv;
      struct timespec ts;

      int result = gettimeofday(&tv,NULL);
      assert(result == 0);

      milliseconds += tv.tv_usec / 1000; // move the usecs onto milliseconds
      ts.tv_sec = tv.tv_sec + (time_t)(milliseconds/1000);
      ts.tv_nsec = (long)((milliseconds % (unsigned long)1000) * (unsigned long)1000000);
      int pthread_cond_timedwait_result = pthread_cond_timedwait(&cond,&lock.get_underlying().mutex,&ts);
      assert (pthread_cond_timedwait_result == 0 || pthread_cond_timedwait_result == ETIMEDOUT);
      return !pthread_cond_timedwait_result;
    }

    inline void wait(CSingleLock& lock) { wait(lock.get_underlying()); }
    inline bool wait(CSingleLock& lock, unsigned long milliseconds) { return wait(lock.get_underlying(), milliseconds); }

    inline void notifyAll() 
    { 
      int pthread_cond_broadcast_result = pthread_cond_broadcast(&cond); 
      assert(pthread_cond_broadcast_result == 0); 
    }

    inline void notify() 
    { 
      int pthread_cond_signal_result = pthread_cond_signal(&cond);
      assert(pthread_cond_signal_result == 0); 
    }
  };

}
