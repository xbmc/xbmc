/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/test/TestGlobalsHandlingPattern1.h"

#include <gtest/gtest.h>

using namespace xbmcutil;
using namespace test;

bool TestGlobalPattern1::ctorCalled = false;
bool TestGlobalPattern1::dtorCalled = false;

TEST(TestGlobal, Pattern1)
{
  EXPECT_TRUE(TestGlobalPattern1::ctorCalled);
  {
    std::shared_ptr<TestGlobalPattern1> ptr = g_testGlobalPattern1Ref;
  }
}
