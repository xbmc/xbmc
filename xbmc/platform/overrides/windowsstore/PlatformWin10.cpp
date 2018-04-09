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

#include "PlatformWin10.h"
#include <stdlib.h>
#include "filesystem/SpecialProtocol.h"
#include "platform/Environment.h"
#include "platform/win10/input/RemoteControlXbox.h"
#include "platform/win10/powermanagement/Win10PowerSyscall.h"
#include "utils/SystemInfo.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWin10();
}

CPlatformWin10::CPlatformWin10() = default;

CPlatformWin10::~CPlatformWin10() = default;

void CPlatformWin10::Init()
{
  CEnvironment::setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 1);
  CPowerSyscall::Register();
  if (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::WindowsDeviceFamily::Xbox)
  {
    CRemoteControlXbox::Register();
  }
}
