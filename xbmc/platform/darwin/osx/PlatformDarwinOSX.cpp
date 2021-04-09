/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDarwinOSX.h"

#include "Util.h"
#include "windowing/osx/WinSystemOSXGL.h"

#include "platform/darwin/osx/XBMCHelper.h"
#include "platform/darwin/osx/powermanagement/CocoaPowerSyscall.h"

#include <stdlib.h>
#include <string>

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformDarwinOSX();
}

bool CPlatformDarwinOSX::InitStageOne()
{
  if (!CPlatformDarwin::InitStageOne())
    return false;

  CWinSystemOSXGL::Register();

  CCocoaPowerSyscall::Register();

  std::string install_path(CUtil::GetHomePath());
  setenv("KODI_HOME", install_path.c_str(), 0);

  install_path += "/tools/darwin/runtime/preflight";
  system(install_path.c_str());

  return true;
}

bool CPlatformDarwinOSX::InitStageTwo()
{
  // Configure and possible manually start the helper.
  XBMCHelper::GetInstance().Configure();

  return true;
}
