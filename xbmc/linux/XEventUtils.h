
#ifndef __X_EVENT_UTIL_H__
#define __X_EVENT_UTIL_H__

#include "XHandle.h"

#ifndef _WIN32

HANDLE CreateEvent(void *pDummySec, bool bManualReset, bool bInitialState, char *szDummyName);
bool SetEvent(HANDLE hEvent);
bool ResetEvent(HANDLE hEvent);
bool PulseEvent(HANDLE hEvent);

#endif


#endif

