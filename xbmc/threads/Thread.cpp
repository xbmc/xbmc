/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
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
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "threads/SystemClock.h"
#include "Thread.h"
#include "utils/log.h"
#include "threads/ThreadLocal.h"
#include "threads/SingleLock.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

static XbmcThreads::ThreadLocal<CThread> currentThread;

XbmcCommons::ILogger* CThread::logger = NULL;

#include "threads/platform/ThreadImpl.cpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CThread::CThread(const char* ThreadName)
: m_StopEvent(true,true), m_TermEvent(true), m_StartEvent(true)
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
: m_StopEvent(true,true), m_TermEvent(true), m_StartEvent(true)
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

bool CThread::IsRunning()
{
  return m_ThreadId ? true : false;
}

THREADFUNC CThread::staticThread(void* data)
{
  CThread* pThread = (CThread*)(data);
  std::string name;
  ThreadIdentifier id;
  bool autodelete;

  if (!pThread) {
    if(logger) logger->Log(LOGERROR,"%s, sanity failed. thread is NULL.",__FUNCTION__);
    return 1;
  }

  name = pThread->m_ThreadName;
  id = pThread->m_ThreadId;
  autodelete = pThread->m_bAutoDelete;

  pThread->SetThreadInfo();

  if(logger) logger->Log(LOGNOTICE,"Thread %s start, auto delete: %s", name.c_str(), (autodelete ? "true" : "false"));

  currentThread.set(pThread);
  pThread->m_StartEvent.Set();

  pThread->OnStartup();
  pThread->Process();
  pThread->OnExit();

  // lock during termination
  CSingleLock lock(pThread->m_CriticalSection);

  pThread->m_ThreadId = 0;
  pThread->m_TermEvent.Set();
  pThread->TermHandler();

  lock.Leave();

  if (autodelete)
  {
    if(logger) logger->Log(LOGDEBUG,"Thread %s %"PRIu64" terminating (autodelete)", name.c_str(), (uint64_t)id);
    delete pThread;
    pThread = NULL;
  }
  else
    if(logger) logger->Log(LOGDEBUG,"Thread %s %"PRIu64" terminating", name.c_str(), (uint64_t)id);

  return 0;
}

bool CThread::IsAutoDelete() const
{
  return m_bAutoDelete;
}

void CThread::StopThread(bool bWait /*= true*/)
{
  m_bStop = true;
  m_StopEvent.Set();
  CSingleLock lock(m_CriticalSection);
  if (m_ThreadId && bWait)
  {
    lock.Leave();
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
  return currentThread.get();
}

void CThread::Sleep(unsigned int milliseconds)
{
  if(milliseconds > 10 && IsCurrentThread())
    m_StopEvent.WaitMSec(milliseconds);
  else
    XbmcThreads::ThreadSleep(milliseconds);
}


