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

  std::ofstream m_output{"/tmp/kodi-test"};
};

TEST_F(TestSysfsPath, SysfsPathTestInt)
{
  int temp{1234};
  m_output << temp;
  m_output.close();

  CSysfsPath path("/tmp/kodi-test");
  ASSERT_TRUE(path.Exists());
  EXPECT_EQ(path.Get<int>(), 1234);
  EXPECT_EQ(path.Get<float>(), 1234);
  EXPECT_EQ(path.Get<double>(), 1234);
  EXPECT_EQ(path.Get<uint64_t>(), 1234);
  EXPECT_EQ(path.Get<uint16_t>(), 1234);
  EXPECT_EQ(path.Get<unsigned int>(), 1234);
  EXPECT_EQ(path.Get<unsigned long int>(), 1234);
}

TEST_F(TestSysfsPath, SysfsPathTestString)
{
  std::string temp{"test"};
  m_output << temp;
  m_output.close();

  CSysfsPath path("/tmp/kodi-test");
  ASSERT_TRUE(path.Exists());
  EXPECT_EQ(path.Get<std::string>(), "test");
}

TEST_F(TestSysfsPath, SysfsPathTestLongString)
{
  std::string temp{"test with spaces"};
  m_output << temp;
  m_output.close();

  CSysfsPath path("/tmp/kodi-test");
  ASSERT_TRUE(path.Exists());
  EXPECT_EQ(path.Get<std::string>(), "test with spaces");
}

TEST_F(TestSysfsPath, SysfsPathTestPathDoesNotExist)
{
  CSysfsPath otherPath{"/thispathdoesnotexist"};
  ASSERT_FALSE(otherPath.Exists());
}
