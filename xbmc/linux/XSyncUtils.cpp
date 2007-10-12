
#include "XSyncUtils.h"
#include "PlatformDefs.h"
#include "XHandle.h"

#include <SDL.h>

#ifdef _LINUX

#include <semaphore.h>
#include <time.h>
#include <errno.h>

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

void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
  if (lpCriticalSection)
    InitializeRecursiveMutex(lpCriticalSection,false);
}

void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
  if (lpCriticalSection) 
    DestroyRecursiveMutex(lpCriticalSection);
}

void WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
  WaitForSingleObject(lpCriticalSection, INFINITE);
}

void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
  ReleaseMutex(lpCriticalSection);
}

void GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer) {
  if (!lpBuffer)
    return;

  memset(lpBuffer,0,sizeof(MEMORYSTATUS));

#ifdef _LINUX
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

DWORD WINAPI WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds ) {
  if (hHandle == NULL ||  hHandle == (HANDLE)-1)
    return WAIT_FAILED;

  DWORD dwRet = WAIT_FAILED;
  BOOL bNeedWait = true;

  switch (hHandle->GetType()) {
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
    case CXHandle::HND_EVENT:
      if (hHandle->m_hSem) {          
        int nRet = 0;
        if (dwMilliseconds == INFINITE)
          nRet = SDL_SemWait(hHandle->m_hSem);
        else
        {
          // See NOTE in SDL_SemWaitTimeout2() definition (towards the end of this file)
          // nRet = SDL_SemWaitTimeout(hHandle->m_hSem, dwMilliseconds);
          nRet = SDL_SemWaitTimeout2(hHandle->m_hSem, dwMilliseconds);
        }

        if (nRet == 0)
          dwRet = WAIT_OBJECT_0;
        else if (nRet == SDL_MUTEX_TIMEDOUT)
          dwRet = WAIT_TIMEOUT;

        // in case of successful wait with manual event - we post to the semaphore again so that
        // all threads that are waiting will wake up.
        if (dwRet == WAIT_OBJECT_0 && (hHandle->GetType() == CXHandle::HND_EVENT || hHandle->GetType() == CXHandle::HND_THREAD) && hHandle->m_bManualEvent) {
          SDL_SemPost(hHandle->m_hSem);
        }
        else if (dwRet == WAIT_OBJECT_0 && hHandle->GetType() == CXHandle::HND_MUTEX) {
          SDL_mutexP(hHandle->m_hMutex);
          hHandle->OwningThread = SDL_ThreadID();
          hHandle->RecursionCount = 1;
          SDL_mutexV(hHandle->m_hMutex);
        }
      }

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

// NOTE: SDL_SemWaitTimeout is implemented using a busy wait, which is
// highly inaccurate. Until it is fixed, we do it the right way using
// sem_timedwait()
int SDL_SemWaitTimeout2(SDL_sem *sem, Uint32 dwMilliseconds)
{
  int nRet = 0;
  struct timespec req;
  clock_gettime(CLOCK_REALTIME, &req);
  req.tv_sec += dwMilliseconds / 1000;
  req.tv_nsec += (dwMilliseconds % 1000) * 1000000;
  req.tv_sec += req.tv_nsec / 1000000000L;
  req.tv_nsec = req.tv_nsec % 1000000000L;
  while (((nRet = sem_timedwait((sem_t*)(sem), &req))==-1) && (errno==EINTR))
  {
    continue;
  }
  if (nRet != 0)
  {
    return SDL_MUTEX_TIMEDOUT;
  }
  return 0;
}

#endif
