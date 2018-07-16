/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
