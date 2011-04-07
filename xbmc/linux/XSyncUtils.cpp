/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#ifndef _WIN32


#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <stack>
#include <functional>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "XSyncUtils.h"
#include "XTimeUtils.h"
#include "PlatformDefs.h"
#include "XHandle.h"
#include "XEventUtils.h"
#include "threads/XBMC_mutex.h"


using namespace std;

#include "../utils/log.h"
#include "../utils/TimeUtils.h"

static SDL_mutex *g_mutex = SDL_CreateMutex();

bool InitializeRecursiveMutex(HANDLE hMutex, BOOL bInitialOwner) {
  if (!hMutex)
    return false;

  // we use semaphores instead of the mutex because in SDL we CANT wait for a mutex
  // to lock with timeout.
  hMutex->m_pSem    = new CSemaphore(bInitialOwner?0:1);
  hMutex->m_hMutex  = SDL_CreateMutex();
  hMutex->ChangeType(CXHandle::HND_MUTEX);

  if (bInitialOwner) {
    hMutex->OwningThread  = pthread_self();
    hMutex->RecursionCount  = 1;
  }

  return true;
}

bool  DestroyRecursiveMutex(HANDLE hMutex) {
  if (hMutex == NULL || hMutex->m_hMutex == NULL || hMutex->m_pSem == NULL)
    return false;

  delete hMutex->m_pSem;
  SDL_DestroyMutex(hMutex->m_hMutex);

  hMutex->m_hMutex = NULL;
  hMutex->m_pSem = NULL;

  return true;
}

HANDLE  WINAPI CreateMutex( LPSECURITY_ATTRIBUTES lpMutexAttributes,  BOOL bInitialOwner,  LPCTSTR lpName ) {
  HANDLE hMutex = new CXHandle(CXHandle::HND_MUTEX);

  InitializeRecursiveMutex(hMutex,bInitialOwner);

  return hMutex;
}

bool WINAPI ReleaseMutex( HANDLE hMutex ) {
  if (hMutex == NULL || hMutex->m_pSem == NULL || hMutex->m_hMutex == NULL)
    return false;

  BOOL bOk = false;

  SDL_mutexP(hMutex->m_hMutex);
  if (hMutex->OwningThread == pthread_self() && hMutex->RecursionCount > 0) {
    bOk = true;
    if (--hMutex->RecursionCount == 0) {
      hMutex->OwningThread = 0;
      hMutex->m_pSem->Post();
    }
  }
  SDL_mutexV(hMutex->m_hMutex);

  return bOk;
}

#if defined(_LINUX) && !defined(__APPLE__)
static FILE* procMeminfoFP = NULL;
#endif

void GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
  if (!lpBuffer)
    return;

  memset(lpBuffer, 0, sizeof(MEMORYSTATUS));
  lpBuffer->dwLength = sizeof(MEMORYSTATUS);

#ifdef __APPLE__
  uint64_t physmem;
  size_t len = sizeof physmem;
  int mib[2] = { CTL_HW, HW_MEMSIZE };
  size_t miblen = sizeof(mib) / sizeof(mib[0]);

  // Total physical memory.
  if (sysctl(mib, miblen, &physmem, &len, NULL, 0) == 0 && len == sizeof (physmem))
      lpBuffer->dwTotalPhys = physmem;

  // Virtual memory.
  mib[0] = CTL_VM; mib[1] = VM_SWAPUSAGE;
  struct xsw_usage swap;
  len = sizeof(struct xsw_usage);
  if (sysctl(mib, miblen, &swap, &len, NULL, 0) == 0)
  {
      lpBuffer->dwAvailPageFile = swap.xsu_avail;
      lpBuffer->dwTotalVirtual = lpBuffer->dwTotalPhys + swap.xsu_total;
  }

  // In use.
  mach_port_t stat_port = mach_host_self();
  vm_statistics_data_t vm_stat;
  mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(natural_t);
  if (host_statistics(stat_port, HOST_VM_INFO, (host_info_t)&vm_stat, &count) == 0)
  {
      // Find page size.
      int pageSize;
      mib[0] = CTL_HW; mib[1] = HW_PAGESIZE;
      len = sizeof(int);
      if (sysctl(mib, miblen, &pageSize, &len, NULL, 0) == 0)
      {
          uint64_t used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pageSize;

          lpBuffer->dwAvailPhys = lpBuffer->dwTotalPhys - used;
          lpBuffer->dwAvailVirtual  = lpBuffer->dwAvailPhys; // FIXME.
      }
  }
#else
  struct sysinfo info;
  char name[32];
  unsigned val;
  if (!procMeminfoFP && (procMeminfoFP = fopen("/proc/meminfo", "r")) == NULL)
    sysinfo(&info);
  else
  {
    memset(&info, 0, sizeof(struct sysinfo));
    info.mem_unit = 4096;
    while (fscanf(procMeminfoFP, "%31s %u%*[^\n]\n", name, &val) != EOF)
    {
      if (strncmp("MemTotal:", name, 9) == 0)
        info.totalram = val/4;
      else if (strncmp("MemFree:", name, 8) == 0)
        info.freeram = val/4;
      else if (strncmp("Buffers:", name, 8) == 0)
        info.bufferram += val/4;
      else if (strncmp("Cached:", name, 7) == 0)
        info.bufferram += val/4;
      else if (strncmp("SwapTotal:", name, 10) == 0)
        info.totalswap = val/4;
      else if (strncmp("SwapFree:", name, 9) == 0)
        info.freeswap = val/4;
      else if (strncmp("HighTotal:", name, 10) == 0)
        info.totalhigh = val/4;
      else if (strncmp("HighFree:", name, 9) == 0)
        info.freehigh = val/4;
    }
    rewind(procMeminfoFP);
    fflush(procMeminfoFP);
  }
  lpBuffer->dwLength        = sizeof(MEMORYSTATUS);
  lpBuffer->dwAvailPageFile = (info.freeswap * info.mem_unit);
  lpBuffer->dwAvailPhys     = ((info.freeram + info.bufferram) * info.mem_unit);
  lpBuffer->dwAvailVirtual  = ((info.freeram + info.bufferram) * info.mem_unit);
  lpBuffer->dwTotalPhys     = (info.totalram * info.mem_unit);
  lpBuffer->dwTotalVirtual  = (info.totalram * info.mem_unit);
#endif
}

//////////////////////////////////////////////////////////////////////////////
static DWORD WINAPI WaitForEvent(HANDLE hHandle, DWORD dwMilliseconds)
{
  DWORD dwRet = 0;
  int   nRet = 0;
  // something like the following would be nice,
  // but I can't figure a wait to check if a mutex is locked:
  // assert(hHandle->m_hMutex.IsLocked());
  if (hHandle->m_bEventSet == false)
  {
    if (dwMilliseconds == 0)
    {
      nRet = SDL_MUTEX_TIMEDOUT;
    }
    else if (dwMilliseconds == INFINITE)
    {
      //wait until event is set
      while( hHandle->m_bEventSet == false )
      {
        nRet = SDL_CondWait(hHandle->m_hCond, hHandle->m_hMutex);
      }
    }
    else
    {
      //wait until event is set, but modify remaining time
      DWORD dwStartTime = CTimeUtils::GetTimeMS();
      DWORD dwRemainingTime = dwMilliseconds;
      while( hHandle->m_bEventSet == false )
      {
        nRet = SDL_CondWaitTimeout(hHandle->m_hCond, hHandle->m_hMutex, dwRemainingTime);
        if(hHandle->m_bEventSet)
          break;

        //fix time to wait because of spurious wakeups
        DWORD dwElapsed = CTimeUtils::GetTimeMS() - dwStartTime;
        if(dwElapsed < dwMilliseconds)
        {
          dwRemainingTime = dwMilliseconds - dwElapsed;
        }
        else
        {
          //ran out of time
          nRet = SDL_MUTEX_TIMEDOUT;
          break;
        }
      }
    }
  }

  if (hHandle->m_bManualEvent == false && nRet == 0)
    hHandle->m_bEventSet = false;

  // Translate return code.
  if (nRet == 0)
    dwRet = WAIT_OBJECT_0;
  else if (nRet == SDL_MUTEX_TIMEDOUT)
    dwRet = WAIT_TIMEOUT;
  else
    dwRet = WAIT_FAILED;

  return dwRet;
}

DWORD WINAPI WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds ) {
  if (hHandle == NULL ||  hHandle == (HANDLE)-1)
    return WAIT_FAILED;

  DWORD dwRet = WAIT_FAILED;

  switch (hHandle->GetType()) {
    case CXHandle::HND_EVENT:
    case CXHandle::HND_THREAD:

      SDL_mutexP(hHandle->m_hMutex);

      // Perform the wait.
      dwRet = WaitForEvent(hHandle, dwMilliseconds);

      SDL_mutexV(hHandle->m_hMutex);
      break;

    case CXHandle::HND_MUTEX:

      SDL_mutexP(hHandle->m_hMutex);
      if (hHandle->OwningThread == pthread_self() &&
        hHandle->RecursionCount > 0) {
        hHandle->RecursionCount++;
        dwRet = WAIT_OBJECT_0;
        SDL_mutexV(hHandle->m_hMutex);
        break;
      }

      // Perform the wait.
      dwRet = WaitForEvent(hHandle, dwMilliseconds);

      if (dwRet == WAIT_OBJECT_0)
      {
        hHandle->OwningThread = pthread_self();
        hHandle->RecursionCount = 1;
      }

      SDL_mutexV(hHandle->m_hMutex);

      break;
    default:
      XXLog(ERROR, "cant wait for this type of object");
  }

  return dwRet;
}

DWORD WINAPI WaitForMultipleObjects( DWORD nCount, HANDLE* lpHandles, BOOL bWaitAll,  DWORD dwMilliseconds) {
  DWORD dwRet = WAIT_FAILED;

  if (nCount < 1 || lpHandles == NULL)
    return dwRet;

  BOOL bWaitEnded    = FALSE;
  DWORD dwStartTime   = CTimeUtils::GetTimeMS();
  BOOL *bDone = new BOOL[nCount];
  CXHandle* multi = CreateEvent(NULL, FALSE, FALSE, NULL);

  for (unsigned int i=0; i < nCount; i++)
  {
    bDone[i] = FALSE;
    SDL_mutexP(lpHandles[i]->m_hMutex);
    lpHandles[i]->m_hParents.push_back(multi);
    SDL_mutexV(lpHandles[i]->m_hMutex);
  }

  DWORD nSignalled = 0;
  while (!bWaitEnded) {

    for (unsigned int i=0; i < nCount; i++) {

      if (!bDone[i]) {
        DWORD dwWaitRC = WaitForSingleObject(lpHandles[i], 0);
        if (dwWaitRC == WAIT_OBJECT_0) {
          dwRet = WAIT_OBJECT_0 + i;

          nSignalled++;

          bDone[i] = TRUE;

          if ( (bWaitAll && nSignalled == nCount) || !bWaitAll ) {
            bWaitEnded = TRUE;
            break;
          }
        }
        else if (dwWaitRC == WAIT_FAILED) {
          dwRet = WAIT_FAILED;
          bWaitEnded = TRUE;
          break;
        }
      }

    }

    if (bWaitEnded)
      break;

    DWORD dwElapsed = CTimeUtils::GetTimeMS() - dwStartTime;
    if (dwMilliseconds != INFINITE && dwElapsed >= dwMilliseconds) {
      dwRet = WAIT_TIMEOUT;
      bWaitEnded = TRUE;
      break;
    }

    SDL_mutexP(multi->m_hMutex);
    DWORD dwWaitRC = WaitForEvent(multi, 200);
    SDL_mutexV(multi->m_hMutex);

    if(dwWaitRC == WAIT_FAILED)
    {
      dwRet = WAIT_FAILED;
      bWaitEnded = TRUE;
      break;
    }
  }

  for (unsigned int i=0; i < nCount; i++)
  {
    SDL_mutexP(lpHandles[i]->m_hMutex);
    lpHandles[i]->m_hParents.remove_if(bind2nd(equal_to<CXHandle*>(), multi));
    SDL_mutexV(lpHandles[i]->m_hMutex);
  }

  delete [] bDone;
  CloseHandle(multi);
  return dwRet;
}

LONG InterlockedIncrement(  LONG * Addend ) {
  if (Addend == NULL)
    return 0;

  SDL_mutexP(g_mutex);
  (* Addend)++;
  LONG nKeep = *Addend;
  SDL_mutexV(g_mutex);

  return nKeep;
}

LONG InterlockedDecrement(  LONG * Addend ) {
  if (Addend == NULL)
    return 0;

  SDL_mutexP(g_mutex);
  (* Addend)--;
  LONG nKeep = *Addend;
  SDL_mutexV(g_mutex);

  return nKeep;
}

LONG InterlockedCompareExchange(
  LONG * Destination,
  LONG Exchange,
  LONG Comparand
) {
  if (Destination == NULL)
    return 0;
  SDL_mutexP(g_mutex);
  LONG nKeep = *Destination;
  if (*Destination == Comparand)
    *Destination = Exchange;
  SDL_mutexV(g_mutex);

  return nKeep;
}

LONG InterlockedExchange(
  LONG volatile* Target,
  LONG Value
)
{
  if (Target == NULL)
    return 0;

  SDL_mutexP(g_mutex);
  LONG nKeep = *Target;
  *Target = Value;
  SDL_mutexV(g_mutex);

  return nKeep;
}

#endif
