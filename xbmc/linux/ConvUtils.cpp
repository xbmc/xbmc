#include "PlatformDefs.h"

#ifdef _LINUX

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include <utils/log.h>


/*
 ** The following two functions together make up an itoa()
 ** implementation. Function i2a() is a 'private' function
 ** called by the public itoa() function.
 **
 ** itoa() takes three arguments:
 ** 1) the integer to be converted,
 ** 2) a pointer to a character conversion buffer,
 ** 3) the radix for the conversion
 ** which can range between 2 and 36 inclusive
 ** range errors on the radix default it to base10
 */

static char *i2a(unsigned i, char *a, unsigned r)
{
	if (i/r > 0) a = i2a(i/r,a,r);
	*a = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i%r];
	return a+1;
}

char *itoa(int i, char *a, int r)
{
	if ((r < 2) || (r > 36)) r = 10;
	if (i<0) {
		*a = '-';
		*i2a(-(unsigned)i,a+1,r) = 0;
	} else *i2a(i,a,r) = 0;
	return a;
}

void OutputDebugString(LPCTSTR lpOuputString)
{
  CLog::Log(LOGDEBUG, "%s", lpOuputString);
}

void strlwr( char* string )
{
  while ( 0 != ( *string++ = (char)tolower( *string ) ) )
		;
}

void strupr( char* string )
{
  while ( 0 != ( *string++ = (char)toupper( *string ) ) )
		;
}

LONGLONG Int32x32To64(LONG Multiplier, LONG Multiplicand)
{
	LONGLONG result = Multiplier;
	result *= Multiplicand;
	return result;
}

int WideCharToMultiByte(
  UINT CodePage, 
  DWORD dwFlags, 
  LPCWSTR lpWideCharStr,
  int cchWideChar, 
  LPSTR lpMultiByteStr, 
  int cbMultiByte,
  LPCSTR lpDefaultChar,    
  LPBOOL lpUsedDefaultChar
) {

#warning need to implement WideCharToMultiByte
   return 0;
}

int MultiByteToWideChar(
  UINT CodePage, 
  DWORD dwFlags,         
  LPCSTR lpMultiByteStr, 
  int cbMultiByte,       
  LPWSTR lpWideCharStr,  
  int cchWideChar        
) {

#warning need to implement MultiByteToWideChar
   return 0;

}

DWORD GetLastError()
{
  return errno;
}

VOID SetLastError(DWORD dwErrCode)
{
  errno = dwErrCode;
}

#endif
