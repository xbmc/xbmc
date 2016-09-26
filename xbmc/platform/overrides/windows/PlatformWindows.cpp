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

#include "PlatformWindows.h"

#include "dwmapi.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "utils/CharsetConverter.h"

#include <stdlib.h>
#include <string.h>
#include "utils/log.h"
#include "utils/md5.h"


CPlatform* CPlatform::CreateInstance()
{
    return new CPlatformWindows();
}

void CPlatformWindows::Init()
{
  // call base init
  CPlatform::Init();
  // add platform specific init stuff here
}

void CPlatformWindows::InitUniqueHardwareIdentifier()
{
  HKEY hKey;
  std::string uuid;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
  {
    wchar_t buf[40]; // f.e. ce39c043-0661-4920-91dc-1210ad294c73
    DWORD bufSize = sizeof(buf);
    DWORD valType;
    if (RegQueryValueExW(hKey, L"MachineGuid", NULL, &valType, (LPBYTE)buf, &bufSize) == ERROR_SUCCESS && valType == REG_SZ)
    {
      g_charsetConverter.wToUTF8(std::wstring(buf, bufSize / sizeof(wchar_t)), uuid);
      size_t zeroPos = uuid.find(char(0));
      if (zeroPos != std::string::npos)
      {
        uuid.erase(zeroPos); // remove any extra zero-terminations
      }
      m_uuid = uuid;
    }
    RegCloseKey(hKey);
  }

  // fallback to base implementation if needed - paranoia
  if (m_uuid == NoValidUUID)
  {
    CPlatform::InitUniqueHardwareIdentifier();
  }
  else
  {
    m_uuid = XBMC::XBMC_MD5::GetMD5(m_uuid);
  }
}
