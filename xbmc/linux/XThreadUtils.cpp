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

#include "PlatformDefs.h"
#include "XHandle.h"
#include "XThreadUtils.h"
#include "XTimeUtils.h"
#include "XEventUtils.h"
#include "system.h"
#include "log.h"
#include "GraphicContext.h"

#ifdef _LINUX
#include <signal.h>
#include <pthread.h>
#include <limits.h>

// a variable which is defined __thread will be defined in thread local storage
// which means it will be different for each thread that accesses it.
#define TLS_INDEXES 16
#define TLS_OUT_OF_INDEXES (DWORD)0xFFFFFFFF

#ifdef __APPLE__
// FIXME, this needs to be converted to use pthread_once.
static LPVOID tls[TLS_INDEXES] = { NULL };
#else
static LPVOID __thread tls[TLS_INDEXES] = { NULL };
#endif

static BOOL tls_used[TLS_INDEXES];

HANDLE WINAPI CreateThread(
      LPSECURITY_ATTRIBUTES lpThreadAttributes,
        SIZE_T dwStackSize,
          LPTHREAD_START_ROUTINE lpStartAddress,
            LPVOID lpParameter,
        DWORD dwCreationFlags,
          LPDWORD lpThreadId
    ) {

  // a thread handle would actually contain an event
  // the event would mark if the thread is running or not. it will be used in the Wait functions.
  HANDLE h = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->ChangeType(CXHandle::HND_THREAD);
  h->m_nRefCount++;
#ifdef __APPLE__
  h->m_machThreadPort = MACH_PORT_NULL;
#endif
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (dwStackSize > PTHREAD_STACK_MIN)
    pthread_attr_setstacksize(&attr, dwStackSize);
  if (pthread_create(&(h->m_hThread), &attr, (void*(*)(void*))lpStartAddress, lpParameter) == 0)
    h->m_threadValid = true;
  else
  {
    CloseHandle(h);
    h = NULL;
  }
  pthread_attr_destroy(&attr);

  if (h && lpThreadId)
    // WARNING: This can truncate thread IDs on x86_64.
    *lpThreadId = (DWORD)h->m_hThread;
  return h;
}


#if 0 // Deprecated, use CThread::GetCurrentThreadId() instead
DWORD WINAPI GetCurrentThreadId(void) {
  // WARNING:  This can truncate thread IDs on x86_64.
  return (DWORD)pthread_self();
}
#endif

HANDLE WINAPI GetCurrentThread(void) {
  return (HANDLE)-1; // -1 a special value - pseudo handle
}

HANDLE WINAPI GetCurrentProcess(void) {
  return (HANDLE)-1; // -1 a special value - pseudo handle
}

HANDLE _beginthreadex(
   void *security,
   unsigned stack_size,
   int ( *start_address )( void * ),
   void *arglist,
   unsigned initflag,
   unsigned *thrdaddr
) {

  HANDLE h = CreateThread(NULL, stack_size, start_address, arglist, initflag, (LPDWORD)thrdaddr);
  return h;

}

uintptr_t _beginthread(
    void( *start_address )( void * ),
    unsigned stack_size,
    void *arglist
) {
  HANDLE h = CreateThread(NULL, stack_size, (LPTHREAD_START_ROUTINE)start_address, arglist, 0, NULL);
  return (uintptr_t)h;
}

BOOL WINAPI GetThreadTimes (
  HANDLE hThread,
  LPFILETIME lpCreationTime,
  LPFILETIME lpExitTime,
  LPFILETIME lpKernelTime,
  LPFILETIME lpUserTime
) {
  if (!hThread)
    return false;
  if (!hThread->m_threadValid)
    return false;

  if (hThread == (HANDLE)-1) {
    if (lpCreationTime)
      TimeTToFileTime(0,lpCreationTime);
    if (lpExitTime)
      TimeTToFileTime(time(NULL),lpExitTime);
    if (lpKernelTime)
      TimeTToFileTime(0,lpKernelTime);
    if (lpUserTime)
      TimeTToFileTime(0,lpUserTime);

    return true;
  }

  if (lpCreationTime)
    TimeTToFileTime(hThread->m_tmCreation,lpCreationTime);
  if (lpExitTime)
    TimeTToFileTime(time(NULL),lpExitTime);
  if (lpKernelTime)
    TimeTToFileTime(0,lpKernelTime);

#ifdef __APPLE__
  thread_info_data_t     threadInfo;
  mach_msg_type_number_t threadInfoCount = THREAD_INFO_MAX;

  if (hThread->m_machThreadPort == MACH_PORT_NULL)
    hThread->m_machThreadPort = pthread_mach_thread_np(hThread->m_hThread);

  kern_return_t ret = thread_info(hThread->m_machThreadPort, THREAD_BASIC_INFO, (thread_info_t)threadInfo, &threadInfoCount);
  if (ret == KERN_SUCCESS)
  {
    thread_basic_info_t threadBasicInfo = (thread_basic_info_t)threadInfo;

    if (lpUserTime)
    {
      // User time.
      unsigned long long time = ((__int64)threadBasicInfo->user_time.seconds * 10000000L) + threadBasicInfo->user_time.microseconds*10L;
      lpUserTime->dwLowDateTime = (time & 0xFFFFFFFF);
      lpUserTime->dwHighDateTime = (time >> 32);
    }

    if (lpKernelTime)
    {
      // System time.
      unsigned long long time = ((__int64)threadBasicInfo->system_time.seconds * 10000000L) + threadBasicInfo->system_time.microseconds*10L;
      lpKernelTime->dwLowDateTime = (time & 0xFFFFFFFF);
      lpKernelTime->dwHighDateTime = (time >> 32);
    }
  }
  else
  {
    if (lpUserTime)
      lpUserTime->dwLowDateTime = lpUserTime->dwHighDateTime = 0;

    if (lpKernelTime)
      lpKernelTime->dwLowDateTime = lpKernelTime->dwHighDateTime = 0;
  }
#elif _POSIX_THREAD_CPUTIME != -1
    if(lpUserTime)
    {
      lpUserTime->dwLowDateTime = 0;
      lpUserTime->dwHighDateTime = 0;
      clockid_t clock;
      if (pthread_getcpuclockid(hThread->m_hThread, &clock) == 0)
      {
        struct timespec tp = {};
        clock_gettime(clock, &tp);
        unsigned long long time = (unsigned long long)tp.tv_sec * 10000000 + (unsigned long long)tp.tv_nsec/100;
        lpUserTime->dwLowDateTime = (time & 0xFFFFFFFF);
        lpUserTime->dwHighDateTime = (time >> 32);
      }
    }
#else
  if (lpUserTime)
    TimeTToFileTime(0,lpUserTime);
#endif
  return true;
}

BOOL WINAPI SetThreadPriority(HANDLE hThread, int nPriority)
{
  return true;
}

int GetThreadPriority(HANDLE hThread)
{
  return 0;
}

// thread local storage -
// we use different method than in windows. TlsAlloc has no meaning since
// we always take the __thread variable "tls".
// so we return static answer in TlsAlloc and do nothing in TlsFree.
LPVOID WINAPI TlsGetValue(DWORD dwTlsIndex) {
   return tls[dwTlsIndex];
}

BOOL WINAPI TlsSetValue(int dwTlsIndex, LPVOID lpTlsValue) {
   tls[dwTlsIndex]=lpTlsValue;
   return true;
}

BOOL WINAPI TlsFree(DWORD dwTlsIndex) {
   tls_used[dwTlsIndex] = false;
   return true;
}

DWORD WINAPI TlsAlloc() {
  for (int i = 0; i < TLS_INDEXES; i++)
  {
    if (!tls_used[i])
    {
      tls_used[i] = TRUE;
      return i;
    }
  }

  return TLS_OUT_OF_INDEXES;
}


#endif

