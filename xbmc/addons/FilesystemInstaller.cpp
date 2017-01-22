/*
 *      Copyright (C) 2016 Team Kodi
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
#include "FilesystemInstaller.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/FileOperationJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#ifdef TARGET_POSIX
#include "XTimeUtils.h"
#endif

using namespace XFILE;

namespace
{

bool renameOrRetry(const std::string & source, const std::string & dest, const char * description)
{
  auto count = 1;
  auto result = false;
  do
  {
    result = CFile::Rename(source, dest);
    if (!result)
    {
      CLog::Log(LOGERROR, "Failed to move %s addon files from '%s' to '%s', retrying in 500ms",
			          description, source.c_str(), dest.c_str());
      Sleep(500);
    }
  } while (!result && count++ < 4);

  return result;
}

} // end namespace unnamed

CFilesystemInstaller::CFilesystemInstaller()
{
  m_addonFolder = CSpecialProtocol::TranslatePath("special://home/addons/");
  m_tempFolder = CSpecialProtocol::TranslatePath("special://home/addons/temp/");
}

bool CFilesystemInstaller::InstallToFilesystem(const std::string& archive, const std::string& addonId)
{
  auto addonFolder = URIUtils::AddFileToFolder(m_addonFolder, addonId);
  auto newAddonData = URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());
  auto oldAddonData = URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());

  if (!CDirectory::Create(newAddonData))
    return false;

  if (!UnpackArchive(archive, newAddonData))
  {
    CLog::Log(LOGERROR, "Failed to unpack archive '%s' to '%s'", archive.c_str(), newAddonData.c_str());
    return false;
  }

  bool hasOldData = CDirectory::Exists(addonFolder);
  if (hasOldData)
  {
    if (!renameOrRetry(addonFolder, oldAddonData, "old"))
      return false;
  }

  if (!renameOrRetry(newAddonData, addonFolder, "new"))
    return false;

  if (hasOldData)
  {
    if (!CDirectory::RemoveRecursive(oldAddonData))
    {
      CLog::Log(LOGWARNING, "Failed to delete old addon files in '%s'", oldAddonData.c_str());
    }
  }
  return true;
}

bool CFilesystemInstaller::UnInstallFromFilesystem(const std::string& addonFolder)
{
  auto tempFolder = URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());
  if (!CFile::Rename(addonFolder, tempFolder))
  {
    CLog::Log(LOGERROR, "Failed to move old addon files from '%s' to '%s'", addonFolder.c_str(), tempFolder.c_str());
    return false;
  }

  if (!CDirectory::RemoveRecursive(tempFolder))
  {
    CLog::Log(LOGWARNING, "Failed to delete old addon files in '%s'", tempFolder.c_str());
  }
  return true;
}

bool CFilesystemInstaller::UnpackArchive(std::string path, const std::string& dest)
{
  if (!URIUtils::IsProtocol(path, "zip"))
    path = URIUtils::CreateArchivePath("zip", CURL(path), "").Get();

  CFileItemList files;
  if (!CDirectory::GetDirectory(path, files))
    return false;

  if (files.Size() == 1 && files[0]->m_bIsFolder)
  {
    path = files[0]->GetPath();
    files.Clear();
    if (!CDirectory::GetDirectory(path, files))
      return false;
  }
  CLog::Log(LOGDEBUG, "Unpacking %s to %s", path.c_str(), dest.c_str());

  for (auto i = 0; i < files.Size(); ++i)
    files[i]->Select(true);

  CFileOperationJob job(CFileOperationJob::ActionCopy, files, dest);
  return job.DoWork();
}
