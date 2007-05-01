
#include "XTimeUtils.h"

#ifdef _LINUX

DWORD timeGetTime(void)
{
  struct timezone tz;
  struct timeval tim;
  gettimeofday(&tim, &tz);
  DWORD result = tim.tv_usec;
  result += ((DWORD) tim.tv_sec) * 1000;
  return result;
}

void Sleep(DWORD dwMilliSeconds)
{
	SDL_Delay(dwMilliSeconds);
}

VOID GetLocalTime(LPSYSTEMTIME sysTime) {
}

DWORD GetTickCount(void) {
	return SDL_GetTicks();
}

// "Load Skin XML: %.2fms", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart
BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
	return true;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency) {
	return true;
}

BOOL   FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime) {
	return true;
}

BOOL   SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime,  LPFILETIME lpFileTime) {
	return true;
}

LONG   CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2) {
	return true;
}

BOOL   FileTimeToSystemTime( const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime) {
	return true;
}

BOOL   LocalFileTimeToFileTime( const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime) {
	return true;
}

#endif

