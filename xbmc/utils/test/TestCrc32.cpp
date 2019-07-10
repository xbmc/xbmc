/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/Crc32.h"

#include <gtest/gtest.h>

static const char refdata[] = "abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "01234567890!@#$%^&*()";

TEST(TestCrc32, Compute_1)
{
  Crc32 a;
  uint32_t varcrc;
  a.Compute(refdata, sizeof(refdata) - 1);
  varcrc = a;
  EXPECT_EQ(0xa4eb60e3, varcrc);
}

TEST(TestCrc32, Compute_2)
{
  uint32_t varcrc;
  std::string s = refdata;
  varcrc = Crc32::Compute(s);
  EXPECT_EQ(0xa4eb60e3, varcrc);
}

TEST(TestCrc32, ComputeFromLowerCase)
{
  std::string s = refdata;
  uint32_t varcrc = Crc32::ComputeFromLowerCase(s);
  EXPECT_EQ((uint32_t)0x7f045b3e, varcrc);
}

TEST(TestCrc32, Reset)
{
  Crc32 a;
  uint32_t varcrc;
  std::string s = refdata;
  a.Compute(s.c_str(), s.length());
  a.Reset();
  varcrc = a;
  EXPECT_EQ(0xffffffff, varcrc);
}
