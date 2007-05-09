
#ifndef __X_TIME_UTILS_
#define __X_TIME_UTILS_

#include "PlatformDefs.h"

VOID GetLocalTime(LPSYSTEMTIME);

DWORD GetTickCount(void);

// "Load Skin XML: %.2fms", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart
BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);

void Sleep(DWORD dwMilliSeconds);

BOOL   FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime);
BOOL   SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime,  LPFILETIME lpFileTime);
LONG   CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2);
BOOL   FileTimeToSystemTime( const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime);
BOOL   LocalFileTimeToFileTime( const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime);

BOOL	FileTimeToTimeT(const FILETIME* lpLocalFileTime, time_t *pTimeT);
BOOL	TimeTToFileTime(time_t timeT, FILETIME* lpLocalFileTime);

// Time
DWORD timeGetTime(VOID); 


#endif

