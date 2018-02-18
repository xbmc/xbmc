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

// Thread.h: interface for the CThread class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <atomic>
#include <string>
#include <stdint.h>
#include "Event.h"

#ifdef TARGET_DARWIN
#include <mach/mach.h>
#endif

#include <thread>
#include <future>

class IRunnable
{
public:
  virtual void Run()=0;
  virtual ~IRunnable() = default;
};

namespace XbmcThreads { class ThreadSettings; }

class CThread
{
protected:
  explicit CThread(const char* ThreadName);

public:
  CThread(IRunnable* pRunnable, const char* ThreadName);
  virtual ~CThread();
  void Create(bool bAutoDelete = false);
  void Sleep(unsigned int milliseconds);
  bool IsAutoDelete() const;
  virtual void StopThread(bool bWait = true);
  bool IsRunning() const;

  bool IsCurrentThread() const;
  bool Join(unsigned int milliseconds);

  inline static const std::thread::id GetCurrentThreadId() {
    CThread* cur = GetCurrentThread();
    return cur == nullptr ? std::thread::id(0) :
        (cur->m_thread == nullptr ? std::thread::id(0) : cur->m_thread->get_id());
  }

  inline static const std::thread::native_handle_type GetCurrentThreadNativeHandle() {
    CThread* cur = GetCurrentThread();
    return cur == nullptr ? 0 :
        (cur->m_thread == nullptr ? 0 : cur->m_thread->native_handle());
  }

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  static int GetMinPriority(void);
  static int GetMaxPriority(void);
  static int GetNormalPriority(void);

  // Get and set the CURRENT thread priority
  static int GetPriority(void);
  static bool SetPriority(const int iPriority);

  float GetRelativeUsage();  // returns the relative cpu usage of this thread since last call
  int64_t GetAbsoluteUsage();
  // -----------------------------------------------------------------------------------

  static CThread* GetCurrentThread();

  virtual void OnException(){} // signal termination handler

  static const std::thread::id nullThreadId;
protected:
  virtual void OnStartup(){};
  virtual void OnExit(){};
  virtual void Process();

  std::atomic<bool> m_bStop;

  enum WaitResponse { WAIT_INTERRUPTED = -1, WAIT_SIGNALED = 0, WAIT_TIMEDOUT = 1 };

  /**
   * This call will wait on a CEvent in an interruptible way such that if
   *  stop is called on the thread the wait will return with a response
   *  indicating what happened.
   */
  inline WaitResponse AbortableWait(CEvent& event, int timeoutMillis = -1 /* indicates wait forever*/)
  {
    XbmcThreads::CEventGroup group{&event, &m_StopEvent};
    CEvent* result = timeoutMillis < 0 ? group.wait() : group.wait(timeoutMillis);
    return  result == &event ? WAIT_SIGNALED :
      (result == NULL ? WAIT_TIMEDOUT : WAIT_INTERRUPTED);
  }

private:
  void Action();

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  void SetThreadInfo();
  void TermHandler();
  void SetSignalHandlers();
  void SpawnThread(unsigned stacksize);
  // -----------------------------------------------------------------------------------

  bool m_bAutoDelete;
  CEvent m_StopEvent;
  CEvent m_StartEvent;
  CCriticalSection m_CriticalSection;
  IRunnable* m_pRunnable;
  uint64_t m_iLastUsage;
  uint64_t m_iLastTime;
  float m_fLastUsage;

  std::string m_ThreadName;
  std::thread* m_thread;
  std::future<bool> m_future;
};
