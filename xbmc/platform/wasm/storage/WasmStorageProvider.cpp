/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WasmStorageProvider.h"

#include "filesystem/wasm/DeviceFolder.h"

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<CWasmStorageProvider>();
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
    CMediaSource folderShare;
    folderShare.strPath = folder.path;
    folderShare.strName = folder.name;
    folderShare.m_iDriveType = SourceType::LOCAL;
    localDrives.push_back(folderShare);
  }
}

std::vector<std::string> CWasmStorageProvider::GetDiskUsage()
{
  return {};
}
