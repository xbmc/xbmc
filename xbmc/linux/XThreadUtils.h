#ifndef __XTHREAD_UTILS__H__
#define __XTHREAD_UTILS__H__

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

#include <stdlib.h>

HANDLE WINAPI CreateThread(
      LPSECURITY_ATTRIBUTES lpThreadAttributes,
        SIZE_T dwStackSize,
          LPTHREAD_START_ROUTINE lpStartAddress,
            LPVOID lpParameter,
        DWORD dwCreationFlags,
          LPDWORD lpThreadId
    );

HANDLE _beginthreadex(
   void *security,
   unsigned stack_size,
   int ( *start_address )( void * ),
   void *arglist,
   unsigned initflag,
   unsigned *thrdaddr
);

uintptr_t _beginthread(
    void( *start_address )( void * ),
    unsigned stack_size,
    void *arglist
);

#if 0 // Deprecated, use CThread::GetCurrentThreadId() instead
DWORD WINAPI GetCurrentThreadId(void);
#endif

HANDLE WINAPI GetCurrentThread(void);
HANDLE WINAPI GetCurrentProcess(void);

BOOL WINAPI GetThreadTimes (
  HANDLE hThread,
  LPFILETIME lpCreationTime,
  LPFILETIME lpExitTime,
  LPFILETIME lpKernelTime,
  LPFILETIME lpUserTime
);

int GetThreadPriority(
  HANDLE hThread
);

BOOL WINAPI SetThreadPriority(
  HANDLE hThread,
  int nPriority
);

// helper class for TLS handling
class TLS
{
public:
  TLS()
  {
    pthread_key_create(&m_key, free);
  }

  ~TLS()
  {
    pthread_key_delete(m_key);
  }

  void *Get()
  {
    if (pthread_getspecific(m_key) == NULL)
      pthread_setspecific(m_key, calloc(8, 1));

    return pthread_getspecific(m_key);
  }

  pthread_key_t m_key;
};

#endif
