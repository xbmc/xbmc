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

#include "Thread.h"
#ifndef _LINUX
#include <process.h>
#include "win32exception.h"
#ifndef _MT
#pragma message( "Please compile using multithreaded run-time libraries" )
#endif
typedef unsigned (WINAPI *PBEGINTHREADEX_THREADFUNC)(LPVOID lpThreadParameter);
#else
#include "PlatformInclude.h"
#include "XHandle.h"
#include <signal.h>
typedef int (*PBEGINTHREADEX_THREADFUNC)(LPVOID lpThreadParameter);
#endif

#include "utils/log.h"
#include "utils/TimeUtils.h"

#ifdef __APPLE__
//
// Use pthread's built-in support for TLS, it's more portable.
//
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t  tlsLocalThread = 0;

//
// Called once and only once.
//
static void MakeTlsKeys()
{
  pthread_key_create(&tlsLocalThread, NULL);
}

#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CThread::CThread(const char* ThreadName)
{
#ifdef __APPLE__
  // Initialize thread local storage and local thread pointer.
  pthread_once(&keyOnce, MakeTlsKeys);
#endif

  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadHandle = NULL;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_StopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

  m_pRunnable=NULL;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::CThread(IRunnable* pRunnable, const char* ThreadName)
{
#ifdef __APPLE__
  // Initialize thread local storage and local thread pointer.
  pthread_once(&keyOnce, MakeTlsKeys);
#endif

  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadHandle = NULL;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_StopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

  m_pRunnable=pRunnable;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::~CThread()
{
  if (m_ThreadHandle != NULL)
  {
    CloseHandle(m_ThreadHandle);
  }
  m_ThreadHandle = NULL;

  if (m_StopEvent)
    CloseHandle(m_StopEvent);
}

#ifdef _LINUX
#ifdef __APPLE__
// Use pthread-based TLS.
#define LOCAL_THREAD ((CThread* )pthread_getspecific(tlsLocalThread))
#else
// Use compiler-based TLS.
__thread CThread* pLocalThread = NULL;
#define LOCAL_THREAD pLocalThread
#endif
void CThread::term_handler (int signum)
{
  CLog::Log(LOGERROR,"thread 0x%lx (%lu) got signal %d. calling OnException and terminating thread abnormally.", pthread_self(), pthread_self(), signum);
  if (LOCAL_THREAD)
  {
    LOCAL_THREAD->m_bStop = TRUE;
    if (LOCAL_THREAD->m_StopEvent)
      SetEvent(LOCAL_THREAD->m_StopEvent);

    LOCAL_THREAD->OnException();
    if( LOCAL_THREAD->IsAutoDelete() )
      delete LOCAL_THREAD;
  }

  pthread_exit(NULL);
}
int CThread::staticThread(void* data)
#else
DWORD WINAPI CThread::staticThread(LPVOID* data)
#endif
{
  CThread* pThread = (CThread*)(data);
  if (!pThread) {
    CLog::Log(LOGERROR,"%s, sanity failed. thread is NULL.",__FUNCTION__);
    return 1;
  }

  if (pThread->m_ThreadName.IsEmpty())
    pThread->SetName(pThread->GetTypeName().c_str());

  CLog::Log(LOGDEBUG,"Thread %s start, auto delete: %d", pThread->m_ThreadName.c_str(), pThread->IsAutoDelete());

#ifndef _LINUX
  /* install win32 exception translator */
  win32_exception::install_handler();
#else
#ifndef __APPLE__
  pLocalThread = pThread;
#endif
  struct sigaction action;
  action.sa_handler = term_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  //sigaction (SIGABRT, &action, NULL);
  //sigaction (SIGSEGV, &action, NULL);
#endif


#ifdef __APPLE__
  // Set the TLS.
  pthread_setspecific(tlsLocalThread, (void*)pThread);
#endif

  try
  {
    pThread->OnStartup();
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
    if( pThread->IsAutoDelete() )
    {
      delete pThread;
      _endthreadex(123);
      return 0;
    }
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - thread %s, Unhandled exception caught in thread startup, aborting. auto delete: %d", __FUNCTION__, pThread->m_ThreadName.c_str(), pThread->IsAutoDelete());
    if( pThread->IsAutoDelete() )
    {
      delete pThread;
#ifndef _LINUX
      _endthreadex(123);
#endif
      return 0;
    }
  }

  try
  {
    pThread->Process();
  }
#ifndef _LINUX
  catch (const access_violation &e)
  {
    e.writelog(__FUNCTION__);
  }
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - thread %s, Unhandled exception caught in thread process, attemping cleanup in OnExit", __FUNCTION__, pThread->m_ThreadName.c_str());
  }

  try
  {
    pThread->OnExit();
  }
#ifndef _LINUX
  catch (const access_violation &e)
  {
    e.writelog(__FUNCTION__);
  }
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - thread %s, Unhandled exception caught in thread exit", __FUNCTION__, pThread->m_ThreadName.c_str());
  }

  if ( pThread->IsAutoDelete() )
  {
    CLog::Log(LOGDEBUG,"Thread %s %"PRIu64" terminating (autodelete)", pThread->m_ThreadName.c_str(), (uint64_t)CThread::GetCurrentThreadId());
    delete pThread;
    pThread = NULL;
  }
  else
    CLog::Log(LOGDEBUG,"Thread %s %"PRIu64" terminating", pThread->m_ThreadName.c_str(), (uint64_t)CThread::GetCurrentThreadId());

// DXMERGE - this looks like it might have used to have been useful for something...
//  g_graphicsContext.DeleteThreadContext();

#ifndef _LINUX
  _endthreadex(123);
#endif
  return 0;
}

void CThread::Create(bool bAutoDelete, unsigned stacksize)
{
  if (m_ThreadHandle != NULL)
  {
    throw 1; //ERROR should not b possible!!!
  }
  m_iLastTime = CTimeUtils::GetTimeMS() * 10000;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_bAutoDelete = bAutoDelete;
  m_bStop = false;
  ::ResetEvent(m_StopEvent);

  m_ThreadHandle = (HANDLE)_beginthreadex(NULL, stacksize, (PBEGINTHREADEX_THREADFUNC)staticThread, (void*)this, 0, &m_ThreadId);

#ifdef _LINUX
  if (m_ThreadHandle && m_ThreadHandle->m_threadValid && m_bAutoDelete)
    // FIXME: WinAPI can truncate 64bit pthread ids
    pthread_detach(m_ThreadHandle->m_hThread);
#endif
}

bool CThread::IsAutoDelete() const
{
  return m_bAutoDelete;
}

void CThread::StopThread(bool bWait /*= true*/)
{
  m_bStop = true;
  SetEvent(m_StopEvent);
  if (m_ThreadHandle && bWait)
  {
    WaitForThreadExit(INFINITE);
    CloseHandle(m_ThreadHandle);
    m_ThreadHandle = NULL;
  }
}

ThreadIdentifier CThread::ThreadId() const
{
#ifdef _LINUX
  if (m_ThreadHandle && m_ThreadHandle->m_threadValid)
    return m_ThreadHandle->m_hThread;
  else
    return 0;
#else
  return m_ThreadId;
#endif
}


CThread::operator HANDLE()
{
  return m_ThreadHandle;
}

CThread::operator HANDLE() const
{
  return m_ThreadHandle;
}

bool CThread::SetPriority(const int iPriority)
// Set thread priority
// Return true for success
{
  bool rtn = false;

  if (m_ThreadHandle)
  {
    rtn = SetThreadPriority( m_ThreadHandle, iPriority ) == TRUE;
  }

  return(rtn);
}

void CThread::SetPrioritySched_RR(void)
{
#ifdef __APPLE__
  // Changing to SCHED_RR is safe under OSX, you don't need elevated privileges and the
  // OSX scheduler will monitor SCHED_RR threads and drop to SCHED_OTHER if it detects
  // the thread running away. OSX automatically does this with the CoreAudio audio
  // device handler thread.
  int32_t result;
  thread_extended_policy_data_t theFixedPolicy;

  // make thread fixed, set to 'true' for a non-fixed thread
  theFixedPolicy.timeshare = false;
  result = thread_policy_set(pthread_mach_thread_np(ThreadId()), THREAD_EXTENDED_POLICY,
    (thread_policy_t)&theFixedPolicy, THREAD_EXTENDED_POLICY_COUNT);

  int policy;
  struct sched_param param;
  result = pthread_getschedparam(ThreadId(), &policy, &param );
  // change from default SCHED_OTHER to SCHED_RR
  policy = SCHED_RR;
  result = pthread_setschedparam(ThreadId(), policy, &param );
#endif
}

int CThread::GetMinPriority(void)
{
#if 0
//#if defined(__APPLE__)
  struct sched_param sched;
  int rtn, policy;

  rtn = pthread_getschedparam(ThreadId(), &policy, &sched);
  int min = sched_get_priority_min(policy);

  return(min);
#else
  return(THREAD_PRIORITY_IDLE);
#endif
}

int CThread::GetMaxPriority(void)
{
#if 0
//#if defined(__APPLE__)
  struct sched_param sched;
  int rtn, policy;

  rtn = pthread_getschedparam(ThreadId(), &policy, &sched);
  int max = sched_get_priority_max(policy);

  return(max);
#else
  return(THREAD_PRIORITY_HIGHEST);
#endif
}

int CThread::GetNormalPriority(void)
{
#if 0
//#if defined(__APPLE__)
  struct sched_param sched;
  int rtn, policy;

  rtn = pthread_getschedparam(ThreadId(), &policy, &sched);
  int min = sched_get_priority_min(policy);
  int max = sched_get_priority_max(policy);

  return( min + ((max-min) / 2)  );
#else
  return(THREAD_PRIORITY_NORMAL);
#endif
}


void CThread::SetName( LPCTSTR szThreadName )
{
  m_ThreadName = szThreadName;

#ifdef _WIN32
  const unsigned int MS_VC_EXCEPTION = 0x406d1388;
  struct THREADNAME_INFO
  {
    DWORD dwType;     // must be 0x1000
    LPCSTR szName;    // pointer to name (in same addr space)
    DWORD dwThreadID; // thread ID (-1 caller thread)
    DWORD dwFlags;    // reserved for future use, most be zero
  } info;

  info.dwType = 0x1000;
  info.szName = szThreadName;
  info.dwThreadID = m_ThreadId;
  info.dwFlags = 0;

  try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
  }
  catch(...)
  {
  }
#endif
}

// Get the thread name using the implementation dependant typeid() class
// and attempt to clean it.
CStdString CThread::GetTypeName(void)
{
  CStdString name;

  name = typeid(*this).name();

  // Visual Studio 2010 returns the name as "class CThread" etc
  if (name.substr(0, 6) == "class ")
    name = name.Right(name.length() - 6);

  // gcc provides __cxa_demangle to demangle the name
#if defined(__GNUC__)
  char* demangled;
  int   status

  demangled = __cxa_demangle(name.c_str(), NULL, 0, &status);
  if (status == 0)
    name = demangled;
  else
    CLog::Log(LOGDEBUG,"%s, __cxa_demangle(%s) failed with status %d", __FUNCTION__, name.c_str(), status);

  if (demangled)
    free(demangled);
#endif

  return name;
}

bool CThread::WaitForThreadExit(unsigned int milliseconds)
// Waits for thread to exit, timeout in given number of msec.
// Returns true when thread ended
{
  if (!m_ThreadHandle) return true;

#ifndef _LINUX
  // boost priority of thread we are waiting on to same as caller
  int callee = GetThreadPriority(m_ThreadHandle);
  int caller = GetThreadPriority(GetCurrentThread());
  if(caller > callee)
    SetThreadPriority(m_ThreadHandle, caller);

  if (::WaitForSingleObject(m_ThreadHandle, milliseconds) != WAIT_TIMEOUT)
    return true;

  // restore thread priority if thread hasn't exited
  if(caller > callee)
    SetThreadPriority(m_ThreadHandle, callee);
#else
  if (!(m_ThreadHandle->m_threadValid) || pthread_join(m_ThreadHandle->m_hThread, NULL) == 0)
  {
    m_ThreadHandle->m_threadValid = false;
    return true;
  }
#endif

  return false;
}

HANDLE CThread::ThreadHandle()
{
  return m_ThreadHandle;
}

void CThread::Process()
{
  if(m_pRunnable)
    m_pRunnable->Run();
}

float CThread::GetRelativeUsage()
{
  unsigned __int64 iTime = CTimeUtils::GetTimeMS();
  iTime *= 10000; // convert into 100ns tics

  // only update every 1 second
  if( iTime < m_iLastTime + 1000*10000 ) return m_fLastUsage;

  FILETIME CreationTime, ExitTime, UserTime, KernelTime;
  if( GetThreadTimes( m_ThreadHandle, &CreationTime, &ExitTime, &KernelTime, &UserTime ) )
  {
    unsigned __int64 iUsage = 0;
    iUsage += (((unsigned __int64)UserTime.dwHighDateTime) << 32) + ((unsigned __int64)UserTime.dwLowDateTime);
    iUsage += (((unsigned __int64)KernelTime.dwHighDateTime) << 32) + ((unsigned __int64)KernelTime.dwLowDateTime);

    if(m_iLastUsage > 0 && m_iLastTime > 0)
      m_fLastUsage = (float)( iUsage - m_iLastUsage ) / (float)( iTime - m_iLastTime );

    m_iLastUsage = iUsage;
    m_iLastTime = iTime;

    return m_fLastUsage;
  }
  return 0.0f;
}

bool CThread::IsCurrentThread() const
{
  return IsCurrentThread(ThreadId());
}


ThreadIdentifier CThread::GetCurrentThreadId()
{
#ifdef _LINUX
  return pthread_self();
#else
  return ::GetCurrentThreadId();
#endif
}

bool CThread::IsCurrentThread(const ThreadIdentifier tid)
{
#ifdef _LINUX
  return pthread_equal(pthread_self(), tid);
#else
  return (::GetCurrentThreadId() == tid);
#endif
}


DWORD CThread::WaitForSingleObject(HANDLE hHandle, unsigned int milliseconds)
{
  if(milliseconds > 10 && IsCurrentThread())
  {
    HANDLE handles[2] = {hHandle, m_StopEvent};
    DWORD result = ::WaitForMultipleObjects(2, handles, false, milliseconds);

    if(result == WAIT_TIMEOUT || result == WAIT_OBJECT_0)
      return result;

    if( milliseconds == INFINITE )
      return WAIT_ABANDONED;
    else
      return WAIT_TIMEOUT;
  }
  else
    return ::WaitForSingleObject(hHandle, milliseconds);
}

DWORD CThread::WaitForMultipleObjects(DWORD nCount, HANDLE *lpHandles, BOOL bWaitAll, unsigned int milliseconds)
{
  // for now not implemented
  return ::WaitForMultipleObjects(nCount, lpHandles, bWaitAll, milliseconds);
}

void CThread::Sleep(unsigned int milliseconds)
{
  if(milliseconds > 10 && IsCurrentThread())
    ::WaitForSingleObject(m_StopEvent, milliseconds);
  else
    ::Sleep(milliseconds);
}
