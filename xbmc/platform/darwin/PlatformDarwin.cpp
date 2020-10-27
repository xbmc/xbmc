/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDarwin.h"

#include "filesystem/SpecialProtocol.h"

// clang-format off
#if defined(TARGET_DARWIN_IOS)
#include "windowing/ios/WinSystemIOS.h"
#endif
#if defined(TARGET_DARWIN_TVOS)
#include "windowing/tvos/WinSystemTVOS.h"
#endif
#if defined(TARGET_DARWIN_OSX)
#include "windowing/osx/WinSystemOSXGL.h"
#endif
// clang-format on

#include <stdlib.h>

void CPlatformDarwin::Init()
{
  CPlatformPosix::Init();
  setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 0);

#if defined(TARGET_DARWIN_IOS)
  CWinSystemIOS::Register();
#endif
#if defined(TARGET_DARWIN_TVOS)
  CWinSystemTVOS::Register();
#endif
#if defined(TARGET_DARWIN_OSX)
  CWinSystemOSXGL::Register();
#endif
}
