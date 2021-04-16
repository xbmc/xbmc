/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformPosix.h"

#include "filesystem/SpecialProtocol.h"

#include <cstdlib>
#include <time.h>

std::atomic_flag CPlatformPosix::ms_signalFlag;

bool CPlatformPosix::Init()
{

  if (!CPlatform::Init())
    return false;

  // Initialize to "set" state
  ms_signalFlag.test_and_set();

  // Initialize timezone information variables
  tzset();

  // set special://envhome
  if (getenv("HOME"))
  {
    CSpecialProtocol::SetEnvHomePath(getenv("HOME"));
  }
  else
  {
    fprintf(stderr, "The HOME environment variable is not set!\n");
    return false;
  }

  return true;
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
