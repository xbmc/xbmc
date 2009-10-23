#ifndef XWINAPI_H__
#define XWINAPI_H__

#include "PlatformDefs.h"

LPTSTR GetCommandLine();
DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer);
BOOL SetCurrentDirectory(LPCTSTR lpPathName);
DWORD GetLastError();
#endif // XWINAPI_H__

