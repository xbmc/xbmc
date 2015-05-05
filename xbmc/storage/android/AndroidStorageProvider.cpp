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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>

#include "AndroidStorageProvider.h"
#include "android/activity/XBMCApp.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"

#include "Util.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

static const char * typeWL[] = { "vfat", "exfat", "sdcardfs", "fuse", "ntfs", "fat32", "ext3", "ext4", "esdfs" };
static const char * mountWL[] = { "/mnt", "/Removable", "/storage" };
static const char * mountBL[] = {
  "/mnt/secure",
  "/mnt/shell",
  "/mnt/asec",
  "/mnt/obb",
  "/mnt/media_rw/extSdCard",
  "/mnt/media_rw/sdcard",
  "/mnt/media_rw/usbdisk",
  "/storage/emulated"
};
static const char * deviceWL[] = {
  "/dev/block/vold",
  "/dev/fuse",
  "/mnt/media_rw"
};

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
  CRegExp                             reMount;
  reMount.RegComp("^(.+?)\\s+(.+?)\\s+(.+?)\\s+(.+?)\\s");

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
        std::string deviceStr   = reMount.GetReplaceString("\\1");
        std::string mountStr = reMount.GetReplaceString("\\2");
        std::string fsStr    = reMount.GetReplaceString("\\3");
        std::string optStr    = reMount.GetReplaceString("\\4");

        // Blacklist
        bool bl_ok = true;

        // Reject unreadable
        if (!XFILE::CDirectory::Exists(mountStr))
          bl_ok = false;

        // What mount points are rejected
        for (unsigned int i=0; i < ARRAY_SIZE(mountBL); ++i)
          if (StringUtils::StartsWithNoCase(mountStr, mountBL[i]))
            bl_ok = false;

        if (bl_ok)
        {
          // What filesystems are accepted
          bool fsok = false;
          for (unsigned int i=0; i < ARRAY_SIZE(typeWL); ++i)
            if (StringUtils::StartsWithNoCase(fsStr, typeWL[i]))
              continue;

          // What devices are accepted
          bool devok = false;
          for (unsigned int i=0; i < ARRAY_SIZE(deviceWL); ++i)
            if (StringUtils::StartsWithNoCase(deviceStr, deviceWL[i]))
              devok = true;

          // What mount points are accepted
          bool mountok = false;
          for (unsigned int i=0; i < ARRAY_SIZE(mountWL); ++i)
            if (StringUtils::StartsWithNoCase(mountStr, mountWL[i]))
              mountok = true;

          if(devok && (fsok || mountok))
          {
            CMediaSource share;
            share.strPath = unescape(mountStr);
            share.strName = URIUtils::GetFileName(mountStr);
            share.m_ignore = true;
            removableDrives.push_back(share);
          }
        }
      }
      line = strtok_r(NULL, "\n", &saveptr);
    }
    free(buf);
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
