//
//  LaunchHost.cpp
//  Plex
//
//  Created by Max Feingold on 10/27/2011.
//  Copyright 2011 Plex Inc. All rights reserved.
//

#include "LaunchHost.h"

#ifdef _WIN32
#include "MediaCenterLaunchHost.h"

// This should be relocated to a utility module somewhere...
static bool ComparePaths(LPCWSTR szPath1, LPCWSTR szPath2) 
{
  bool fSame = false;

  HANDLE handle1 = CreateFileW(szPath1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  HANDLE handle2 = CreateFileW(szPath2, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);

  if (handle1 != INVALID_HANDLE_VALUE && handle2 != INVALID_HANDLE_VALUE)
  {
    BY_HANDLE_FILE_INFORMATION fileInfo1; 
    BY_HANDLE_FILE_INFORMATION fileInfo2;

    if (GetFileInformationByHandle(handle1, &fileInfo1) && GetFileInformationByHandle(handle2, &fileInfo2))
    {
      // Same file (fileindex) on the same volume (volume serial number)
      fSame = fileInfo1.dwVolumeSerialNumber == fileInfo2.dwVolumeSerialNumber &&
              fileInfo1.nFileIndexHigh == fileInfo2.nFileIndexHigh &&
              fileInfo1.nFileIndexLow == fileInfo2.nFileIndexLow;
    }
  }

  if (handle1 != INVALID_HANDLE_VALUE)
  {
    CloseHandle(handle1);
  }

  if (handle2 != INVALID_HANDLE_VALUE)
  {
    CloseHandle(handle2);
  }

  return fSame;
}
#endif

LaunchHost* DetectLaunchHost()
{
#ifdef _WIN32
  DWORD ch = GetCurrentDirectoryW(0, NULL);
  if (ch)
  {
    LPWSTR pwszWorkingDir = (LPWSTR)_alloca(ch * sizeof(WCHAR));
    if (GetCurrentDirectoryW(ch, pwszWorkingDir))
    {
      ch = GetWindowsDirectoryW(NULL, 0);
      if (ch)
      {
        ch += 1 + 10;
        LPWSTR pwszeHomeDirectory = (LPWSTR)_alloca(ch * sizeof(WCHAR));
        if (GetWindowsDirectoryW(pwszeHomeDirectory, ch))
        {
          wcscat(pwszeHomeDirectory, L"\\eHome");
          if (ComparePaths(pwszWorkingDir, pwszeHomeDirectory))
          {
            return new MediaCenterLaunchHost();
          }
        }
      }
    }
  }

#endif

  return 0;
}