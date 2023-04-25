/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ThreadImplWin.h"

#include "utils/Map.h"
#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <array>
#include <mutex>

#include <process.h>
#include <windows.h>

namespace
{

constexpr auto nativeThreadPriorityMap = make_map<ThreadPriority, int>({
    {ThreadPriority::LOWEST, THREAD_PRIORITY_IDLE},
    {ThreadPriority::BELOW_NORMAL, THREAD_PRIORITY_BELOW_NORMAL},
    {ThreadPriority::NORMAL, THREAD_PRIORITY_NORMAL},
    {ThreadPriority::ABOVE_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL},
    {ThreadPriority::HIGHEST, THREAD_PRIORITY_HIGHEST},
});

static_assert(static_cast<size_t>(ThreadPriority::PRIORITY_COUNT) == nativeThreadPriorityMap.size(),
              "nativeThreadPriorityMap doesn't match the size of ThreadPriority, did you forget to "
              "add/remove a mapping?");

constexpr int ThreadPriorityToNativePriority(const ThreadPriority& priority)
{
  const auto it = nativeThreadPriorityMap.find(priority);
  if (it != nativeThreadPriorityMap.cend())
  {
    return it->second;
  }
  else
  {
    throw std::range_error("Priority not found");
  }
}

} // namespace

std::unique_ptr<IThreadImpl> IThreadImpl::CreateThreadImpl(std::thread::native_handle_type handle)
{
  return std::make_unique<CThreadImplWin>(handle);
}

CThreadImplWin::CThreadImplWin(std::thread::native_handle_type handle) : IThreadImpl(handle)
{
}

void CThreadImplWin::SetThreadInfo(const std::string& name)
{
  // Modern way to name threads first (Windows 10 1607 and above)
  if (!CWIN32Util::SetThreadName(static_cast<HANDLE>(m_handle), name))
  {
    // Fallback on legacy method - specially configured exception
    const unsigned int MS_VC_EXCEPTION = 0x406d1388;

#pragma pack(push, 8)
    struct THREADNAME_INFO
    {
      DWORD dwType; // must be 0x1000
      LPCSTR szName; // pointer to name (in same addr space)
      DWORD dwThreadID; // thread ID (-1 = caller thread)
      DWORD dwFlags; // reserved for future use, must be zero
    } info;
#pragma pack(pop)

    info.dwType = 0x1000;
    info.szName = name.c_str();
    info.dwThreadID = GetThreadId(static_cast<HANDLE>(m_handle));
    info.dwFlags = 0;

    __try
    {
      RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
  }

  CWIN32Util::SetThreadLocalLocale(true); // avoid crashing with setlocale(), see https://connect.microsoft.com/VisualStudio/feedback/details/794122
}

bool CThreadImplWin::SetPriority(const ThreadPriority& priority)
{
  bool bReturn = false;

  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  if (m_handle)
    bReturn = SetThreadPriority(m_handle, ThreadPriorityToNativePriority(priority)) == TRUE;

  return bReturn;
}
