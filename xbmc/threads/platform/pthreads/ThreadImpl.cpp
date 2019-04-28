/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <limits.h>
#if defined(TARGET_ANDROID)
#include <unistd.h>
#else
#include <sys/syscall.h>
#endif
#include <sys/resource.h>
#include <string.h>
#ifdef TARGET_FREEBSD
#include <sys/param.h>
#include <pthread_np.h>
#endif

#include <signal.h>
#include "utils/log.h"

namespace XbmcThreads
{
  // ==========================================================
  static pthread_mutexattr_t recursiveAttr;

  static bool setRecursiveAttr()
  {
    static bool alreadyCalled = false; // initialized to 0 in the data segment prior to startup init code running
    if (!alreadyCalled)
    {
      pthread_mutexattr_init(&recursiveAttr);
      pthread_mutexattr_settype(&recursiveAttr,PTHREAD_MUTEX_RECURSIVE);
#if !defined(TARGET_ANDROID)
      pthread_mutexattr_setprotocol(&recursiveAttr,PTHREAD_PRIO_INHERIT);
#endif
      alreadyCalled = true;
    }
    return true; // note, we never call destroy.
  }

  static bool recursiveAttrSet = setRecursiveAttr();

  pthread_mutexattr_t* CRecursiveMutex::getRecursiveAttr()
  {
    if (!recursiveAttrSet) // this is only possible in the single threaded startup code
      recursiveAttrSet = setRecursiveAttr();
    return &recursiveAttr;
  }
  // ==========================================================
}
void CThread::SpawnThread(unsigned stacksize)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
#if !defined(TARGET_ANDROID) // http://code.google.com/p/android/issues/detail?id=7808
  if (stacksize > PTHREAD_STACK_MIN)
    pthread_attr_setstacksize(&attr, stacksize);
#endif
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&m_ThreadId, &attr, (void*(*)(void*))staticThread, this) != 0)
  {
    CLog::Log(LOGNOTICE, "%s - fatal error creating thread",__FUNCTION__);
  }
  pthread_attr_destroy(&attr);
}

void CThread::TermHandler() { }

void CThread::SetThreadInfo()
{
#ifdef TARGET_FREEBSD
  m_ThreadOpaque.LwpId = pthread_getthreadid_np();
#elif defined(TARGET_ANDROID)
  m_ThreadOpaque.LwpId = gettid();
#elif defined(TARGET_DARWIN_TVOS)
  m_ThreadOpaque.LwpId = pthread_mach_thread_np(pthread_self());
#else
  m_ThreadOpaque.LwpId = syscall(SYS_gettid);
#endif

#if defined(TARGET_DARWIN)
  pthread_setname_np(m_ThreadName.c_str());
#elif defined(TARGET_LINUX) && defined(__GLIBC__)
  pthread_setname_np(m_ThreadId, m_ThreadName.c_str());
#endif

#ifdef RLIMIT_NICE
  // get user max prio
  struct rlimit limit;
  int userMaxPrio;
  if (getrlimit(RLIMIT_NICE, &limit) == 0)
  {
    userMaxPrio = limit.rlim_cur - 20;
    if (userMaxPrio < 0)
      userMaxPrio = 0;
  }
  else
    userMaxPrio = 0;

  if (geteuid() == 0)
    userMaxPrio = GetMaxPriority();

  // if the user does not have an entry in limits.conf the following
  // call will fail
  if (userMaxPrio > 0)
  {
    // start thread with nice level of application
    int appNice = getpriority(PRIO_PROCESS, getpid());
    if (setpriority(PRIO_PROCESS, m_ThreadOpaque.LwpId, appNice) != 0)
      CLog::Log(LOGERROR, "%s: error %s", __FUNCTION__, strerror(errno));
  }
#endif
}

ThreadIdentifier CThread::GetCurrentThreadId()
{
  return pthread_self();
}

ThreadIdentifier CThread::GetDisplayThreadId(const ThreadIdentifier tid)
{
#if defined(TARGET_ANDROID)
  return pthread_gettid_np(tid);
#else
  return tid;
#endif
}

bool CThread::IsCurrentThread(const ThreadIdentifier tid)
{
  return pthread_equal(pthread_self(), tid);
}

int CThread::GetMinPriority(void)
{
  // one level lower than application
  return -1;
}

int CThread::GetMaxPriority(void)
{
  // one level higher than application
  return 1;
}

int CThread::GetNormalPriority(void)
{
  // same level as application
  return 0;
}

bool CThread::SetPriority(const int iPriority)
{
  bool bReturn = false;

  // wait until thread is running, it needs to get its lwp id
  m_StartEvent.Wait();

  CSingleLock lock(m_CriticalSection);

  // get min prio for SCHED_RR
  int minRR = GetMaxPriority() + 1;

  if (!m_ThreadId)
    bReturn = false;
  else if (iPriority >= minRR)
    bReturn = SetPrioritySched_RR(iPriority);
#ifdef RLIMIT_NICE
  else
  {
    // get user max prio
    struct rlimit limit;
    int userMaxPrio;
    if (getrlimit(RLIMIT_NICE, &limit) == 0)
    {
      userMaxPrio = limit.rlim_cur - 20;
      // is a user has no entry in limits.conf rlim_cur is zero
      if (userMaxPrio < 0)
        userMaxPrio = 0;
    }
    else
      userMaxPrio = 0;

    if (geteuid() == 0)
      userMaxPrio = GetMaxPriority();

    // keep priority in bounds
    int prio = iPriority;
    if (prio >= GetMaxPriority())
      prio = std::min(GetMaxPriority(), userMaxPrio);
    if (prio < GetMinPriority())
      prio = GetMinPriority();

    // nice level of application
    int appNice = getpriority(PRIO_PROCESS, getpid());
    if (prio)
      prio = prio > 0 ? appNice-1 : appNice+1;

    if (setpriority(PRIO_PROCESS, m_ThreadOpaque.LwpId, prio) == 0)
      bReturn = true;
    else
      CLog::Log(LOGERROR, "%s: error %s", __FUNCTION__, strerror(errno));
  }
#endif

  return bReturn;
}

int CThread::GetPriority()
{
  int iReturn;

  // lwp id is valid after start signal has fired
  m_StartEvent.Wait();

  CSingleLock lock(m_CriticalSection);

  int appNice = getpriority(PRIO_PROCESS, getpid());
  int prio = getpriority(PRIO_PROCESS, m_ThreadOpaque.LwpId);
  iReturn = appNice - prio;

  return iReturn;
}

bool CThread::WaitForThreadExit(unsigned int milliseconds)
{
  bool bReturn = m_TermEvent.WaitMSec(milliseconds);

  return bReturn;
}

int64_t CThread::GetAbsoluteUsage()
{
  CSingleLock lock(m_CriticalSection);

  if (!m_ThreadId)
  return 0;

  int64_t time = 0;
#ifdef TARGET_DARWIN
  thread_basic_info threadInfo;
  mach_msg_type_number_t threadInfoCount = THREAD_BASIC_INFO_COUNT;

  kern_return_t ret = thread_info(pthread_mach_thread_np(m_ThreadId),
    THREAD_BASIC_INFO, (thread_info_t)&threadInfo, &threadInfoCount);

  if (ret == KERN_SUCCESS)
  {
    // User time.
    time = ((int64_t)threadInfo.user_time.seconds * 10000000L) + threadInfo.user_time.microseconds*10L;

    // System time.
    time += (((int64_t)threadInfo.system_time.seconds * 10000000L) + threadInfo.system_time.microseconds*10L);
  }

#else
  clockid_t clock;
  if (pthread_getcpuclockid(m_ThreadId, &clock) == 0)
  {
    struct timespec tp;
    clock_gettime(clock, &tp);
    time = (int64_t)tp.tv_sec * 10000000 + tp.tv_nsec/100;
  }
#endif

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

void term_handler (int signum)
{
  CLog::Log(LOGERROR,"thread 0x%lx (%lu) got signal %d. calling OnException and terminating thread abnormally.", (long unsigned int)pthread_self(), (long unsigned int)pthread_self(), signum);
  CThread* curThread = CThread::GetCurrentThread();
  if (curThread)
  {
    curThread->StopThread(false);
    curThread->OnException();
    if( curThread->IsAutoDelete() )
      delete curThread;
  }
  pthread_exit(NULL);
}

void CThread::SetSignalHandlers()
{
  struct sigaction action;
  action.sa_handler = term_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  //sigaction (SIGABRT, &action, NULL);
  //sigaction (SIGSEGV, &action, NULL);
}

