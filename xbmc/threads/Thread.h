/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Thread.h: interface for the CThread class.
//
//////////////////////////////////////////////////////////////////////

#include "Event.h"

#include "threads/platform/ThreadImpl.h"

#include <atomic>
#include <future>
#ifdef TARGET_DARWIN
#include <mach/mach.h>
#endif
#include <stdint.h>
#include <string>
#include <thread>

class IRunnable;

class CThread
{
protected:
  explicit CThread(const char* ThreadName);

public:
  CThread(IRunnable* pRunnable, const char* ThreadName);
  virtual ~CThread();
  void Create(bool bAutoDelete = false);

  template<typename Rep, typename Period>
  void Sleep(std::chrono::duration<Rep, Period> duration)
  {
    if (duration > std::chrono::milliseconds(10) && IsCurrentThread())
      m_StopEvent.Wait(duration);
    else
      std::this_thread::sleep_for(duration);
  }

  bool IsAutoDelete() const;
  virtual void StopThread(bool bWait = true);
  bool IsRunning() const;

  bool IsCurrentThread() const;
  bool Join(std::chrono::milliseconds duration);

  inline static const std::thread::id GetCurrentThreadId()
  {
    return std::this_thread::get_id();
  }

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  static int GetMinPriority(void);
  static int GetMaxPriority(void);
  static int GetNormalPriority(void);
  static std::uintptr_t GetCurrentThreadNativeHandle();
  static uint64_t GetCurrentThreadNativeId();

  // Get and set the thread's priority
  int GetPriority(void);
  bool SetPriority(const int iPriority);

  // -----------------------------------------------------------------------------------

  static CThread* GetCurrentThread();

  virtual void OnException(){} // signal termination handler

protected:
  virtual void OnStartup() {}
  virtual void OnExit() {}
  virtual void Process();

  std::atomic<bool> m_bStop;

  enum WaitResponse { WAIT_INTERRUPTED = -1, WAIT_SIGNALED = 0, WAIT_TIMEDOUT = 1 };

  /**
   * This call will wait on a CEvent in an interruptible way such that if
   *  stop is called on the thread the wait will return with a response
   *  indicating what happened.
   */
  inline WaitResponse AbortableWait(CEvent& event,
                                    std::chrono::milliseconds duration =
                                        std::chrono::milliseconds(-1) /* indicates wait forever*/)
  {
    XbmcThreads::CEventGroup group{&event, &m_StopEvent};
    CEvent* result =
        duration < std::chrono::milliseconds::zero() ? group.wait() : group.wait(duration);
    return  result == &event ? WAIT_SIGNALED :
      (result == NULL ? WAIT_TIMEDOUT : WAIT_INTERRUPTED);
  }

private:
  void Action();

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  void SetThreadInfo(); // called from the spawned thread
  void TermHandler();
  void SetSignalHandlers();
  // -----------------------------------------------------------------------------------

  bool m_bAutoDelete = false;
  CEvent m_StopEvent;
  CEvent m_StartEvent;
  CCriticalSection m_CriticalSection;
  IRunnable* m_pRunnable;

  std::string m_ThreadName;
  std::thread* m_thread = nullptr;
  std::future<bool> m_future;

  // Platform specific hangers-on
  ThreadLwpId m_lwpId = 0;
};
