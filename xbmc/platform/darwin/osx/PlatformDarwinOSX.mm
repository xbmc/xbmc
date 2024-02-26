/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDarwinOSX.h"

#include "Util.h"

#if defined(HAS_GL)
#include "windowing/osx/OpenGL/WinSystemOSXGL.h"
#endif

#if defined(HAS_XBMCHELPER)
#include "platform/darwin/osx/XBMCHelper.h"
#endif
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

#if defined(HAS_GL)
  CWinSystemOSXGL::Register();
#endif

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
#if defined(HAS_XBMCHELPER)
  XBMCHelper::GetInstance().Configure();
#endif

  return true;
}
