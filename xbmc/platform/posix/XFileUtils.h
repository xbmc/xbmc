/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "XHandlePublic.h"

#include "PlatformDefs.h"

int WriteFile(HANDLE hFile,
              const void* lpBuffer,
              uint32_t nNumberOfBytesToWrite,
              unsigned int* lpNumberOfBytesWritten,
              void* lpOverlapped);
int ReadFile(HANDLE hFile,
             void* lpBuffer,
             uint32_t nNumberOfBytesToRead,
             unsigned int* lpNumberOfBytesRead,
             void* unsupportedlpOverlapped);

uint32_t SetFilePointer(HANDLE hFile,
                        int32_t lDistanceToMove,
                        int32_t* lpDistanceToMoveHigh,
                        uint32_t dwMoveMethod);
int SetFilePointerEx(HANDLE hFile,
                     LARGE_INTEGER liDistanceToMove,
                     PLARGE_INTEGER lpNewFilePointer,
                     uint32_t dwMoveMethod);

int _stat64(const char *path, struct __stat64 *buffer);
int _fstat64(int fd, struct __stat64 *buffer);
