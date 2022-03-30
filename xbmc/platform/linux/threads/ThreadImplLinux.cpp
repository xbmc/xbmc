/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ThreadImplLinux.h"

#include "utils/log.h"

#include <algorithm>
#include <array>
#include <limits.h>

#include <sys/resource.h>
#include <unistd.h>

#if !defined(TARGET_ANDROID) && (defined(__GLIBC__) || defined(__UCLIBC__))
#if defined(__UCLIBC__) || !__GLIBC_PREREQ(2, 30)
#include <sys/syscall.h>
#endif
#endif

namespace
{

constexpr std::array<ThreadPriorityStruct, 5> nativeThreadPriorityMap = {{
    {ThreadPriority::LOWEST, -1},
    {ThreadPriority::BELOW_NORMAL, -1},
    {ThreadPriority::NORMAL, 0},
    {ThreadPriority::ABOVE_NORMAL, 1},
    {ThreadPriority::HIGHEST, 1},
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

#if !defined(TARGET_ANDROID) && (defined(__GLIBC__) || defined(__UCLIBC__))
#if defined(__UCLIBC__) || !__GLIBC_PREREQ(2, 30)
static pid_t gettid()
{
  return syscall(__NR_gettid);
}
#endif
#endif

} // namespace

static int s_maxPriority;
static bool s_maxPriorityIsSet{false};

// We need to return what the best number than can be passed
// to SetPriority is. It will basically be relative to the
// the main thread's nice level, inverted (since "higher" priority
// nice levels are actually lower numbers).
static int GetUserMaxPriority(int maxPriority)
{
  if (s_maxPriorityIsSet)
    return s_maxPriority;

  // if we're root, then we can do anything. So we'll allow
  // max priority.
  if (geteuid() == 0)
    return maxPriority;

  // get user max prio
  struct rlimit limit;
  if (getrlimit(RLIMIT_NICE, &limit) != 0)
  {
    // If we fail getting the limit for nice we just assume we can't raise the priority
    return 0;
  }

  const int appNice = getpriority(PRIO_PROCESS, getpid());
  const int rlimVal = limit.rlim_cur;

  // according to the docs, limit.rlim_cur shouldn't be zero, yet, here we are.
  // if a user has no entry in limits.conf rlim_cur is zero. In this case the best
  //   nice value we can hope to achieve is '0' as a regular user
  const int userBestNiceValue = (rlimVal == 0) ? 0 : (20 - rlimVal);

  //          running the app with nice -n 10 ->
  // e.g.         +10                 10    -     0   // default non-root user.
  // e.g.         +30                 10    -     -20 // if root with rlimits set.
  //          running the app default ->
  // e.g.          0                  0    -     0   // default non-root user.
  // e.g.         +20                 0    -     -20 // if root with rlimits set.
  const int bestUserSetPriority = appNice - userBestNiceValue; // nice is inverted from prio.

  // static because we only need to check this once.
  // we shouldn't expect a user to change RLIMIT_NICE while running
  // and it won't work anyway for threads that already set their priority.
  s_maxPriority = std::min(maxPriority, bestUserSetPriority);
  s_maxPriorityIsSet = true;

  return s_maxPriority;
}

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

  // get user max prio
  const int maxPrio = ThreadPriorityToNativePriority(ThreadPriority::HIGHEST);
  const int userMaxPrio = GetUserMaxPriority(maxPrio);

  // if the user does not have an entry in limits.conf the following
  // call will fail
  if (userMaxPrio > 0)
  {
    // start thread with nice level of application
    const int appNice = getpriority(PRIO_PROCESS, getpid());
    if (setpriority(PRIO_PROCESS, m_threadID, appNice) != 0)
      CLog::Log(LOGERROR, "[threads] failed to set priority: {}", strerror(errno));
  }
}

bool CThreadImplLinux::SetPriority(const ThreadPriority& priority)
{
  // keep priority in bounds
  const int prio = ThreadPriorityToNativePriority(priority);
  const int maxPrio = ThreadPriorityToNativePriority(ThreadPriority::HIGHEST);
  const int minPrio = ThreadPriorityToNativePriority(ThreadPriority::LOWEST);

  // get user max prio given max prio (will take the min)
  const int userMaxPrio = GetUserMaxPriority(maxPrio);

  // clamp to min and max priorities
  const int adjustedPrio = std::clamp(prio, minPrio, userMaxPrio);

  // nice level of application
  const int appNice = getpriority(PRIO_PROCESS, getpid());
  const int newNice = appNice - adjustedPrio;

  if (setpriority(PRIO_PROCESS, m_threadID, newNice) != 0)
  {
    CLog::Log(LOGERROR, "[threads] failed to set priority: {}", strerror(errno));
    return false;
  }

  return true;
}
