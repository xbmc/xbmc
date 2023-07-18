/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "Win32StorageProvider.h"

#include "ServiceBroker.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <SetupAPI.h>
#include <ShlObj.h>
#include <winioctl.h>

bool CWin32StorageProvider::xbevent = false;

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<CWin32StorageProvider>();
}

void CWin32StorageProvider::Initialize()
{
  // check for a DVD drive
  VECSOURCES vShare;
  GetDrivesByType(vShare, DVD_DRIVES);
  if(!vShare.empty())
    CServiceBroker::GetMediaManager().SetHasOpticalDrive(true);
  else
    CLog::Log(LOGDEBUG, "{}: No optical drive found.", __FUNCTION__);

#ifdef HAS_OPTICAL_DRIVE
  // Can be removed once the StorageHandler supports optical media
  for (const auto& it : vShare)
    if (CServiceBroker::GetMediaManager().GetDriveStatus(it.strPath) ==
        DriveState::CLOSED_MEDIA_PRESENT)
      CServiceBroker::GetJobManager()->AddJob(new CDetectDisc(it.strPath, false), nullptr);
      // remove end
#endif
}

void CWin32StorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;
  wchar_t profilePath[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PROFILE, nullptr, 0, profilePath)) ||
      GetEnvironmentVariable(L"USERPROFILE", profilePath, MAX_PATH) > 0)
    share.strPath = KODI::PLATFORM::WINDOWS::FromW(profilePath);
  else
    share.strPath = CSpecialProtocol::TranslatePath("special://home");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localDrives.push_back(share);

  GetDrivesByType(localDrives, LOCAL_DRIVES);
}

void CWin32StorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  GetDrivesByType(removableDrives, REMOVABLE_DRIVES, true);
}

std::string CWin32StorageProvider::GetFirstOpticalDeviceFileName()
{
  VECSOURCES vShare;
  std::string strdevice = "\\\\.\\";
  GetDrivesByType(vShare, DVD_DRIVES);

  if(!vShare.empty())
    return strdevice.append(vShare.front().strPath);
  else
    return "";
}

bool CWin32StorageProvider::Eject(const std::string& mountpath)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  if (mountpath.empty())
  {
    return false;
  }

  if( !mountpath[0] )
    return false;

  auto strVolFormat = ToW(StringUtils::Format("\\\\.\\{}:", mountpath[0]));

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
  wchar_t VetoName[MAX_PATH];
  bool bSuccess = false;

  CM_Get_Parent(&DevInst, DevInst, 0); // disk's parent, e.g. the USB bridge, the SATA controller....
  CM_Get_DevNode_Status(&Status, &ProblemNumber, DevInst, 0);

  for(int i=0;i<3;i++)
  {
    res = CM_Request_Device_Eject(DevInst, &VetoType, VetoName, MAX_PATH, 0);
    bSuccess = (res==CR_SUCCESS && VetoType==PNP_VetoTypeUnknown);
   if ( bSuccess )
    break;
  }

  return bSuccess;
}

std::vector<std::string > CWin32StorageProvider::GetDiskUsage()
{
  std::vector<std::string> result;
  ULARGE_INTEGER ULTotal = { { 0 } };
  ULARGE_INTEGER ULTotalFree = { { 0 } };

  std::unique_ptr<wchar_t[]> pcBuffer;
  DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer.get() );
  if( dwStrLength != 0 )
  {
    std::string strRet;

    dwStrLength+= 1;
    pcBuffer.reset(new wchar_t[dwStrLength]);
    GetLogicalDriveStrings( dwStrLength, pcBuffer.get() );
    int iPos= 0;
    do
    {
      std::wstring strDrive = pcBuffer.get() + iPos;
      if( DRIVE_FIXED == GetDriveType( strDrive.c_str()  ) &&
        GetDiskFreeSpaceEx( ( strDrive.c_str() ), nullptr, &ULTotal, &ULTotalFree ) )
      {
        strRet = KODI::PLATFORM::WINDOWS::FromW(
            StringUtils::Format(L"{} {} MB {}", strDrive, int(ULTotalFree.QuadPart / (1024 * 1024)),
                                KODI::PLATFORM::WINDOWS::ToW(g_localizeStrings.Get(160))));
        result.push_back(strRet);
      }
      iPos += (wcslen( pcBuffer.get() + iPos) + 1 );
    }while( wcslen( pcBuffer.get() + iPos ) > 0 );
  }
  return result;
}

bool CWin32StorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  bool b = xbevent;
  xbevent = false;
  return b;
}

void CWin32StorageProvider::GetDrivesByType(VECSOURCES &localDrives, Drive_Types eDriveType, bool bonlywithmedia)
{
  using KODI::PLATFORM::WINDOWS::FromW;

  std::unique_ptr<wchar_t[]> pcBuffer;
  DWORD dwStrLength= GetLogicalDriveStringsW( 0, pcBuffer.get() );
  if( dwStrLength != 0 )
  {
    CMediaSource share;

    dwStrLength+= 1;
    pcBuffer.reset(new wchar_t[dwStrLength]);
    GetLogicalDriveStringsW( dwStrLength, pcBuffer.get() );

    int iPos= 0;
    do{
      std::wstring strWdrive = pcBuffer.get() + iPos;
      std::wstring letter;
      if (strWdrive.size() >= 2)
        letter = strWdrive.substr(0, 2);

      UINT uDriveType = GetDriveTypeW(strWdrive.c_str());

      share.strPath= share.strName= "";

      bool bUseDCD= false;
      if( uDriveType > DRIVE_UNKNOWN &&
        (( eDriveType == ALL_DRIVES && (uDriveType == DRIVE_FIXED || uDriveType == DRIVE_REMOTE || uDriveType == DRIVE_CDROM || uDriveType == DRIVE_REMOVABLE )) ||
         ( eDriveType == LOCAL_DRIVES && (uDriveType == DRIVE_FIXED || uDriveType == DRIVE_REMOTE)) ||
         ( eDriveType == REMOVABLE_DRIVES && ( uDriveType == DRIVE_REMOVABLE )) ||
         ( eDriveType == DVD_DRIVES && ( uDriveType == DRIVE_CDROM ))))
      {
        std::wstring remoteName;
        // for remote drives (mapped network shares) use "remote name" instead of "volume name"
        // GetVolumeInformation fails on inaccessible network drive letters and cause delays
        if (uDriveType == DRIVE_REMOTE)
        {
          wchar_t cRemoteName[MAX_PATH] = {};
          DWORD len = sizeof(cRemoteName) / sizeof(wchar_t);
          if (NO_ERROR != WNetGetConnectionW(letter.c_str(), cRemoteName, &len))
          {
            iPos += (wcslen(pcBuffer.get() + iPos) + 1);
            continue;
          }
          remoteName = cRemoteName;
        }
        wchar_t cVolumeName[100] = {};
        int nResult = 0;
        // don't use GetVolumeInformation on fdd's as the floppy controller may be enabled in Bios but
        // no floppy HW is attached which causes huge delays.
        if (uDriveType != DRIVE_REMOTE && !letter.empty() && letter != L"A:" && letter != L"B:")
        {
          DWORD len = sizeof(cVolumeName) / sizeof(wchar_t);
          nResult = GetVolumeInformationW(strWdrive.c_str(), cVolumeName, len, 0, 0, 0, nullptr, 0);
        }
        if (nResult == 0 && bonlywithmedia)
        {
          iPos += (wcslen(pcBuffer.get() + iPos) + 1);
          continue;
        }

        share.strPath = FromW(strWdrive);

        if (uDriveType == DRIVE_REMOTE)
          share.strName = FromW(remoteName);
        else if (cVolumeName[0] != L'\0')
          share.strName = FromW(cVolumeName);

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
          // Lets show it, like Windows explorer do...
          //! @todo Sorting should depend on driver letter
          switch(uDriveType)
          {
          case DRIVE_CDROM:
            share.strName =
                StringUtils::Format("{} ({})", share.strPath, g_localizeStrings.Get(218));
            break;
          case DRIVE_REMOVABLE:
            if(share.strName.empty())
              share.strName =
                  StringUtils::Format("{} ({})", g_localizeStrings.Get(437), share.strPath);
            break;
          default:
            if(share.strName.empty())
              share.strName = share.strPath;
            else
              share.strName = StringUtils::Format("{} ({})", share.strPath, share.strName);
            break;
          }
        }
        StringUtils::Replace(share.strName, ":\\", ":");
        StringUtils::Replace(share.strPath, ":\\", ":");
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
      iPos += (wcslen( pcBuffer.get() + iPos) + 1 );
    } while( wcslen( pcBuffer.get() + iPos ) > 0 );
  }
}

// safe removal of USB drives:
// http://www.codeproject.com/KB/system/RemoveDriveByLetter.aspx
// http://www.techtalkz.com/microsoft-device-drivers/250734-remove-usb-device-c-3.html
DEVINST CWin32StorageProvider::GetDrivesDevInstByDiskNumber(long DiskNumber)
{

  GUID* guid = (GUID*)(void*)&GUID_DEVINTERFACE_DISK;

  // Get device interface info set handle for all devices attached to system
  HDEVINFO hDevInfo = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

  if (hDevInfo == INVALID_HANDLE_VALUE)
    return 0;

  // Retrieve a context structure for a device interface of a device
  // information set.
  DWORD dwIndex = 0;
  SP_DEVICE_INTERFACE_DATA devInterfaceData = {};
  devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  BOOL bRet = FALSE;

  PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd;
  SP_DEVICE_INTERFACE_DATA spdid;
  SP_DEVINFO_DATA spdd;
  DWORD dwSize;

  spdid.cbSize = sizeof(spdid);

  while (true)
  {
    bRet = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, guid, dwIndex, &devInterfaceData);
    if (!bRet)
      break;

    SetupDiEnumInterfaceDevice(hDevInfo, NULL, guid, dwIndex, &spdid);

    dwSize = 0;
    SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL);

    if (dwSize)
    {
      pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
      if (pspdidd == NULL)
        continue;

      pspdidd->cbSize = sizeof(*pspdidd);
      memset(&spdd, 0, sizeof(spdd));
      spdd.cbSize = sizeof(spdd);

      long res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid,
        pspdidd, dwSize, &dwSize, &spdd);
      if (res)
      {
        HANDLE hDrive = CreateFile(pspdidd->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
        if (hDrive != INVALID_HANDLE_VALUE)
        {
          STORAGE_DEVICE_NUMBER sdn;
          DWORD dwBytesReturned = 0;
          res = DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL);
          if (res)
          {
            if (DiskNumber == (long)sdn.DeviceNumber)
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

CDetectDisc::CDetectDisc(const std::string &strPath, const bool bautorun)
  : m_strPath(strPath), m_bautorun(bautorun)
{
}

bool CDetectDisc::DoWork()
{
#ifdef HAS_OPTICAL_DRIVE
  CLog::Log(LOGDEBUG, "{}: Optical media found in drive {}", __FUNCTION__, m_strPath);
  CMediaSource share;
  share.strPath = m_strPath;
  share.strStatus = CServiceBroker::GetMediaManager().GetDiskLabel(share.strPath);
  share.strDiskUniqueId = CServiceBroker::GetMediaManager().GetDiskUniqueId(share.strPath);
  if (CServiceBroker::GetMediaManager().IsAudio(share.strPath))
    share.strStatus = "Audio-CD";
  else if(share.strStatus == "")
    share.strStatus = g_localizeStrings.Get(446);
  share.strName = share.strPath;
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
  CServiceBroker::GetMediaManager().AddAutoSource(share, m_bautorun);
#endif
  return true;
}
