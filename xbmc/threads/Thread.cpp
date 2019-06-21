/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#define __STDC_FORMAT_MACROS

#include "Thread.h"
#include "IRunnable.h"

#include "commons/Exception.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

#include <atomic>
#include <inttypes.h>
#include <iostream>
#include <stdlib.h>

static thread_local CThread* currentThread;

// This is including .cpp code so should be after the other #includes
#include "threads/platform/ThreadImpl.cpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CThread::CThread(const char* ThreadName)
:
    m_bStop(false), m_StopEvent(true, true), m_StartEvent(true, true), m_pRunnable(nullptr)
{
  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::CThread(IRunnable* pRunnable, const char* ThreadName)
:
    m_bStop(false), m_StopEvent(true, true), m_StartEvent(true, true), m_pRunnable(pRunnable)
{
  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::~CThread()
{
  StopThread();
  if (m_thread != nullptr)
  {
    m_thread->detach();
    delete m_thread;
  }
}

void CThread::Create(bool bAutoDelete)
{
  if (m_thread != nullptr)
  {
    // if the thread exited on it's own, without a call to StopThread, then we can get here
    // incorrectly. We should be able to determine this by checking the promise.
    std::future_status stat = m_future.wait_for(std::chrono::milliseconds(0));
    // a status of 'ready' means the future contains the value so the thread has exited
    // since the thread can't exit without setting the future.
    if (stat == std::future_status::ready) // this is an indication the thread has exited.
      StopThread(true);  // so let's just clean up
    else
    { // otherwise we have a problem.
      CLog::Log(LOGERROR, "%s - fatal error creating thread %s - old thread id not null", __FUNCTION__, m_ThreadName.c_str());
      exit(1);
    }
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

  {
    // The std::thread internals must be set prior to the lambda doing
    //   any work. This will cause the lambda to wait until m_thread
    //   is fully initialized. Interestingly, using a std::atomic doesn't
    //   have the appropriate memory barrier behavior to accomplish the
    //   same thing so a full system mutex needs to be used.
    CSingleLock blockLambdaTillDone(m_CriticalSection);
    m_thread = new std::thread([](CThread* pThread, std::promise<bool> promise)
    {
      try
      {

        {
          // Wait for the pThread->m_thread internals to be set. Otherwise we could
          // get to a place where we're reading, say, the thread id inside this
          // lambda's call stack prior to the thread that kicked off this lambda
          // having it set. Once this lock is released, the CThread::Create function
          // that kicked this off is done so everything should be set.
          CSingleLock waitForThreadInternalsToBeSet(pThread->m_CriticalSection);
        }

        // This is used in various helper methods like GetCurrentThread so it needs
        // to be set before anything else is done.
        currentThread = pThread;

        std::string name;
        bool autodelete;

        if (pThread == nullptr)
        {
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
      }
      catch (const std::exception& e)
      {
        CLog::Log(LOGDEBUG,"Thread Terminating with Exception: %s", e.what());
      }
      catch (...)
      {
        CLog::Log(LOGDEBUG,"Thread Terminating with Exception");
      }

      promise.set_value(true);
    }, this, std::move(prom));
  } // let the lambda proceed

  m_StartEvent.Wait(); // wait for the thread just spawned to set its internals
}

bool CThread::IsRunning() const
{
  if (m_thread != nullptr) {
    // it's possible that the thread exited on it's own without a call to StopThread. If so then
    // the promise should be fulfilled.
    std::future_status stat = m_future.wait_for(std::chrono::milliseconds(0));
    // a status of 'ready' means the future contains the value so the thread has exited
    // since the thread can't exit without setting the future.
    if (stat == std::future_status::ready) // this is an indication the thread has exited.
      return false;
    return true; // otherwise the thread is still active.
  } else
    return false;
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
  if (m_pRunnable)
    m_pRunnable->Run();
}

bool CThread::IsCurrentThread() const
{
  CThread* pThread = currentThread;
  if (pThread != nullptr)
    return pThread == this;
  else
    return false;
}

CThread* CThread::GetCurrentThread()
{
  return currentThread;
}

void CThread::TermHandler()
{
}

void CThread::Sleep(unsigned int milliseconds)
{
  if (milliseconds > 10 && IsCurrentThread())
    m_StopEvent.WaitMSec(milliseconds);
  else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

bool CThread::Join(unsigned int milliseconds)
{
  CSingleLock l(m_CriticalSection);
  std::thread* lthread = m_thread;
  if (lthread != nullptr)
  {
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
  }
  else
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

float CThread::GetRelativeUsage()
{
  unsigned int iTime = XbmcThreads::SystemClockMillis();
  iTime *= 10000; // convert into 100ns tics

  // only update every 1 second
  if (iTime < m_iLastTime + 1000 * 10000)
    return m_fLastUsage;

  int64_t iUsage = GetAbsoluteUsage();

  if (m_iLastUsage > 0 && m_iLastTime > 0)
    m_fLastUsage = static_cast<float>(iUsage - m_iLastUsage) / static_cast<float>(iTime - m_iLastTime);

  m_iLastUsage = iUsage;
  m_iLastTime = iTime;

  return m_fLastUsage;
}

