/*
 *  Copyright (C) 2016-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformDarwinEmbedded.h"

#include "Util.h"

// clang-format off
#if defined(TARGET_DARWIN_IOS)
#include "windowing/ios/WinSystemIOS.h"
#endif
#if defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/powermanagement/TVOSPowerSyscall.h"
#import "platform/darwin/tvos/TVOSSettingsHandler.h"
#include "windowing/tvos/WinSystemTVOS.h"
#endif
// clang-format on

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformDarwinEmbedded();
}

bool CPlatformDarwinEmbedded::InitStageOne()
{
  if (!CPlatformDarwin::InitStageOne())
    return false;

#if defined(TARGET_DARWIN_IOS)
  CUtil::CopyUserDataIfNeeded("special://masterprofile/", "iOS/sources.xml", "sources.xml");
  CWinSystemIOS::Register();
#endif
#if defined(TARGET_DARWIN_TVOS)
  CWinSystemTVOS::Register();

  CTVOSPowerSyscall::Register();
#endif

  return true;
}

bool CPlatformDarwinEmbedded::InitStageTwo()
{
#if defined(TARGET_DARWIN_TVOS)
  // Retrieve specific tvOS input settings from user settings
  CTVOSInputSettings::GetInstance().Initialize();
#endif

  return true;
}

bool CPlatformDarwinEmbedded::SupportsUserInstalledBinaryAddons()
{
  return false;
}
