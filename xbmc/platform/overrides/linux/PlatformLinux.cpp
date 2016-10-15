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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlatformLinux.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils/log.h"
#include "utils/md5.h"

CPlatform* CPlatform::CreateInstance()
{
    return new CPlatformLinux();
}

void CPlatformLinux::Init()
{
  // call base init
  CPlatform::Init();
  // add platform specific init stuff here
}

void CPlatformLinux::InitUniqueHardwareIdentifier()
{
  // try to read the machine-id from etc first
  FILE *f = fopen("/etc/machine-id", "r");
  if (f)
  {
      char line[33]; // 32 + zero byte
      if (fgets(line, sizeof line, f) != nullptr)
      {
          m_uuid = line;
      }
      fclose(f);
  }
    
  // if we didn't get an uuid yet - try to get a serialnumber
  if (m_uuid == NoValidUUID)
  {
    f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
      char line[256];
      while (fgets(line, sizeof line, f))
      {
        if (strncmp(line, "Serial", 6) == 0)
        {
          char *colon = strchr(line, ':');
          if (colon)
          {
            m_uuid = colon + 2;
          }
        }
      }
      fclose(f);
    }
  }

  // fallback to base implementation if needed
  if (m_uuid == NoValidUUID)
  {
    CPlatform::InitUniqueHardwareIdentifier();
  }
  else
  {
    m_uuid = XBMC::XBMC_MD5::GetMD5(m_uuid);
  }
}
