
#include "XSyncUtils.h"
#include "PlatformDefs.h"
#include "XHandle.h"


#ifdef _LINUX

bool InitializeRecursiveMutex(HANDLE hMutex, BOOL bInitialOwner) {
	if (!hMutex)
		return false;

	// we use semaphores instead of the mutex because in SDL we CANT wait for a mutex
	// to lock with timeout.
	hMutex->m_hSem		= SDL_CreateSemaphore(bInitialOwner?0:1);
	hMutex->m_hMutex	= SDL_CreateMutex();
	hMutex->m_type		= CXHandle::HND_MUTEX;

	if (bInitialOwner) {
		hMutex->OwningThread	= SDL_ThreadID();
		hMutex->RecursionCount	= 1;
	}

	return true;
}

bool	DestroyRecursiveMutex(HANDLE hMutex) {
	if (hMutex == NULL || hMutex->m_hMutex == NULL || hMutex->m_hSem == NULL) 
		return false;

	SDL_DestroySemaphore(hMutex->m_hSem);
	SDL_DestroyMutex(hMutex->m_hMutex);

	hMutex->m_hMutex = NULL;
	hMutex->m_hSem = NULL;
}

HANDLE	CreateMutex( LPSECURITY_ATTRIBUTES lpMutexAttributes,  BOOL bInitialOwner,  LPCTSTR lpName ) {
	HANDLE hMutex = new CXHandle(CXHandle::HND_MUTEX);

	InitializeRecursiveMutex(hMutex,bInitialOwner);

	return hMutex;
}

bool ReleaseMutex( HANDLE hMutex ) {
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

void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
	if (lpCriticalSection)
		InitializeRecursiveMutex(lpCriticalSection,false);
}

void DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
	if (lpCriticalSection) 
		DestroyRecursiveMutex(lpCriticalSection);
}

void EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
	WaitForSingleObject(lpCriticalSection, INFINITE);
}

void LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
	ReleaseMutex(lpCriticalSection);
}

DWORD GetCurrentThreadId(void) {
	return SDL_ThreadID();
}

void GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer) {
	if (!lpBuffer)
		return;

	memset(lpBuffer,0,sizeof(MEMORYSTATUS));

#ifdef _LINUX
	struct sysinfo info;
	sysinfo(&info);

	lpBuffer->dwLength = sizeof(MEMORYSTATUS);
	lpBuffer->dwAvailPageFile	= (info.freeswap * info.mem_unit);
	lpBuffer->dwAvailPhys		= (info.freeram * info.mem_unit);
	lpBuffer->dwAvailVirtual	= (info.freeram * info.mem_unit);
	lpBuffer->dwTotalPhys		= (info.totalram * info.mem_unit); 
	lpBuffer->dwTotalVirtual	= (info.totalram * info.mem_unit);
#endif
}

DWORD WINAPI WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds ) {
	if (hHandle == NULL)
		return WAIT_FAILED;

	DWORD dwRet = WAIT_FAILED;
	BOOL bNeedWait = true;

	switch (hHandle->m_type) {
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

		case CXHandle::HND_EVENT:
			if (hHandle->m_hSem) {					
				int nRet = 0;
				if (dwMilliseconds == INFINITE)
					nRet = SDL_SemWait(hHandle->m_hSem);
				else
					nRet = SDL_SemWaitTimeout(hHandle->m_hSem, dwMilliseconds);

				if (nRet == 0)
					dwRet = WAIT_OBJECT_0;
				else if (nRet == SDL_MUTEX_TIMEDOUT)
					dwRet = WAIT_TIMEOUT;

				// in case of successful wait with manual event - we post to the semaphore again so that
				// all threads that are waiting will wake up.
				if (dwRet == WAIT_OBJECT_0 && hHandle->m_type == CXHandle::HND_EVENT && hHandle->m_bManualEvent) {
					SDL_SemPost(hHandle->m_hSem);
				}
				else if (dwRet == WAIT_OBJECT_0 && hHandle->m_type == CXHandle::HND_MUTEX) {
					SDL_mutexP(hHandle->m_hMutex);
					hHandle->OwningThread = SDL_ThreadID();
					hHandle->RecursionCount = 1;
					SDL_mutexV(hHandle->m_hMutex);
				}
			}

			break;
		case CXHandle::HND_THREAD:
			if (hHandle->m_hThread) {
				SDL_WaitThread(hHandle->m_hThread, NULL);
			}
			break;
		default:
			XXLog(ERROR, "cant wait for this type of object");
	}

	return dwRet;
}

DWORD WaitForMultipleObjects( DWORD nCount, HANDLE* lpHandles, BOOL bWaitAll,  DWORD dwMilliseconds) {
	DWORD dwRet = WAIT_FAILED;

	if (nCount < 1 || lpHandles == NULL)
		return dwRet;

	BOOL bWaitEnded		= FALSE;
	DWORD dwStartTime   = SDL_GetTicks();
	BOOL *bDone = new BOOL[nCount];
	
	for (int nFlag=0; nFlag<nCount; nFlag++ ) 
		bDone[nFlag] = FALSE;

	int nSignalled = 0;
	while (!bWaitEnded) {
	
		for (int i=0; i < nCount; i++) {

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

#endif
