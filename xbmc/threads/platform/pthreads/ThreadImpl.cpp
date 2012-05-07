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

#include <limits.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <string.h>

void CThread::Create(bool bAutoDelete, unsigned stacksize)
{
  if (m_ThreadId != 0)
  {
    if (logger) logger->Log(LOGERROR, "%s - fatal error creating thread- old thread id not null", __FUNCTION__);
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

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (stacksize > PTHREAD_STACK_MIN)
    pthread_attr_setstacksize(&attr, stacksize);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&m_ThreadId, &attr, (void*(*)(void*))staticThread, this) != 0)
  {
    if (logger) logger->Log(LOGNOTICE, "%s - fatal error creating thread",__FUNCTION__);
  }
  pthread_attr_destroy(&attr);
}

void CThread::TermHandler()
{

}

void CThread::SetThreadInfo()
{
  m_ThreadOpaque.LwpId = syscall(SYS_gettid);

  // start thread with nice level of appication
  int appNice = getpriority(PRIO_PROCESS, getpid());
  if (setpriority(PRIO_PROCESS, m_ThreadOpaque.LwpId, appNice) != 0)
    if (logger) logger->Log(LOGERROR, "%s: error %s", __FUNCTION__, strerror(errno));
}

ThreadIdentifier CThread::GetCurrentThreadId()
{
  return pthread_self();
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
    }
    else
      userMaxPrio = 0;

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
      if (logger) logger->Log(LOGERROR, "%s: error %s", __FUNCTION__, strerror(errno));
  }
#endif

  return bReturn;
}

int CThread::GetPriority()
{
  int iReturn;

  // lwp id is valid after start signel has fired
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
  thread_info_data_t     threadInfo;
  mach_msg_type_number_t threadInfoCount = THREAD_INFO_MAX;

  if (m_machThreadPort == MACH_PORT_NULL)
    m_machThreadPort = pthread_mach_thread_np(m_ThreadId);

  kern_return_t ret = thread_info(m_machThreadPort, THREAD_BASIC_INFO, (thread_info_t)threadInfo, &threadInfoCount);

  if (ret == KERN_SUCCESS)
  {
    thread_basic_info_t threadBasicInfo = (thread_basic_info_t)threadInfo;

    // User time.
    time = ((int64_t)threadBasicInfo->user_time.seconds * 10000000L) + threadBasicInfo->user_time.microseconds*10L;

    // System time.
    time += (((int64_t)threadBasicInfo->system_time.seconds * 10000000L) + threadBasicInfo->system_time.microseconds*10L);
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


