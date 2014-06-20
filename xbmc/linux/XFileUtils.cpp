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

#include "system.h"
#include "PlatformInclude.h"
#include "XFileUtils.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"

#ifdef TARGET_POSIX
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

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess,
  DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
  // Fail on unsupported items
  if (lpSecurityAttributes != NULL )
  {
    CLog::Log(LOGERROR, "CreateFile does not support security attributes");
    return INVALID_HANDLE_VALUE;
  }

  if (hTemplateFile != (HANDLE) 0)
  {
    CLog::Log(LOGERROR, "CreateFile does not support template file");
    return INVALID_HANDLE_VALUE;
  }

  int flags = 0, mode=S_IRUSR | S_IRGRP | S_IROTH;
  if (dwDesiredAccess & FILE_WRITE_DATA)
  {
    flags = O_RDWR;
    mode |= S_IWUSR;
  }
  else if ( (dwDesiredAccess & FILE_READ_DATA) == FILE_READ_DATA)
    flags = O_RDONLY;
  else
  {
    CLog::Log(LOGERROR, "CreateFile does not permit access other than read and/or write");
    return INVALID_HANDLE_VALUE;
  }

  switch (dwCreationDisposition)
  {
    case OPEN_ALWAYS:
      flags |= O_CREAT;
      break;
    case TRUNCATE_EXISTING:
      flags |= O_TRUNC;
      mode  |= S_IWUSR;
      break;
    case CREATE_ALWAYS:
      flags |= O_CREAT|O_TRUNC;
      mode  |= S_IWUSR;
      break;
    case CREATE_NEW:
      flags |= O_CREAT|O_TRUNC|O_EXCL;
      mode  |= S_IWUSR;
      break;
    case OPEN_EXISTING:
      break;
  }

  int fd = 0;

  if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
    flags |= O_SYNC;

  // we always open files with fileflag O_NONBLOCK to support
  // cdrom devices, but we then turn it of for actual reads
  // apperently it's used for multiple things, read mode
  // and how opens are handled. devices must be opened
  // with this flag set to work correctly
  flags |= O_NONBLOCK;

  std::string strResultFile(lpFileName);

  fd = open(lpFileName, flags, mode);

  // Important to check reason for fail. Only if its
  // "file does not exist" shall we try to find the file
  if (fd == -1 && errno == ENOENT)
  {
    // Failed to open file. maybe due to case sensitivity.
    // Try opening the same name in lower case.
    std::string igFileName = CSpecialProtocol::TranslatePathConvertCase(lpFileName);
    fd = open(igFileName.c_str(), flags, mode);
    if (fd != -1)
    {
      CLog::Log(LOGWARNING,"%s, successfuly opened <%s> instead of <%s>", __FUNCTION__, igFileName.c_str(), lpFileName);
      strResultFile = igFileName;
    }
  }

  if (fd == -1)
  {
    if (errno == 20)
      CLog::Log(LOGWARNING,"%s, error %d opening file <%s>, flags:%x, mode:%x. ", __FUNCTION__, errno, lpFileName, flags, mode);
    return INVALID_HANDLE_VALUE;
  }

  // turn of nonblocking reads/writes as we don't
  // support this anyway currently
  fcntl(fd, F_GETFL, &flags);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

  HANDLE result = new CXHandle(CXHandle::HND_FILE);
  result->fd = fd;

#if (defined(TARGET_LINUX) || defined(TARGET_FREEBSD)) && defined(HAS_DVD_DRIVE) 
  // special case for opening the cdrom device
  if (strcmp(lpFileName, MEDIA_DETECT::CLibcdio::GetInstance()->GetDeviceFileName())==0)
    result->m_bCDROM = true;
  else
#endif
    result->m_bCDROM = false;

  // if FILE_FLAG_DELETE_ON_CLOSE then "unlink" the file (delete)
  // the file will be deleted when the last open descriptor is closed.
  if (dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE)
    unlink(strResultFile.c_str());

  return result;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
  LPDWORD lpNumberOfBytesRead, LPVOID lpOverlapped)
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

BOOL WriteFile(HANDLE hFile, const void * lpBuffer, DWORD nNumberOfBytesToWrite,
  LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped)
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

DWORD  SetFilePointer(HANDLE hFile, int32_t lDistanceToMove,
                      int32_t *lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
  if (hFile == NULL)
    return 0;

  LONGLONG offset = lDistanceToMove;
  if (lpDistanceToMoveHigh)
  {
    LONGLONG helper = *lpDistanceToMoveHigh;
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

// uses statfs
BOOL GetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,
  PULARGE_INTEGER lpFreeBytesAvailable,
  PULARGE_INTEGER lpTotalNumberOfBytes,
  PULARGE_INTEGER lpTotalNumberOfFreeBytes
  )

{
#if defined(TARGET_ANDROID) || defined(TARGET_DARWIN)
  struct statfs fsInfo;
  // is 64-bit on android and darwin (10.6SDK + any iOS)
  if (statfs(CSpecialProtocol::TranslatePath(lpDirectoryName).c_str(), &fsInfo) != 0)
    return false;
#else
  struct statfs64 fsInfo;
  if (statfs64(CSpecialProtocol::TranslatePath(lpDirectoryName).c_str(), &fsInfo) != 0)
    return false;
#endif

  if (lpFreeBytesAvailable)
    lpFreeBytesAvailable->QuadPart =  (ULONGLONG)fsInfo.f_bavail * (ULONGLONG)fsInfo.f_bsize;

  if (lpTotalNumberOfBytes)
    lpTotalNumberOfBytes->QuadPart = (ULONGLONG)fsInfo.f_blocks * (ULONGLONG)fsInfo.f_bsize;

  if (lpTotalNumberOfFreeBytes)
    lpTotalNumberOfFreeBytes->QuadPart = (ULONGLONG)fsInfo.f_bfree * (ULONGLONG)fsInfo.f_bsize;

  return true;
}

DWORD GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation )
{
  if (lpTimeZoneInformation == NULL)
    return TIME_ZONE_ID_INVALID;

  memset(lpTimeZoneInformation, 0, sizeof(TIME_ZONE_INFORMATION));

  struct tm t;
  time_t tt = time(NULL);
  if(localtime_r(&tt, &t))
    lpTimeZoneInformation->Bias = -t.tm_gmtoff / 60;

  swprintf(lpTimeZoneInformation->StandardName, 31, L"%s", tzname[0]);
  swprintf(lpTimeZoneInformation->DaylightName, 31, L"%s", tzname[1]);

  return TIME_ZONE_ID_UNKNOWN;
}

BOOL SetFilePointerEx(  HANDLE hFile,
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

  return true;
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
#endif

