/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <cstdlib>

#include "PosixMountProvider.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

CPosixMountProvider::CPosixMountProvider()
{
  m_removableLength = 0;
  PumpDriveChangeEvents(NULL);
}

void CPosixMountProvider::Initialize()
{
  CLog::Log(LOGDEBUG, "Selected Posix mount as storage provider");
}

void CPosixMountProvider::GetDrives(VECSOURCES &drives)
{
  std::vector<std::string> result;

  CRegExp reMount;
#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
  reMount.RegComp("on (.+) \\(([^,]+)");
#else
  reMount.RegComp("on (.+) type ([^ ]+)");
#endif
  char line[1024];

  FILE* pipe = popen("mount", "r");

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      if (reMount.RegFind(line) != -1)
      {
        bool accepted = false;
        std::string mountStr = reMount.GetReplaceString("\\1");
        std::string fsStr    = reMount.GetReplaceString("\\2");
        const char* mount = mountStr.c_str();
        const char* fs    = fsStr.c_str();

        // Here we choose which filesystems are approved
        if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0
            || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0
            || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0
            || strcmp(fs, "ntfs-3g") == 0 || strcmp(fs, "iso9660") == 0
            || strcmp(fs, "exfat") == 0
            || strcmp(fs, "fusefs") == 0 || strcmp(fs, "hfs") == 0)
          accepted = true;

        // Ignore root
        if (strcmp(mount, "/") == 0)
          accepted = false;

        if(accepted)
          result.push_back(mount);
      }
    }
    pclose(pipe);
  }

  for (unsigned int i = 0; i < result.size(); i++)
  {
    CMediaSource share;
    share.strPath = result[i];
    share.strName = URIUtils::GetFileName(result[i]);
    share.m_ignore = true;
    drives.push_back(share);
  }
}

std::vector<std::string> CPosixMountProvider::GetDiskUsage()
{
  std::vector<std::string> result;
  char line[1024];

#if defined(TARGET_DARWIN)
  FILE* pipe = popen("df -hT ufs,cd9660,hfs,udf", "r");
#elif defined(TARGET_FREEBSD)
  FILE* pipe = popen("df -h -t ufs,cd9660,hfs,udf,zfs", "r");
#else
  FILE* pipe = popen("df -h", "r");
#endif

  static const char* excludes[] = {"rootfs","devtmpfs","tmpfs","none","/dev/loop", "udev", NULL};

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      bool ok=true;
      for (int i=0;excludes[i];++i)
      {
        if (strstr(line,excludes[i]))
        {
          ok=false;
          break;
        }
      }
      if (ok)
        result.push_back(line);
    }
    pclose(pipe);
  }

  return result;
}

bool CPosixMountProvider::Eject(const std::string& mountpath)
{

#if !defined(TARGET_DARWIN_EMBEDDED)
  // just go ahead and try to umount the disk
  // if it does umount, life is good, if not, no loss.
  std::string cmd = "umount \"" + mountpath + "\"";
  int status = system(cmd.c_str());

  if (status == 0)
    return true;
#endif

  return false;
}

bool CPosixMountProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  VECSOURCES drives;
  GetRemovableDrives(drives);
  bool changed = drives.size() != m_removableLength;
  m_removableLength = drives.size();
  return changed;
}
