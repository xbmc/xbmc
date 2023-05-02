/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ThreadImplLinux.h"

#include "utils/Map.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <mutex>

#include <sys/resource.h>
#include <unistd.h>

#if !defined(TARGET_ANDROID) && (defined(__GLIBC__) || defined(__UCLIBC__))
#if defined(__UCLIBC__) || !__GLIBC_PREREQ(2, 30)
#include <sys/syscall.h>
#endif
#endif

namespace
{

constexpr auto nativeThreadPriorityMap = make_map<ThreadPriority, int>({
    {ThreadPriority::LOWEST, -1},
    {ThreadPriority::BELOW_NORMAL, -1},
    {ThreadPriority::NORMAL, 0},
    {ThreadPriority::ABOVE_NORMAL, 1},
    {ThreadPriority::HIGHEST, 1},
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

#if !defined(TARGET_ANDROID) && (defined(__GLIBC__) || defined(__UCLIBC__))
#if defined(__UCLIBC__) || !__GLIBC_PREREQ(2, 30)
static pid_t gettid()
{
  return syscall(__NR_gettid);
}
#endif
#endif

std::once_flag flag;

} // namespace

static int s_appPriority = getpriority(PRIO_PROCESS, getpid());

std::unique_ptr<IThreadImpl> IThreadImpl::CreateThreadImpl(std::thread::native_handle_type handle)
{
  return std::make_unique<CThreadImplLinux>(handle);
}

CThreadImplLinux::CThreadImplLinux(std::thread::native_handle_type handle)
  : IThreadImpl(handle), m_threadID(gettid())
{
}

void CThreadImplLinux::SetThreadInfo(const std::string& name)
{
#if defined(__GLIBC__)
  pthread_setname_np(m_handle, name.c_str());
#endif

  m_name = name;
}

bool CThreadImplLinux::SetPriority(const ThreadPriority& priority)
{
  std::call_once(flag,
                 []() { CLog::Log(LOGDEBUG, "[threads] app priority: '{}'", s_appPriority); });

  const int prio = ThreadPriorityToNativePriority(priority);

  const int newPriority = s_appPriority - prio;

  setpriority(PRIO_PROCESS, m_threadID, newPriority);

  const int actualPriority = getpriority(PRIO_PROCESS, m_threadID);

  CLog::Log(LOGDEBUG, "[threads] name: '{}' priority: '{}'", m_name, actualPriority);

  return true;
}
