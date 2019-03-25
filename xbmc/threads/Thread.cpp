/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/SystemClock.h"
#include "Thread.h"
#include "IRunnable.h"
#include "threads/SingleLock.h"
#include "commons/Exception.h"
#include <stdlib.h>
#include "utils/log.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

static thread_local CThread* currentThread;

#include "threads/platform/ThreadImpl.cpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CThread::CThread(const char* ThreadName)
: m_StopEvent(true,true), m_TermEvent(true), m_StartEvent(true, true)
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;

  m_pRunnable=NULL;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::CThread(IRunnable* pRunnable, const char* ThreadName)
: m_StopEvent(true, true), m_TermEvent(true), m_StartEvent(true, true)
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;

  m_pRunnable=pRunnable;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::~CThread()
{
  StopThread();
}

void CThread::Create(bool bAutoDelete, unsigned stacksize)
{
  if (m_ThreadId != 0)
  {
    CLog::Log(LOGERROR, "%s - fatal error creating thread %s - old thread id not null", __FUNCTION__, m_ThreadName.c_str());
    exit(1);
  }
  m_iLastTime = XbmcThreads::SystemClockMillis() * 10000ULL;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_bAutoDelete = bAutoDelete;
  m_bStop = false;
  m_StopEvent.Reset();
  m_TermEvent.Reset();
  m_StartEvent.Reset();

  SpawnThread(stacksize);
}

bool CThread::IsRunning() const
{
  return m_ThreadId ? true : false;
}

THREADFUNC CThread::staticThread(void* data)
{
  CThread* pThread = static_cast<CThread*>(data);
  std::string name;
  ThreadIdentifier id;
  bool autodelete;

  if (!pThread) {
    CLog::Log(LOGERROR,"%s, sanity failed. thread is NULL.",__FUNCTION__);
    return 1;
  }

  name = pThread->m_ThreadName;
  id = GetDisplayThreadId(pThread->m_ThreadId);
  autodelete = pThread->m_bAutoDelete;

  pThread->SetThreadInfo();

  CLog::Log(LOGDEBUG,"Thread %s start, auto delete: %s", name.c_str(), (autodelete ? "true" : "false"));

  currentThread = pThread;
  pThread->m_StartEvent.Set();

  pThread->Action();

  // lock during termination
  CSingleLock lock(pThread->m_CriticalSection);

  pThread->m_ThreadId = 0;
  pThread->m_TermEvent.Set();
  pThread->TermHandler();

  lock.Leave();

  if (autodelete)
  {
    CLog::Log(LOGDEBUG,"Thread %s %" PRIu64" terminating (autodelete)", name.c_str(), (uint64_t)id);
    delete pThread;
    pThread = NULL;
  }
  else
    CLog::Log(LOGDEBUG,"Thread %s %" PRIu64" terminating", name.c_str(), (uint64_t)id);

  return 0;
}

bool CThread::IsAutoDelete() const
{
  return m_bAutoDelete;
}

void CThread::StopThread(bool bWait /*= true*/)
{
  m_StartEvent.Wait();

  m_bStop = true;
  m_StopEvent.Set();
  CSingleLock lock(m_CriticalSection);
  if (m_ThreadId && bWait && !IsCurrentThread(m_ThreadId))
  {
    // We will leave the critical Section before WaitingForThreadExit.
    // But we have to Reacquire it after the wait so that CThread::staticThread
    // does not get removed its thread->m_CriticalSection before it can unlock
    // it. As CThread::staticThread holds the lock already at this point when
    // sending the term signal, StopThread will synchronize so that CThread-Object
    // is not removed prior the staticThread is accessing its members
    CSingleExit exit(m_CriticalSection);
    WaitForThreadExit(0xFFFFFFFF);
  }
}

ThreadIdentifier CThread::ThreadId() const
{
  return m_ThreadId;
}

void CThread::Process()
{
  if(m_pRunnable)
    m_pRunnable->Run();
}

bool CThread::IsCurrentThread() const
{
  return IsCurrentThread(ThreadId());
}

CThread* CThread::GetCurrentThread()
{
  return currentThread;
}

void CThread::Sleep(unsigned int milliseconds)
{
  if(milliseconds > 10 && IsCurrentThread())
    m_StopEvent.WaitMSec(milliseconds);
  else
    XbmcThreads::ThreadSleep(milliseconds);
}

void CThread::Action()
{
  try
  {
    OnStartup();
  }
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("OnStartup");
    if (IsAutoDelete())
      return;
  }

  try
  {
    Process();
  }
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("Process");
  }

  try
  {
    OnExit();
  }
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("OnExit");
  }
}

