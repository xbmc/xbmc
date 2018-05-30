/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vector>

#include "URL.h"
#include "MediaSource.h"
#include "utils/Geometry.h"

#define BONJOUR_EVENT             ( WM_USER + 0x100 )	// Message sent to the Window when a Bonjour event occurs.
#define BONJOUR_BROWSER_EVENT     ( WM_USER + 0x110 )

class CURL; // forward declaration

class CWIN32Util
{
public:
  CWIN32Util(void);
  virtual ~CWIN32Util(void);

  static char FirstDriveFromMask (ULONG unitmask);
  static int GetDriveStatus(const std::string &strPath, bool bStatusEx=false);
  static bool XBMCShellExecute(const std::string &strPath, bool bWaitForScriptExit=false);
  static std::string GetResInfoString();
  static int GetDesktopColorDepth();

  static std::string GetSystemPath();
  static std::string GetProfilePath();
  static std::string UncToSmb(const std::string &strPath);
  static std::string SmbToUnc(const std::string &strPath);
  static bool AddExtraLongPathPrefix(std::wstring& path);
  static bool RemoveExtraLongPathPrefix(std::wstring& path);
  static std::wstring ConvertPathToWin32Form(const std::string& pathUtf8);
  static std::wstring ConvertPathToWin32Form(const CURL& url);
  static inline __time64_t fileTimeToTimeT(const __int64 ftimei64)
  {
    // FILETIME is 100-nanoseconds from 00:00:00 UTC 01 Jan 1601
    // __time64_t is seconds from 00:00:00 UTC 01 Jan 1970
    return (ftimei64 - 116444736000000000) / 10000000;
  }
  static __time64_t fileTimeToTimeT(const FILETIME& ftimeft);
  static __time64_t fileTimeToTimeT(const LARGE_INTEGER& ftimeli);
  static HRESULT ToggleTray(const char cDriveLetter='\0');
  static HRESULT EjectTray(const char cDriveLetter='\0');
  static HRESULT CloseTray(const char cDriveLetter='\0');
  static BOOL IsCurrentUserLocalAdministrator();

#ifdef TARGET_WINDOWS_DESKTOP
  static std::string GetSpecialFolder(int csidl);
  static LONG UtilRegGetValue( const HKEY hKey, const char *const pcKey, DWORD *const pdwType, char **const ppcBuffer, DWORD *const pdwSizeBuff, const DWORD dwSizeAdd );
  static bool UtilRegOpenKeyEx( const HKEY hKeyParent, const char *const pcKey, const REGSAM rsAccessRights, HKEY *hKey, const bool bReadX64= false );
  static bool GetFocussedProcess(std::string &strProcessFile);
#endif // TARGET_WINDOWS_DESKTOP
  static void CropSource(CRect& src, CRect& dst, CRect target, UINT rotation = 0);

  static bool IsUsbDevice(const std::wstring &strWdrive);

  static std::string WUSysMsg(DWORD dwError);
  static bool SetThreadLocalLocale(bool enable = true);
};
