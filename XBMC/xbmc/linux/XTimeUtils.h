
#ifndef __X_TIME_UTILS_
#define __X_TIME_UTILS_

#include "PlatformDefs.h"

VOID GetLocalTime(LPSYSTEMTIME);

DWORD GetTickCount(void);

// timing methods.
// when bUseHighRes is true - the cpu clock is used so it may cause bogus values on SMP when thread changes cpu.
BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount, bool bUseHighRes=false);
BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);

void WINAPI Sleep(DWORD dwMilliSeconds);

BOOL   FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime);
BOOL   SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime,  LPFILETIME lpFileTime);
LONG   CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2);
BOOL   FileTimeToSystemTime( const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime);
BOOL   LocalFileTimeToFileTime( const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime);
VOID   GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);

BOOL	FileTimeToTimeT(const FILETIME* lpLocalFileTime, time_t *pTimeT);
BOOL	TimeTToFileTime(time_t timeT, FILETIME* lpLocalFileTime);

// Time
DWORD timeGetTime(VOID); 


#endif

