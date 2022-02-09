/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <array>

#include <process.h>
#include <windows.h>

namespace
{

constexpr std::array<ThreadPriorityStruct, 5> nativeThreadPriorityMap = {{
    {ThreadPriority::LOWEST, THREAD_PRIORITY_IDLE},
    {ThreadPriority::BELOW_NORMAL, THREAD_PRIORITY_BELOW_NORMAL},
    {ThreadPriority::NORMAL, THREAD_PRIORITY_NORMAL},
    {ThreadPriority::ABOVE_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL},
    {ThreadPriority::HIGHEST, THREAD_PRIORITY_HIGHEST},
}};

//! @todo: c++20 has constexpr std::find_if
int ThreadPriorityToNativePriority(const ThreadPriority& priority)
{
  auto it = std::find_if(nativeThreadPriorityMap.cbegin(), nativeThreadPriorityMap.cend(),
                         [&priority](const auto& map) { return map.priority == priority; });

  if (it != nativeThreadPriorityMap.cend())
  {
    return it->nativePriority;
  }

  throw std::runtime_error("priority not implemented");
}

} // namespace

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

bool CThread::SetPriority(const ThreadPriority& priority)
{
  bool bReturn = false;

  CSingleLock lock(m_CriticalSection);
  if (m_thread)
    bReturn = SetThreadPriority(m_lwpId, ThreadPriorityToNativePriority(priority)) == TRUE;

  return bReturn;
}
