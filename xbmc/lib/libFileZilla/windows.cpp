
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
  return 0;
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
  return 0;
}

