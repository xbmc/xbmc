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

#include "WIN32Util.h"
#include "settings/GUISettings.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "storage/cdioSupport.h"
#include "PowrProf.h"
#include "WindowHelper.h"
#include "Application.h"
#include <shlobj.h>
#include "filesystem/SpecialProtocol.h"
#include "my_ntddscsi.h"
#if _MSC_VER > 1400
#include "Setupapi.h"
#endif
#include "storage/MediaManager.h"
#include "windowing/WindowingFactory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "DllPaths_win32.h"
#include "FileSystem/File.h"
#include "utils/URIUtils.h"

// default Broadcom registy bits (setup when installing a CrystalHD card)
#define BC_REG_PATH       "Software\\Broadcom\\MediaPC"
#define BC_REG_PRODUCT    "CrystalHD" // 70012/70015
#define BC_BCM_DLL        "bcmDIL.dll"
#define BC_REG_INST_PATH  "InstallPath"

#define DLL_ENV_PATH "special://xbmc/system/;" \
                     "special://xbmc/system/players/dvdplayer/;" \
                     "special://xbmc/system/players/paplayer/;" \
                     "special://xbmc/system/cdrip/;" \
                     "special://xbmc/system/python/;" \
                     "special://xbmc/system/webserver/;" \
                     "special://xbmc/"

extern HWND g_hWnd;

using namespace std;
using namespace MEDIA_DETECT;

CWIN32Util::CWIN32Util(void)
{
}

CWIN32Util::~CWIN32Util(void)
{
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

int CWIN32Util::BatteryLevel()
{
  SYSTEM_POWER_STATUS SystemPowerStatus;

  if (GetSystemPowerStatus(&SystemPowerStatus) && SystemPowerStatus.BatteryLifePercent != 255)
      return SystemPowerStatus.BatteryLifePercent;

  return 0;
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

  CStdStringW WstrExe, WstrParams, WstrWorkingDir;
  g_charsetConverter.utf8ToW(strExe, WstrExe);
  g_charsetConverter.utf8ToW(strParams, WstrParams);
  g_charsetConverter.utf8ToW(strWorkingDir, WstrWorkingDir);

  bool ret;
  SHELLEXECUTEINFOW ShExecInfo = {0};
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
    // Todo: Pause music and video playback
    WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
  }

  return ret;
}

std::vector<CStdString> CWIN32Util::GetDiskUsage()
{
  vector<CStdString> result;
  ULARGE_INTEGER ULTotal= { { 0 } };
  ULARGE_INTEGER ULTotalFree= { { 0 } };

  char* pcBuffer= NULL;
  DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer );
  if( dwStrLength != 0 )
  {
    CStdString strRet;
    
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

CStdString CWIN32Util::GetSpecialFolder(int csidl)
{
  CStdString strProfilePath;
  WCHAR szPath[MAX_PATH];

  if(SUCCEEDED(SHGetFolderPathW(NULL,csidl,NULL,0,szPath)))
  {
    g_charsetConverter.wToUTF8(szPath, strProfilePath);
    strProfilePath = UncToSmb(strProfilePath);
  }  
  else
    strProfilePath = "";

  return strProfilePath;
}

CStdString CWIN32Util::GetSystemPath()
{
  return GetSpecialFolder(CSIDL_SYSTEM);
}

CStdString CWIN32Util::GetProfilePath()
{
  CStdString strProfilePath;
  CStdString strHomePath;

  CUtil::GetHomePath(strHomePath);
  
  if(g_application.PlatformDirectoriesEnabled())
    strProfilePath = URIUtils::AddFileToFolder(GetSpecialFolder(CSIDL_APPDATA|CSIDL_FLAG_CREATE), "XBMC");
  else
    strProfilePath = URIUtils::AddFileToFolder(strHomePath , "portable_data");
  
  if (strProfilePath.length() == 0)
    strProfilePath = strHomePath;

  URIUtils::AddSlashAtEnd(strProfilePath);

  return strProfilePath;
}

CStdString CWIN32Util::UncToSmb(const CStdString &strPath)
{
  CStdString strRetPath(strPath);
  if(strRetPath.Left(2).Equals("\\\\"))
  {
    strRetPath = "smb:" + strPath;
    strRetPath.Replace("\\","/");
  }
  return strRetPath;
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
  DWORD dwReq;
  char cDL = cDriveLetter;
  if( !cDL )
  {
    CStdString dvdDevice = g_mediaManager.TranslateDevicePath("");
    if(dvdDevice == "")
      return S_FALSE;
    cDL = dvdDevice[0];
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
    dwReq = (GetDriveStatus(strVolFormat) == 1) ? IOCTL_STORAGE_LOAD_MEDIA : IOCTL_STORAGE_EJECT_MEDIA;
    bRet = DeviceIoControl( hDrive, dwReq, NULL, 0, NULL, 0, &dwDummy, NULL);
    CloseHandle( hDrive );
  }
  // Windows doesn't seem to send always DBT_DEVICEREMOVECOMPLETE
  // unmount it here too as it won't hurt
  if(dwReq == IOCTL_STORAGE_EJECT_MEDIA && bRet == 1)
  {
    strRootFormat.Format( _T("%c:"), cDL);
    CMediaSource share;
    share.strPath = strRootFormat;
    share.strName = share.strPath;
    g_mediaManager.RemoveAutoSource(share);
  }
  return bRet? S_OK : S_FALSE;
}

HRESULT CWIN32Util::EjectTray(const char cDriveLetter)
{
  char cDL = cDriveLetter;
  if( !cDL )
  {
    CStdString dvdDevice = g_mediaManager.TranslateDevicePath("");
    if(dvdDevice == "")
      return S_FALSE;
    cDL = dvdDevice[0];
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
    CStdString dvdDevice = g_mediaManager.TranslateDevicePath("");
    if(dvdDevice == "")
      return S_FALSE;
    cDL = dvdDevice[0];
  }
  
  CStdString strVolFormat; 
  strVolFormat.Format( _T("\\\\.\\%c:" ), cDL);

  if(GetDriveStatus(strVolFormat) == 1)
    return ToggleTray(cDL);
  else 
    return S_OK;
}

// safe removal of USB drives: 
// http://www.codeproject.com/KB/system/RemoveDriveByLetter.aspx
// http://www.techtalkz.com/microsoft-device-drivers/250734-remove-usb-device-c-3.html

#if _MSC_VER > 1400
DEVINST CWIN32Util::GetDrivesDevInstByDiskNumber(long DiskNumber) 
{

  GUID* guid = (GUID*)(void*)&GUID_DEVINTERFACE_DISK;

  // Get device interface info set handle for all devices attached to system
  HDEVINFO hDevInfo = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

  if (hDevInfo == INVALID_HANDLE_VALUE)
    return 0;

  // Retrieve a context structure for a device interface of a device
  // information set.
  DWORD dwIndex = 0;
  SP_DEVICE_INTERFACE_DATA devInterfaceData = {0};
  devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  BOOL bRet = FALSE;

  PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd;
  SP_DEVICE_INTERFACE_DATA spdid;
  SP_DEVINFO_DATA spdd;
  DWORD dwSize;

  spdid.cbSize = sizeof(spdid);

  while ( true ) 
  {
    bRet = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, guid, dwIndex, &devInterfaceData);
    if (!bRet)
      break;

    SetupDiEnumInterfaceDevice(hDevInfo, NULL, guid, dwIndex, &spdid);

    dwSize = 0;
    SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL);

    if ( dwSize ) 
    {
      pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
      if ( pspdidd == NULL )
        continue;

      pspdidd->cbSize = sizeof(*pspdidd);
      ZeroMemory((PVOID)&spdd, sizeof(spdd));
      spdd.cbSize = sizeof(spdd);

      long res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid,
      pspdidd, dwSize, &dwSize, &spdd);
      if ( res ) 
      {
        HANDLE hDrive = CreateFile(pspdidd->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
        if ( hDrive != INVALID_HANDLE_VALUE ) 
        {
          STORAGE_DEVICE_NUMBER sdn;
          DWORD dwBytesReturned = 0;
          res = DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL);
          if ( res ) 
          {
            if ( DiskNumber == (long)sdn.DeviceNumber ) 
            {
              CloseHandle(hDrive);
              SetupDiDestroyDeviceInfoList(hDevInfo);
              return spdd.DevInst;
            }
          }
          CloseHandle(hDrive);
        }
      }
      HeapFree(GetProcessHeap(), 0, pspdidd);
    }
    dwIndex++;
  }
  SetupDiDestroyDeviceInfoList(hDevInfo);
  return 0;
}
#endif

bool CWIN32Util::EjectDrive(const char cDriveLetter)
{
#if _MSC_VER > 1400
  if( !cDriveLetter )
    return false;

  CStdString strVolFormat; 
  strVolFormat.Format( _T("\\\\.\\%c:" ), cDriveLetter);

  long DiskNumber = -1;

  HANDLE hVolume = CreateFile(strVolFormat.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
  if (hVolume == INVALID_HANDLE_VALUE)
    return false;

  STORAGE_DEVICE_NUMBER sdn;
  DWORD dwBytesReturned = 0;
  long res = DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER,NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL);
  CloseHandle(hVolume);
  if ( res )
    DiskNumber = sdn.DeviceNumber;
  else
    return false;

  DEVINST DevInst = GetDrivesDevInstByDiskNumber(DiskNumber);

  if ( DevInst == 0 )
    return false;

  ULONG Status = 0;
  ULONG ProblemNumber = 0;
  PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown;
  char VetoName[MAX_PATH];
  bool bSuccess = false;

  res = CM_Get_Parent(&DevInst, DevInst, 0); // disk's parent, e.g. the USB bridge, the SATA controller....
  res = CM_Get_DevNode_Status(&Status, &ProblemNumber, DevInst, 0);

  for(int i=0;i<3;i++)
  {
    res = CM_Request_Device_Eject(DevInst, &VetoType, VetoName, MAX_PATH, 0);
    bSuccess = (res==CR_SUCCESS && VetoType==PNP_VetoTypeUnknown);
   if ( bSuccess )
    break;
  }

  return bSuccess;
#else
  return false;
#endif
}

#ifdef HAS_GL
void CWIN32Util::CheckGLVersion()
{
  if(CWIN32Util::HasGLDefaultDrivers())
  {
    MessageBox(NULL, "MS default OpenGL drivers detected. Please get OpenGL drivers from your video card vendor", "XBMC: Fatal Error", MB_OK|MB_ICONERROR);
    exit(1);
  }

  if(!CWIN32Util::HasReqGLVersion())
  {
    if(MessageBox(NULL, "Your OpenGL version doesn't meet the XBMC requirements", "XBMC: Warning", MB_OKCANCEL|MB_ICONWARNING) == IDCANCEL)
    {
      exit(1);
    }
  }
}

bool CWIN32Util::HasGLDefaultDrivers()
{
  unsigned int a=0,b=0;

  CStdString strVendor = g_Windowing.GetRenderVendor();
  g_Windowing.GetRenderVersion(a, b);

  if(strVendor.find("Microsoft")!=strVendor.npos && a==1 && b==1)
    return true;
  else
    return false;
}

bool CWIN32Util::HasReqGLVersion()
{
  unsigned int a=0,b=0;

  g_Windowing.GetRenderVersion(a, b);
  if((a>=2) || (a == 1 && b >= 3))
    return true;
  else
    return false;
}
#endif

BOOL CWIN32Util::IsCurrentUserLocalAdministrator()
{
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
}

void CWIN32Util::GetDrivesByType(VECSOURCES &localDrives, Drive_Types eDriveType, bool bonlywithmedia)
{
  WCHAR* pcBuffer= NULL;
  DWORD dwStrLength= GetLogicalDriveStringsW( 0, pcBuffer );
  if( dwStrLength != 0 )
  {
    CMediaSource share;
    
    dwStrLength+= 1;
    pcBuffer= new WCHAR [dwStrLength];
    GetLogicalDriveStringsW( dwStrLength, pcBuffer );

    int iPos= 0;
    WCHAR cVolumeName[100];
    do{
      int nResult = 0;
      cVolumeName[0]= L'\0';

      CStdStringW strWdrive = pcBuffer + iPos;
      
      UINT uDriveType= GetDriveTypeW( strWdrive.c_str()  );
      // don't use GetVolumeInformation on fdd's as the floppy controller may be enabled in Bios but
      // no floppy HW is attached which causes huge delays.
      if(!strWdrive.Left(2).Equals(L"A:") && !strWdrive.Left(2).Equals(L"B:"))
        nResult= GetVolumeInformationW( strWdrive.c_str() , cVolumeName, 100, 0, 0, 0, NULL, 25);
      if(nResult == 0 && bonlywithmedia)
      {
        iPos += (wcslen( pcBuffer + iPos) + 1 );
        continue;
      }

      // usb hard drives are reported as DRIVE_FIXED and won't be returned by queries with REMOVABLE_DRIVES set
      // so test for usb hard drives
      /*if(uDriveType == DRIVE_FIXED)
      {
        if(IsUsbDevice(strWdrive))
          uDriveType = DRIVE_REMOVABLE;
      }*/

      share.strPath= share.strName= "";

      bool bUseDCD= false;
      if( uDriveType > DRIVE_UNKNOWN && 
        (( eDriveType == ALL_DRIVES && (uDriveType == DRIVE_FIXED || uDriveType == DRIVE_REMOTE || uDriveType == DRIVE_CDROM || uDriveType == DRIVE_REMOVABLE )) ||
         ( eDriveType == LOCAL_DRIVES && (uDriveType == DRIVE_FIXED || uDriveType == DRIVE_REMOTE)) ||
         ( eDriveType == REMOVABLE_DRIVES && ( uDriveType == DRIVE_REMOVABLE )) ||
         ( eDriveType == DVD_DRIVES && ( uDriveType == DRIVE_CDROM ))))
      {
        //share.strPath = strWdrive;
        g_charsetConverter.wToUTF8(strWdrive, share.strPath);
        if( cVolumeName[0] != L'\0' )
          g_charsetConverter.wToUTF8(cVolumeName, share.strName);
        if( uDriveType == DRIVE_CDROM && nResult)
        {
          // Has to be the same as auto mounted devices
          share.strStatus = share.strName;
          share.strName = share.strPath;
          share.m_iDriveType= CMediaSource::SOURCE_TYPE_LOCAL;
          bUseDCD= true;
        }
        else
        {
          // Lets show it, like Windows explorer do... TODO: sorting should depend on driver letter
          switch(uDriveType)
          {
          case DRIVE_CDROM:
            share.strName.Format( "%s (%s)", share.strPath, g_localizeStrings.Get(218));
            break;
          case DRIVE_REMOVABLE:
            if(share.strName.IsEmpty())
              share.strName.Format( "%s (%s)", g_localizeStrings.Get(437), share.strPath);
            break;
          case DRIVE_UNKNOWN:
            share.strName.Format( "%s (%s)", share.strPath, g_localizeStrings.Get(13205));
            break;
          default:
            if(share.strName.empty())
              share.strName = share.strPath;
            else
              share.strName.Format( "%s (%s)", share.strPath, share.strName);
            break;
          }
        }
        share.strName.Replace(":\\",":");
        share.strPath.Replace(":\\",":");
        share.m_ignore= true;
        if( !bUseDCD )
        {
          share.m_iDriveType= (
           ( uDriveType == DRIVE_FIXED  )    ? CMediaSource::SOURCE_TYPE_LOCAL :
           ( uDriveType == DRIVE_REMOTE )    ? CMediaSource::SOURCE_TYPE_REMOTE :
           ( uDriveType == DRIVE_CDROM  )    ? CMediaSource::SOURCE_TYPE_DVD :
           ( uDriveType == DRIVE_REMOVABLE ) ? CMediaSource::SOURCE_TYPE_REMOVABLE :
             CMediaSource::SOURCE_TYPE_UNKNOWN );
        }

        AddOrReplace(localDrives, share);
      }
      iPos += (wcslen( pcBuffer + iPos) + 1 );
    } while( wcslen( pcBuffer + iPos ) > 0 );
    delete[] pcBuffer;
  }
}

bool CWIN32Util::IsAudioCD(const CStdString& strPath)
{
  CStdString strDrive = strPath;
  char cVolumenName[256];
  char cFSName[256];
  URIUtils::AddSlashAtEnd(strDrive);
  if(GetVolumeInformation(strDrive.c_str(), cVolumenName, 255, NULL, NULL, NULL, cFSName, 255)==0)
    return false;
  return (strncmp(cFSName,"CDFS",4)==0 && strncmp(cVolumenName,"Audio CD",4)==0);
}

CStdString CWIN32Util::GetDiskLabel(const CStdString& strPath)
{
  CStdString strDrive = strPath;
  char cVolumenName[128];
  char cFSName[128];
  URIUtils::AddSlashAtEnd(strDrive);
  if(GetVolumeInformation(strDrive.c_str(), cVolumenName, 127, NULL, NULL, NULL, cFSName, 127)==0)
    return "";
  return CStdString(cVolumenName).TrimRight(" ");
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


bool CWIN32Util::Is64Bit()
{
  bool bRet= false;
  typedef VOID (WINAPI *LPFN_GETNATIVESYSTEMINFO) (LPSYSTEM_INFO);
  LPFN_GETNATIVESYSTEMINFO fnGetNativeSystemInfo= ( LPFN_GETNATIVESYSTEMINFO ) GetProcAddress( GetModuleHandle( TEXT( "kernel32" ) ), "GetNativeSystemInfo" );
  SYSTEM_INFO sSysInfo;
  memset( &sSysInfo, 0, sizeof( sSysInfo ) );
  if (fnGetNativeSystemInfo != NULL)
  {
    fnGetNativeSystemInfo(&sSysInfo);
    if (sSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) bRet= true;
  }
  return bRet;
}

LONG CWIN32Util::UtilRegGetValue( const HKEY hKey, const char *const pcKey, DWORD *const pdwType, char **const ppcBuffer, DWORD *const pdwSizeBuff, const DWORD dwSizeAdd )
{
  DWORD dwSize;
  LONG lRet= RegQueryValueEx(hKey, pcKey, NULL, pdwType, NULL, &dwSize );
  if (lRet == ERROR_SUCCESS)
  {
    if (ppcBuffer)
    {
      char *pcValue=*ppcBuffer;
      if (!pcValue || !pdwSizeBuff || dwSize +dwSizeAdd > *pdwSizeBuff) pcValue= (char*)realloc(pcValue, dwSize +dwSizeAdd);
      lRet= RegQueryValueEx(hKey,pcKey,NULL,NULL,(LPBYTE)pcValue,&dwSize);

      if ( lRet == ERROR_SUCCESS || *ppcBuffer ) *ppcBuffer= pcValue;
      else free( pcValue );
    }
    if (pdwSizeBuff) *pdwSizeBuff= dwSize +dwSizeAdd;
  }
  return lRet;
}

bool CWIN32Util::UtilRegOpenKeyEx( const HKEY hKeyParent, const char *const pcKey, const REGSAM rsAccessRights, HKEY *hKey, const bool bReadX64 )
{
  const REGSAM rsAccessRightsTmp= ( Is64Bit() ? rsAccessRights | ( bReadX64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY ) : rsAccessRights );
  bool bRet= ( ERROR_SUCCESS == RegOpenKeyEx(hKeyParent, pcKey, 0, rsAccessRightsTmp, hKey));
  return bRet;
}

bool CWIN32Util::GetCrystalHDLibraryPath(CStdString &strPath)
{
  // support finding library by windows registry
  HKEY hKey;
  CStdString strRegKey;

  CLog::Log(LOGDEBUG, "CrystalHD: detecting CrystalHD installation path");
  strRegKey.Format("%s\\%s", BC_REG_PATH, BC_REG_PRODUCT );

  if( CWIN32Util::UtilRegOpenKeyEx( HKEY_LOCAL_MACHINE, strRegKey.c_str(), KEY_READ, &hKey ))
  {
    DWORD dwType;
    char *pcPath= NULL;
    if( CWIN32Util::UtilRegGetValue( hKey, BC_REG_INST_PATH, &dwType, &pcPath, NULL, sizeof( pcPath ) ) == ERROR_SUCCESS )
    {
      strPath = URIUtils::AddFileToFolder(pcPath, BC_BCM_DLL);
      CLog::Log(LOGDEBUG, "CrystalHD: got CrystalHD installation path (%s)", strPath.c_str());
      return true;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CrystalHD: getting CrystalHD installation path failed");
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "CrystalHD: CrystalHD software seems to be not installed.");
  }
  // check for dll in system dir
  if(XFILE::CFile::Exists(DLL_PATH_LIBCRYSTALHD))
    return true;
  else
    return false;
}

// Retrieve the filename of the process that currently has the focus.
// Typically this will be some process using the system tray grabbing
// the focus and causing XBMC to minimise. Logging the offending
// process name can help the user fix the problem.
bool CWIN32Util::GetFocussedProcess(CStdString &strProcessFile)
{
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
  char procfile[MAX_PATH+1];
  DWORD procfilelen = MAX_PATH;

  HINSTANCE hkernel32 = LoadLibrary("kernel32.dll");
  if (hkernel32)
  {
    DWORD (WINAPI *pQueryFullProcessImageNameA)(HANDLE,DWORD,LPTSTR,PDWORD);
    pQueryFullProcessImageNameA = (DWORD (WINAPI *)(HANDLE,DWORD,LPTSTR,PDWORD)) GetProcAddress(hkernel32, "QueryFullProcessImageNameA");
    if (pQueryFullProcessImageNameA)
      if (pQueryFullProcessImageNameA(hproc, 0, procfile, &procfilelen))
        strProcessFile = procfile;
    FreeLibrary(hkernel32);
  }

  // If QueryFullProcessImageName failed fall back to GetModuleFileNameEx.
  // Note this does not work across x86-x64 boundaries.
  if (strProcessFile == "")
  {
    HINSTANCE hpsapi = LoadLibrary("psapi.dll");
    if (hpsapi)
    {
      DWORD (WINAPI *pGetModuleFileNameExA)(HANDLE,HMODULE,LPTSTR,DWORD); 
      pGetModuleFileNameExA = (DWORD (WINAPI*)(HANDLE,HMODULE,LPTSTR,DWORD)) GetProcAddress(hpsapi, "GetModuleFileNameExA");
      if (pGetModuleFileNameExA)
        if (pGetModuleFileNameExA(hproc, NULL, procfile, MAX_PATH))
          strProcessFile = procfile;
      FreeLibrary(hpsapi);
    }
  }

  CloseHandle(hproc);

  return true;
}

// Adjust the src rectangle so that the dst is always contained in the target rectangle.
void CWIN32Util::CropSource(CRect& src, CRect& dst, CRect target)
{
  if(dst.x1 < target.x1)
  {
    src.x1 -= (dst.x1 - target.x1)
            * (src.x2 - src.x1)
            / (dst.x2 - dst.x1);
    dst.x1  = target.x1;
  }
  if(dst.y1 < target.y1)
  {
    src.y1 -= (dst.y1 - target.y1)
            * (src.y2 - src.y1)
            / (dst.y2 - dst.y1);
    dst.y1  = target.y1;
  }
  if(dst.x2 > target.x2)
  {
    src.x2 -= (dst.x2 - target.x2)
            * (src.x2 - src.x1)
            / (dst.x2 - dst.x1);
    dst.x2  = target.x2;
  }
  if(dst.y2 > target.y2)
  {
    src.y2 -= (dst.y2 - target.y2)
            * (src.y2 - src.y1)
            / (dst.y2 - dst.y1);
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

void CWinIdleTimer::StartZero()
{
  SetThreadExecutionState(ES_SYSTEM_REQUIRED);
  CStopWatch::StartZero();
}

extern "C"
{
  /* case-independent string matching, similar to strstr but
  * matching */
  char * strcasestr(const char* haystack, const char* needle) 
  {
    int i;
    int nlength = (int) strlen (needle);
    int hlength = (int) strlen (haystack);

    if (nlength > hlength) return NULL;
    if (hlength <= 0) return NULL;
    if (nlength <= 0) return (char *)haystack;
    /* hlength and nlength > 0, nlength <= hlength */
    for (i = 0; i <= (hlength - nlength); i++) 
    {
      if (strncasecmp (haystack + i, needle, nlength) == 0) 
      {
        return (char *)haystack + i;
      }
    }
    /* substring not found */
    return NULL;
  }
}

// detect if a drive is a usb device
// code taken from http://banderlogi.blogspot.com/2011/06/enum-drive-letters-attached-for-usb.html

bool CWIN32Util::IsUsbDevice(const CStdStringW &strWdrive)
{
  CStdStringW strWDevicePath;
  strWDevicePath.Format(L"\\\\.\\%s",strWdrive.Left(2));
 
  HANDLE deviceHandle = CreateFileW(
    strWDevicePath.c_str(),
   0,                // no access to the drive
   FILE_SHARE_READ | // share mode
   FILE_SHARE_WRITE,
   NULL,             // default security attributes
   OPEN_EXISTING,    // disposition
   0,                // file attributes
   NULL);            // do not copy file attributes

  if(deviceHandle == INVALID_HANDLE_VALUE)
    return false;
 
  // setup query
  STORAGE_PROPERTY_QUERY query;
  memset(&query, 0, sizeof(query));
  query.PropertyId = StorageDeviceProperty;
  query.QueryType = PropertyStandardQuery;
   
  // issue query
  DWORD bytes;
  STORAGE_DEVICE_DESCRIPTOR devd;
  STORAGE_BUS_TYPE busType = BusTypeUnknown;
 
  if (DeviceIoControl(deviceHandle,
   IOCTL_STORAGE_QUERY_PROPERTY,
   &query, sizeof(query),
   &devd, sizeof(devd),
   &bytes, NULL))
  {
   busType = devd.BusType;
  }
   
  CloseHandle(deviceHandle);
 
  return BusTypeUsb == busType;
 }