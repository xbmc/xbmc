/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WasmStorageProvider.h"

#include <algorithm>
#include <mutex>
#include <utility>

namespace
{
std::vector<XFILE::DeviceFolder> GetStorages()
{
  std::vector<XFILE::DeviceFolder> storages = XFILE::CDeviceFolderDirectory::GetFolders();
  std::erase_if(storages, [](const XFILE::DeviceFolder& folder) { return !folder.removable; });
  return storages;
}
} // namespace

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<CWasmStorageProvider>();
}

void CWasmStorageProvider::Initialize()
{
  XFILE::CDeviceFolderDirectory::WatchStorages();
  std::vector<XFILE::DeviceFolder> storages = GetStorages();

  std::unique_lock lock(m_storagesLock);
  m_storages = std::move(storages);
}

void CWasmStorageProvider::GetLocalDrives(std::vector<CMediaSource>& localDrives)
{
  CMediaSource share;
  share.strPath = "/";
  share.strName = "Root";
  share.m_iDriveType = SourceType::LOCAL;
  localDrives.push_back(share);

  for (const XFILE::DeviceFolder& folder : XFILE::CDeviceFolderDirectory::GetFolders())
  {
    if (folder.removable)
      continue;

    CMediaSource folderShare;
    folderShare.strPath = folder.path;
    folderShare.strName = folder.name;
    folderShare.m_iDriveType = SourceType::LOCAL;
    localDrives.push_back(folderShare);
  }
}

void CWasmStorageProvider::GetRemovableDrives(std::vector<CMediaSource>& removableDrives)
{
  std::unique_lock lock(m_storagesLock);
  for (const XFILE::DeviceFolder& storage : m_storages)
  {
    CMediaSource share;
    share.strPath = storage.path;
    share.strName = storage.name;
    share.m_iDriveType = SourceType::REMOVABLE;
    removableDrives.push_back(share);
  }
}

bool CWasmStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback* callback)
{
  if (!XFILE::CDeviceFolderDirectory::HaveStoragesChanged())
    return false;

  const std::vector<XFILE::DeviceFolder> current = GetStorages();
  std::vector<XFILE::DeviceFolder> previous;
  {
    std::unique_lock lock(m_storagesLock);
    previous = std::exchange(m_storages, current);
  }

  if (callback)
  {
    for (const XFILE::DeviceFolder& storage : current)
    {
      if (std::ranges::find(previous, storage) == previous.end())
        callback->OnStorageAdded({storage.name, storage.path});
    }
    for (const XFILE::DeviceFolder& storage : previous)
    {
      if (std::ranges::find(current, storage) == current.end())
        callback->OnStorageSafelyRemoved({storage.name, storage.path});
    }
  }
  return current != previous;
}

std::vector<std::string> CWasmStorageProvider::GetDiskUsage()
{
  return {};
}
