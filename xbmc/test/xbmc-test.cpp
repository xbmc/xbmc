/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TestBasicEnvironment.h"
#include "TestUtils.h"
#include "commons/ilog.h"
#include "threads/Thread.h"

#include <cstdio>
#include <cstdlib>

#include <gtest/gtest.h>

#if defined(TARGET_WINDOWS)
#include <Windows.h>
#include <mmsystem.h>
#endif

int main(int argc, char **argv)
{
#if defined(TARGET_WINDOWS)
  // Match kodi.exe: at the default 15.6 ms timer resolution a 1 ms sleep takes a whole tick.
  // Windows 11 ignores the request from a process with no visible window, such as a CI agent's,
  // unless the process opts out.
  PROCESS_POWER_THROTTLING_STATE throttling{};
  throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
  throttling.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
  throttling.StateMask = 0;
  SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &throttling,
                        sizeof(throttling));
  timeBeginPeriod(1);
#endif
  testing::InitGoogleTest(&argc, argv);
  CXBMCTestUtils::Instance().ParseArgs(argc, argv);

  if (!testing::AddGlobalTestEnvironment(new TestBasicEnvironment()))
  {
    fprintf(stderr, "Unable to add basic test environment.\n");
    exit(EXIT_FAILURE);
  }
  int ret = RUN_ALL_TESTS();

  return ret;
}
