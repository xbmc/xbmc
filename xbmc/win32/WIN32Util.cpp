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

#include "stdafx.h"
#include "WIN32Util.h"
#include "GUISettings.h"
#include "../Util.h"
#include "FileSystem/cdioSupport.h"
#include "PowrProf.h"
#include "WindowHelper.h"
#include "Application.h"
#include <shlobj.h>
#include "SpecialProtocol.h"
#include "my_ntddscsi.h"

#define DLL_ENV_PATH "special://xbmc/system/;special://xbmc/system/players/dvdplayer/;special://xbmc/system/players/paplayer/;special://xbmc/system/python/"

extern HWND g_hWnd;

using namespace std;
using namespace MEDIA_DETECT;

DWORD CWIN32Util::dwDriveMask = 0;



CWIN32Util::CWIN32Util(void)
{
}

CWIN32Util::~CWIN32Util(void)
{
}


const CStdString CWIN32Util::GetNextFreeDriveLetter()
{
  for(int iDrive='a';iDrive<='z';iDrive++)
  {
    CStdString strDrive;
    strDrive.Format("%c:",iDrive);
    int iType = GetDriveType(strDrive);
    if(iType == DRIVE_NO_ROOT_DIR && iDrive != 'a' && iDrive != 'b')
      return strDrive;
  }
  return StringUtils::EmptyString;
}

CStdString CWIN32Util::MountShare(const CStdString &smbPath, const CStdString &strUser, const CStdString &strPass, DWORD *dwError)
{
  NETRESOURCE nr;
  memset(&nr,0,sizeof(nr));
  CStdString strRemote = smbPath;
  CStdString strDrive = CWIN32Util::GetNextFreeDriveLetter();

  if(strDrive == StringUtils::EmptyString)
    return StringUtils::EmptyString;

  strRemote.Replace('/', '\\');

  nr.lpRemoteName = (LPTSTR)(LPCTSTR)strRemote.c_str();
  nr.lpLocalName  = (LPTSTR)(LPCTSTR)strDrive.c_str();
  nr.dwType       = RESOURCETYPE_DISK;

  DWORD dwRes = WNetAddConnection2(&nr,(LPCTSTR)strPass.c_str(), (LPCTSTR)strUser.c_str(), NULL);

  if(dwError != NULL)
    *dwError = dwRes;

  if(dwRes != NO_ERROR)
  {
    CLog::Log(LOGERROR, "Can't mount %s to %s. Error code %d",strRemote.c_str(), strDrive.c_str(),dwRes);
    return StringUtils::EmptyString;
  }

  return strDrive;
}

DWORD CWIN32Util::UmountShare(const CStdString &strPath)
{
  return WNetCancelConnection2((LPCTSTR)strPath.c_str(),NULL,true);
}

CStdString CWIN32Util::MountShare(const CStdString &strPath, DWORD *dwError)
{
  CStdString strURL = strPath;
  CURL url(strURL);
  url.GetURL(strURL);
  CStdString strPassword = url.GetPassWord();
  CStdString strUserName = url.GetUserName();
  CStdString strPathToShare = "\\\\"+url.GetHostName() + "\\" + url.GetShareName();
  if(!url.GetUserName().IsEmpty())
    return CWIN32Util::MountShare(strPathToShare, strUserName, strPassword, dwError);
  else
    return CWIN32Util::MountShare(strPathToShare, "", "", dwError);
}

CStdString CWIN32Util::URLEncode(const CURL &url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  CStdString flat = "smb://";

  if(url.GetDomain().length() > 0)
  {
    flat += url.GetDomain();
    flat += ";";
  }

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    flat += url.GetUserName();
    flat += ":";
    flat += url.GetPassWord();
    flat += "@";
  }
  else if( !url.GetHostName().IsEmpty() && !g_guiSettings.GetString("smb.username").IsEmpty() )
  {
    /* okey this is abit uggly to do this here, as we don't really only url encode */
    /* but it's the simplest place to do so */
    flat += g_guiSettings.GetString("smb.username");
    flat += ":";
    flat += g_guiSettings.GetString("smb.password");
    flat += "@";
  }

  flat += url.GetHostName();

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<CStdString> parts;
  std::vector<CStdString>::iterator it;
  CUtil::Tokenize(url.GetFileName(), parts, "/");
  for( it = parts.begin(); it != parts.end(); it++ )
  {
    flat += "/";
    flat += (*it);
  }

  /* okey options should go here, thou current samba doesn't support any */

  return flat;
}

int CWIN32Util::GetDriveStatus(const CStdString &strPath)
{
  HANDLE hDevice;               // handle to the drive to be examined
  int iResult;                  // results flag
  ULONG ulChanges=0;
  DWORD dwBytesReturned;
  T_SPDT_SBUF sptd_sb;  //SCSI Pass Through Direct variable.
  byte DataBuf[16];  //Buffer for holding data to/from drive.

  hDevice = CreateFile( strPath.c_str(),                  // drive
                        0,                                // no access to the drive
                        FILE_SHARE_READ,                  // share mode
                        NULL,                             // default security attributes
                        OPEN_EXISTING,                    // disposition
                        FILE_ATTRIBUTE_READONLY,          // file attributes
                        NULL);

  if (hDevice == INVALID_HANDLE_VALUE)                    // cannot open the drive
  {
    return -1;
  }

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

  hDevice = CreateFile( strPath.c_str(),
                        GENERIC_READ | GENERIC_WRITE, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_READONLY,
                        NULL);

  if (hDevice == INVALID_HANDLE_VALUE)
  {
    return -1;
  }

  sptd_sb.sptd.Length=sizeof(SCSI_PASS_THROUGH_DIRECT);
  sptd_sb.sptd.PathId=0; 
  sptd_sb.sptd.TargetId=0;
  sptd_sb.sptd.Lun=0;
  sptd_sb.sptd.CdbLength=10;
  sptd_sb.sptd.SenseInfoLength=MAX_SENSE_LEN;
  sptd_sb.sptd.DataIn=SCSI_IOCTL_DATA_IN;
  sptd_sb.sptd.DataTransferLength=8;
  sptd_sb.sptd.TimeOutValue=108000;
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

  ZeroMemory(sptd_sb.SenseBuf, MAX_SENSE_LEN);

  //Send the command to drive
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
  return -1;
}

CStdString CWIN32Util::GetLocalPath(const CStdString &strPath)
{
  CURL url(strPath);
  CStdString strLocalPath = url.GetFileName();
  strLocalPath.Replace(url.GetShareName()+"/","");
  return strLocalPath;
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


// Workaround to get the added and removed drives
// Seems to be that the lParam in SDL is empty
// MS way: http://msdn.microsoft.com/en-us/library/aa363215(VS.85).aspx

void CWIN32Util::UpdateDriveMask()
{
  dwDriveMask = GetLogicalDrives();
}

CStdString CWIN32Util::GetChangedDrive()
{
  CStdString strDrive;
  DWORD dwDriveMask2 = GetLogicalDrives();
  DWORD dwDriveMaskResult = dwDriveMask ^ dwDriveMask2;
  if(dwDriveMaskResult == 0)
    return "";
  dwDriveMask = dwDriveMask2;
  strDrive.Format("%c:",FirstDriveFromMask(dwDriveMaskResult));
  return strDrive;
}

// End Workaround

bool CWIN32Util::PowerManagement(PowerState State)
{
// SetSuspendState not available in vs2003
#if _MSC_VER > 1400
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;
  // Get a token for this process.
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
  {
    return false;
  }
  // Get the LUID for the shutdown privilege.
  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;  // one privilege to set   
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  // Get the shutdown privilege for this process.
  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
  CloseHandle(hToken);

  if (GetLastError() != ERROR_SUCCESS)
  {
    return false;
  }

  switch (State)
  {
  case POWERSTATE_HIBERNATE:
    CLog::Log(LOGINFO, "Asking Windows to hibernate...");
    return SetSuspendState(true,true,false) == TRUE;
    break;
  case POWERSTATE_SUSPEND:
    CLog::Log(LOGINFO, "Asking Windows to suspend...");
    return SetSuspendState(false,true,false) == TRUE;
    break;
  case POWERSTATE_SHUTDOWN:
    CLog::Log(LOGINFO, "Shutdown Windows...");
    return ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED) == TRUE;
    break;
  case POWERSTATE_REBOOT:
    CLog::Log(LOGINFO, "Rebooting Windows...");
    return ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED) == TRUE;
    break;
  default:
    CLog::Log(LOGERROR, "Unknown PowerState called.");
    return false;
    break;
  }
#else
  return false;
#endif
}

bool CWIN32Util::XBMCShellExecute(const CStdString &strPath, bool bWaitForScriptExit)
{
  CStdString strCommand = strPath;
  CStdString strExe = strPath;
  CStdString strParams;
  CStdString strWorkingDir;

  strCommand.Trim();
  if (strCommand.IsEmpty())
  {
    return false;
  }
  int iIndex = -1;
  char split = ' ';
  if (strCommand[0] == '\"')
  {
    split = '\"';
  }
  iIndex = strCommand.Find(split, 1);
  if (iIndex != -1)
  {
    strExe = strCommand.substr(0, iIndex + 1);
    strParams = strCommand.substr(iIndex + 1);
  }

  strExe.Replace("\"","");

  strWorkingDir = strExe; 
  iIndex = strWorkingDir.ReverseFind('\\'); 
  if(iIndex != -1) 
  { 
    strWorkingDir[iIndex+1] = '\0'; 
  } 

  bool ret;
  SHELLEXECUTEINFO ShExecInfo = {0};
  ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
  ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ShExecInfo.hwnd = NULL;
  ShExecInfo.lpVerb = NULL;
  ShExecInfo.lpFile = strExe.c_str();
  ShExecInfo.lpParameters = strParams.c_str();
  ShExecInfo.lpDirectory = strWorkingDir.c_str();
  ShExecInfo.nShow = SW_SHOW;
  ShExecInfo.hInstApp = NULL;

  g_windowHelper.StopThread();

  LockSetForegroundWindow(LSFW_UNLOCK);
  ShowWindow(g_hWnd,SW_MINIMIZE);
  ret = ShellExecuteEx(&ShExecInfo) == TRUE;
  g_windowHelper.SetHANDLE(ShExecInfo.hProcess);

  // ShellExecute doesn't return the window of the started process
  // we need to gather it from somewhere to allow switch back to XBMC
  // when a program is minimized instead of stopped.
  //g_windowHelper.SetHWND(ShExecInfo.hwnd);
  g_windowHelper.Create();

  if(bWaitForScriptExit)
  {
    // Todo: Pause music and video playback
    WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
  }

  return ret;
}

std::vector<CStdString> CWIN32Util::GetDiskUsage()
{
  CStdString strRet;
  vector<CStdString> result;
  ULARGE_INTEGER ULTotal= { { 0 } };
  ULARGE_INTEGER ULTotalFree= { { 0 } };

  char* pcBuffer= NULL;
  DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer );
  if( dwStrLength != 0 )
  {
    dwStrLength+= 1;
    pcBuffer= new char [dwStrLength];
    GetLogicalDriveStrings( dwStrLength, pcBuffer );
    int iPos= 0;
    do 
    {
      CStdString strDrive = pcBuffer + iPos;
      if( DRIVE_FIXED == GetDriveType( strDrive.c_str()  ) &&
        GetDiskFreeSpaceEx( ( strDrive.c_str() ), NULL, &ULTotal, &ULTotalFree ) )
      {
        strRet.Format("%s %d MB %s",strDrive.c_str(), int(ULTotalFree.QuadPart/(1024*1024)),g_localizeStrings.Get(160));
        result.push_back(strRet);
      }
      iPos += (strlen( pcBuffer + iPos) + 1 );
    }while( strlen( pcBuffer + iPos ) > 0 );
  }
  delete[] pcBuffer;
  return result;
}

void CWIN32Util::MaximizeWindow(bool bRemoveBorder)
{
  /*int w=0,h=0;
  g_videoConfig.GetDesktopResolution(&w,&h);*/
}

CStdString CWIN32Util::GetResInfoString()
{
  CStdString strRes;
  DEVMODE devmode;
  ZeroMemory(&devmode, sizeof(devmode));
  devmode.dmSize = sizeof(devmode);
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
  strRes.Format("Desktop Resolution: %dx%d %dBit at %dHz",devmode.dmPelsWidth,devmode.dmPelsHeight,devmode.dmBitsPerPel,devmode.dmDisplayFrequency);
  return strRes;
}

int CWIN32Util::GetDesktopColorDepth()
{
  DEVMODE devmode;
  ZeroMemory(&devmode, sizeof(devmode));
  devmode.dmSize = sizeof(devmode);
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
  return (int)devmode.dmBitsPerPel;
}

CStdString CWIN32Util::GetProfilePath()
{
  CStdString strProfilePath;
  WCHAR szPath[MAX_PATH];
  bool bpDirs = g_application.PlatformDirectoriesEnabled();

  if(bpDirs && SUCCEEDED(SHGetFolderPathW(NULL,CSIDL_APPDATA|CSIDL_FLAG_CREATE,NULL,0,szPath)))
  {
    g_charsetConverter.wToUTF8(szPath, strProfilePath);
    CUtil::AddFileToFolder(strProfilePath, "XBMC\\", strProfilePath);
  }  
  else
    CUtil::GetHomePath(strProfilePath);

  return strProfilePath;
}

void CWIN32Util::ExtendDllPath()
{
  CStdStringW strEnvW;
  CStdStringArray vecEnv;
  WCHAR wctemp[32768];
  if(GetEnvironmentVariableW(L"PATH",wctemp,32767) != 0)
    strEnvW = wctemp;

  StringUtils::SplitString(DLL_ENV_PATH, ";", vecEnv);
  for (int i=0; i<(int)vecEnv.size(); ++i)
  {
    CStdStringW strFileW;
    g_charsetConverter.utf8ToW(CSpecialProtocol::TranslatePath(vecEnv[i]), strFileW, false);
    strEnvW.append(L";" + strFileW);
  }
  if(SetEnvironmentVariableW(L"PATH",strEnvW.c_str())!=0)
    CLog::Log(LOGDEBUG,"Setting system env PATH to %S",strEnvW.c_str());
  else
    CLog::Log(LOGDEBUG,"Can't set system env PATH to %S",strEnvW.c_str());

}

HRESULT CWIN32Util::ToggleTray(const char cDriveLetter)
{
  BOOL bRet= FALSE;
  char cDL = cDriveLetter;
  if( !cDL )
  {
    char* dvdDevice = CLibcdio::GetInstance()->GetDeviceFileName();
    cDL = dvdDevice[4];
  }
  
  CStdString strVolFormat; 
  strVolFormat.Format( _T("\\\\.\\%c:" ), cDL);
  HANDLE hDrive= CreateFile( strVolFormat.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CStdString strRootFormat; 
  strRootFormat.Format( _T("%c:\\"), cDL);
  if( ( hDrive != INVALID_HANDLE_VALUE || GetLastError() == NO_ERROR) && 
      ( GetDriveType( strRootFormat ) == DRIVE_CDROM ) )
  {
    DWORD dwDummy;
    bRet= DeviceIoControl( hDrive, ( (GetDriveStatus(strVolFormat) == 1) ? IOCTL_STORAGE_LOAD_MEDIA : IOCTL_STORAGE_EJECT_MEDIA), 
                                    NULL, 0, NULL, 0, &dwDummy, NULL);
    CloseHandle( hDrive );
  }
  return bRet? S_OK : S_FALSE;
}

HRESULT CWIN32Util::EjectTray(const char cDriveLetter)
{
  char cDL = cDriveLetter;
  if( !cDL )
  {
    char* dvdDevice = CLibcdio::GetInstance()->GetDeviceFileName();
    cDL = dvdDevice[4];
  }
  
  CStdString strVolFormat; 
  strVolFormat.Format( _T("\\\\.\\%c:" ), cDL);

  if(GetDriveStatus(strVolFormat) != 1)
    return ToggleTray(cDL);
  else 
    return S_OK;
}

HRESULT CWIN32Util::CloseTray(const char cDriveLetter)
{
  char cDL = cDriveLetter;
  if( !cDL )
  {
    char* dvdDevice = CLibcdio::GetInstance()->GetDeviceFileName();
    cDL = dvdDevice[4];
  }
  
  CStdString strVolFormat; 
  strVolFormat.Format( _T("\\\\.\\%c:" ), cDL);

  if(GetDriveStatus(strVolFormat) == 1)
    return ToggleTray(cDL);
  else 
    return S_OK;
}

extern "C"
{
  FILE *fopen_utf8(const char *_Filename, const char *_Mode)
  {
    CStdStringW wfilename, wmode;
    g_charsetConverter.utf8ToW(_Filename, wfilename, false);
    wmode = _Mode;
    return _wfopen(wfilename.c_str(), wmode.c_str());
  }
}

extern "C" {
  /*
   * Ported from NetBSD to Windows by Ron Koenderink, 2007
   */

  /*  $NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $  */

  /*-
   * Copyright (c) 1997, 1998, 2005 The NetBSD Foundation, Inc.
   * All rights reserved.
   *
   * This code was contributed to The NetBSD Foundation by Klaus Klein.
   * Heavily optimised by David Laight
   *
   * Redistribution and use in source and binary forms, with or without
   * modification, are permitted provided that the following conditions
   * are met:
   * 1. Redistributions of source code must retain the above copyright
   *    notice, this list of conditions and the following disclaimer.
   * 2. Redistributions in binary form must reproduce the above copyright
   *    notice, this list of conditions and the following disclaimer in the
   *    documentation and/or other materials provided with the distribution.
   * 3. All advertising materials mentioning features or use of this software
   *    must display the following acknowledgement:
   *        This product includes software developed by the NetBSD
   *        Foundation, Inc. and its contributors.
   * 4. Neither the name of The NetBSD Foundation nor the names of its
   *    contributors may be used to endorse or promote products derived
   *    from this software without specific prior written permission.
   *
   * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
   * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
   * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   * POSSIBILITY OF SUCH DAMAGE.
   */

  #if !defined(_WIN32)
  #include <sys/cdefs.h>
  #endif

  #if defined(LIBC_SCCS) && !defined(lint)
  __RCSID("$NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $");
  #endif

  #if !defined(_WIN32)
  #include "namespace.h"
  #include <sys/localedef.h>
  #else
  typedef unsigned char u_char;
  typedef unsigned int uint;
  #endif
  #include <ctype.h>
  #include <locale.h>
  #include <string.h>
  #include <time.h>
  #if !defined(_WIN32)
  #include <tzfile.h>
  #endif

  #ifdef __weak_alias
  __weak_alias(strptime,_strptime)
  #endif

  #if !defined(_WIN32)
  #define  _ctloc(x)    (_CurrentTimeLocale->x)
  #else
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
  char *d_t_fmt = "%a %Ef %T %Y";
  char *t_fmt_ampm = "%I:%M:%S %p";
  char *t_fmt = "%H:%M:%S";
  char *d_fmt = "%m/%d/%y";
  #define TM_YEAR_BASE 1900
  #define __UNCONST(x) ((char *)(((const char *)(x) - (const char *)0) + (char *)0))

  #endif
  /*
   * We do not implement alternate representations. However, we always
   * check whether a given modifier is allowed for a certain conversion.
   */
  #define ALT_E      0x01
  #define ALT_O      0x02
  #define  LEGAL_ALT(x)    { if (alt_format & ~(x)) return NULL; }


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
        /* FALLTHROUGH */
      case 'H':
        bp = conv_num(bp, &tm->tm_hour, 0, 23);
        LEGAL_ALT(ALT_O);
        continue;

      case 'l':  /* The hour (12-hour clock representation). */
        LEGAL_ALT(0);
        /* FALLTHROUGH */
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
    unsigned int len;

    /* check full name - then abbreviated ones */
    for (; n1 != NULL; n1 = n2, n2 = NULL) {
      for (i = 0; i < c; i++, n1++) {
        len = strlen(*n1);
        if (strnicmp(*n1, (const char *)bp, len) == 0) {
          *tgt = i;
          return bp + len;
        }
      }
    }

    /* Nothing matched */
    return NULL;
  }
}
