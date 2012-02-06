#pragma once
/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

namespace PLATFORM
{
  inline pthread_mutexattr_t *GetRecursiveMutexAttribute(void)
  {
    static pthread_mutexattr_t g_mutexAttr;
    static bool bAttributeInitialised = false;
    if (!bAttributeInitialised)
    {
      pthread_mutexattr_init(&g_mutexAttr);
      pthread_mutexattr_settype(&g_mutexAttr, PTHREAD_MUTEX_RECURSIVE);
      bAttributeInitialised = true;
    }
    return &g_mutexAttr;
  }

  inline struct timespec GetAbsTime(uint64_t iIncreaseBy = 0)
  {
    struct timespec abstime;
    struct timeval now;
    gettimeofday(&now, NULL);
    iIncreaseBy += now.tv_usec / 1000;
    abstime.tv_sec = now.tv_sec + (time_t)(iIncreaseBy / 1000);
    abstime.tv_nsec = (int32_t)((iIncreaseBy % (uint32_t)1000) * (uint32_t)1000000);
    return abstime;
  }

  typedef pthread_t thread_t;

  #define ThreadsCreate(thread, func, arg)         (pthread_create(&thread, NULL, (void *(*) (void *))func, (void *)arg) == 0)
  #define ThreadsWait(thread, retval)              (pthread_join(thread, retval) == 0)

  typedef pthread_mutex_t mutex_t;
  #define MutexCreate(mutex)                       pthread_mutex_init(&mutex, GetRecursiveMutexAttribute());
  #define MutexDelete(mutex)                       pthread_mutex_destroy(&mutex);
  #define MutexLock(mutex)                         (pthread_mutex_lock(&mutex) == 0)
  #define MutexTryLock(mutex)                      (pthread_mutex_trylock(&mutex) == 0)
  #define MutexUnlock(mutex)                       pthread_mutex_unlock(&mutex)

  class CConditionImpl
  {
  public:
    CConditionImpl(void)
    {
      pthread_cond_init(&m_condition, NULL);
    }

    virtual ~CConditionImpl(void)
    {
      pthread_cond_destroy(&m_condition);
    }

    void Signal(void)
    {
      pthread_cond_signal(&m_condition);
    }

    void Broadcast(void)
    {
      pthread_cond_broadcast(&m_condition);
    }

    bool Wait(mutex_t &mutex, uint32_t iTimeoutMs)
    {
      sched_yield();
      if (iTimeoutMs > 0)
      {
        struct timespec timeout = GetAbsTime(iTimeoutMs);
        return (pthread_cond_timedwait(&m_condition, &mutex, &timeout) == 0);
      }
      return (pthread_cond_wait(&m_condition, &mutex) == 0);
    }

    pthread_cond_t m_condition;
  };
}
