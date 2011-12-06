#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PVRCLIENT_MEDIAPORTAL_OS_POSIX_H
#define PVRCLIENT_MEDIAPORTAL_OS_POSIX_H

#define _FILE_OFFSET_BITS 64

// Success codes
#define S_OK                             0L
#define S_FALSE                          1L
//
// Error codes
#define ERROR_FILENAME_EXCED_RANGE       206L
#define E_OUTOFMEMORY                    0x8007000EL
#define E_FAIL                           0x8004005EL

#define THREAD_FUNC_PREFIX void *
#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (MAXLONG)

#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

#ifdef TARGET_LINUX
#include <limits.h>
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 256
#endif

#include <string.h>
#define strnicmp(X,Y,N) strncasecmp(X,Y,N)

typedef pthread_mutex_t criticalsection_t;
typedef sem_t wait_event_t;
typedef unsigned char byte;
typedef pid_t tThreadId;

// Various Windows typedefs
// Unused for Linux, but needed for compilation at the moment
typedef struct _SECURITY_ATTRIBUTES {
    unsigned long  nLength;
    void*          lpSecurityDescriptor;
    int            bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#define PATH_SEPARATOR_CHAR '/'

#ifdef TARGET_LINUX
// Retrieve the number of milliseconds that have elapsed since the system was started
#include <time.h>
inline unsigned long GetTickCount(void)
{
  struct timespec ts;
  if(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
  {
    return 0;
  }
  return (unsigned long)( (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000) );
};
#else
#include <time.h>
inline unsigned long GetTickCount(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long)( (ts.tv_sec * 1000) + (ts.tv_usec / 1000) );
};
#endif /* TARGET_LINUX || TARGET_DARWIN */

#endif
