/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * Most of this code is taken from thread.c in the Video Disk Recorder ('VDR')
 */

#include "thread.h"
#include <errno.h>
#ifndef __APPLE__
#include <malloc.h>
#endif

#if !defined(__WINDOWS__)
#include <sys/signal.h>
#endif

#include <stdarg.h>
#include <stdlib.h>

using namespace ADDON;

static bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow)
{
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0) {           // get current time
     now.tv_sec  += MillisecondsFromNow / 1000;  // add full seconds
     now.tv_usec += (MillisecondsFromNow % 1000) * 1000;  // add microseconds
     if (now.tv_usec >= 1000000) {               // take care of an overflow
        now.tv_sec++;
        now.tv_usec -= 1000000;
        }
     Abstime->tv_sec = now.tv_sec;          // seconds
     Abstime->tv_nsec = now.tv_usec * 1000; // nano seconds
     return true;
     }
  return false;
}

// --- cCondWait -------------------------------------------------------------

cCondWait::cCondWait(void)
{
  signaled = false;
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
}

cCondWait::~cCondWait()
{
  pthread_cond_broadcast(&cond); // wake up any sleepers
  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&mutex);
}

void cCondWait::SleepMs(int TimeoutMs)
{
  cCondWait w;
  w.Wait(max(TimeoutMs, 3)); // making sure the time is >2ms to avoid a possible busy wait
}

bool cCondWait::Wait(int TimeoutMs)
{
  pthread_mutex_lock(&mutex);
  if (!signaled)
  {
    if (TimeoutMs)
	{
      struct timespec abstime;
      if (GetAbsTime(&abstime, TimeoutMs))
	  {
        while (!signaled)
        {
		  int iResult = pthread_cond_timedwait(&cond, &mutex, &abstime);
		  if (iResult != 0)
            break;
        }
      }
     }
     else
        pthread_cond_wait(&cond, &mutex);
     }
  bool r = signaled;
  signaled = false;
  pthread_mutex_unlock(&mutex);
  return r;
}

void cCondWait::Signal(void)
{
  pthread_mutex_lock(&mutex);
  signaled = true;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

// --- cCondVar --------------------------------------------------------------

cCondVar::cCondVar(void)
{
  pthread_cond_init(&cond, 0);
}

cCondVar::~cCondVar()
{
  pthread_cond_broadcast(&cond); // wake up any sleepers
  pthread_cond_destroy(&cond);
}

void cCondVar::Wait(cMutex &Mutex)
{
  if (Mutex.locked) {
     int locked = Mutex.locked;
     Mutex.locked = 0; // have to clear the locked count here, as pthread_cond_wait
                       // does an implicit unlock of the mutex
     pthread_cond_wait(&cond, &Mutex.mutex);
     Mutex.locked = locked;
     }
}

bool cCondVar::TimedWait(cMutex &Mutex, int TimeoutMs)
{
  bool r = true; // true = condition signaled, false = timeout

  if (Mutex.locked) {
     struct timespec abstime;
     if (GetAbsTime(&abstime, TimeoutMs)) {
        int locked = Mutex.locked;
        Mutex.locked = 0; // have to clear the locked count here, as pthread_cond_timedwait
                          // does an implicit unlock of the mutex.
        if (pthread_cond_timedwait(&cond, &Mutex.mutex, &abstime) == ETIMEDOUT)
           r = false;
        Mutex.locked = locked;
        }
     }
  return r;
}

void cCondVar::Broadcast(void)
{
  pthread_cond_broadcast(&cond);
}

// --- cMutex ----------------------------------------------------------------

cMutex::cMutex(void)
{
  locked = 0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
#ifndef __APPLE__
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#else
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
  pthread_mutex_init(&mutex, &attr);
}

cMutex::~cMutex()
{
  pthread_mutex_destroy(&mutex);
}

void cMutex::Lock(void)
{
  pthread_mutex_lock(&mutex);
  locked++;
}

void cMutex::Unlock(void)
{
 if (!--locked)
    pthread_mutex_unlock(&mutex);
}

// --- cThread ---------------------------------------------------------------

tThreadId cThread::mainThreadId = 0;

cThread::cThread(const char *Description)
{
  active = running = false;
#if !defined(__WINDOWS__)
  childTid = 0;
#endif
  childThreadId = 0;
  description = NULL;
  if (Description)
     SetDescription("%s", Description);
}

cThread::~cThread()
{
  Cancel(); // just in case the derived class didn't call it
  free(description);
}

void cThread::SetPriority(int Priority)
{
#if !defined(__WINDOWS__)
  if (setpriority(PRIO_PROCESS, 0, Priority) < 0)
     XBMC->Log(LOG_ERROR, "ERROR (%s,%d): %m", __FILE__, __LINE__);
#endif
}

void cThread::SetIOPriority(int Priority)
{
#if !defined(__WINDOWS__)
#ifdef HAVE_LINUXIOPRIO
  if (syscall(SYS_ioprio_set, 1, 0, (Priority & 0xff) | (2 << 13)) < 0) // best effort class
     XBMC->Log(LOG_ERROR, "ERROR (%s,%d): %m", __FILE__, __LINE__);
#endif
#endif
}

void cThread::SetDescription(const char *Description, ...)
{
  free(description);
  description = NULL;
  if (Description)
  {
     va_list ap;
     va_start(ap, Description);
     CStdString desc;
     desc.FormatV(Description, ap);
     description = strdup(desc.c_str());
     va_end(ap);
  }
}

void *cThread::StartThread(cThread *Thread)
{
  Thread->childThreadId = ThreadId();
  if (Thread->description) {
     XBMC->Log(LOG_DEBUG, "%s thread started (pid=%d, tid=%d)", Thread->description, getpid(), Thread->childThreadId);
#ifdef PR_SET_NAME
     if (prctl(PR_SET_NAME, Thread->description, 0, 0, 0) < 0)
        XBMC->Log(LOG_ERROR, "%s thread naming failed (pid=%d, tid=%d)", Thread->description, getpid(), Thread->childThreadId);
#endif
     }
  Thread->Action();
  if (Thread->description)
     XBMC->Log(LOG_DEBUG, "%s thread ended (pid=%d, tid=%d)", Thread->description, getpid(), Thread->childThreadId);
  Thread->running = false;
  Thread->active = false;
  return NULL;
}

#define THREAD_STOP_TIMEOUT  3000 // ms to wait for a thread to stop before newly starting it
#define THREAD_STOP_SLEEP      30 // ms to sleep while waiting for a thread to stop

bool cThread::Start(void)
{
  if (!running) {
     if (active) {
        // Wait until the previous incarnation of this thread has completely ended
        // before starting it newly:
        cTimeMs RestartTimeout;
        while (!running && active && RestartTimeout.Elapsed() < THREAD_STOP_TIMEOUT)
              cCondWait::SleepMs(THREAD_STOP_SLEEP);
        }
     if (!active) {
        active = running = true;
        if (pthread_create(&childTid, NULL, (void *(*) (void *))&StartThread, (void *)this) == 0) {
           pthread_detach(childTid); // auto-reap
           }
        else {
           XBMC->Log(LOG_ERROR, "ERROR (%s,%d): %m", __FILE__, __LINE__);
           active = running = false;
           return false;
           }
        }
     }
  return true;
}

bool cThread::Active(void)
{
  if (active) {
     //
     // Single UNIX Spec v2 says:
     //
     // The pthread_kill() function is used to request
     // that a signal be delivered to the specified thread.
     //
     // As in kill(), if sig is zero, error checking is
     // performed but no signal is actually sent.
     //
     int err;
     if ((err = pthread_kill(childTid, 0)) != 0) {
        if (err != ESRCH)
           XBMC->Log(LOG_ERROR, "ERROR (%s,%d): %m", __FILE__, __LINE__);
#if !defined(__WINDOWS__)
        childTid = 0;
#endif
        active = running = false;
        }
     else
        return true;
     }
  return false;
}

void cThread::Cancel(int WaitSeconds)
{
  running = false;
  if (active && WaitSeconds > -1)
  {
    if (WaitSeconds > 0)
    {
      for (time_t t0 = time(NULL) + WaitSeconds; time(NULL) < t0; )
      {
        if (!Active())
          return;
        cCondWait::SleepMs(10);
      }
      XBMC->Log(LOG_ERROR, "ERROR: %s thread %d won't end (waited %d seconds) - canceling it...", description ? description : "", childThreadId, WaitSeconds);
    }
    pthread_cancel(childTid);
#if !defined(__WINDOWS__)
    childTid = 0;
#endif
    active = false;
  }
}

tThreadId cThread::ThreadId(void)
{
#ifdef __APPLE__
    return (int)pthread_self();
#else
#ifdef __WINDOWS__
  return GetCurrentThreadId();
#else
  return syscall(__NR_gettid);
#endif
#endif
}

void cThread::SetMainThreadId(void)
{
  if (mainThreadId == 0)
     mainThreadId = ThreadId();
  else
     XBMC->Log(LOG_ERROR, "ERROR: attempt to set main thread id to %d while it already is %d", ThreadId(), mainThreadId);
}

// --- cMutexLock ------------------------------------------------------------

cMutexLock::cMutexLock(cMutex *Mutex)
{
  mutex = NULL;
  locked = false;
  Lock(Mutex);
}

cMutexLock::~cMutexLock()
{
  if (mutex && locked)
    mutex->Unlock();
}

bool cMutexLock::Lock(cMutex *Mutex)
{
  if (Mutex && !mutex)
  {
    mutex = Mutex;
    Mutex->Lock();
    locked = true;
    return true;
  }
  return false;
}

// --- cThreadLock -----------------------------------------------------------

cThreadLock::cThreadLock(cThread *Thread)
{
  thread = NULL;
  locked = false;
  Lock(Thread);
}

cThreadLock::~cThreadLock()
{
  if (thread && locked)
    thread->Unlock();
}

bool cThreadLock::Lock(cThread *Thread)
{
  if (Thread && !thread)
  {
  thread = Thread;
  Thread->Lock();
  locked = true;
  return true;
  }
  return false;
}
