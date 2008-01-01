#include "system.h"
#include "PlatformDefs.h"
#include "XEventUtils.h"

HANDLE WINAPI CreateEvent(void *pDummySec, bool bManualReset, bool bInitialState, char *szDummyName) 
{
	CXHandle *pHandle = new CXHandle(CXHandle::HND_EVENT);
	pHandle->m_bManualEvent = bManualReset;
    pHandle->m_hCond = SDL_CreateCond();
	pHandle->m_hMutex = SDL_CreateMutex();
    pHandle->m_bEventSet = false;

	if (bInitialState)
		SetEvent(pHandle);

	return pHandle;
}

bool WINAPI SetEvent(HANDLE hEvent) 
{
	if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
		return false;
	
	SDL_mutexP(hEvent->m_hMutex);
	if (hEvent->m_bEventSet == false)
	{
        hEvent->m_bEventSet = true;
        SDL_CondSignal(hEvent->m_hCond);
	}
	SDL_mutexV(hEvent->m_hMutex);

	return true;
}

bool WINAPI ResetEvent(HANDLE hEvent) 
{
	if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
		return false;

	SDL_mutexP(hEvent->m_hMutex);
    hEvent->m_bEventSet = false;
	SDL_mutexV(hEvent->m_hMutex);

	return true;
}

bool WINAPI PulseEvent(HANDLE hEvent) {
	if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
		return false;

	SetEvent(hEvent);
	SDL_Delay(100);
	ResetEvent(hEvent);

	return true;
}

