
#ifndef __X_EVENT_UTIL_H__
#define __X_EVENT_UTIL_H__

#include "XHandle.h"

#ifdef _LINUX

HANDLE WINAPI CreateEvent(void *pDummySec, bool bManualReset, bool bInitialState, char *szDummyName);
bool WINAPI SetEvent(HANDLE hEvent);
bool WINAPI ResetEvent(HANDLE hEvent);
bool WINAPI PulseEvent(HANDLE hEvent);

#endif


#endif

