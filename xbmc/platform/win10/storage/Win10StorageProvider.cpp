/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "Win10StorageProvider.h"

#include "ServiceBroker.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win10/AsyncHelpers.h"
#include "platform/win10/filesystem/WinLibraryDirectory.h"
#include "platform/win32/CharsetConverter.h"

#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>

namespace winrt
{
  using namespace Windows::Foundation;
}
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;

::IStorageProvider* ::IStorageProvider::CreateInstance()
{
  return new CStorageProvider();
}

CStorageProvider::~CStorageProvider()
{
  if (m_watcher && m_watcher.Status() == DeviceWatcherStatus::Started)
    m_watcher.Stop();
}

void CStorageProvider::Initialize()
{
  m_changed = false;
  VECSOURCES vShare;
  GetDrivesByType(vShare, DVD_DRIVES);
  if (!vShare.empty())
    CServiceBroker::GetMediaManager().SetHasOpticalDrive(true);
  else
    CLog::Log(LOGDEBUG, "%s: No optical drive found.", __FUNCTION__);

  m_watcher = DeviceInformation::CreateWatcher(DeviceClass::PortableStorageDevice);
  m_watcher.Added([this](auto&&, auto&&)
  {
    m_changed = true;
  });
  m_watcher.Removed([this](auto&&, auto&&)
  {
    m_changed = true;
  });
  m_watcher.Updated([this](auto&&, auto&&)
  {
    m_changed = true;
  });
  m_watcher.Start();
}

void CStorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;
  share.strPath = CSpecialProtocol::TranslatePath("special://home");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localDrives.push_back(share);

  GetDrivesByType(localDrives, LOCAL_DRIVES, true);
}

void CStorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  using KODI::PLATFORM::WINDOWS::FromW;

  // get drives which we have direct access for (in case of broad file system access)
  GetDrivesByType(removableDrives, REMOVABLE_DRIVES, true);

  try
  {
    auto devicesView = Wait(winrt::Windows::Storage::KnownFolders::RemovableDevices().GetFoldersAsync());
    for (unsigned i = 0; i < devicesView.Size(); i++)
    {
      auto device = devicesView.GetAt(i);
      if (device.Path().empty())
        continue;
      CMediaSource source;
      source.strName = FromW(device.DisplayName().c_str());
      std::string driveLetter = FromW(device.Path().c_str());

      // skip exiting in case if we have direct access
      auto exiting = std::find_if(removableDrives.begin(), removableDrives.end(), [&driveLetter](CMediaSource& m) {
        return m.strPath == driveLetter;
      });
      if (exiting != removableDrives.end())
        continue;

      UINT uDriveType = GetDriveTypeA(driveLetter.c_str());
      source.strPath = "win-lib://removable/" + driveLetter + "/";
      source.m_iDriveType = (
        (uDriveType == DRIVE_FIXED) ? CMediaSource::SOURCE_TYPE_LOCAL :
        (uDriveType == DRIVE_REMOTE) ? CMediaSource::SOURCE_TYPE_REMOTE :
        (uDriveType == DRIVE_CDROM) ? CMediaSource::SOURCE_TYPE_DVD :
        (uDriveType == DRIVE_REMOVABLE) ? CMediaSource::SOURCE_TYPE_REMOVABLE :
        CMediaSource::SOURCE_TYPE_UNKNOWN);

      removableDrives.push_back(source);
    }
  }
  catch (const winrt::hresult_error&)
  {
  }
}

std::string CStorageProvider::GetFirstOpticalDeviceFileName()
{
  VECSOURCES vShare;
  std::string strdevice = "\\\\.\\";
  GetDrivesByType(vShare, DVD_DRIVES);

  if (!vShare.empty())
    return strdevice.append(vShare.front().strPath);
  else
    return "";
}

bool CStorageProvider::Eject(const std::string& mountpath)
{
  return false;
}

std::vector<std::string> CStorageProvider::GetDiskUsage()
{
  using KODI::PLATFORM::WINDOWS::FromW;

  std::vector<std::string> result;
  ULARGE_INTEGER ULTotal = { { 0 } };
  ULARGE_INTEGER ULTotalFree = { { 0 } };
  std::string strRet;

  auto localfolder = ApplicationData::Current().LocalFolder().Path();
  GetDiskFreeSpaceExW(localfolder.c_str(), nullptr, &ULTotal, &ULTotalFree);
  strRet = StringUtils::Format("%s: %d MB %s", g_localizeStrings.Get(21440),
                               (ULTotalFree.QuadPart / (1024 * 1024)),
                               g_localizeStrings.Get(160).c_str());
  result.push_back(strRet);

  DWORD drivesBits = GetLogicalDrives();
  if (drivesBits == 0)
    return result;

  CMediaSource share;

  drivesBits >>= 2;       // skip A and B
  char driveLetter = 'C'; // start with C
  for (; drivesBits > 0; drivesBits >>= 1, driveLetter++)
  {
    if (!(drivesBits & 1))
      continue;

    std::string strDrive = std::string(1, driveLetter) + ":\\";
    if (DRIVE_FIXED == GetDriveTypeA(strDrive.c_str())
      && GetDiskFreeSpaceExA((strDrive.c_str()), nullptr, &ULTotal, &ULTotalFree))
    {
      strRet = StringUtils::Format("%s %d MB %s", strDrive.c_str(), int(ULTotalFree.QuadPart / (1024 * 1024)), g_localizeStrings.Get(160).c_str());
      result.push_back(strRet);
    }
  }
  return result;
}

bool CStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  bool res = m_changed.load();
  m_changed = false;
  return res;
}

void CStorageProvider::GetDrivesByType(VECSOURCES & localDrives, Drive_Types eDriveType, bool bonlywithmedia)
{
  DWORD drivesBits = GetLogicalDrives();
  if (drivesBits == 0)
    return;

  CMediaSource share;
  char volumeName[100];

  drivesBits >>= 2;       // skip A and B
  char driveLetter = 'C'; // start with C
  for (; drivesBits > 0; drivesBits >>= 1, driveLetter++)
  {
    if (!(drivesBits & 1))
      continue;

    std::string strDrive = std::string(1, driveLetter) + ":\\";
    UINT uDriveType = GetDriveTypeA(strDrive.c_str());
    int nResult = GetVolumeInformationA(strDrive.c_str(), volumeName, 100, 0, 0, 0, NULL, 25);
    if (nResult == 0 && bonlywithmedia)
    {
      continue;
    }

    bool bUseDCD = false;
    // skip unsupported types
    if (uDriveType < DRIVE_REMOVABLE || uDriveType > DRIVE_CDROM)
      continue;
    // only fixed and remote
    if (eDriveType == LOCAL_DRIVES && uDriveType != DRIVE_FIXED && uDriveType != DRIVE_REMOTE)
      continue;
    // only removable
    if (eDriveType == REMOVABLE_DRIVES && uDriveType != DRIVE_REMOVABLE)
      continue;
    // only CD-ROMs
    if (eDriveType == DVD_DRIVES && uDriveType != DRIVE_CDROM)
      continue;

    share.strPath = strDrive;
    if (volumeName[0] != L'\0')
      share.strName = volumeName;
    if (uDriveType == DRIVE_CDROM && nResult)
    {
      // Has to be the same as auto mounted devices
      share.strStatus = share.strName;
      share.strName = share.strPath;
      share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
      bUseDCD = true;
    }
    else
    {
      // Lets show it, like Windows explorer do...
      switch (uDriveType)
      {
      case DRIVE_CDROM:
        share.strName = StringUtils::Format("%s (%s)", share.strPath.c_str(), g_localizeStrings.Get(218).c_str());
        break;
      case DRIVE_REMOVABLE:
        if (share.strName.empty())
          share.strName = StringUtils::Format("%s (%s)", g_localizeStrings.Get(437).c_str(), share.strPath.c_str());
        break;
      default:
        if (share.strName.empty())
          share.strName = share.strPath;
        else
          share.strName = StringUtils::Format("%s (%s)", share.strPath.c_str(), share.strName.c_str());
        break;
      }
    }

    StringUtils::Replace(share.strName, ":\\", ":");
    StringUtils::Replace(share.strPath, ":\\", ":");
    share.m_ignore = true;
    if (!bUseDCD)
    {
      share.m_iDriveType = (
        (uDriveType == DRIVE_FIXED) ? CMediaSource::SOURCE_TYPE_LOCAL :
        (uDriveType == DRIVE_REMOTE) ? CMediaSource::SOURCE_TYPE_REMOTE :
        (uDriveType == DRIVE_CDROM) ? CMediaSource::SOURCE_TYPE_DVD :
        (uDriveType == DRIVE_REMOVABLE) ? CMediaSource::SOURCE_TYPE_REMOVABLE :
        CMediaSource::SOURCE_TYPE_UNKNOWN);
    }

    AddOrReplace(localDrives, share);
  }
}

