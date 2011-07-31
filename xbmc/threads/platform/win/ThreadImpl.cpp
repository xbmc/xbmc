/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <windows.h>


void CThread::Create(bool bAutoDelete, unsigned stacksize)
{
  if (m_ThreadId != 0)
  {
    CLog::Log(LOGERROR, "%s - fatal error creating thread- old thread id not null", __FUNCTION__);
    exit(1);
  }
  m_iLastTime = XbmcThreads::SystemClockMillis() * 10000;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_bAutoDelete = bAutoDelete;
  m_bStop = false;
  m_StopEvent.Reset();
  m_TermEvent.Reset();
  m_StartEvent.Reset();

  m_ThreadOpaque.handle = CreateThread(NULL,stacksize, (LPTHREAD_START_ROUTINE)&staticThread, this, 0, &m_ThreadId);
  if (m_ThreadOpaque.handle == NULL)
  {
    CLog::Log(LOGERROR, "%s - fatal error creating thread", __FUNCTION__);
  }
}

void CThread::TermHandler()
{
  CloseHandle(m_ThreadOpaque.handle);
  m_ThreadOpaque.handle = NULL;
}

void CThread::SetThreadInfo()
{
  const unsigned int MS_VC_EXCEPTION = 0x406d1388;
  struct THREADNAME_INFO
  {
    DWORD dwType; // must be 0x1000
    LPCSTR szName; // pointer to name (in same addr space)
    DWORD dwThreadID; // thread ID (-1 caller thread)
    DWORD dwFlags; // reserved for future use, most be zero
  } info;

  info.dwType = 0x1000;
  info.szName = m_ThreadName.c_str();
  info.dwThreadID = m_ThreadId;
  info.dwFlags = 0;

  try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
  }
  catch(...)
  {
  }
}

ThreadIdentifier CThread::GetCurrentThreadId()
{
  return ::GetCurrentThreadId();
}

bool CThread::IsCurrentThread(const ThreadIdentifier tid)
{
  return (::GetCurrentThreadId() == tid);
}

int CThread::GetMinPriority(void)
{
  return(THREAD_PRIORITY_IDLE);
}

int CThread::GetMaxPriority(void)
{
  return(THREAD_PRIORITY_HIGHEST);
}

int CThread::GetNormalPriority(void)
{
  return(THREAD_PRIORITY_NORMAL);
}

int CThread::GetSchedRRPriority(void)
{
  return GetNormalPriority();
}

bool CThread::SetPriority(const int iPriority)
{
  bool bReturn = false;

  CSingleLock lock(m_CriticalSection);
  if (m_ThreadOpaque.handle)
  {
    bReturn = SetThreadPriority(m_ThreadOpaque.handle, iPriority) == TRUE;
  }

  return bReturn;
}

int CThread::GetPriority()
{
  CSingleLock lock(m_CriticalSection);

  int iReturn = THREAD_PRIORITY_NORMAL;
  if (m_ThreadOpaque.handle)
  {
    iReturn = GetThreadPriority(m_ThreadOpaque.handle);
  }
  return iReturn;
}

bool CThread::WaitForThreadExit(unsigned int milliseconds)
{
  bool bReturn = true;

  CSingleLock lock(m_CriticalSection);
  if (m_ThreadId && m_ThreadOpaque.handle != NULL)
  {
    // boost priority of thread we are waiting on to same as caller
    int callee = GetThreadPriority(m_ThreadOpaque.handle);
    int caller = GetThreadPriority(GetCurrentThread());
    if(caller > callee)
      SetThreadPriority(m_ThreadOpaque.handle, caller);

    lock.Leave();
    bReturn = m_TermEvent.WaitMSec(milliseconds);
    lock.Enter();

    // restore thread priority if thread hasn't exited
    if(caller > callee && m_ThreadOpaque.handle)
      SetThreadPriority(m_ThreadOpaque.handle, callee);
  }
  return bReturn;
}

int64_t CThread::GetAbsoluteUsage()
{
  CSingleLock lock(m_CriticalSection);

  if (!m_ThreadOpaque.handle)
    return 0;

  uint64_t time = 0;
  FILETIME CreationTime, ExitTime, UserTime, KernelTime;
  if( GetThreadTimes(m_ThreadOpaque.handle, &CreationTime, &ExitTime, &KernelTime, &UserTime ) )
  {
    time = (((uint64_t)UserTime.dwHighDateTime) << 32) + ((uint64_t)UserTime.dwLowDateTime);
    time += (((uint64_t)KernelTime.dwHighDateTime) << 32) + ((uint64_t)KernelTime.dwLowDateTime);
  }
  return time;
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

int64_t CThread::GetCurrentThreadUsage()
{
  HANDLE h = GetCurrentThread();
  
  uint64_t time = 0;
  FILETIME CreationTime, ExitTime, UserTime, KernelTime;
  if( GetThreadTimes(h, &CreationTime, &ExitTime, &KernelTime, &UserTime ) )
  {
    time = (((uint64_t)UserTime.dwHighDateTime) << 32) + ((uint64_t)UserTime.dwLowDateTime);
    time += (((uint64_t)KernelTime.dwHighDateTime) << 32) + ((uint64_t)KernelTime.dwLowDateTime);
  }
  return time;
}

