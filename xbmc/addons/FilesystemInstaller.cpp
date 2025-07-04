/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "FilesystemInstaller.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/FileOperationJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

namespace
{
bool UnpackArchive(std::string path, const std::string& dest)
{
  if (!URIUtils::IsProtocol(path, "zip"))
    path = URIUtils::CreateArchivePath("zip", CURL(path), "").Get();

  CFileItemList files;
  if (!CDirectory::GetDirectory(path, files, "", DIR_FLAG_DEFAULTS))
    return false;

  if (files.Size() == 1 && files[0]->IsFolder())
  {
    path = files[0]->GetPath();
    files.Clear();
    if (!CDirectory::GetDirectory(path, files, "", DIR_FLAG_DEFAULTS))
      return false;
  }
  CLog::Log(LOGDEBUG, "Unpacking {} to {}", path, dest);

  for (auto i = 0; i < files.Size(); ++i)
    files[i]->Select(true);

  CFileOperationJob job(CFileOperationJob::ActionCopy, files, dest);
  return job.DoWork();
}
} // unnamed namespace

CFilesystemInstaller::CFilesystemInstaller()
  : m_addonFolder(CSpecialProtocol::TranslatePath("special://home/addons/")),
    m_tempFolder(CSpecialProtocol::TranslatePath("special://home/addons/temp/"))
{
}

bool CFilesystemInstaller::InstallToFilesystem(const std::string& archive,
                                               const std::string& addonId) const
{
  const std::string addonFolder = URIUtils::AddFileToFolder(m_addonFolder, addonId);
  const std::string newAddonData =
      URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());
  const std::string oldAddonData =
      URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());

  if (!CDirectory::Create(newAddonData))
    return false;

  if (!UnpackArchive(archive, newAddonData))
  {
    CLog::Log(LOGERROR, "Failed to unpack archive '{}' to '{}'", archive, newAddonData);
    return false;
  }

  const bool hasOldData = CDirectory::Exists(addonFolder);
  if (hasOldData && !CFile::Rename(addonFolder, oldAddonData))
  {
    CLog::Log(LOGERROR, "Failed to move old addon files from '{}' to '{}'", addonFolder,
              oldAddonData);
    return false;
  }

  if (!CFile::Rename(newAddonData, addonFolder))
  {
    CLog::Log(LOGERROR, "Failed to move new addon files from '{}' to '{}'", newAddonData,
              addonFolder);
    return false;
  }

  if (hasOldData && !CDirectory::RemoveRecursive(oldAddonData))
  {
    CLog::Log(LOGWARNING, "Failed to delete old addon files in '{}'", oldAddonData);
  }
  return true;
}

bool CFilesystemInstaller::UnInstallFromFilesystem(const std::string& addonFolder) const
{
  const std::string tempFolder = URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());
  if (!CFile::Rename(addonFolder, tempFolder))
  {
    CLog::Log(LOGERROR, "Failed to move old addon files from '{}' to '{}'", addonFolder,
              tempFolder);
    return false;
  }

  if (!CDirectory::RemoveRecursive(tempFolder))
  {
    CLog::Log(LOGWARNING, "Failed to delete old addon files in '{}'", tempFolder);
  }
  return true;
}
