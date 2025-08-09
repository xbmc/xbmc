/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformWin32.h"

#include "filesystem/SpecialProtocol.h"
#include "platform/Environment.h"
#include "win32util.h"
#include "windowing/windows/WinSystemWin32DX.h"

#include "platform/win32/powermanagement/Win32PowerSyscall.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWin32();
}

bool CPlatformWin32::InitStageOne()
{
  if (!CPlatform::InitStageOne())
    return false;

  CEnvironment::setenv("PYTHONHOME",
                       CSpecialProtocol::TranslatePath("special://xbmc/system/python"));

  const std::string pyPath = fmt::format(
      "{};{};{}", CSpecialProtocol::TranslatePath("special://xbmc/system/python/DLLs"),
      CSpecialProtocol::TranslatePath("special://xbmc/system/python/Lib"),
      CSpecialProtocol::TranslatePath("special://xbmc/system/python/Lib/site-packages"));

  CEnvironment::setenv("PYTHONPATH", pyPath);
  CEnvironment::setenv("OS", "win32"); // for python scripts that check the OS
  CEnvironment::setenv("PYTHONOPTIMIZE", "1");

  // enable independent locale for each thread, see https://connect.microsoft.com/VisualStudio/feedback/details/794122
  CWIN32Util::SetThreadLocalLocale(true);

  CWinSystemWin32DX::Register();

  CWin32PowerSyscall::Register();

  return true;
}

void CPlatformWin32::PlatformSyslog()
{
  CWIN32Util::PlatformSyslog();
}
