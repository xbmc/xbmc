/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/linux/SysfsPath.h"

#include <chrono>
#include <string>
#include <sstream>
#include <fstream>

#include <gtest/gtest.h>

struct TestSysfsPath : public ::testing::Test
{
  std::string GetTestFilePath()
  {
    using namespace std::chrono;

    std::string tmpdir{"/tmp"};
    const char *test_tmpdir = getenv("TMPDIR");
    if (test_tmpdir && test_tmpdir[0] != '\0') {
      tmpdir.assign(test_tmpdir);
    }

    std::stringstream ss;
    unsigned long long timestamp = duration_cast< nanoseconds >(
      system_clock::now().time_since_epoch()).count();

    ss << tmpdir << "/kodi-test-" << timestamp;
    return ss.str();
  }
};

TEST_F(TestSysfsPath, SysfsPathTestInt)
{
  std::string filepath = GetTestFilePath();
  std::ofstream m_output(filepath);

  int temp{1234};
  m_output << temp;
  m_output.close();

  CSysfsPath path(filepath);
  ASSERT_TRUE(path.Exists());
  EXPECT_EQ(path.Get<int>(), 1234);
  EXPECT_EQ(path.Get<float>(), 1234);
  EXPECT_EQ(path.Get<double>(), 1234);
  EXPECT_EQ(path.Get<uint64_t>(), 1234);
  EXPECT_EQ(path.Get<uint16_t>(), 1234);
  EXPECT_EQ(path.Get<unsigned int>(), 1234);
  EXPECT_EQ(path.Get<unsigned long int>(), 1234);

  std::remove(filepath.c_str());
}

TEST_F(TestSysfsPath, SysfsPathTestString)
{
  std::string filepath = GetTestFilePath();
  std::ofstream m_output{filepath};

  std::string temp{"test"};
  m_output << temp;
  m_output.close();

  CSysfsPath path(filepath);
  ASSERT_TRUE(path.Exists());
  EXPECT_EQ(path.Get<std::string>(), "test");

  std::remove(filepath.c_str());
}

TEST_F(TestSysfsPath, SysfsPathTestLongString)
{
  std::string filepath = GetTestFilePath();
  std::ofstream m_output{filepath};

  std::string temp{"test with spaces"};
  m_output << temp;
  m_output.close();

  CSysfsPath path(filepath);
  ASSERT_TRUE(path.Exists());
  EXPECT_EQ(path.Get<std::string>(), "test with spaces");

  std::remove(filepath.c_str());
}

TEST_F(TestSysfsPath, SysfsPathTestPathDoesNotExist)
{
  CSysfsPath otherPath{"/thispathdoesnotexist"};
  ASSERT_FALSE(otherPath.Exists());
}
