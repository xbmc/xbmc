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

#include "system.h"
#include "RWLock.h"

#ifdef _LINUX
static bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow)
{
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0) // get current time
  {
    now.tv_sec  += MillisecondsFromNow / 1000;  // add full seconds
    now.tv_usec += (MillisecondsFromNow % 1000) * 1000;  // add microseconds
    if (now.tv_usec >= 1000000)               // take care of an overflow
    {
      now.tv_sec++;
      now.tv_usec -= 1000000;
    }
    Abstime->tv_sec = now.tv_sec;          // seconds
    Abstime->tv_nsec = now.tv_usec * 1000; // nano seconds
    return true;
  }
  return false;
}
#endif

cRwLock::cRwLock(bool PreferWriter)
{
#ifdef _LINUX
  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init(&attr);
  pthread_rwlockattr_setkind_np(&attr, PreferWriter ? PTHREAD_RWLOCK_PREFER_WRITER_NP : PTHREAD_RWLOCK_PREFER_READER_NP);
  pthread_rwlock_init(&m_rwLock, &attr);
#else
  m_hWriterEvent    = CreateEvent(NULL, TRUE, TRUE, NULL);
  m_hNoReadersEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
  m_nReaderCount    = 0;
  SetEvent(m_hNoReadersEvent);
#endif
}

cRwLock::~cRwLock()
{
#ifdef _LINUX
  pthread_rwlock_destroy(&m_rwLock);
#else
  CloseHandle(m_hWriterEvent);
  CloseHandle(m_hNoReadersEvent);
#endif
}

bool cRwLock::ReadLock(int TimeoutMs)
{
#ifdef _LINUX
  struct timespec abstime;
  if (TimeoutMs)
  {
    if (!GetAbsTime(&abstime, TimeoutMs))
      TimeoutMs = 0;
  }
  int Result = TimeoutMs ? pthread_rwlock_timedrdlock(&m_rwLock, &abstime) : pthread_rwlock_rdlock(&m_rwLock);
  return Result == 0;
#else
  if (WaitForSingleObject(m_hWriterEvent, (TimeoutMs == 0) ? INFINITE : TimeoutMs) == WAIT_OBJECT_0) // Wait for signalled = no writer
  {
    ResetEvent(m_hNoReadersEvent); // Set to non-signalled = no writers allowed
    InterlockedIncrement(&m_nReaderCount);

    if (WaitForSingleObject(m_hWriterEvent, 0) == WAIT_OBJECT_0) // Check if a writer came in
      return true;
    else
    {
      InterlockedDecrement(&m_nReaderCount);
      return false;
    }
  }
  else
    return false;
#endif
}

void cRwLock::ReadUnlock()
{
#ifdef _LINUX
  pthread_rwlock_unlock(&m_rwLock);
#else
  if (InterlockedDecrement(&m_nReaderCount) <= 0)
    SetEvent(m_hNoReadersEvent);
#endif
}

bool cRwLock::WriteLock(int TimeoutMs)
{
#ifdef _LINUX
  struct timespec abstime;
  if (TimeoutMs)
  {
    if (!GetAbsTime(&abstime, TimeoutMs))
      TimeoutMs = 0;
  }
  int Result = TimeoutMs ? pthread_rwlock_timedwrlock(&m_rwLock, &abstime) : pthread_rwlock_wrlock(&m_rwLock);
  return Result == 0;
#else
  ResetEvent(m_hWriterEvent); // Set to non-signalled = make readers wait

  if (WaitForSingleObject(m_hNoReadersEvent, (TimeoutMs == 0) ? INFINITE : TimeoutMs) == WAIT_OBJECT_0) // Readers didn't clear by the timeout
    return true;
  else
  {
    SetEvent(m_hWriterEvent); // Set to signalled = readers allowed
    return false;
  }
#endif
}

void cRwLock::WriteUnlock()
{
#ifdef _LINUX
  pthread_rwlock_unlock(&m_rwLock);
#else
  SetEvent(m_hWriterEvent);
#endif
}
