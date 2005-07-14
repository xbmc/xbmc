
#include "stdafx.h"
#include "windows.h"



int GetDateFormat(
  LCID Locale,               // locale for which date is to be formatted
  DWORD dwFlags,             // flags specifying function options
  CONST SYSTEMTIME *lpDate,  // date to be formatted
  LPCTSTR lpFormat,          // date format string
  LPTSTR lpDateStr,          // buffer for storing formatted string
  int cchDate                // size of buffer
)
{
  // will always return just current local date in YYYY-MM-DD format
  SYSTEMTIME time;
  if (cchDate < 10) return 0;
  GetLocalTime(&time);
  _stprintf(lpDateStr, _T("%04d-%02d-%02d"), time.wYear, time.wMonth, time.wDay);
  return _tcslen(lpDateStr);
}

int GetTimeFormat(
  LCID Locale,       // locale for which time is to be formatted
  DWORD dwFlags,             // flags specifying function options
  CONST SYSTEMTIME *lpTime,  // time to be formatted
  LPCTSTR lpFormat,          // time format string
  LPTSTR lpTimeStr,          // buffer for storing formatted string
  int cchTime                // size, in bytes or characters, of the buffer
)
{
  // will always return just current local time in HH:MM:SS format
  SYSTEMTIME time;
  if (cchTime < 8) return 0;
  GetLocalTime(&time);
  _stprintf(lpTimeStr, _T("%0d:%02d:%02d"), time.wHour, time.wMinute, time.wSecond);
  return _tcslen(lpTimeStr);
}

