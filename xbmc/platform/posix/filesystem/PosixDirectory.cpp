/*
 *      Copyright (C) 2014 Team XBMC
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

#if defined(TARGET_POSIX)

#include "PosixDirectory.h"
#include "utils/AliasShortcutUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "platform/linux/XTimeUtils.h"
#include "URL.h"

#include <dirent.h>
#include <sys/stat.h>

using namespace XFILE;

CPosixDirectory::CPosixDirectory(void) = default;

CPosixDirectory::~CPosixDirectory(void) = default;

bool CPosixDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string root = url.Get();

  if (IsAliasShortcut(root, true))
    TranslateAliasShortcut(root);

  DIR *dir = opendir(root.c_str());
  if (!dir)
    return false;

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    std::string itemLabel(entry->d_name);
    CCharsetConverter::unknownToUTF8(itemLabel);
    CFileItemPtr pItem(new CFileItem(itemLabel));
    std::string itemPath(URIUtils::AddFileToFolder(root, entry->d_name));

    bool bStat = false;
    struct stat buffer;

    // Unix-based readdir implementations may return an incorrect dirent.d_ino value that
    // is not equal to the (correct) stat() obtained one. In this case the file type
    // could not be determined and the value of dirent.d_type is set to DT_UNKNOWN.
    // In order to get a correct value we have to incur the cost of calling stat.
    if (entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK)
    {
      if (stat(itemPath.c_str(), &buffer) == 0)
        bStat = true;
    }

    if (entry->d_type == DT_DIR || (bStat && S_ISDIR(buffer.st_mode)))
    {
      pItem->m_bIsFolder = true;
      URIUtils::AddSlashAtEnd(itemPath);
    }
    else
    {
      pItem->m_bIsFolder = false;
    }

    if (StringUtils::StartsWith(entry->d_name, "."))
      pItem->SetProperty("file:hidden", true);

    pItem->SetPath(itemPath);

    if (!(m_flags & DIR_FLAG_NO_FILE_INFO))
    {
      if (bStat || stat(pItem->GetPath().c_str(), &buffer) == 0)
      {
        FILETIME fileTime, localTime;
        TimeTToFileTime(buffer.st_mtime, &fileTime);
        FileTimeToLocalFileTime(&fileTime, &localTime);
        pItem->m_dateTime = localTime;

        if (!pItem->m_bIsFolder)
          pItem->m_dwSize = buffer.st_size;
      }
    }
    items.Add(pItem);
  }
  closedir(dir);
  return true;
}

bool CPosixDirectory::Create(const CURL& url)
{
  if (!Create(url.Get()))
    return Exists(url);

  return true;
}

bool CPosixDirectory::Create(std::string path)
{
  if (mkdir(path.c_str(), 0755) != 0)
  {
    if (errno == ENOENT)
    {
      auto sep = path.rfind('/');
      if (sep == std::string::npos)
        return false;

      if (Create(path.substr(0, sep)))
        return Create(path);
    }

    return false;
  }
  return true;
}

bool CPosixDirectory::Remove(const CURL& url)
{
  if (rmdir(url.Get().c_str()) == 0)
    return true;

  return !Exists(url);
}

bool CPosixDirectory::RemoveRecursive(const CURL& url)
{
  std::string root = url.Get();

  if (IsAliasShortcut(root, true))
    TranslateAliasShortcut(root);

  DIR *dir = opendir(root.c_str());
  if (!dir)
    return false;

  bool success(true);
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    std::string itemLabel(entry->d_name);
    CCharsetConverter::unknownToUTF8(itemLabel);
    std::string itemPath(URIUtils::AddFileToFolder(root, entry->d_name));

    bool bStat = false;
    struct stat buffer;

    // Unix-based readdir implementations may return an incorrect dirent.d_ino value that
    // is not equal to the (correct) stat() obtained one. In this case the file type
    // could not be determined and the value of dirent.d_type is set to DT_UNKNOWN.
    // In order to get a correct value we have to incur the cost of calling stat.
    if (entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK)
    {
      if (stat(itemPath.c_str(), &buffer) == 0)
        bStat = true;
    }

    if (entry->d_type == DT_DIR || (bStat && S_ISDIR(buffer.st_mode)))
    {
      if (!RemoveRecursive(CURL{ itemPath }))
      {
        success = false;
        break;
      }
    }
    else
    {
      if (unlink(itemPath.c_str()) != 0)
      {
        success = false;
        break;
      }
    }
  }

  closedir(dir);

  if (success)
  {
    if (rmdir(root.c_str()) != 0)
      success = false;
  }

  return success;
}

bool CPosixDirectory::Exists(const CURL& url)
{
  std::string path = url.Get();

  if (IsAliasShortcut(path, true))
    TranslateAliasShortcut(path);

  struct stat buffer;
  if (stat(path.c_str(), &buffer) != 0)
    return false;
  return S_ISDIR(buffer.st_mode) ? true : false;
}
#endif
