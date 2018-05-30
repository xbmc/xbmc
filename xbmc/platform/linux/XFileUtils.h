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

#pragma once

#include "PlatformDefs.h"
#include "XHandlePublic.h"

int WriteFile(HANDLE hFile, const void * lpBuffer, DWORD nNumberOfBytesToWrite,  unsigned int* lpNumberOfBytesWritten, void* lpOverlapped);
int ReadFile( HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead, unsigned int* lpNumberOfBytesRead, void* unsupportedlpOverlapped);

uint32_t SetFilePointer(HANDLE hFile, int32_t lDistanceToMove,
                      int32_t *lpDistanceToMoveHigh, DWORD dwMoveMethod);
int SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove,PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);

uint32_t GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation );
int _stat64(const char *path, struct __stat64 *buffer);
int _fstat64(int fd, struct __stat64 *buffer);
