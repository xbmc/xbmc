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

//
// The state of a manual-reset event object remains signaled until it is set explicitly to the nonsignaled 
// state by the ResetEvent function. Any number of waiting threads, or threads that subsequently begin wait 
// operations for the specified event object by calling one of the wait functions, can be released while the 
// object's state is signaled.
//
// The state of an auto-reset event object remains signaled until a single waiting thread is released, at 
// which time the system automatically sets the state to nonsignaled. If no threads are waiting, the event 
// object's state remains signaled.
//
bool WINAPI SetEvent(HANDLE hEvent) 
{
	if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
		return false;
	
	SDL_mutexP(hEvent->m_hMutex);
	if (hEvent->m_bEventSet == false)
	{
	  hEvent->m_bEventSet = true;

	  // FIXME: Don't we need to wake all threads with a manual event?
	  //        Doing so results in weirdness.
	  //if (hEvent->m_bManualEvent == true)
	  //SDL_CondBroadcast(hEvent->m_hCond) < 0);
	  //else
	  SDL_CondSignal(hEvent->m_hCond);

	  // FIXME: Why dont' we need to reset if it's an automatic event?
	  //        Doing so results in weirdness.
    //if (hEvent->m_bManualEvent == false)
    //  hEvent->m_bEventSet = false;
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

bool WINAPI PulseEvent(HANDLE hEvent) 
{
	if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
		return false;

	SetEvent(hEvent);
	SDL_Delay(100);
	ResetEvent(hEvent);

	return true;
}

