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

#include "PlatformRbpi.h"
#include <stdlib.h>
#include <string.h>
#include "utils/log.h"
#include "utils/md5.h"

CPlatform* CPlatform::CreateInstance()
{
    return new CPlatformRbpi();
}

void CPlatformRbpi::Init()
{
  // call base init
  CPlatform::Init();
  // add platform specific init stuff here
}

void CPlatformRbpi::InitUniqueHardwareIdentifier()
{
  FILE *f = fopen("/proc/cpuinfo", "r");
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
          m_uuid = XBMC::XBMC_MD5::GetMD5(m_uuid);
        }
      }
    }
    fclose(f);
  }
}
