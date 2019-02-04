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

#include <atomic>
#include <string>
#include <stdint.h>
#include "Event.h"
#include "threads/ThreadImpl.h"

#ifdef TARGET_DARWIN
#include <mach/mach.h>
#endif

class IRunnable;

// minimum as mandated by XTL
#define THREAD_MINSTACKSIZE 0x10000

namespace XbmcThreads { class ThreadSettings; }

class CThread
{
protected:
  explicit CThread(const char* ThreadName);

public:
  CThread(IRunnable* pRunnable, const char* ThreadName);
  virtual ~CThread();
  void Create(bool bAutoDelete = false, unsigned stacksize = 0);
  void Sleep(unsigned int milliseconds);
  int GetSchedRRPriority(void);
  bool SetPrioritySched_RR(int iPriority);
  bool IsAutoDelete() const;
  virtual void StopThread(bool bWait = true);
  bool IsRunning() const;

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  bool IsCurrentThread() const;
  int GetMinPriority(void);
  int GetMaxPriority(void);
  int GetNormalPriority(void);
  int GetPriority(void);
  bool SetPriority(const int iPriority);
  bool WaitForThreadExit(unsigned int milliseconds);
  float GetRelativeUsage();  // returns the relative cpu usage of this thread since last call
  int64_t GetAbsoluteUsage();
  // -----------------------------------------------------------------------------------

  static bool IsCurrentThread(const ThreadIdentifier tid);
  static ThreadIdentifier GetCurrentThreadId();
  static ThreadIdentifier GetDisplayThreadId(const ThreadIdentifier tid);
  static CThread* GetCurrentThread();

  virtual void OnException(){} // signal termination handler
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
  static THREADFUNC staticThread(void *data);
  void Action();

  // -----------------------------------------------------------------------------------
  // These are platform specific and can be found in ./platform/[platform]/ThreadImpl.cpp
  // -----------------------------------------------------------------------------------
  ThreadIdentifier ThreadId() const;
  void SetThreadInfo();
  void TermHandler();
  void SetSignalHandlers();
  void SpawnThread(unsigned stacksize);
  // -----------------------------------------------------------------------------------

  ThreadIdentifier m_ThreadId;
  ThreadOpaque m_ThreadOpaque = {};
  bool m_bAutoDelete;
  CEvent m_StopEvent;
  CEvent m_TermEvent;
  CEvent m_StartEvent;
  CCriticalSection m_CriticalSection;
  IRunnable* m_pRunnable;
  uint64_t m_iLastUsage;
  uint64_t m_iLastTime;
  float m_fLastUsage;

  std::string m_ThreadName;
};
