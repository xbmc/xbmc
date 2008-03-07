#ifndef __X_FILE_UTILS_
#define __X_FILE_UTILS_

#include "PlatformDefs.h"
#include "XHandle.h"

#ifdef _LINUX
#define XBMC_FILE_SEP '/'
#else
#define XBMC_FILE_SEP '\\'
#endif

HANDLE FindFirstFile(LPCSTR,LPWIN32_FIND_DATA);

BOOL   FindNextFile(HANDLE,LPWIN32_FIND_DATA);
BOOL   FindClose(HANDLE hFindFile);

#define CreateFileA CreateFile
HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
						LPSECURITY_ATTRIBUTES lpSecurityAttributes,  DWORD dwCreationDisposition, 
						DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

BOOL   CopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists);
BOOL   DeleteFile(LPCSTR);
BOOL   MoveFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);

BOOL   WriteFile(HANDLE hFile, const void * lpBuffer, DWORD nNumberOfBytesToWrite,  LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped);
BOOL   ReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void* unsupportedlpOverlapped);
BOOL   FlushFileBuffers(HANDLE hFile);

BOOL   CreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL   RemoveDirectory(LPCTSTR lpPathName);
DWORD  GetCurrentDirectory(DWORD nBufferLength, LPSTR lpBuffer);

DWORD  SetFilePointer(HANDLE hFile, int32_t lDistanceToMove,
                      int32_t *lpDistanceToMoveHigh, DWORD dwMoveMethod);
BOOL   SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove,PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
BOOL   SetEndOfFile(HANDLE hFile);

DWORD SleepEx( DWORD dwMilliseconds,  BOOL bAlertable);
DWORD GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation );
DWORD  GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
BOOL   GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
int    _stat64(const char *path, struct __stat64 *buffer);
int    _stati64(const char *path,struct _stati64 *buffer);

DWORD  GetFileAttributes(LPCTSTR lpFileName);

#define ERROR_ALREADY_EXISTS EEXIST

// uses statfs
BOOL GetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,
  PULARGE_INTEGER lpFreeBytesAvailable,
  PULARGE_INTEGER lpTotalNumberOfBytes,
  PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

#endif
