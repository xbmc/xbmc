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

#include "../os.h"

#if defined(__WINDOWS__)
#include "../windows/os-threads.h"
#else
#include "../posix/os-threads.h"
#endif

namespace PLATFORM
{
  class PreventCopy
  {
  public:
    inline PreventCopy(void) {}
    inline ~PreventCopy(void) {}

  private:
    inline PreventCopy(const PreventCopy &c) { *this = c; }
    inline PreventCopy &operator=(const PreventCopy &c){ *this = c; return *this; }
  };

  class CCondition;

  class CMutex : public PreventCopy
  {
    friend class CCondition;
  public:
    inline CMutex(void) :
      m_iLockCount(0)
    {
      MutexCreate(m_mutex);
    }

    inline ~CMutex(void)
    {
      Clear();
      MutexDelete(m_mutex);
    }

    inline bool TryLock(void)
    {
      if (MutexTryLock(m_mutex))
      {
        ++m_iLockCount;
        return true;
      }
      return false;
    }

    inline bool Lock(void)
    {
      MutexLock(m_mutex);
      ++m_iLockCount;
      return true;
    }

    inline void Unlock(void)
    {
      if (Lock())
      {
        if (m_iLockCount >= 2)
        {
          --m_iLockCount;
          MutexUnlock(m_mutex);
        }

        --m_iLockCount;
        MutexUnlock(m_mutex);
      }
    }

    inline bool Clear(void)
    {
      bool bReturn(false);
      if (TryLock())
      {
        unsigned int iLockCount = m_iLockCount;
        for (unsigned int iPtr = 0; iPtr < iLockCount; iPtr++)
          Unlock();
        bReturn = true;
      }
      return bReturn;
    }

  private:
    mutex_t               m_mutex;
    volatile unsigned int m_iLockCount;
  };

  class CLockObject : public PreventCopy
  {
  public:
    inline CLockObject(CMutex &mutex, bool bClearOnExit = false) :
      m_mutex(mutex),
      m_bClearOnExit(bClearOnExit)
    {
      m_mutex.Lock();
    }

    inline ~CLockObject(void)
    {
      if (m_bClearOnExit)
        Clear();
      else
        Unlock();
    }

    inline bool TryLock(void)
    {
      return m_mutex.TryLock();
    }

    inline void Unlock(void)
    {
      m_mutex.Unlock();
    }

    inline bool Clear(void)
    {
      return m_mutex.Clear();
    }

    inline bool Lock(void)
    {
      return m_mutex.Lock();
    }

  private:
    CMutex &m_mutex;
    bool    m_bClearOnExit;
  };

  class CCondition : public PreventCopy
  {
  public:
    inline CCondition(void) {}
    inline ~CCondition(void)
    {
      m_condition.Broadcast();
    }

    inline void Broadcast(void)
    {
      m_condition.Broadcast();
    }

    inline void Signal(void)
    {
      m_condition.Signal();
    }

    inline bool Wait(CMutex &mutex, uint32_t iTimeout = 0)
    {
      return m_condition.Wait(mutex.m_mutex, iTimeout);
    }

    static void Sleep(uint32_t iTimeout)
    {
      CCondition w;
      CMutex m;
      CLockObject lock(m);
      w.Wait(m, iTimeout);
    }

  private:
    CConditionImpl m_condition;
  };
}
