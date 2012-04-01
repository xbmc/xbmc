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
  #define thread_t                                 HANDLE
  #define ThreadsWait(thread, retVal)              (::WaitForSingleObject(thread, INFINITE) < 0)
  #define ThreadsCreate(thread, func, arg)         ((thread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL)) == NULL ? false : true)

  typedef CRITICAL_SECTION* mutex_t;
  #define MutexCreate(mutex)                       ::InitializeCriticalSection(mutex = new CRITICAL_SECTION)
  #define MutexDelete(mutex)                       ::DeleteCriticalSection(mutex); delete mutex
  #define MutexLock(mutex)                         ::EnterCriticalSection(mutex)
  #define MutexTryLock(mutex)                      (::TryEnterCriticalSection(mutex) != 0)
  #define MutexUnlock(mutex)                       ::LeaveCriticalSection(mutex)

  // windows vista+ conditions
  typedef VOID (WINAPI *ConditionArg)     (CONDITION_VARIABLE*);
  typedef BOOL (WINAPI *ConditionMutexArg)(CONDITION_VARIABLE*, CRITICAL_SECTION*, DWORD);

  class CConditionImpl
  {
  public:
    CConditionImpl(void);
    virtual ~CConditionImpl(void);
    void Signal(void);
    void Broadcast(void);
    bool Wait(mutex_t &mutex);
    bool Wait(mutex_t &mutex, uint32_t iTimeoutMs);

    bool                m_bOnVista;
    CONDITION_VARIABLE *m_conditionVista;
    HANDLE              m_conditionPreVista;
  };
}
