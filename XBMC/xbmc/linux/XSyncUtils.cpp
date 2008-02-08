
#include "XSyncUtils.h"
#include "PlatformDefs.h"
#include "XHandle.h"

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

#ifdef _LINUX

#include <semaphore.h>
#include <time.h>
#include <errno.h>

#include "../utils/log.h"

SDL_mutex *g_mutex = SDL_CreateMutex();

bool InitializeRecursiveMutex(HANDLE hMutex, BOOL bInitialOwner) {
  if (!hMutex)
    return false;

  // we use semaphores instead of the mutex because in SDL we CANT wait for a mutex
  // to lock with timeout.
  hMutex->m_hSem    = SDL_CreateSemaphore(bInitialOwner?0:1);
  hMutex->m_hMutex  = SDL_CreateMutex();
  hMutex->ChangeType(CXHandle::HND_MUTEX);

  if (bInitialOwner) {
    hMutex->OwningThread  = SDL_ThreadID();
    hMutex->RecursionCount  = 1;
  }

  return true;
}

bool  DestroyRecursiveMutex(HANDLE hMutex) {
  if (hMutex == NULL || hMutex->m_hMutex == NULL || hMutex->m_hSem == NULL) 
    return false;

  SDL_DestroySemaphore(hMutex->m_hSem);
  SDL_DestroyMutex(hMutex->m_hMutex);

  hMutex->m_hMutex = NULL;
  hMutex->m_hSem = NULL;

  return true;
}

HANDLE  WINAPI CreateMutex( LPSECURITY_ATTRIBUTES lpMutexAttributes,  BOOL bInitialOwner,  LPCTSTR lpName ) {
  HANDLE hMutex = new CXHandle(CXHandle::HND_MUTEX);

  InitializeRecursiveMutex(hMutex,bInitialOwner);

  return hMutex;
}

bool WINAPI ReleaseMutex( HANDLE hMutex ) {
  if (hMutex == NULL || hMutex->m_hSem == NULL || hMutex->m_hMutex == NULL)
    return false;

  BOOL bOk = false;

  SDL_mutexP(hMutex->m_hMutex);
  if (hMutex->OwningThread == SDL_ThreadID() && hMutex->RecursionCount > 0) {
    bOk = true;
    if (--hMutex->RecursionCount == 0) {
      hMutex->OwningThread = 0;
      SDL_SemPost(hMutex->m_hSem);
    }
  }
  SDL_mutexV(hMutex->m_hMutex);

  return bOk;
}

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

#elif defined(_LINUX)
  struct sysinfo info;
  sysinfo(&info);

  lpBuffer->dwLength = sizeof(MEMORYSTATUS);
  lpBuffer->dwAvailPageFile  = (info.freeswap * info.mem_unit);
  lpBuffer->dwAvailPhys    = (info.freeram * info.mem_unit);
  lpBuffer->dwAvailVirtual  = (info.freeram * info.mem_unit);
  lpBuffer->dwTotalPhys    = (info.totalram * info.mem_unit); 
  lpBuffer->dwTotalVirtual  = (info.totalram * info.mem_unit);
#endif
}

//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WaitForEvent(HANDLE hHandle, DWORD dwMilliseconds)
{
	DWORD dwRet = 0;
	int   nRet = 0;

	if (dwMilliseconds == 0)
	{
		if (hHandle->m_bEventSet == true)
			nRet = 0;
		else
			nRet = SDL_MUTEX_TIMEDOUT;
	}
	else if (dwMilliseconds == INFINITE)
	{
		if (hHandle->m_bEventSet == false)
			nRet = SDL_CondWait(hHandle->m_hCond, hHandle->m_hMutex);
		if (hHandle->m_bManualEvent == false)
			hHandle->m_bEventSet = false;
	}
	else
	{
		if (hHandle->m_bEventSet == false)
			nRet = SDL_CondWaitTimeout(hHandle->m_hCond, hHandle->m_hMutex, dwMilliseconds);
		if (hHandle->m_bManualEvent == false && nRet != SDL_MUTEX_TIMEDOUT)
			hHandle->m_bEventSet = false;
	}

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
  BOOL bNeedWait = true;

  switch (hHandle->GetType()) {
    case CXHandle::HND_EVENT:
		
			SDL_mutexP(hHandle->m_hMutex);
			
			// Perform the wait.
			dwRet = WaitForEvent(hHandle, dwMilliseconds);
			
			SDL_mutexV(hHandle->m_hMutex);
			break;
      
    case CXHandle::HND_MUTEX:

      SDL_mutexP(hHandle->m_hMutex);
      if (hHandle->OwningThread == SDL_ThreadID() && 
        hHandle->RecursionCount > 0) {
        hHandle->RecursionCount++;
        dwRet = WAIT_OBJECT_0;
        bNeedWait = false;
      }
      SDL_mutexV(hHandle->m_hMutex);

      if (!bNeedWait)
        break; // no need for the wait.

    case CXHandle::HND_THREAD:
			
			SDL_mutexP(hHandle->m_hMutex);
			
			// Perform the wait.
			dwRet = WaitForEvent(hHandle, dwMilliseconds);

      // In case of successful wait with manual event wake up all threads that are waiting.
      if (dwRet == WAIT_OBJECT_0 && (hHandle->GetType() == CXHandle::HND_EVENT || hHandle->GetType() == CXHandle::HND_THREAD) && hHandle->m_bManualEvent)
			{
				SDL_CondBroadcast(hHandle->m_hCond);
			}
			else if (dwRet == WAIT_OBJECT_0 && hHandle->GetType() == CXHandle::HND_MUTEX)
			{
				hHandle->OwningThread = SDL_ThreadID();
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
  DWORD dwStartTime   = SDL_GetTicks();
  BOOL *bDone = new BOOL[nCount];
  
  for (unsigned int nFlag=0; nFlag<nCount; nFlag++ ) 
    bDone[nFlag] = FALSE;

  DWORD nSignalled = 0;
  while (!bWaitEnded) {
  
    for (unsigned int i=0; i < nCount; i++) {

      if (!bDone[i]) {
        DWORD dwWaitRC = WaitForSingleObject(lpHandles[i], 20);
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
          bWaitEnded = TRUE;
          break;
        }
      }

    }

    DWORD dwElapsed = SDL_GetTicks() - dwStartTime;
    if (dwMilliseconds != INFINITE && dwElapsed >= dwMilliseconds) {
      dwRet = WAIT_TIMEOUT;
      bWaitEnded = TRUE;
      break;
    }

    // wait 100ms between rounds
    SDL_Delay(100);
  }

  delete [] bDone;
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
