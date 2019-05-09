/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/linux/SysfsPath.h"

#include <fstream>

#include <gtest/gtest.h>

struct TestSysfsPath : public ::testing::Test
{
  ~TestSysfsPath() { std::remove("/tmp/kodi-test"); }
};

TEST_F(TestSysfsPath, SysfsPathTest)
{
  int temp{1234};
  std::ofstream output{"/tmp/kodi-test"};
  output << temp;
  output.close();

  CSysfsPath path("/tmp/kodi-test");
  ASSERT_TRUE(path.Exists());
  ASSERT_TRUE(path.Get<int>() == 1234);
  ASSERT_TRUE(path.Get<float>() == 1234);
  ASSERT_TRUE(path.Get<double>() == 1234);
  ASSERT_TRUE(path.Get<uint64_t>() == 1234);
  ASSERT_TRUE(path.Get<uint16_t>() == 1234);
  ASSERT_TRUE(path.Get<unsigned int>() == 1234);
  ASSERT_TRUE(path.Get<unsigned long int>() == 1234);
  ASSERT_TRUE(path.Get<std::string>() == "1234");

  CSysfsPath otherPath{"/thispathdoesnotexist"};
  ASSERT_FALSE(otherPath.Exists());
}
