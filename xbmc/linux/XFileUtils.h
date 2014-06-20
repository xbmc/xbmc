#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlatformDefs.h"
#include "XHandlePublic.h"

#define CreateFileA CreateFile
HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
            LPSECURITY_ATTRIBUTES lpSecurityAttributes,  DWORD dwCreationDisposition,
            DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

BOOL   WriteFile(HANDLE hFile, const void * lpBuffer, DWORD nNumberOfBytesToWrite,  LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped);
BOOL   ReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void* unsupportedlpOverlapped);

DWORD  SetFilePointer(HANDLE hFile, int32_t lDistanceToMove,
                      int32_t *lpDistanceToMoveHigh, DWORD dwMoveMethod);
BOOL   SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove,PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);

DWORD GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation );
int    _stat64(const char *path, struct __stat64 *buffer);
int _fstat64(int fd, struct __stat64 *buffer);

// uses statfs
BOOL GetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,
  PULARGE_INTEGER lpFreeBytesAvailable,
  PULARGE_INTEGER lpTotalNumberOfBytes,
  PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

