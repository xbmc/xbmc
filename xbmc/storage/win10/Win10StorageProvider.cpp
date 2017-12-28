/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "Win10StorageProvider.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/win10/WinLibraryDirectory.h"
#include "platform/win32/CharsetConverter.h"
#include "storage/MediaManager.h"
#include "utils/JobManager.h"
#include "utils/log.h"

void CStorageProvider::Initialize()
{
  // TODO check for a optical drive (available on desktop)
  g_mediaManager.SetHasOpticalDrive(false);
}

void CStorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;
  share.strPath = CSpecialProtocol::TranslatePath("special://home");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;

  localDrives.push_back(share);
}

void CStorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
}

std::string CStorageProvider::GetFirstOpticalDeviceFileName()
{
  return "";
}

bool CStorageProvider::Eject(const std::string& mountpath)
{
  return false;
}

std::vector<std::string > CStorageProvider::GetDiskUsage()
{
  std::vector<std::string> result;
  ULARGE_INTEGER ULTotal = { { 0 } };
  ULARGE_INTEGER ULTotalFree = { { 0 } };
  std::string strRet;

  Platform::String^ localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
  std::wstring folderNameW(localfolder->Data());

  GetDiskFreeSpaceExW(folderNameW.c_str(), nullptr, &ULTotal, &ULTotalFree);
  strRet = KODI::PLATFORM::WINDOWS::FromW(StringUtils::Format(L"%d MB %s", (ULTotalFree.QuadPart / (1024 * 1024)), g_localizeStrings.Get(160).c_str()));
  result.push_back(strRet);

  return result;
}

bool CStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  return false;
}

