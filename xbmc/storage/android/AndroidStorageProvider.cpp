/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "AndroidStorageProvider.h"
#include "android/activity/XBMCApp.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"

#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>

CAndroidStorageProvider::CAndroidStorageProvider()
{
  m_removableLength = 0;
  PumpDriveChangeEvents(NULL);
}

std::string CAndroidStorageProvider::unescape(const std::string& str)
{
  std::string retString;
  for (uint32_t i=0; i < str.length(); ++i)
  {
    if (str[i] != '\\')
      retString += str[i];
    else
    {
      i += 1;
      if (str[i] == 'u') // unicode
      {
        // TODO
      }
      else if (str[i] >= '0' && str[i] <= '7') // octal
      {
        std::string octString;
        while (str[i] >= '0' && str[i] <= '7')
        {
          octString += str[i];
          i += 1;
        }
        if (octString.length() != 0)
        {
          uint8_t val = 0;
          for (int j=octString.length()-1; j>=0; --j)
          {
            val += ((uint8_t)(octString[j] - '0')) * (1 << ((octString.length() - (j+1)) * 3));
          }
          retString += (char)val;
          i -= 1;
        }
      }
    }
  }
  return retString;
}

void CAndroidStorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;

  // external directory
  std::string path;
  if (CXBMCApp::GetExternalStorage(path) && !path.empty()  && XFILE::CDirectory::Exists(path))
  {
    share.strPath = path;
    share.strName = g_localizeStrings.Get(21456);
    share.m_ignore = true;
    localDrives.push_back(share);
  }

  // root directory
  share.strPath = "/";
  share.strName = g_localizeStrings.Get(21453);
  localDrives.push_back(share);
}

void CAndroidStorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  // mounted usb disks
  char*                               buf     = NULL;
  FILE*                               pipe;
  std::map<std::string, std::string>  result;
  CRegExp                             reMount;
  reMount.RegComp("^(.+?)\\s+(.+?)\\s+(.+?)\\s");

  /* /proc/mounts is only guaranteed atomic for the current read
   * operation, so we need to read it all at once.
   */
  if ((pipe = fopen("/proc/mounts", "r")))
  {
    char*   new_buf;
    size_t  buf_len = 4096;

    while ((new_buf = (char*)realloc(buf, buf_len * sizeof(char))))
    {
      size_t nread;

      buf   = new_buf;
      nread = fread(buf, sizeof(char), buf_len, pipe);

      if (nread == buf_len)
      {
        rewind(pipe);
        buf_len *= 2;
      }
      else
      {
        buf[nread] = '\0';
        if (!feof(pipe))
          new_buf = NULL;
        break;
      }
    }

    if (!new_buf)
    {
      free(buf);
      buf = NULL;
    }
    fclose(pipe);
  }
  else
    CLog::Log(LOGERROR, "Cannot read mount points");

  if (buf)
  {
    char* line;
    char* saveptr = NULL;

    line = strtok_r(buf, "\n", &saveptr);

    while (line)
    {
      if (reMount.RegFind(line) != -1)
      {
        bool accepted = false;
        std::string device   = reMount.GetReplaceString("\\1");
        std::string mountStr = reMount.GetReplaceString("\\2");
        std::string fsStr    = reMount.GetReplaceString("\\3");
        const char* fs    = fsStr.c_str();

        // Here we choose which filesystems are approved
        if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0
            || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0 || strcmp(fs, "ext4") == 0
            || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0
            || strcmp(fs, "ntfs-3g") == 0 || strcmp(fs, "iso9660") == 0
            || strcmp(fs, "exfat") == 0
            || strcmp(fs, "fusefs") == 0 || strcmp(fs, "hfs") == 0)
          accepted = true;

        // Ignore sdcards
        if (!StringUtils::StartsWith(device, "/dev/block/vold/") ||
            mountStr.find("sdcard") != std::string::npos ||
            mountStr.find("secure/asec") != std::string::npos)
          accepted = false;

        if(accepted)
          result[device] = mountStr;
      }
      line = strtok_r(NULL, "\n", &saveptr);
    }
    free(buf);
  }

  for (std::map<std::string, std::string>::const_iterator i = result.begin(); i != result.end(); ++i)
  {
    CMediaSource share;
    share.strPath = unescape(i->second);
    share.strName = URIUtils::GetFileName(share.strPath);
    share.m_ignore = true;
    removableDrives.push_back(share);
  }
}

std::vector<std::string> CAndroidStorageProvider::GetDiskUsage()
{
  std::vector<std::string> result;

  std::string usage;
  // add header
  CXBMCApp::GetStorageUsage("", usage);
  result.push_back(usage);

  usage.clear();
  // add rootfs
  if (CXBMCApp::GetStorageUsage("/", usage) && !usage.empty())
    result.push_back(usage);

  usage.clear();
  // add external storage if available
  std::string path;
  if (CXBMCApp::GetExternalStorage(path) && !path.empty() &&
      CXBMCApp::GetStorageUsage(path, usage) && !usage.empty())
    result.push_back(usage);

  // add removable storage
  VECSOURCES drives;
  GetRemovableDrives(drives);
  for (unsigned int i = 0; i < drives.size(); i++)
  {
    usage.clear();
    if (CXBMCApp::GetStorageUsage(drives[i].strPath, usage) && !usage.empty())
      result.push_back(usage);
  }

  return result;
}

bool CAndroidStorageProvider::Eject(const std::string& mountpath)
{
  return false;
}

bool CAndroidStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  VECSOURCES drives;
  GetRemovableDrives(drives);
  bool changed = drives.size() != m_removableLength;
  m_removableLength = drives.size();
  return changed;
}
