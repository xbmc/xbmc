/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDarwinOSX.h"

#include "windowing/osx/WinSystemOSXGL.h"

#include "platform/darwin/osx/XBMCHelper.h"
#include "platform/darwin/osx/powermanagement/CocoaPowerSyscall.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformDarwinOSX();
}

bool CPlatformDarwinOSX::Init()
{
  if (!CPlatformDarwin::Init())
    return false;

  // Configure and possible manually start the helper.
  XBMCHelper::GetInstance().Configure();

  CWinSystemOSXGL::Register();

  CCocoaPowerSyscall::Register();

  return true;
}
