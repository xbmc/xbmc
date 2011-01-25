#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "URL.h"
#include "settings/GUISettings.h"
#if _MSC_VER > 1400
#include "Cfgmgr32.h"
#endif
#include "MediaSource.h"
#include "utils/Stopwatch.h"

enum Drive_Types
{
  ALL_DRIVES = 0,
  LOCAL_DRIVES,
  REMOVABLE_DRIVES,
  DVD_DRIVES
};


class CWIN32Util
{
public:
  CWIN32Util(void);
  virtual ~CWIN32Util(void);

  static CStdString URLEncode(const CURL &url);
  static CStdString GetLocalPath(const CStdString &strPath);
  static char FirstDriveFromMask (ULONG unitmask);
  static int GetDriveStatus(const CStdString &strPath);
  static bool PowerManagement(PowerState State);
  static bool XBMCShellExecute(const CStdString &strPath, bool bWaitForScriptExit=false);
  static std::vector<CStdString> GetDiskUsage();
  static CStdString GetResInfoString();
  static int GetDesktopColorDepth();
  static CStdString GetSpecialFolder(int csidl);
  static CStdString CWIN32Util::GetSystemPath();
  static CStdString GetProfilePath();
  static CStdString UncToSmb(const CStdString &strPath);
  static void ExtendDllPath();
  static HRESULT ToggleTray(const char cDriveLetter='\0');
  static HRESULT EjectTray(const char cDriveLetter='\0');
  static HRESULT CloseTray(const char cDriveLetter='\0');
  static bool EjectDrive(const char cDriveLetter='\0');
#ifdef HAS_GL
  static void CheckGLVersion();
  static bool HasGLDefaultDrivers();
  static bool HasReqGLVersion();
#endif
  static BOOL IsCurrentUserLocalAdministrator();
  static void GetDrivesByType(VECSOURCES &localDrives, Drive_Types eDriveType=ALL_DRIVES, bool bonlywithmedia=false);
  static bool IsAudioCD(const CStdString& strPath);
  static CStdString GetDiskLabel(const CStdString& strPath);

  static bool Is64Bit();
  static LONG UtilRegGetValue( const HKEY hKey, const char *const pcKey, DWORD *const pdwType, char **const ppcBuffer, DWORD *const pdwSizeBuff, const DWORD dwSizeAdd );
  static bool UtilRegOpenKeyEx( const HKEY hKeyParent, const char *const pcKey, const REGSAM rsAccessRights, HKEY *hKey, const bool bReadX64= false );

  static bool GetCrystalHDLibraryPath(CStdString &strPath);
  
private:
#if _MSC_VER > 1400
  static DEVINST GetDrivesDevInstByDiskNumber(long DiskNumber);
#endif
};


class CWinIdleTimer : public CStopWatch
{
public:
  void StartZero();
};