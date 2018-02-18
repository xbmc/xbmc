/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
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

#include "threads/SystemClock.h"
#include "Thread.h"
#include "threads/SingleLock.h"
#include "commons/Exception.h"
#include <stdlib.h>
#include "utils/log.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

static thread_local CThread* currentThread;

#include "threads/platform/ThreadImpl.cpp"
#include <iostream>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const std::thread::id CThread::nullThreadId(0);

CThread::CThread(const char* ThreadName)
: m_StopEvent(true,true), m_StartEvent(true), m_thread(nullptr)
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;

  m_pRunnable=NULL;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::CThread(IRunnable* pRunnable, const char* ThreadName)
: m_StopEvent(true,true), m_StartEvent(true), m_thread(nullptr)
{
  m_bStop = false;

  m_bAutoDelete = false;
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
  if (m_thread != nullptr) {
    m_thread->detach();
    delete m_thread;
  }
}

void CThread::Create(bool bAutoDelete)
{
  if (m_thread != nullptr)
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
  m_StartEvent.Reset();

  // lock?
  //CSingleLock l(m_CriticalSection);

  std::promise<bool> prom;
  m_future = prom.get_future();
  m_thread = new std::thread([](CThread* pThread, std::promise<bool> promise) {
    std::string name;
    bool autodelete;

    if (pThread == nullptr) {
      CLog::Log(LOGERROR,"%s, sanity failed. thread is NULL.",__FUNCTION__);
      promise.set_value(false);
      return;
    }

    name = pThread->m_ThreadName;

    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string id = ss.str();
    autodelete = pThread->m_bAutoDelete;

    pThread->SetThreadInfo();

    CLog::Log(LOGDEBUG,"Thread %s start, auto delete: %s", name.c_str(), (autodelete ? "true" : "false"));

    currentThread = pThread;
    pThread->m_StartEvent.Set();

    pThread->Action();

    // lock during termination
    {
      CSingleLock lock(pThread->m_CriticalSection);
      pThread->TermHandler();
    }

    if (autodelete)
    {
      CLog::Log(LOGDEBUG,"Thread %s %s terminating (autodelete)", name.c_str(), id.c_str());
      delete pThread;
      pThread = NULL;
    }
    else
      CLog::Log(LOGDEBUG,"Thread %s %s terminating", name.c_str(), id.c_str());

    promise.set_value(true);
    return;
  }, this, std::move(prom));
}

bool CThread::IsRunning() const
{
  return m_thread != nullptr ? true : false;
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
  std::thread* lthread = m_thread;
  if (lthread != nullptr && bWait && !IsCurrentThread())
  {
    lock.Leave();
    if (!Join(0xFFFFFFFF)) // eh?
      lthread->join();
    m_thread = nullptr;
  }
}

void CThread::Process()
{
  if(m_pRunnable)
    m_pRunnable->Run();
}

bool CThread::IsCurrentThread() const
{
  if (m_thread != nullptr)
    return std::this_thread::get_id() == m_thread->get_id();
  else
    return false;
}

CThread* CThread::GetCurrentThread()
{
  return currentThread;
}

void CThread::TermHandler() { }

void CThread::Sleep(unsigned int milliseconds)
{
  if(milliseconds > 10 && IsCurrentThread())
    m_StopEvent.WaitMSec(milliseconds);
  else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

bool CThread::Join(unsigned int milliseconds)
{
  CSingleLock l(m_CriticalSection);
  std::thread* lthread = m_thread;
  if (lthread != nullptr) {
    if (IsCurrentThread())
      return false;

    {
      CSingleExit exit(m_CriticalSection); // don't hold the thread lock while we're waiting
      std::future_status stat = m_future.wait_for(std::chrono::milliseconds(milliseconds));
      if (stat != std::future_status::ready)
        return false;
    }

    // it's possible it's already joined since we released the lock above.
    if (lthread->joinable())
      m_thread->join();
    return true;
  } else
    return false;
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
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - thread %s, Unhandled exception caught in thread startup, aborting. auto delete: %d", __FUNCTION__, m_ThreadName.c_str(), IsAutoDelete());
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
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - thread %s, Unhandled exception caught in thread process, aborting. auto delete: %d", __FUNCTION__, m_ThreadName.c_str(), IsAutoDelete());
  }

  try
  {
    OnExit();
  }
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("OnExit");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - thread %s, Unhandled exception caught in thread OnExit, aborting. auto delete: %d", __FUNCTION__, m_ThreadName.c_str(), IsAutoDelete());
  }
}

float CThread::GetRelativeUsage()
{
  unsigned int iTime = XbmcThreads::SystemClockMillis();
  iTime *= 10000; // convert into 100ns tics

  // only update every 1 second
  if( iTime < m_iLastTime + 1000*10000 ) return m_fLastUsage;

  int64_t iUsage = GetAbsoluteUsage();

  if (m_iLastUsage > 0 && m_iLastTime > 0)
    m_fLastUsage = (float)( iUsage - m_iLastUsage ) / (float)( iTime - m_iLastTime );

  m_iLastUsage = iUsage;
  m_iLastTime = iTime;

  return m_fLastUsage;
}

