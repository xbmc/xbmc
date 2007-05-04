
#include "XTimeUtils.h"
#include <time.h>

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

VOID GetLocalTime(LPSYSTEMTIME sysTime) 
{
  const time_t t = time(NULL);
  struct tm* now = localtime(&t);
  sysTime->wYear = now->tm_year + 1900;
  sysTime->wMonth = now->tm_mon + 1;
  sysTime->wDayOfWeek = now->tm_wday;
  sysTime->wDay = now->tm_mday;
  sysTime->wHour = now->tm_hour;
  sysTime->wMinute = now->tm_min;
  sysTime->wSecond = now->tm_sec;
  sysTime->wMilliseconds = 0;
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

