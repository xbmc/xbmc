/*
 *      Copyright (C) 2014 Team XBMC
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

#if defined(TARGET_POSIX)

#include "PosixDirectory.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"

#include <dirent.h>
#include <sys/stat.h>

using namespace std;
using namespace XFILE;

CPosixDirectory::CPosixDirectory(void)
{}

CPosixDirectory::~CPosixDirectory(void)
{}

bool CPosixDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  const char* root = strPath;
  struct dirent* entry;
  DIR *dir = opendir(root);

  if (!dir)
    return false;

  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    CFileItemPtr pItem(new CFileItem(entry->d_name));
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

    if (entry->d_type == DT_DIR || (bStat && buffer.st_mode & S_IFDIR))
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
      if (bStat || stat(pItem->GetPath(), &buffer) == 0)
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

bool CPosixDirectory::Create(const char* strPath)
{
  if (!strPath || !*strPath)
    return false;

  return (mkdir(strPath, 0755) == 0 || errno == EEXIST);
}

bool CPosixDirectory::Remove(const char* strPath)
{
  if (!strPath || !*strPath)
    return false;

  return (rmdir(strPath) == 0);
}

bool CPosixDirectory::Exists(const char* strPath)
{
  if (!strPath || !*strPath)
    return false;

  struct stat buffer;
  if (stat(strPath, &buffer) != 0)
    return false;
  return S_ISDIR(buffer.st_mode) ? true : false;
}
#endif
