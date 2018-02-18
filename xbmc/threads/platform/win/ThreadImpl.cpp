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

#include <windows.h>
#include <process.h>
#include "platform/win32/WIN32Util.h"
#include "utils/log.h"

static HANDLE getCurrentThreadHANDLE()
{
  HANDLE tid = 0;
  CThread* cur = CThread::GetCurrentThread();
  if (cur != nullptr && cur->m_thread != nullptr)
    tid = cur->m_thread->native_handle();
  return tid;
}

void CThread::SetThreadInfo()
{
  const unsigned int MS_VC_EXCEPTION = 0x406d1388;

#pragma pack(push,8)
  struct THREADNAME_INFO
  {
    DWORD dwType; // must be 0x1000
    LPCSTR szName; // pointer to name (in same addr space)
    DWORD dwThreadID; // thread ID (-1 caller thread)
    DWORD dwFlags; // reserved for future use, most be zero
  } info;
#pragma pack(pop)

  info.dwType = 0x1000;
  info.szName = m_ThreadName.c_str();
  info.dwThreadID = m_ThreadId;
  info.dwFlags = 0;

  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
  }

  CWIN32Util::SetThreadLocalLocale(true); // avoid crashing with setlocale(), see https://connect.microsoft.com/VisualStudio/feedback/details/794122
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

  HANDLE tid = getCurrentThreadHANDLE();
  if (tid)
    bReturn = SetThreadPriority(tid, iPriority) == TRUE;

  return bReturn;
}

int CThread::GetPriority()
{
  int iReturn = THREAD_PRIORITY_NORMAL;

  HANDLE tid = getCurrentThreadHANDLE();

  if (tid)
    iReturn = GetThreadPriority(tid);

  return iReturn;
}

int64_t CThread::GetAbsoluteUsage()
{
#ifdef TARGET_WINDOWS_STORE
  // GetThreadTimes is available since 10.0.15063 only
  return 0;
#else
  CSingleLock lock(m_CriticalSection);

  if (m_thread == nullptr)
    return 0;

  HANDLE tid = static_cast<HANDLE>(m_thread->native_handle());

  if (!tid)
    return 0;

  uint64_t time = 0;
  FILETIME CreationTime, ExitTime, UserTime, KernelTime;
  if( GetThreadTimes(tid, &CreationTime, &ExitTime, &KernelTime, &UserTime ) )
  {
    time = (((uint64_t)UserTime.dwHighDateTime) << 32) + ((uint64_t)UserTime.dwLowDateTime);
    time += (((uint64_t)KernelTime.dwHighDateTime) << 32) + ((uint64_t)KernelTime.dwLowDateTime);
  }
  return time;
#endif
}

void CThread::SetSignalHandlers()
{
}
