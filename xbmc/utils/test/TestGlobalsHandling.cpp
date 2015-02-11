/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/test/TestGlobalsHandlingPattern1.h"

#include "gtest/gtest.h"

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
