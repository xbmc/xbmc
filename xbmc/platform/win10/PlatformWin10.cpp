/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformWin10.h"

#include "filesystem/SpecialProtocol.h"
#include "platform/Environment.h"
#include "win32util.h"
#include "windowing/win10/WinSystemWin10DX.h"

#include "platform/win10/powermanagement/Win10PowerSyscall.h"

#include <stdlib.h>

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWin10();
}

bool CPlatformWin10::Init()
{
  if (!CPlatform::Init())
    return false;

  CEnvironment::setenv("OS", "win10"); // for python scripts that check the OS

  // enable independent locale for each thread, see https://connect.microsoft.com/VisualStudio/feedback/details/794122
  CWIN32Util::SetThreadLocalLocale(true);

  CWinSystemWin10DX::Register();

  CPowerSyscall::Register();

  return true;
}
