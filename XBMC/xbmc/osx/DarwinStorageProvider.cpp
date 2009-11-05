/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "DarwinStorageProvider.h"
#include "RegExp.h"
#include "StdString.h"
#include "Util.h"
#include "LocalizeStrings.h"

bool CDarwinStorageProvider::m_event = false;

CDarwinStorageProvider::CDarwinStorageProvider()
{
  m_removableLength = 0;
  PumpDriveChangeEvents(NULL);
}

void CDarwinStorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;
  share.strPath = getenv("HOME");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  localDrives.push_back(share);

  GetDrives(localDrives);
}

void CDarwinStorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  /*GetDrives(removableDrives);*/
}

void CDarwinStorageProvider::GetDrives(VECSOURCES &drives)
{
  std::vector<CStdString> result;

  CRegExp reMount;
  
  reMount.RegComp("on (.+) \\(([^,]+)");
  
  char line[1024];

  FILE* pipe = popen("mount", "r");

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      if (reMount.RegFind(line) != -1)
      {
        bool accepted = false;
        char* mount = reMount.GetReplaceString("\\1");
        char* fs    = reMount.GetReplaceString("\\2");
          
        // Here we choose which filesystems are approved
        if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0
            || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0
            || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0
            || strcmp(fs, "ntfs-3g") == 0 || strcmp(fs, "iso9660") == 0
            || strcmp(fs, "fusefs") == 0 || strcmp(fs, "hfs") == 0)
          accepted = true;

        // Ignore root
        if (strcmp(mount, "/") == 0)
          accepted = false;          
        
        if(accepted)
          result.push_back(mount);

        free(fs);
        free(mount);
      }
    }
    pclose(pipe);
  }

  for (unsigned int i = 0; i < result.size(); i++)
  {
    CMediaSource share;
    share.strPath = result[i];
    share.strName = CUtil::GetFileName(result[i]);
    share.m_ignore = true;
    drives.push_back(share);
  }
}

std::vector<CStdString> CDarwinStorageProvider::GetDiskUsage()
{
  std::vector<CStdString> result;
  char line[1024];

  FILE* pipe = popen("df -hT ufs,cd9660,hfs,udf", "r");

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      result.push_back(line);
    }
    pclose(pipe);
  }

  return result;
}

bool CDarwinStorageProvider::Eject(CStdString mountpath)
{
  return false;
}

bool CDarwinStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  bool event = m_event;
  m_event = false;
  return event;
}

void CDarwinStorageProvider::SetEvent(void)
{
  CDarwinStorageProvider::m_event = true;
}

/*
bool CDarwinStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  VECSOURCES drives;
  GetRemovableDrives(drives);
  bool changed = drives.size() != m_removableLength;
  m_removableLength = drives.size();
  return changed;
}
*/

