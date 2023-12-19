/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WIN32Util.h"

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "WindowHelper.h"
#include "guilib/LocalizeStrings.h"
#include "my_ntddscsi.h"
#include "rendering/dx/DirectXHelper.h"
#include "storage/MediaManager.h"
#include "storage/cdioSupport.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <PowrProf.h>

#ifdef TARGET_WINDOWS_DESKTOP
#include <cassert>
#endif
#include <array>
#include <locale.h>

#include <shellapi.h>
#include <shlobj.h>
#include <winioctl.h>

#ifdef TARGET_WINDOWS_DESKTOP
extern HWND g_hWnd;
#endif

using namespace MEDIA_DETECT;

#ifdef TARGET_WINDOWS_STORE
#include "platform/win10/AsyncHelpers.h"

#include <ppltasks.h>
#include <winrt/Windows.Devices.Display.Core.h>
#include <winrt/Windows.Devices.Power.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Display.Core.h>
#include <winrt/Windows.Storage.h>

using namespace winrt::Windows::Devices::Power;
using namespace winrt::Windows::Devices::Display::Core;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::Graphics::Display::Core;
using namespace winrt::Windows::Storage;
#endif

CWIN32Util::CWIN32Util(void)
{
}

CWIN32Util::~CWIN32Util(void)
{
}

int CWIN32Util::GetDriveStatus(const std::string &strPath, bool bStatusEx)
{
#ifdef TARGET_WINDOWS_STORE
  CLog::LogF(LOGDEBUG, "is not implemented");
  CLog::LogF(LOGDEBUG, "Could not determine tray status {}", GetLastError());
  return -1;
#else
  using KODI::PLATFORM::WINDOWS::ToW;

  auto strPathW = ToW(strPath);
  HANDLE hDevice;               // handle to the drive to be examined
  int iResult;                  // results flag
  ULONG ulChanges=0;
  DWORD dwBytesReturned;
  T_SPDT_SBUF sptd_sb = {}; // SCSI Pass Through Direct variable.
  byte DataBuf[8] = {}; // Buffer for holding data to/from drive.

  CLog::LogF(LOGDEBUG, "Requesting status for drive {}.", strPath);

  hDevice = CreateFile( strPathW.c_str(),                  // drive
                        0,                                // no access to the drive
                        FILE_SHARE_READ,                  // share mode
                        NULL,                             // default security attributes
                        OPEN_EXISTING,                    // disposition
                        FILE_ATTRIBUTE_READONLY,          // file attributes
                        NULL);

  if (hDevice == INVALID_HANDLE_VALUE)                    // cannot open the drive
  {
    CLog::LogF(LOGERROR, "Failed to CreateFile for {}.", strPath);
    return -1;
  }

  CLog::LogF(LOGDEBUG, "Requesting media status for drive {}.", strPath);
  iResult = DeviceIoControl((HANDLE) hDevice,             // handle to device
                             IOCTL_STORAGE_CHECK_VERIFY2, // dwIoControlCode
                             NULL,                        // lpInBuffer
                             0,                           // nInBufferSize
                             &ulChanges,                  // lpOutBuffer
                             sizeof(ULONG),               // nOutBufferSize
                             &dwBytesReturned ,           // number of bytes returned
                             NULL );                      // OVERLAPPED structure

  CloseHandle(hDevice);

  if(iResult == 1)
    return 2;

  // don't request the tray status as we often doesn't need it
  if(!bStatusEx)
    return 0;

  hDevice = CreateFile( strPathW.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_READONLY,
                        NULL);

  if (hDevice == INVALID_HANDLE_VALUE)
  {
    CLog::LogF(LOGERROR, "Failed to CreateFile2 for {}.", strPath);
    return -1;
  }

  sptd_sb.sptd.Length=sizeof(SCSI_PASS_THROUGH_DIRECT);
  sptd_sb.sptd.PathId=0;
  sptd_sb.sptd.TargetId=0;
  sptd_sb.sptd.Lun=0;
  sptd_sb.sptd.CdbLength=10;
  sptd_sb.sptd.SenseInfoLength=MAX_SENSE_LEN;
  sptd_sb.sptd.DataIn=SCSI_IOCTL_DATA_IN;
  sptd_sb.sptd.DataTransferLength=sizeof(DataBuf);
  sptd_sb.sptd.TimeOutValue=2;
  sptd_sb.sptd.DataBuffer=(PVOID)&(DataBuf);
  sptd_sb.sptd.SenseInfoOffset=sizeof(SCSI_PASS_THROUGH_DIRECT);

  sptd_sb.sptd.Cdb[0]=0x4a;
  sptd_sb.sptd.Cdb[1]=1;
  sptd_sb.sptd.Cdb[2]=0;
  sptd_sb.sptd.Cdb[3]=0;
  sptd_sb.sptd.Cdb[4]=0x10;
  sptd_sb.sptd.Cdb[5]=0;
  sptd_sb.sptd.Cdb[6]=0;
  sptd_sb.sptd.Cdb[7]=0;
  sptd_sb.sptd.Cdb[8]=8;
  sptd_sb.sptd.Cdb[9]=0;
  sptd_sb.sptd.Cdb[10]=0;
  sptd_sb.sptd.Cdb[11]=0;
  sptd_sb.sptd.Cdb[12]=0;
  sptd_sb.sptd.Cdb[13]=0;
  sptd_sb.sptd.Cdb[14]=0;
  sptd_sb.sptd.Cdb[15]=0;

  //Send the command to drive
  CLog::LogF(LOGDEBUG, "Requesting tray status for drive {}.", strPath);
  iResult = DeviceIoControl((HANDLE) hDevice,
                            IOCTL_SCSI_PASS_THROUGH_DIRECT,
                            (PVOID)&sptd_sb, (DWORD)sizeof(sptd_sb),
                            (PVOID)&sptd_sb, (DWORD)sizeof(sptd_sb),
                            &dwBytesReturned,
                            NULL);

  CloseHandle(hDevice);

  if(iResult)
  {

    if(DataBuf[5] == 0) // tray close
      return 0;
    else if(DataBuf[5] == 1) // tray open
      return 1;
    else
      return 2; // tray closed, media present
  }
  CLog::LogF(LOGERROR, "Could not determine tray status {}", GetLastError());
  return -1;
#endif
}

char CWIN32Util::FirstDriveFromMask (ULONG unitmask)
{
    char i;
    for (i = 0; i < 26; ++i)
    {
        if (unitmask & 0x1) break;
        unitmask = unitmask >> 1;
    }
    return (i + 'A');
}

bool CWIN32Util::XBMCShellExecute(const std::string &strPath, bool bWaitForScriptExit)
{
#ifdef TARGET_WINDOWS_STORE
  CLog::LogF(LOGDEBUG, "s not implemented");
  return false;
#else
  std::string strCommand = strPath;
  std::string strExe = strPath;
  std::string strParams;
  std::string strWorkingDir;

  StringUtils::Trim(strCommand);
  if (strCommand.empty())
  {
    return false;
  }
  size_t iIndex = std::string::npos;
  char split = ' ';
  if (strCommand[0] == '\"')
  {
    split = '\"';
  }
  iIndex = strCommand.find(split, 1);
  if (iIndex != std::string::npos)
  {
    strExe = strCommand.substr(0, iIndex + 1);
    strParams = strCommand.substr(iIndex + 1);
  }

  StringUtils::Replace(strExe, "\"", "");

  strWorkingDir = strExe;
  iIndex = strWorkingDir.rfind('\\');
  if(iIndex != std::string::npos)
  {
    strWorkingDir[iIndex+1] = '\0';
  }

  std::wstring WstrExe, WstrParams, WstrWorkingDir;
  g_charsetConverter.utf8ToW(strExe, WstrExe);
  g_charsetConverter.utf8ToW(strParams, WstrParams);
  g_charsetConverter.utf8ToW(strWorkingDir, WstrWorkingDir);

  bool ret;
  SHELLEXECUTEINFOW ShExecInfo = {};
  ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
  ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ShExecInfo.hwnd = NULL;
  ShExecInfo.lpVerb = NULL;
  ShExecInfo.lpFile = WstrExe.c_str();
  ShExecInfo.lpParameters = WstrParams.c_str();
  ShExecInfo.lpDirectory = WstrWorkingDir.c_str();
  ShExecInfo.nShow = SW_SHOW;
  ShExecInfo.hInstApp = NULL;

  g_windowHelper.StopThread();

  LockSetForegroundWindow(LSFW_UNLOCK);
  ShowWindow(g_hWnd,SW_MINIMIZE);
  ret = ShellExecuteExW(&ShExecInfo) == TRUE;
  g_windowHelper.SetHANDLE(ShExecInfo.hProcess);

  // ShellExecute doesn't return the window of the started process
  // we need to gather it from somewhere to allow switch back to XBMC
  // when a program is minimized instead of stopped.
  //g_windowHelper.SetHWND(ShExecInfo.hwnd);
  g_windowHelper.Create();

  if(bWaitForScriptExit)
  {
    //! @todo Pause music and video playback
    WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
  }

  return ret;
#endif
}

std::string CWIN32Util::GetResInfoString()
{
#ifdef TARGET_WINDOWS_STORE
  auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
  if (hdmiInfo) // Xbox
  {
    auto mode = hdmiInfo.GetCurrentDisplayMode();
    return StringUtils::Format(
        "Desktop Resolution: {}x{} {}Bit at {:.2f}Hz", mode.ResolutionWidthInRawPixels(),
        mode.ResolutionHeightInRawPixels(), mode.BitsPerPixel(), mode.RefreshRate());
  }
  else // Windows 10 UWP
  {
    auto info = DisplayInformation::GetForCurrentView();
    return StringUtils::Format("Desktop Resolution: {}x{}", info.ScreenWidthInRawPixels(),
                               info.ScreenHeightInRawPixels());
  }
#else
  DEVMODE devmode = {};
  devmode.dmSize = sizeof(devmode);
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
  return StringUtils::Format("Desktop Resolution: {}x{} {}Bit at {}Hz", devmode.dmPelsWidth,
                             devmode.dmPelsHeight, devmode.dmBitsPerPel,
                             devmode.dmDisplayFrequency);
#endif
}

int CWIN32Util::GetDesktopColorDepth()
{
#ifdef TARGET_WINDOWS_STORE
  CLog::LogF(LOGDEBUG, "s not implemented");
  return 32;
#else
  DEVMODE devmode = {};
  devmode.dmSize = sizeof(devmode);
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
  return (int)devmode.dmBitsPerPel;
#endif
}

size_t CWIN32Util::GetSystemMemorySize()
{
#ifdef TARGET_WINDOWS_STORE
  MEMORYSTATUSEX statex = {};
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  return static_cast<size_t>(statex.ullTotalPhys / KB);
#else
  ULONGLONG ramSize = 0;
  GetPhysicallyInstalledSystemMemory(&ramSize);
  return static_cast<size_t>(ramSize);
#endif
}

#ifdef TARGET_WINDOWS_DESKTOP
std::string CWIN32Util::GetSpecialFolder(int csidl)
{
  std::string strProfilePath;
  static const int bufSize = MAX_PATH;
  WCHAR* buf = new WCHAR[bufSize];

  if(SUCCEEDED(SHGetFolderPathW(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, buf)))
  {
    buf[bufSize-1] = 0;
    g_charsetConverter.wToUTF8(buf, strProfilePath);
    strProfilePath = UncToSmb(strProfilePath);
  }
  else
    strProfilePath = "";

  delete[] buf;
  return strProfilePath;
}
#endif

std::string CWIN32Util::GetSystemPath()
{
#ifdef TARGET_WINDOWS_STORE
  // access to system folder is not allowed in a UWP app
  return "";
#else
  return GetSpecialFolder(CSIDL_SYSTEM);
#endif
}

std::string CWIN32Util::GetProfilePath(const bool platformDirectories)
{
  std::string strProfilePath;
#ifdef TARGET_WINDOWS_STORE
  auto localFolder = ApplicationData::Current().LocalFolder();
  strProfilePath = KODI::PLATFORM::WINDOWS::FromW(localFolder.Path().c_str());
#else
  std::string strHomePath = CUtil::GetHomePath();

  if (platformDirectories)
    strProfilePath = URIUtils::AddFileToFolder(GetSpecialFolder(CSIDL_APPDATA|CSIDL_FLAG_CREATE), CCompileInfo::GetAppName());
  else
    strProfilePath = URIUtils::AddFileToFolder(strHomePath , "portable_data");

  if (strProfilePath.length() == 0)
    strProfilePath = strHomePath;

  URIUtils::AddSlashAtEnd(strProfilePath);
#endif
  return strProfilePath;
}

std::string CWIN32Util::UncToSmb(const std::string &strPath)
{
  std::string strRetPath(strPath);
  if(StringUtils::StartsWith(strRetPath, "\\\\"))
  {
    strRetPath = "smb:" + strPath;
    StringUtils::Replace(strRetPath, '\\', '/');
  }
  return strRetPath;
}

std::string CWIN32Util::SmbToUnc(const std::string &strPath)
{
  std::string strRetPath(strPath);
  if(StringUtils::StartsWithNoCase(strRetPath, "smb://"))
  {
    StringUtils::Replace(strRetPath, "smb://", "\\\\");
    StringUtils::Replace(strRetPath, '/', '\\');
  }
  return strRetPath;
}

bool CWIN32Util::AddExtraLongPathPrefix(std::wstring& path)
{
  const wchar_t* const str = path.c_str();
  if (path.length() < 4 || str[0] != L'\\' || str[1] != L'\\' || str[3] != L'\\' || str[2] != L'?')
  {
    path.insert(0, L"\\\\?\\");
    return true;
  }
  return false;
}

bool CWIN32Util::RemoveExtraLongPathPrefix(std::wstring& path)
{
  const wchar_t* const str = path.c_str();
  if (path.length() >= 4 && str[0] == L'\\' && str[1] == L'\\' && str[3] == L'\\' && str[2] == L'?')
  {
    path.erase(0, 4);
    return true;
  }
  return false;
}

std::wstring CWIN32Util::ConvertPathToWin32Form(const std::string& pathUtf8)
{
  std::wstring result;
  if (pathUtf8.empty())
    return result;

  bool convertResult;

  if (pathUtf8.compare(0, 2, "\\\\", 2) != 0) // pathUtf8 don't start from "\\"
  { // assume local file path in form 'C:\Folder\File.ext'
    std::string formedPath("\\\\?\\"); // insert "\\?\" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8, '\\'), '\\'); // fix duplicated and forward slashes, resolve relative path
    convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true);
  }
  else if (pathUtf8.compare(0, 8, "\\\\?\\UNC\\", 8) == 0) // pathUtf8 starts from "\\?\UNC\"
  {
    std::string formedPath("\\\\?\\UNC"); // start from "\\?\UNC" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8.substr(7), '\\'), '\\'); // fix duplicated and forward slashes, resolve relative path, don't touch "\\?\UNC" prefix,
    convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true);
  }
  else if (pathUtf8.compare(0, 4, "\\\\?\\", 4) == 0) // pathUtf8 starts from "\\?\", but it's not UNC path
  {
    std::string formedPath("\\\\?"); // start from "\\?" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8.substr(3), '\\'), '\\'); // fix duplicated and forward slashes, resolve relative path, don't touch "\\?" prefix,
    convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true);
  }
  else // pathUtf8 starts from "\\", but not from "\\?\UNC\"
  { // assume UNC path in form '\\server\share\folder\file.ext'
    std::string formedPath("\\\\?\\UNC"); // append "\\?\UNC" prefix
    formedPath += URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(pathUtf8), '\\'); // fix duplicated and forward slashes, resolve relative path, transform "\\" prefix to single "\"
    convertResult = g_charsetConverter.utf8ToW(formedPath, result, false, false, true);
  }

  if (!convertResult)
  {
    CLog::Log(LOGERROR, "Error converting path \"{}\" to Win32 wide string!", pathUtf8);
    return L"";
  }

  return result;
}

std::wstring CWIN32Util::ConvertPathToWin32Form(const CURL& url)
{
  assert(url.GetProtocol().empty() || url.IsProtocol("smb"));

  if (url.GetFileName().empty())
    return std::wstring(); // empty string

  if (url.GetProtocol().empty())
  {
    std::wstring result;
    if (g_charsetConverter.utf8ToW("\\\\?\\" +
          URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(url.GetFileName(), '\\'), '\\'), result, false, false, true))
      return result;
  }
  else if (url.IsProtocol("smb"))
  {
    if (url.GetHostName().empty())
      return std::wstring(); // empty string

    std::wstring result;
    if (g_charsetConverter.utf8ToW("\\\\?\\UNC\\" +
          URIUtils::CanonicalizePath(URIUtils::FixSlashesAndDups(url.GetHostName() + '\\' + url.GetFileName(), '\\'), '\\'),
          result, false, false, true))
      return result;
  }
  else
    return std::wstring(); // unsupported protocol, return empty string

  CLog::LogF(LOGERROR, "Error converting path \"{}\" to Win32 form", url.Get());
  return std::wstring(); // empty string
}

__time64_t CWIN32Util::fileTimeToTimeT(const FILETIME& ftimeft)
{
  if (!ftimeft.dwHighDateTime && !ftimeft.dwLowDateTime)
    return 0;

  return fileTimeToTimeT((__int64(ftimeft.dwHighDateTime) << 32) + __int64(ftimeft.dwLowDateTime));
}

__time64_t CWIN32Util::fileTimeToTimeT(const LARGE_INTEGER& ftimeli)
{
  if (ftimeli.QuadPart == 0)
    return 0;

  return fileTimeToTimeT(__int64(ftimeli.QuadPart));
}

HRESULT CWIN32Util::ToggleTray(const char cDriveLetter)
{
#ifdef TARGET_WINDOWS_STORE
  CLog::LogF(LOGDEBUG, "s not implemented");
  return false;
#else
  using namespace KODI::PLATFORM::WINDOWS;
  BOOL bRet= FALSE;
  DWORD dwReq = 0;
  char cDL = cDriveLetter;
  if( !cDL )
  {
    std::string dvdDevice = CServiceBroker::GetMediaManager().TranslateDevicePath("");
    if(dvdDevice == "")
      return S_FALSE;
    cDL = dvdDevice[0];
  }

  auto strVolFormat = ToW(StringUtils::Format("\\\\.\\{}:", cDL));
  HANDLE hDrive= CreateFile( strVolFormat.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  auto strRootFormat = ToW(StringUtils::Format("{}:\\", cDL));
  if( ( hDrive != INVALID_HANDLE_VALUE || GetLastError() == NO_ERROR) &&
    ( GetDriveType( strRootFormat.c_str() ) == DRIVE_CDROM ) )
  {
    DWORD dwDummy;
    dwReq = (GetDriveStatus(FromW(strVolFormat), true) == 1) ? IOCTL_STORAGE_LOAD_MEDIA : IOCTL_STORAGE_EJECT_MEDIA;
    bRet = DeviceIoControl( hDrive, dwReq, NULL, 0, NULL, 0, &dwDummy, NULL);
  }
  // Windows doesn't seem to send always DBT_DEVICEREMOVECOMPLETE
  // unmount it here too as it won't hurt
  if(dwReq == IOCTL_STORAGE_EJECT_MEDIA && bRet == 1)
  {
    CMediaSource share;
    share.strPath = StringUtils::Format("{}:", cDL);
    share.strName = share.strPath;
    CServiceBroker::GetMediaManager().RemoveAutoSource(share);
  }
  CloseHandle(hDrive);
  return bRet? S_OK : S_FALSE;
#endif
}

HRESULT CWIN32Util::EjectTray(const char cDriveLetter)
{
  char cDL = cDriveLetter;
  if( !cDL )
  {
    std::string dvdDevice = CServiceBroker::GetMediaManager().TranslateDevicePath("");
    if(dvdDevice.empty())
      return S_FALSE;
    cDL = dvdDevice[0];
  }

  std::string strVolFormat = StringUtils::Format("\\\\.\\{}:", cDL);

  if(GetDriveStatus(strVolFormat, true) != 1)
    return ToggleTray(cDL);
  else
    return S_OK;
}

HRESULT CWIN32Util::CloseTray(const char cDriveLetter)
{
  char cDL = cDriveLetter;
  if( !cDL )
  {
    std::string dvdDevice = CServiceBroker::GetMediaManager().TranslateDevicePath("");
    if(dvdDevice.empty())
      return S_FALSE;
    cDL = dvdDevice[0];
  }

  std::string strVolFormat = StringUtils::Format("\\\\.\\{}:", cDL);

  if(GetDriveStatus(strVolFormat, true) == 1)
    return ToggleTray(cDL);
  else
    return S_OK;
}

BOOL CWIN32Util::IsCurrentUserLocalAdministrator()
{
#ifdef TARGET_WINDOWS_STORE
  // UWP apps never run as admin
  return false;
#else
  BOOL b;
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  b = AllocateAndInitializeSid(
      &NtAuthority,
      2,
      SECURITY_BUILTIN_DOMAIN_RID,
      DOMAIN_ALIAS_RID_ADMINS,
      0, 0, 0, 0, 0, 0,
      &AdministratorsGroup);
  if(b)
  {
    if (!CheckTokenMembership( NULL, AdministratorsGroup, &b))
    {
         b = FALSE;
    }
    FreeSid(AdministratorsGroup);
  }

  return(b);
#endif
}

extern "C"
{
  FILE *fopen_utf8(const char *_Filename, const char *_Mode)
  {
    std::string modetmp = _Mode;
    std::wstring wfilename, wmode(modetmp.begin(), modetmp.end());
    g_charsetConverter.utf8ToW(_Filename, wfilename, false);
    return _wfopen(wfilename.c_str(), wmode.c_str());
  }
}

extern "C" {
/*
 *  Copyright (c) 1997, 1998, 2005 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-4-Clause
 *  See LICENSES/README.md for more information.
 *
 *  Ported from NetBSD to Windows by Ron Koenderink, 2007
 *  $NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $
 *
 *  This code was contributed to The NetBSD Foundation by Klaus Klein.
 *  Heavily optimised by David Laight
 */

  #if defined(LIBC_SCCS) && !defined(lint)
  __RCSID("$NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $");
  #endif

  typedef unsigned char u_char;
  typedef unsigned int uint;
  #include <ctype.h>
  #include <locale.h>
  #include <string.h>
  #include <time.h>

  #ifdef __weak_alias
  __weak_alias(strptime,_strptime)
  #endif

  #define _ctloc(x)   (x)
  const char *abday[] = {
    "Sun", "Mon", "Tue", "Wed",
    "Thu", "Fri", "Sat"
  };
  const char *day[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };
  const char *abmon[] =  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  const char *mon[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
  const char *am_pm[] = {
    "AM", "PM"
  };
  const char* d_t_fmt = "%a %Ef %T %Y";
  const char* t_fmt_ampm = "%I:%M:%S %p";
  const char* t_fmt = "%H:%M:%S";
  const char* d_fmt = "%m/%d/%y";
#define TM_YEAR_BASE 1900
#define __UNCONST(x) ((char*)(((const char*)(x) - (const char*)0) + (char*)0))

/*
   * We do not implement alternate representations. However, we always
   * check whether a given modifier is allowed for a certain conversion.
   */
#define ALT_E 0x01
#define ALT_O 0x02
#define LEGAL_ALT(x) \
  { \
    if (alt_format & ~(x)) \
      return NULL; \
  }


  static const u_char *conv_num(const unsigned char *, int *, uint, uint);
  static const u_char *find_string(const u_char *, int *, const char * const *,
    const char * const *, int);


  char *
  strptime(const char *buf, const char *fmt, struct tm *tm)
  {
    unsigned char c;
    const unsigned char *bp;
    int alt_format, i, split_year = 0;
    const char *new_fmt;

    bp = (const u_char *)buf;

    while (bp != NULL && (c = *fmt++) != '\0') {
      /* Clear `alternate' modifier prior to new conversion. */
      alt_format = 0;
      i = 0;

      /* Eat up white-space. */
      if (isspace(c)) {
        while (isspace(*bp))
          bp++;
        continue;
      }

      if (c != '%')
        goto literal;


  again:    switch (c = *fmt++) {
      case '%':  /* "%%" is converted to "%". */
  literal:
        if (c != *bp++)
          return NULL;
        LEGAL_ALT(0);
        continue;

      /*
       * "Alternative" modifiers. Just set the appropriate flag
       * and start over again.
       */
      case 'E':  /* "%E?" alternative conversion modifier. */
        LEGAL_ALT(0);
        alt_format |= ALT_E;
        goto again;

      case 'O':  /* "%O?" alternative conversion modifier. */
        LEGAL_ALT(0);
        alt_format |= ALT_O;
        goto again;

      /*
       * "Complex" conversion rules, implemented through recursion.
       */
      case 'c':  /* Date and time, using the locale's format. */
        new_fmt = _ctloc(d_t_fmt);
        goto recurse;

      case 'D':  /* The date as "%m/%d/%y". */
        new_fmt = "%m/%d/%y";
        LEGAL_ALT(0);
        goto recurse;

      case 'R':  /* The time as "%H:%M". */
        new_fmt = "%H:%M";
        LEGAL_ALT(0);
        goto recurse;

      case 'r':  /* The time in 12-hour clock representation. */
        new_fmt =_ctloc(t_fmt_ampm);
        LEGAL_ALT(0);
        goto recurse;

      case 'T':  /* The time as "%H:%M:%S". */
        new_fmt = "%H:%M:%S";
        LEGAL_ALT(0);
        goto recurse;

      case 'X':  /* The time, using the locale's format. */
        new_fmt =_ctloc(t_fmt);
        goto recurse;

      case 'x':  /* The date, using the locale's format. */
        new_fmt =_ctloc(d_fmt);
          recurse:
        bp = (const u_char *)strptime((const char *)bp,
                    new_fmt, tm);
        LEGAL_ALT(ALT_E);
        continue;

      /*
       * "Elementary" conversion rules.
       */
      case 'A':  /* The day of week, using the locale's form. */
      case 'a':
        bp = find_string(bp, &tm->tm_wday, _ctloc(day),
            _ctloc(abday), 7);
        LEGAL_ALT(0);
        continue;

      case 'B':  /* The month, using the locale's form. */
      case 'b':
      case 'h':
        bp = find_string(bp, &tm->tm_mon, _ctloc(mon),
            _ctloc(abmon), 12);
        LEGAL_ALT(0);
        continue;

      case 'C':  /* The century number. */
        i = 20;
        bp = conv_num(bp, &i, 0, 99);

        i = i * 100 - TM_YEAR_BASE;
        if (split_year)
          i += tm->tm_year % 100;
        split_year = 1;
        tm->tm_year = i;
        LEGAL_ALT(ALT_E);
        continue;

      case 'd':  /* The day of month. */
      case 'e':
        bp = conv_num(bp, &tm->tm_mday, 1, 31);
        LEGAL_ALT(ALT_O);
        continue;

      case 'k':  /* The hour (24-hour clock representation). */
        LEGAL_ALT(0);
        [[fallthrough]];
      case 'H':
        bp = conv_num(bp, &tm->tm_hour, 0, 23);
        LEGAL_ALT(ALT_O);
        continue;

      case 'l':  /* The hour (12-hour clock representation). */
        LEGAL_ALT(0);
        [[fallthrough]];
      case 'I':
        bp = conv_num(bp, &tm->tm_hour, 1, 12);
        if (tm->tm_hour == 12)
          tm->tm_hour = 0;
        LEGAL_ALT(ALT_O);
        continue;

      case 'j':  /* The day of year. */
        i = 1;
        bp = conv_num(bp, &i, 1, 366);
        tm->tm_yday = i - 1;
        LEGAL_ALT(0);
        continue;

      case 'M':  /* The minute. */
        bp = conv_num(bp, &tm->tm_min, 0, 59);
        LEGAL_ALT(ALT_O);
        continue;

      case 'm':  /* The month. */
        i = 1;
        bp = conv_num(bp, &i, 1, 12);
        tm->tm_mon = i - 1;
        LEGAL_ALT(ALT_O);
        continue;

      case 'p':  /* The locale's equivalent of AM/PM. */
        bp = find_string(bp, &i, _ctloc(am_pm), NULL, 2);
        if (tm->tm_hour > 11)
          return NULL;
        tm->tm_hour += i * 12;
        LEGAL_ALT(0);
        continue;

      case 'S':  /* The seconds. */
        bp = conv_num(bp, &tm->tm_sec, 0, 61);
        LEGAL_ALT(ALT_O);
        continue;

      case 'U':  /* The week of year, beginning on sunday. */
      case 'W':  /* The week of year, beginning on monday. */
        /*
         * XXX This is bogus, as we can not assume any valid
         * information present in the tm structure at this
         * point to calculate a real value, so just check the
         * range for now.
         */
         bp = conv_num(bp, &i, 0, 53);
         LEGAL_ALT(ALT_O);
         continue;

      case 'w':  /* The day of week, beginning on sunday. */
        bp = conv_num(bp, &tm->tm_wday, 0, 6);
        LEGAL_ALT(ALT_O);
        continue;

      case 'Y':  /* The year. */
        i = TM_YEAR_BASE;  /* just for data sanity... */
        bp = conv_num(bp, &i, 0, 9999);
        tm->tm_year = i - TM_YEAR_BASE;
        LEGAL_ALT(ALT_E);
        continue;

      case 'y':  /* The year within 100 years of the epoch. */
        /* LEGAL_ALT(ALT_E | ALT_O); */
        bp = conv_num(bp, &i, 0, 99);

        if (split_year)
          /* preserve century */
          i += (tm->tm_year / 100) * 100;
        else {
          split_year = 1;
          if (i <= 68)
            i = i + 2000 - TM_YEAR_BASE;
          else
            i = i + 1900 - TM_YEAR_BASE;
        }
        tm->tm_year = i;
        continue;

      /*
       * Miscellaneous conversions.
       */
      case 'n':  /* Any kind of white-space. */
      case 't':
        while (isspace(*bp))
          bp++;
        LEGAL_ALT(0);
        continue;


      default:  /* Unknown/unsupported conversion. */
        return NULL;
      }
    }

    return __UNCONST(bp);
  }


  static const u_char *
  conv_num(const unsigned char *buf, int *dest, uint llim, uint ulim)
  {
    uint result = 0;
    unsigned char ch;

    /* The limit also determines the number of valid digits. */
    uint rulim = ulim;

    ch = *buf;
    if (ch < '0' || ch > '9')
      return NULL;

    do {
      result *= 10;
      result += ch - '0';
      rulim /= 10;
      ch = *++buf;
    } while ((result * 10 <= ulim) && rulim && ch >= '0' && ch <= '9');

    if (result < llim || result > ulim)
      return NULL;

    *dest = result;
    return buf;
  }

  static const u_char *
  find_string(const u_char *bp, int *tgt, const char * const *n1,
      const char * const *n2, int c)
  {
    int i;
    size_t len;

    /* check full name - then abbreviated ones */
    for (; n1 != NULL; n1 = n2, n2 = NULL) {
      for (i = 0; i < c; i++, n1++) {
        len = strlen(*n1);
        if (StringUtils::CompareNoCase(*n1, (const char*)bp, len) == 0)
        {
          *tgt = i;
          return bp + len;
        }
      }
    }

    /* Nothing matched */
    return NULL;
  }
}

#ifdef TARGET_WINDOWS_DESKTOP
LONG CWIN32Util::UtilRegGetValue( const HKEY hKey, const char *const pcKey, DWORD *const pdwType, char **const ppcBuffer, DWORD *const pdwSizeBuff, const DWORD dwSizeAdd )
{
  using KODI::PLATFORM::WINDOWS::ToW;

  auto pcKeyW = ToW(pcKey);

  DWORD dwSize;
  LONG lRet= RegQueryValueEx(hKey, pcKeyW.c_str(), nullptr, pdwType, nullptr, &dwSize );
  if (lRet == ERROR_SUCCESS)
  {
    if (ppcBuffer)
    {
      char *pcValue=*ppcBuffer, *pcValueTmp;
      if (!pcValue || !pdwSizeBuff || dwSize +dwSizeAdd > *pdwSizeBuff) {
        pcValueTmp = static_cast<char*>(realloc(pcValue, dwSize + dwSizeAdd));
        if(pcValueTmp != nullptr)
        {
          pcValue = pcValueTmp;
        }
      }
      lRet= RegQueryValueEx(hKey, pcKeyW.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(pcValue), &dwSize);

      if ( lRet == ERROR_SUCCESS || *ppcBuffer )
        *ppcBuffer= pcValue;
      else
        free( pcValue );
    }
    if (pdwSizeBuff) *pdwSizeBuff= dwSize +dwSizeAdd;
  }
  return lRet;
}

bool CWIN32Util::UtilRegOpenKeyEx( const HKEY hKeyParent, const char *const pcKey, const REGSAM rsAccessRights, HKEY *hKey, const bool bReadX64 )
{
  using KODI::PLATFORM::WINDOWS::ToW;

  const REGSAM rsAccessRightsTmp = (
    CSysInfo::GetKernelBitness() == 64 ? rsAccessRights |
      ( bReadX64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY ) : rsAccessRights );
  bool bRet = ( ERROR_SUCCESS == RegOpenKeyEx(hKeyParent, ToW(pcKey).c_str(), 0, rsAccessRightsTmp, hKey));
  return bRet;
}

// Retrieve the filename of the process that currently has the focus.
// Typically this will be some process using the system tray grabbing
// the focus and causing XBMC to minimise. Logging the offending
// process name can help the user fix the problem.
bool CWIN32Util::GetFocussedProcess(std::string &strProcessFile)
{
  using KODI::PLATFORM::WINDOWS::FromW;
  strProcessFile = "";

  // Get the window that has the focus
  HWND hfocus = GetForegroundWindow();
  if (!hfocus)
    return false;

  // Get the process ID from the window handle
  DWORD pid = 0;
  GetWindowThreadProcessId(hfocus, &pid);

  // Use OpenProcess to get the process handle from the process ID
  HANDLE hproc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid);
  if (!hproc)
    return false;

  // Load QueryFullProcessImageName dynamically because it isn't available
  // in all versions of Windows.
  wchar_t procfile[MAX_PATH+1];
  DWORD procfilelen = MAX_PATH;

  if (QueryFullProcessImageNameW(hproc, 0, procfile, &procfilelen))
    strProcessFile = FromW(procfile);

  CloseHandle(hproc);

  return true;
}
#endif

// Adjust the src rectangle so that the dst is always contained in the target rectangle.
void CWIN32Util::CropSource(CRect& src, CRect& dst, CRect target, UINT rotation /* = 0 */)
{
  float s_width = src.Width(), s_height = src.Height();
  float d_width = dst.Width(), d_height = dst.Height();

  if (dst.x1 < target.x1)
  {
    switch (rotation)
    {
    case 90:
      src.y1 -= (dst.x1 - target.x1) * s_height / d_width;
      break;
    case 180:
      src.x2 += (dst.x1 - target.x1) * s_width  / d_width;
      break;
    case 270:
      src.y2 += (dst.x1 - target.x1) * s_height / d_width;
      break;
    default:
      src.x1 -= (dst.x1 - target.x1) * s_width  / d_width;
      break;
    }
    dst.x1  = target.x1;
  }
  if(dst.y1 < target.y1)
  {
    switch (rotation)
    {
    case 90:
      src.x1 -= (dst.y1 - target.y1) * s_width  / d_height;
      break;
    case 180:
      src.y2 += (dst.y1 - target.y1) * s_height / d_height;
      break;
    case 270:
      src.x2 += (dst.y1 - target.y1) * s_width  / d_height;
      break;
    default:
      src.y1 -= (dst.y1 - target.y1) * s_height / d_height;
      break;
    }
    dst.y1  = target.y1;
  }
  if(dst.x2 > target.x2)
  {
    switch (rotation)
    {
    case 90:
      src.y2 -= (dst.x2 - target.x2) * s_height / d_width;
      break;
    case 180:
      src.x1 += (dst.x2 - target.x2) * s_width  / d_width;
      break;
    case 270:
      src.y1 += (dst.x2 - target.x2) * s_height / d_width;
      break;
    default:
      src.x2 -= (dst.x2 - target.x2) * s_width  / d_width;
      break;
    }
    dst.x2  = target.x2;
  }
  if(dst.y2 > target.y2)
  {
    switch (rotation)
    {
    case 90:
      src.x2 -= (dst.y2 - target.y2) * s_width / d_height;
      break;
    case 180:
      src.y1 += (dst.y2 - target.y2) * s_height / d_height;
      break;
    case 270:
      src.x1 += (dst.y2 - target.y2) * s_width / d_height;
      break;
    default:
      src.y2 -= (dst.y2 - target.y2) * s_height / d_height;
      break;
    }
    dst.y2  = target.y2;
  }
  // Callers expect integer coordinates.
  src.x1 = floor(src.x1);
  src.y1 = floor(src.y1);
  src.x2 = ceil(src.x2);
  src.y2 = ceil(src.y2);
  dst.x1 = floor(dst.x1);
  dst.y1 = floor(dst.y1);
  dst.x2 = ceil(dst.x2);
  dst.y2 = ceil(dst.y2);
}

std::string CWIN32Util::WUSysMsg(DWORD dwError)
{
  #define SS_DEFLANGID MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT)
  CHAR szBuf[512];

  if ( 0 != ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError,
                             SS_DEFLANGID, szBuf, 511, NULL) )
    return StringUtils::Format("{} (0x{:X})", szBuf, dwError);
  else
    return StringUtils::Format("Unknown error (0x{:X})", dwError);
}

bool CWIN32Util::SetThreadLocalLocale(bool enable /* = true */)
{
  const int param = enable ? _ENABLE_PER_THREAD_LOCALE : _DISABLE_PER_THREAD_LOCALE;
  return _configthreadlocale(param) != -1;
}

HDR_STATUS CWIN32Util::ToggleWindowsHDR(DXGI_MODE_DESC& modeDesc)
{
  HDR_STATUS status = HDR_STATUS::HDR_TOGGLE_FAILED;

#ifdef TARGET_WINDOWS_STORE
  auto hdmi = HdmiDisplayInformation::GetForCurrentView();

  if (!hdmi)
    return status;

  const auto current = hdmi.GetCurrentDisplayMode();

  for (const auto& mode : hdmi.GetSupportedDisplayModes())
  {
    if (mode.IsSmpte2084Supported() != current.IsSmpte2084Supported() &&
        mode.ResolutionHeightInRawPixels() == current.ResolutionHeightInRawPixels() &&
        mode.ResolutionWidthInRawPixels() == current.ResolutionWidthInRawPixels() &&
        mode.StereoEnabled() == false &&
        fabs(mode.RefreshRate() - current.RefreshRate()) <= 0.00001)
    {
      if (current.IsSmpte2084Supported()) // HDR is ON
      {
        CLog::LogF(LOGINFO, "Toggle Windows HDR Off (ON => OFF).");
        if (Wait(hdmi.RequestSetCurrentDisplayModeAsync(mode, HdmiDisplayHdrOption::None)))
          status = HDR_STATUS::HDR_OFF;
      }
      else // HDR is OFF
      {
        CLog::LogF(LOGINFO, "Toggle Windows HDR On (OFF => ON).");
        if (Wait(hdmi.RequestSetCurrentDisplayModeAsync(mode, HdmiDisplayHdrOption::Eotf2084)))
          status = HDR_STATUS::HDR_ON;
      }
      break;
    }
  }
#else
  uint32_t pathCount = 0;
  uint32_t modeCount = 0;

  MONITORINFOEXW mi = {};
  mi.cbSize = sizeof(mi);
  GetMonitorInfoW(MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
  const std::wstring deviceNameW = mi.szDevice;

  if (ERROR_SUCCESS == GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount))
  {
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);

    if (ERROR_SUCCESS == QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(),
                                            &modeCount, modes.data(), nullptr))
    {
      DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO getColorInfo = {};
      getColorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
      getColorInfo.header.size = sizeof(getColorInfo);

      DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE setColorState = {};
      setColorState.header.type = DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE;
      setColorState.header.size = sizeof(setColorState);

      DISPLAYCONFIG_SOURCE_DEVICE_NAME getSourceName = {};
      getSourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
      getSourceName.header.size = sizeof(getSourceName);

      // Only try to toggle display currently used by Kodi
      for (const auto& path : paths)
      {
        getSourceName.header.adapterId.HighPart = path.sourceInfo.adapterId.HighPart;
        getSourceName.header.adapterId.LowPart = path.sourceInfo.adapterId.LowPart;
        getSourceName.header.id = path.sourceInfo.id;

        if (ERROR_SUCCESS == DisplayConfigGetDeviceInfo(&getSourceName.header))
        {
          const std::wstring sourceNameW = getSourceName.viewGdiDeviceName;
          if (deviceNameW == sourceNameW)
          {
            const auto& mode = modes.at(path.targetInfo.modeInfoIdx);

            getColorInfo.header.adapterId.HighPart = mode.adapterId.HighPart;
            getColorInfo.header.adapterId.LowPart = mode.adapterId.LowPart;
            getColorInfo.header.id = mode.id;

            setColorState.header.adapterId.HighPart = mode.adapterId.HighPart;
            setColorState.header.adapterId.LowPart = mode.adapterId.LowPart;
            setColorState.header.id = mode.id;

            if (ERROR_SUCCESS == DisplayConfigGetDeviceInfo(&getColorInfo.header))
            {
              if (getColorInfo.advancedColorSupported)
              {
                if (getColorInfo.advancedColorEnabled) // HDR is ON
                {
                  setColorState.enableAdvancedColor = FALSE;
                  status = HDR_STATUS::HDR_OFF;
                  CLog::LogF(LOGINFO, "Toggle Windows HDR Off (ON => OFF).");
                }
                else // HDR is OFF
                {
                  setColorState.enableAdvancedColor = TRUE;
                  status = HDR_STATUS::HDR_ON;
                  CLog::LogF(LOGINFO, "Toggle Windows HDR On (OFF => ON).");
                }
                if (ERROR_SUCCESS != DisplayConfigSetDeviceInfo(&setColorState.header))
                  status = HDR_STATUS::HDR_TOGGLE_FAILED;
              }
            }
            break;
          }
        }
      }
    }
  }

  // Restores previous graphics mode before toggle HDR
  if (status != HDR_STATUS::HDR_TOGGLE_FAILED && modeDesc.RefreshRate.Denominator != 0)
  {
    float fps = static_cast<float>(modeDesc.RefreshRate.Numerator) /
                static_cast<float>(modeDesc.RefreshRate.Denominator);
    int32_t est;
    DEVMODEW devmode = {};
    devmode.dmSize = sizeof(devmode);
    devmode.dmPelsWidth = modeDesc.Width;
    devmode.dmPelsHeight = modeDesc.Height;
    devmode.dmDisplayFrequency = static_cast<uint32_t>(fps);
    if (modeDesc.ScanlineOrdering &&
        modeDesc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE)
      devmode.dmDisplayFlags = DM_INTERLACED;
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;
    est = ChangeDisplaySettingsExW(deviceNameW.c_str(), &devmode, nullptr, CDS_FULLSCREEN, nullptr);
    if (est == DISP_CHANGE_SUCCESSFUL)
      CLog::LogF(LOGDEBUG, "Previous graphics mode restored OK");
    else
      CLog::LogF(LOGERROR, "Previous graphics mode cannot be restored (error# {})", est);
  }
#endif

  return status;
}

HDR_STATUS CWIN32Util::GetWindowsHDRStatus()
{
  bool advancedColorSupported = false;
  bool advancedColorEnabled = false;
  HDR_STATUS status = HDR_STATUS::HDR_UNSUPPORTED;

#ifdef TARGET_WINDOWS_STORE
  auto displayInformation = DisplayInformation::GetForCurrentView();

  if (displayInformation)
  {
    auto advancedColorInfo = displayInformation.GetAdvancedColorInfo();

    if (advancedColorInfo)
    {
      if (advancedColorInfo.CurrentAdvancedColorKind() == AdvancedColorKind::HighDynamicRange)
      {
        advancedColorSupported = true;
        advancedColorEnabled = true;
      }
    }
  }
  // Try to find out if the display supports HDR even if Windows HDR switch is OFF
  if (!advancedColorEnabled)
  {
    auto displayManager = DisplayManager::Create(DisplayManagerOptions::None);

    if (displayManager)
    {
      auto targets = displayManager.GetCurrentTargets();

      for (const auto& target : targets)
      {
        if (target.IsConnected())
        {
          auto displayMonitor = target.TryGetMonitor();
          if (displayMonitor.MaxLuminanceInNits() >= 400.0f)
          {
            advancedColorSupported = true;
            break;
          }
        }
      }
      displayManager.Close();
    }
  }
#else
  uint32_t pathCount = 0;
  uint32_t modeCount = 0;

  MONITORINFOEXW mi = {};
  mi.cbSize = sizeof(mi);
  GetMonitorInfoW(MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
  const std::wstring deviceNameW = mi.szDevice;

  if (ERROR_SUCCESS == GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount))
  {
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);

    if (ERROR_SUCCESS == QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(),
                                            &modeCount, modes.data(), 0))
    {
      DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO getColorInfo = {};
      getColorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
      getColorInfo.header.size = sizeof(getColorInfo);

      DISPLAYCONFIG_SOURCE_DEVICE_NAME getSourceName = {};
      getSourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
      getSourceName.header.size = sizeof(getSourceName);

      for (const auto& path : paths)
      {
        getSourceName.header.adapterId.HighPart = path.sourceInfo.adapterId.HighPart;
        getSourceName.header.adapterId.LowPart = path.sourceInfo.adapterId.LowPart;
        getSourceName.header.id = path.sourceInfo.id;

        if (ERROR_SUCCESS == DisplayConfigGetDeviceInfo(&getSourceName.header))
        {
          const std::wstring sourceNameW = getSourceName.viewGdiDeviceName;
          if (g_hWnd == nullptr || deviceNameW == sourceNameW)
          {
            const auto& mode = modes.at(path.targetInfo.modeInfoIdx);

            getColorInfo.header.adapterId.HighPart = mode.adapterId.HighPart;
            getColorInfo.header.adapterId.LowPart = mode.adapterId.LowPart;
            getColorInfo.header.id = mode.id;

            if (ERROR_SUCCESS == DisplayConfigGetDeviceInfo(&getColorInfo.header))
            {
              if (getColorInfo.advancedColorEnabled)
                advancedColorEnabled = true;

              if (getColorInfo.advancedColorSupported)
                advancedColorSupported = true;
            }

            if (g_hWnd != nullptr)
              break;
          }
        }
      }
    }
  }
#endif

  if (!advancedColorSupported)
  {
    status = HDR_STATUS::HDR_UNSUPPORTED;
    if (CServiceBroker::IsServiceManagerUp())
      CLog::LogF(LOGDEBUG, "Display is not HDR capable or cannot be detected");
  }
  else
  {
    status = advancedColorEnabled ? HDR_STATUS::HDR_ON : HDR_STATUS::HDR_OFF;
    if (CServiceBroker::IsServiceManagerUp())
      CLog::LogF(LOGDEBUG, "Display is HDR capable and current HDR status is {}",
                 advancedColorEnabled ? "ON" : "OFF");
  }

  return status;
}

/*!
 * \brief Retrieve from the system the max luminance of SDR content in HDR.
 *
 * Retrieve from the system the max luminance of SDR content in HDR.
 * Note: always returns 80 nits when the screen is in SDR mode.
 *
 * \param gdiDeviceName The screen to retrieve the information for
 * \param sdrWhiteLevel Max luminance in nits, clamped to 10000
 * \return true if a value could be read from the system and copied to sdrWhiteLevel, false otherwise
*/
bool CWIN32Util::GetSystemSdrWhiteLevel(const std::wstring& gdiDeviceName, float* sdrWhiteLevel)
{
#ifdef TARGET_WINDOWS_STORE
  auto displayInformation = DisplayInformation::GetForCurrentView();

  if (displayInformation)
  {
    auto advancedColorInfo = displayInformation.GetAdvancedColorInfo();

    if (advancedColorInfo)
    {
      const float sdrNits = advancedColorInfo.SdrWhiteLevelInNits();
      if (sdrWhiteLevel)
      {
        if (sdrNits > 10000.0f)
          *sdrWhiteLevel = 10000.0f;
        else
          *sdrWhiteLevel = sdrNits;
      }
      return true;
    }
  }
  return false;
#else
  // DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL was added in Windows 10 1709

  uint32_t pathCount{0};
  uint32_t modeCount{0};
  std::vector<DISPLAYCONFIG_PATH_INFO> paths;
  std::vector<DISPLAYCONFIG_MODE_INFO> modes;

  uint32_t flags = QDC_ONLY_ACTIVE_PATHS;
  LONG result = ERROR_SUCCESS;

  do
  {
    if (GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount) != ERROR_SUCCESS)
      return false;

    paths.resize(pathCount);
    modes.resize(modeCount);

    result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);
  } while (result == ERROR_INSUFFICIENT_BUFFER);

  if (result == ERROR_SUCCESS)
  {
    paths.resize(pathCount);
    modes.resize(modeCount);

    for (const auto& path : paths)
    {
      DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
      source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
      source.header.size = sizeof(source);
      source.header.adapterId = path.sourceInfo.adapterId;
      source.header.id = path.sourceInfo.id;

      if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS)
      {
        if (gdiDeviceName == source.viewGdiDeviceName)
        {
          DISPLAYCONFIG_SDR_WHITE_LEVEL config{};
          config.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
          config.header.size = sizeof(config);
          config.header.adapterId = path.targetInfo.adapterId;
          config.header.id = path.targetInfo.id;
          if (DisplayConfigGetDeviceInfo(&config.header) == ERROR_SUCCESS)
          {
            if (sdrWhiteLevel)
            {
              const float sdrNits = static_cast<const float>(config.SDRWhiteLevel * 80 / 1000);
              if (sdrNits > 10000.0f)
                *sdrWhiteLevel = 10000.0f;
              else
                *sdrWhiteLevel = sdrNits;
            }
            return true;
          }
          break;
        }
      }
    }
  }
  return (false);
#endif
}

void CWIN32Util::PlatformSyslog()
{
  CLog::Log(LOGINFO, "System has {:.1f} GB of RAM installed",
            GetSystemMemorySize() / static_cast<double>(MB));
  CLog::Log(LOGINFO, "{}", GetResInfoString());
  CLog::Log(LOGINFO, "Running with {} rights",
            (IsCurrentUserLocalAdministrator() == TRUE) ? "administrator" : "restricted");
  CLog::Log(LOGINFO, "Aero is {}", (g_sysinfo.IsAeroDisabled() == true) ? "disabled" : "enabled");
  HDR_STATUS hdrStatus = GetWindowsHDRStatus();
  if (hdrStatus == HDR_STATUS::HDR_UNSUPPORTED)
    CLog::Log(LOGINFO, "Display is not HDR capable or cannot be detected");
  else
    CLog::Log(LOGINFO, "Display HDR capable is detected and Windows HDR switch is {}",
              (hdrStatus == HDR_STATUS::HDR_ON) ? "ON" : "OFF");
}

VideoDriverInfo CWIN32Util::GetVideoDriverInfo(const UINT vendorId, const std::wstring& driverDesc)
{
  VideoDriverInfo info = {};

#ifdef TARGET_WINDOWS_DESKTOP
  HKEY hKey = nullptr;
  const wchar_t* SUBKEY = L"SYSTEM\\CurrentControlSet\\Control\\Video";

  if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_LOCAL_MACHINE, SUBKEY, 0, KEY_ENUMERATE_SUB_KEYS, &hKey))
    return {};

  LSTATUS sta = ERROR_SUCCESS;
  wchar_t keyName[128] = {};
  DWORD index = 0;
  DWORD len;

  using KODI::PLATFORM::WINDOWS::FromW;

  do
  {
    len = sizeof(keyName) / sizeof(wchar_t);
    sta = RegEnumKeyExW(hKey, index, keyName, &len, nullptr, nullptr, nullptr, nullptr);
    index++;

    if (sta != ERROR_SUCCESS)
      continue;

    std::wstring subkey(SUBKEY);
    subkey.append(L"\\");
    subkey.append(keyName);
    subkey.append(L"\\");
    subkey.append(L"0000");
    DWORD lg;

    wchar_t desc[128] = {};
    lg = sizeof(desc) / sizeof(wchar_t);
    if (ERROR_SUCCESS != RegGetValueW(HKEY_LOCAL_MACHINE, subkey.c_str(), L"DriverDesc",
                                      RRF_RT_REG_SZ, nullptr, desc, &lg))
      continue;

    std::wstring s_desc(desc);
    if (s_desc != driverDesc)
      continue;

    // driver of interest found, we read version
    wchar_t version[64] = {};
    lg = sizeof(version) / sizeof(wchar_t);
    if (ERROR_SUCCESS != RegGetValueW(HKEY_LOCAL_MACHINE, subkey.c_str(), L"DriverVersion",
                                      RRF_RT_REG_SZ, nullptr, version, &lg))
      continue;

    info.valid = true;
    info.version = FromW(std::wstring(version));

    // convert driver store version to Nvidia version
    if (vendorId == PCIV_NVIDIA)
    {
      std::string ver(info.version);
      StringUtils::Replace(ver, ".", "");
      info.majorVersion = std::stoi(ver.substr(ver.length() - 5, 3));
      info.minorVersion = std::stoi(ver.substr(ver.length() - 2, 2));
    }
    else // for Intel/AMD fill major version only. Single-digit for WDDM < 1.3.
    {
      info.majorVersion = std::stoi(info.version.substr(0, info.version.find('.')));
    }

  } while (sta == ERROR_SUCCESS && !info.valid);

  RegCloseKey(hKey);
#endif

  return info;
}

std::wstring CWIN32Util::GetDisplayFriendlyName(const std::wstring& gdiDeviceName)
{
#ifdef TARGET_WINDOWS_STORE
  // Not supported
  return std::wstring();
#else

  uint32_t pathCount{};
  uint32_t modeCount{};
  std::vector<DISPLAYCONFIG_PATH_INFO> paths;
  std::vector<DISPLAYCONFIG_MODE_INFO> modes;

  uint32_t flags = QDC_ONLY_ACTIVE_PATHS;
  LONG result = ERROR_SUCCESS;

  do
  {
    if (GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount) != ERROR_SUCCESS)
      return std::wstring();

    paths.resize(pathCount);
    modes.resize(modeCount);

    result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);
  } while (result == ERROR_INSUFFICIENT_BUFFER);

  if (result == ERROR_SUCCESS)
  {
    paths.resize(pathCount);
    modes.resize(modeCount);

    for (const auto& path : paths)
    {
      DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {};
      source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
      source.header.size = sizeof(source);
      source.header.adapterId = path.sourceInfo.adapterId;
      source.header.id = path.sourceInfo.id;

      if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS)
      {
        if (gdiDeviceName == source.viewGdiDeviceName)
        {
          DISPLAYCONFIG_TARGET_DEVICE_NAME target = {};
          target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
          target.header.size = sizeof(target);
          target.header.adapterId = path.targetInfo.adapterId;
          target.header.id = path.targetInfo.id;
          if (DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS)
            return target.monitorFriendlyDeviceName;

          break;
        }
      }
    }
  }
  return std::wstring();
#endif
}

using SETTHREADDESCRIPTION = HRESULT(WINAPI*)(HANDLE hThread, PCWSTR lpThreadDescription);

bool CWIN32Util::SetThreadName(const HANDLE handle, const std::string& name)
{
#if defined(TARGET_WINDOWS_STORE)
  //not supported
  return false;
#else
  static bool initialized = false;
  static HINSTANCE hinstLib = NULL;
  static SETTHREADDESCRIPTION pSetThreadDescription = nullptr;

  if (!initialized)
  {
    initialized = true;

    // MS documentation: SetThreadDescription available since Windows 10 1607
    // function located in Kernel32.dll
    // except for Windows 10 1607, where it is located in KernelBase.dll
    CSysInfo::WindowsVersion winver = CSysInfo::GetWindowsVersion();

    if (winver < CSysInfo::WindowsVersion::WindowsVersionWin10_1607)
      return false;
    else if (winver == CSysInfo::WindowsVersion::WindowsVersionWin10_1607)
      hinstLib = LoadLibrary(L"KernelBase.dll");
    else if (winver > CSysInfo::WindowsVersion::WindowsVersionWin10_1607)
      hinstLib = LoadLibrary(L"Kernel32.dll");

    if (hinstLib != NULL)
    {
      pSetThreadDescription = reinterpret_cast<SETTHREADDESCRIPTION>(
          ::GetProcAddress(hinstLib, "SetThreadDescription"));
    }

    if (pSetThreadDescription == nullptr && hinstLib)
      FreeLibrary(hinstLib);
  }

  if (pSetThreadDescription != nullptr &&
      SUCCEEDED(pSetThreadDescription(handle, KODI::PLATFORM::WINDOWS::ToW(name).c_str())))
    return true;
  else
    return false;

#endif
}

static std::array<int, 4> ParseVideoDriverInfo(const std::string& version)
{
  std::array<int, 4> result{};

  // the string is destroyed in the process, make a copy first.
  std::string v{version};

  char* p = std::strtok(v.data(), ".");
  for (int idx = 0; p && idx < 4; ++idx)
  {
    result[idx] = std::stoi(p);
    p = std::strtok(NULL, ".");
  }

  return result;
}

bool CWIN32Util::IsDriverVersionAtLeast(const std::string& version1, const std::string& version2)
{
  const std::array<int, 4> v1 = ParseVideoDriverInfo(version1);
  const std::array<int, 4> v2 = ParseVideoDriverInfo(version2);

  for (int idx = 0; idx < 4; ++idx)
  {
    if (v1[idx] > v2[idx])
      return true;
    else if (v1[idx] < v2[idx])
      return false;
    // equality: compare the next segment.
  }
  return true;
}
