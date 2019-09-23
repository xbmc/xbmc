/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <process.h>
#include <windows.h>

void CThread::SetThreadInfo()
{
  const unsigned int MS_VC_EXCEPTION = 0x406d1388;

  m_lwpId = m_thread->native_handle();

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
  info.dwThreadID = reinterpret_cast<std::uintptr_t>(m_lwpId);
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

std::uintptr_t CThread::GetCurrentThreadNativeHandle()
{
  return reinterpret_cast<std::uintptr_t>(::GetCurrentThread());
}

uint64_t CThread::GetCurrentThreadNativeId()
{
  return static_cast<uint64_t>(::GetCurrentThreadId());
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

bool CThread::SetPriority(const int iPriority)
{
  bool bReturn = false;

  CSingleLock lock(m_CriticalSection);
  if (m_thread)
    bReturn = SetThreadPriority(m_lwpId, iPriority) == TRUE;

  return bReturn;
}

int CThread::GetPriority()
{
  CSingleLock lock(m_CriticalSection);

  int iReturn = THREAD_PRIORITY_NORMAL;
  if (m_thread)
    iReturn = GetThreadPriority(m_lwpId);
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

  HANDLE tid = static_cast<HANDLE>(m_lwpId);

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
