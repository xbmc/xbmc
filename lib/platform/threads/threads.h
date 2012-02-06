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

#include "mutex.h"

namespace PLATFORM
{
  class CThread
  {
  public:
    CThread(void) :
        m_bStop(false),
        m_bRunning(false) {}

    virtual ~CThread(void)
    {
      StopThread();
    }

    static void *ThreadHandler(CThread *thread)
    {
      void *retVal = NULL;

      if (thread)
      {
        CLockObject lock(thread->m_threadMutex);
        thread->m_bRunning = true;
        lock.Unlock();
        thread->m_threadCondition.Broadcast();

        retVal = thread->Process();

        lock.Lock();
        thread->m_bRunning = false;
        lock.Unlock();
        thread->m_threadCondition.Broadcast();
      }

      return retVal;
    }

    virtual bool IsRunning(void)
    {
      CLockObject lock(m_threadMutex);
      return m_bRunning;
    }

    virtual bool IsStopped(void)
    {
      CLockObject lock(m_threadMutex);
      return m_bStop;
    }

    virtual bool CreateThread(bool bWait = true)
    {
        bool bReturn(false);
        CLockObject lock(m_threadMutex);
        if (!IsRunning())
        {
          m_bStop = false;
          if (ThreadsCreate(m_thread, CThread::ThreadHandler, ((void*)static_cast<CThread *>(this))))
          {
            if (bWait)
              m_threadCondition.Wait(m_threadMutex);
            bReturn = true;
          }
        }
      return bReturn;
    }

    virtual bool StopThread(bool bWaitForExit = true)
    {
      bool bReturn(true);
      bool bRunning(false);
      {
        CLockObject lock(m_threadMutex);
        bRunning = IsRunning();
        m_bStop = true;
        m_threadCondition.Broadcast();
      }

      if (bRunning && bWaitForExit)
      {
        void *retVal = NULL;
        bReturn = ThreadsWait(m_thread, &retVal);
      }
      return true;
    }

    virtual bool Sleep(uint32_t iTimeout)
    {
      CLockObject lock(m_threadMutex);
      return m_bStop ? false : m_threadCondition.Wait(m_threadMutex, iTimeout);
    }

    virtual void *Process(void) = 0;

  protected:
    void SetRunning(bool bSetTo);

  private:
    bool       m_bStop;
    bool       m_bRunning;
    CCondition m_threadCondition;
    CMutex     m_threadMutex;
    thread_t   m_thread;
  };
};
