#include "system.h"
#include "PlatformDefs.h"
#include "XEventUtils.h"

HANDLE WINAPI CreateEvent(void *pDummySec, bool bManualReset, bool bInitialState, char *szDummyName) {
	CXHandle *pHandle = new CXHandle(CXHandle::HND_EVENT);
	pHandle->m_bManualEvent = bManualReset;
	
	pHandle->m_hSem		= SDL_CreateSemaphore(0);
	pHandle->m_hMutex	= SDL_CreateMutex();

	if (bInitialState)
		SetEvent(pHandle);

	return pHandle;
}

bool WINAPI SetEvent(HANDLE hEvent) {
	if (hEvent == NULL || hEvent->m_hSem == NULL || hEvent->m_hMutex == NULL)
		return false;
	
	SDL_mutexP(hEvent->m_hMutex);
	
	if (SDL_SemValue(hEvent->m_hSem) == 0) {
		SDL_SemPost(hEvent->m_hSem);
	}
	
	SDL_mutexV(hEvent->m_hMutex);

	return true;
}

bool WINAPI ResetEvent(HANDLE hEvent) {
	if (hEvent == NULL || hEvent->m_hSem == NULL || hEvent->m_hMutex == NULL)
		return false;

	SDL_mutexP(hEvent->m_hMutex);

	while (SDL_SemTryWait(hEvent->m_hSem) == 0) 
		;

	SDL_mutexV(hEvent->m_hMutex);

	return true;
}

bool WINAPI PulseEvent(HANDLE hEvent) {
	if (hEvent == NULL || hEvent->m_hSem == NULL || hEvent->m_hMutex == NULL)
		return false;

	SetEvent(hEvent);
	SDL_Delay(100);
	ResetEvent(hEvent);

	return true;
}

