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

int main(int argc, char **argv)
{
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
