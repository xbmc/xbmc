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

#include "LocalDirectoryImpl.h"
#include "FileItem.h"
#include "URL.h"
#include "platform/linux/XTimeUtils.h"
#include "utils/AliasShortcutUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <dirent.h>
#include <sys/stat.h>

namespace KODI
{
namespace PLATFORM
{
namespace DETAILS
{

bool CreateInternal(std::string path)
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

bool GetDirectory(const std::string &root, std::vector<std::string> &items)
{
  if (IsAliasShortcut(root, true))
    TranslateAliasShortcut(root);

  DIR *dir = opendir(root.c_str());
  if (!dir)
    return false;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (entry->d_type == DT_DIR)
      items.emplace_back(URIUtils::AppendSlash(URIUtils::AddFileToFolder(root, entry->d_name)));
    else
      items.emplace_back(URIUtils::AddFileToFolder(root, entry->d_name));
  }
  closedir(dir);
  return true;
}

bool Create(const std::string &url)
{
  if (!Create(url))
    return Exists(url);

  return true;
}

bool Remove(const std::string &url)
{
  if (rmdir(url.Get().c_str()) == 0)
    return true;

  return !Exists(url);
}

bool RemoveRecursive(const std::string &root)
{

  if (IsAliasShortcut(root, true))
    TranslateAliasShortcut(root);

  DIR *dir = opendir(root.c_str());
  if (!dir)
    return false;

  bool success(true);
  struct dirent *entry;
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
      if (!RemoveRecursive(CURL{itemPath}))
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

bool Exists(const std::string &url)
{
  if (IsAliasShortcut(url, true))
    TranslateAliasShortcut(url);

  struct stat buffer;
  if (stat(url.c_str(), &buffer) != 0)
    return false;
  return S_ISDIR(buffer.st_mode) ? true : false;
}

std::string CreateSystemTempDirectory(std::string directory)
{
  char buf[MAX_PATH];
  char *tmp;
  strcpy(buf, ("/tmp/" + directory).c_str());

  if ((tmp = mkdtemp(buf)) == NULL)
    return std::string();
  
  return std::string(tmp);
}

}
}
}