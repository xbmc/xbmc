/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDefs.h"
#include "XFileUtils.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"

#include "XHandle.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#if !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD) && !defined(TARGET_ANDROID)
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <dirent.h>
#include <errno.h>

#if defined(TARGET_ANDROID)
#include <sys/file.h>
#include <sys/statfs.h>

/* from android header: note: this corresponds to the kernel's statfs64 type */
//typedef struct statfs statfs64;
#endif

#include "storage/cdioSupport.h"

#include "utils/log.h"

int ReadFile(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead,
  unsigned int* lpNumberOfBytesRead, void* lpOverlapped)
{
  if (lpOverlapped)
  {
    CLog::Log(LOGERROR, "ReadFile does not support overlapped I/O");
    return 0;
  }

  size_t bytesRead = read(hFile->fd, lpBuffer, nNumberOfBytesToRead);
  if (bytesRead == (size_t) -1)
    return 0;

  if (lpNumberOfBytesRead)
    *lpNumberOfBytesRead = bytesRead;

  return 1;
}

int WriteFile(HANDLE hFile, const void * lpBuffer, DWORD nNumberOfBytesToWrite,
  unsigned int* lpNumberOfBytesWritten, void* lpOverlapped)
{
  if (lpOverlapped)
  {
    CLog::Log(LOGERROR, "ReadFile does not support overlapped I/O");
    return 0;
  }

  size_t bytesWritten = write(hFile->fd, lpBuffer, nNumberOfBytesToWrite);

  if (bytesWritten == (size_t) -1)
    return 0;

  *lpNumberOfBytesWritten = bytesWritten;

  return 1;
}

uint32_t SetFilePointer(HANDLE hFile, int32_t lDistanceToMove,
                      int32_t *lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
  if (hFile == NULL)
    return 0;

  long long offset = lDistanceToMove;
  if (lpDistanceToMoveHigh)
  {
    long long helper = *lpDistanceToMoveHigh;
    helper <<= 32;
    offset &= 0xFFFFFFFF;   // Zero out the upper half (sign ext)
    offset |= helper;
  }

  int nMode = SEEK_SET;
  if (dwMoveMethod == FILE_CURRENT)
    nMode = SEEK_CUR;
  else if (dwMoveMethod == FILE_END)
    nMode = SEEK_END;

  off64_t currOff;
#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
  currOff = lseek(hFile->fd, offset, nMode);
#else
  currOff = lseek64(hFile->fd, offset, nMode);
#endif

  if (lpDistanceToMoveHigh)
  {
    *lpDistanceToMoveHigh = (int32_t)(currOff >> 32);
  }

  return (DWORD)currOff;
}

int SetFilePointerEx(  HANDLE hFile,
            LARGE_INTEGER liDistanceToMove,
            PLARGE_INTEGER lpNewFilePointer,
            DWORD dwMoveMethod )
{

  int nMode = SEEK_SET;
  if (dwMoveMethod == FILE_CURRENT)
    nMode = SEEK_CUR;
  else if (dwMoveMethod == FILE_END)
    nMode = SEEK_END;

  off64_t toMove = liDistanceToMove.QuadPart;

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
  off64_t currOff = lseek(hFile->fd, toMove, nMode);
#else
  off64_t currOff = lseek64(hFile->fd, toMove, nMode);
#endif

  if (lpNewFilePointer)
    lpNewFilePointer->QuadPart = currOff;

  return 1;
}

int _fstat64(int fd, struct __stat64 *buffer)
{
  if (buffer == NULL)
    return -1;

  return fstat64(fd, buffer);
}

int _stat64(   const char *path,   struct __stat64 *buffer )
{

  if (buffer == NULL || path == NULL)
    return -1;

  return stat64(path, buffer);
}
