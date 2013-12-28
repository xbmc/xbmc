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

#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

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
  if (CXBMCApp::GetExternalStorage(path) && !path.empty()  && XFILE::CFile::Exists(path))
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
  std::vector<CStdString> result;
  CRegExp reMount;
  reMount.RegComp("^(.+?)\\s+(.+?)\\s+(.+?)\\s");
  char line[1024];

  FILE* pipe = fopen("/proc/mounts", "r");

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      if (reMount.RegFind(line) != -1)
      {
        bool accepted = false;
        std::string mountStr = reMount.GetReplaceString("\\2");
        std::string fsStr    = reMount.GetReplaceString("\\3");
        const char* mount = mountStr.c_str();
        const char* fs    = fsStr.c_str();

        // Here we choose which filesystems are approved
        if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0
            || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0 || strcmp(fs, "ext4") == 0
            || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0
            || strcmp(fs, "ntfs-3g") == 0 || strcmp(fs, "iso9660") == 0
            || strcmp(fs, "exfat") == 0
            || strcmp(fs, "fusefs") == 0 || strcmp(fs, "hfs") == 0)
          accepted = true;

        // Ignore everything but usb
        if (!StringUtils::StartsWith(mountStr, "/mnt/usb"))
          accepted = false;

        if(accepted)
          result.push_back(mount);
      }
    }
    fclose(pipe);
  } else
    CLog::Log(LOGERROR, "Cannot read mount points");

  for (unsigned int i = 0; i < result.size(); i++)
  {
    CMediaSource share;
    share.strPath = unescape(result[i]);
    share.strName = URIUtils::GetFileName(share.strPath);
    share.m_ignore = true;
    removableDrives.push_back(share);
  }
}

std::vector<CStdString> CAndroidStorageProvider::GetDiskUsage()
{
  std::vector<CStdString> result;

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

bool CAndroidStorageProvider::Eject(CStdString mountpath)
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
