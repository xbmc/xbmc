/*
* XBoxMediaPlayer
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

#include "stdafx.h"
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
#include <signal.h>
typedef int (*PBEGINTHREADEX_THREADFUNC)(LPVOID lpThreadParameter);
#endif

#include "log.h"
#include "GraphicContext.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


#define MS_VC_EXCEPTION 0x406d1388
typedef struct tagTHREADNAME_INFO 
{ 
  DWORD dwType; // must be 0x1000 
  LPCSTR szName; // pointer to name (in same addr space) 
  DWORD dwThreadID; // thread ID (-1 caller thread) 
  DWORD dwFlags; // reserved for future use, most be zero 
} THREADNAME_INFO;

CThread::CThread()
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadHandle = NULL;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_StopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

  m_pRunnable=NULL;
}

CThread::CThread(IRunnable* pRunnable)
{  
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadHandle = NULL;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_StopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

  m_pRunnable=pRunnable;
}

CThread::~CThread()
{
  if (m_ThreadHandle != NULL)
  {
    CloseHandle(m_ThreadHandle);
  }
  m_ThreadHandle = NULL;
}


#ifdef _LINUX
__thread CThread* pLocalThread = NULL;
void CThread::term_handler (int signum)
{
  CLog::Log(LOGERROR,"thread 0x%x (%lu) got signal %d. calling OnException and terminating thread abnormally.", SDL_ThreadID(), SDL_ThreadID(), signum);
  if (pLocalThread)
  {
    pLocalThread->m_bStop = TRUE;
    if (pLocalThread->m_StopEvent)
      SetEvent(pLocalThread->m_StopEvent);

    pLocalThread->OnException();
    if( pLocalThread->IsAutoDelete() )
      delete pLocalThread;
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

  CLog::Log(LOGDEBUG,"thread start, auto delete: %d",pThread->IsAutoDelete());

#ifndef _LINUX
  /* install win32 exception translator */
  win32_exception::install_handler();
#else
  pLocalThread = pThread;
  struct sigaction action;
  action.sa_handler = term_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  //sigaction (SIGABRT, &action, NULL);
  //sigaction (SIGSEGV, &action, NULL);
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
    CLog::Log(LOGERROR, "%s - Unhandled exception caught in thread startup, aborting. auto delete: %d", __FUNCTION__, pThread->IsAutoDelete());
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
    CLog::Log(LOGERROR, "%s - Unhandled exception caught in thread process, attemping cleanup in OnExit", __FUNCTION__); 
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
    CLog::Log(LOGERROR, "%s - Unhandled exception caught in thread exit", __FUNCTION__); 
  }

  if ( pThread->IsAutoDelete() )
  {
    CLog::Log(LOGDEBUG,"%s, deleting thread object", __FUNCTION__);
    delete pThread;
    pThread = NULL;
  }

  CLog::Log(LOGDEBUG,"%s, deleting thread graphic context", __FUNCTION__);
  g_graphicsContext.DeleteThreadContext();

  CLog::Log(LOGDEBUG,"Thread %lu terminating",GetCurrentThreadId());

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
  m_iLastTime = GetTickCount() * 10000;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_bAutoDelete = bAutoDelete;
  m_bStop = false;
  ::ResetEvent(m_StopEvent);
 
  m_ThreadHandle = (HANDLE)_beginthreadex(NULL, stacksize, (PBEGINTHREADEX_THREADFUNC)staticThread, (void*)this, 0, &m_ThreadId);

}


bool CThread::IsAutoDelete() const
{
  return m_bAutoDelete;
}

void CThread::StopThread()
{
  m_bStop = true;
  SetEvent(m_StopEvent);
  if (m_ThreadHandle)
  {
    WaitForThreadExit(INFINITE);
    CloseHandle(m_ThreadHandle);
    m_ThreadHandle = NULL;
  }
}

DWORD CThread::ThreadId() const
{
  return (DWORD)m_ThreadId;
}


CThread::operator HANDLE()
{
  return m_ThreadHandle;
}

CThread::operator const HANDLE() const
{
  return m_ThreadHandle;
}

bool CThread::SetPriority(const int iPriority)
// Set thread priority
// Return true for success
{
  if (m_ThreadHandle)
  {
#ifndef _LINUX
    return ( SetThreadPriority( m_ThreadHandle, iPriority ) == TRUE );
#else
	return true;
#endif

  }
  else
  {
    return false;
  }
}

void CThread::SetName( LPCTSTR szThreadName )
{
  THREADNAME_INFO info; 
  info.dwType = 0x1000; 
  info.szName = szThreadName; 
  info.dwThreadID = m_ThreadId;
  info.dwFlags = 0; 
#ifndef _LINUX
  try 
  { 
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info); 
  } 
  catch(...)
  { 
  }  
#endif
}

bool CThread::WaitForThreadExit(DWORD dwMilliseconds)
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
#endif

  if (::WaitForSingleObject(m_ThreadHandle, dwMilliseconds) != WAIT_TIMEOUT)
    return true;

#ifndef _LINUX
  // restore thread priority if thread hasn't exited
  if(caller > callee)
    SetThreadPriority(m_ThreadHandle, callee);
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
  unsigned __int64 iTime = GetTickCount();
  iTime *= 10000; // convert into 100ns tics

  // only update every 1 second
  if( iTime < m_iLastTime + 1000*10000 ) return m_fLastUsage;

#ifndef _LINUX
  FILETIME CreationTime, ExitTime, UserTime, KernelTime;
  if( GetThreadTimes( m_ThreadHandle, &CreationTime, &ExitTime, &KernelTime, &UserTime ) )
  {    

    unsigned __int64 iUsage = 0;
    iUsage += (((unsigned __int64)UserTime.dwHighDateTime) << 32) + ((unsigned __int64)UserTime.dwLowDateTime);
    iUsage += (((unsigned __int64)KernelTime.dwHighDateTime) << 32) + ((unsigned __int64)KernelTime.dwLowDateTime);

    m_fLastUsage = (float)( iUsage - m_iLastUsage ) / (float)( iTime - m_iLastTime );
    m_iLastUsage = iUsage;
    m_iLastTime = iTime;

    return m_fLastUsage;
  }    
#endif
  return 0.0f; 
}

DWORD CThread::WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
  if(dwMilliseconds > 10)
  {
    HANDLE handles[2] = {hHandle, m_StopEvent};
    DWORD result = ::WaitForMultipleObjects(2, handles, false, dwMilliseconds);

    if(result == WAIT_TIMEOUT 
    || result == WAIT_OBJECT_0)
      return result;

    if( dwMilliseconds == INFINITE )
      return WAIT_ABANDONED;
    else
      return WAIT_TIMEOUT;
  }
  else
    return ::WaitForSingleObject(hHandle, dwMilliseconds);
}

DWORD CThread::WaitForMultipleObjects(DWORD nCount, HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
  // for now not implemented
  return ::WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
}

void CThread::Sleep(DWORD dwMilliseconds)
{
  if(dwMilliseconds > 10)
    ::WaitForSingleObject(m_StopEvent, dwMilliseconds);
  else
    ::Sleep(dwMilliseconds);
}
