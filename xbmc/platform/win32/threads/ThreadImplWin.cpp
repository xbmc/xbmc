/*
 *  Copyright (C) 2005-2026 Team Kodi
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
#include <string_view>

#include <process.h>
#include <processthreadsapi.h>
#include <windows.h>

namespace
{

struct NativeThreadPriority
{
  int priority;
  std::string_view name;
};

constexpr auto nativeThreadPriorityMap = make_map<ThreadPriority, NativeThreadPriority>({
    {ThreadPriority::LOWEST, {THREAD_PRIORITY_IDLE, "idle"}},
    {ThreadPriority::BELOW_NORMAL, {THREAD_PRIORITY_BELOW_NORMAL, "below normal"}},
    {ThreadPriority::NORMAL, {THREAD_PRIORITY_NORMAL, "normal"}},
    {ThreadPriority::ABOVE_NORMAL, {THREAD_PRIORITY_ABOVE_NORMAL, "above normal"}},
    {ThreadPriority::HIGHEST, {THREAD_PRIORITY_HIGHEST, "highest"}},
});

static_assert(static_cast<size_t>(ThreadPriority::PRIORITY_COUNT) == nativeThreadPriorityMap.size(),
              "nativeThreadPriorityMap doesn't match the size of ThreadPriority, did you forget to "
              "add/remove a mapping?");

constexpr auto priorityClasses = make_map<DWORD, std::string_view>({
    {IDLE_PRIORITY_CLASS, "idle"},
    {BELOW_NORMAL_PRIORITY_CLASS, "below normal"},
    {NORMAL_PRIORITY_CLASS, "normal"},
    {ABOVE_NORMAL_PRIORITY_CLASS, "above normal"},
    {HIGH_PRIORITY_CLASS, "high"},
    {REALTIME_PRIORITY_CLASS, "realtime"},
});

constexpr NativeThreadPriority ThreadPriorityToNativePriority(ThreadPriority priority)
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

void LogProcessPriorityClass()
{
  const DWORD prioClass = GetPriorityClass(GetCurrentProcess());

  if (prioClass == 0)
  {
    CLog::LogF(LOGERROR, "unable to retrieve the process priority class ({})", GetLastError());
    return;
  }

  if (const auto it = priorityClasses.find(prioClass); it != priorityClasses.end())
    CLog::Log(LOGDEBUG, "[threads] app priority: {}", it->second);
  else
    CLog::Log(LOGDEBUG, "[threads] app priority: unrecognized (0x{:08X})", prioClass);
}

std::once_flag flag;

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

  // avoid crashing with setlocale(), see https://connect.microsoft.com/VisualStudio/feedback/details/794122
  CWIN32Util::SetThreadLocalLocale(true);

  m_name = name;
}

bool CThreadImplWin::SetPriority(const ThreadPriority& priority)
{
  std::call_once(flag, LogProcessPriorityClass);

  bool bReturn = false;

  std::unique_lock lock(m_criticalSection);
  if (m_handle)
  {
    const NativeThreadPriority native = ThreadPriorityToNativePriority(priority);

    if (SetThreadPriority(m_handle, native.priority) == 0)
    {
      CLog::LogF(LOGERROR, "unable to set the priority of thread {} ({})", m_name, GetLastError());
      bReturn = false;
    }
    else
    {
      CLog::Log(LOGDEBUG, "[threads] name: '{}' priority: {}", m_name, native.name);
      bReturn = true;
    }
  }
  return bReturn;
}
