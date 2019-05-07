/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformPosix.h"

std::atomic_flag CPlatformPosix::ms_signalFlag;

void CPlatformPosix::Init()
{
  CPlatform::Init();

  // Initialize to "set" state
  ms_signalFlag.test_and_set();
}

bool CPlatformPosix::TestQuitFlag()
{
  // Keep set, return true when it was cleared before
  return !ms_signalFlag.test_and_set();
}

void CPlatformPosix::RequestQuit()
{
  ms_signalFlag.clear();
}
