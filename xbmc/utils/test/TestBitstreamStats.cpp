/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/BitstreamStats.h"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
// CalculateBitrate() updates only once 2 s have passed since the last update.
constexpr auto UPDATE_INTERVAL = 2100ms;
} // namespace

TEST(TestBitstreamStats, General)
{
  BitstreamStats a;

  a.Start();
  EXPECT_EQ(0.0, a.GetBitrate());

  a.AddSampleBits(256 * 8);
  std::this_thread::sleep_for(UPDATE_INTERVAL);
  a.CalculateBitrate();
  EXPECT_GT(a.GetBitrate(), 0.0);

  a.AddSampleBytes(256);
  std::this_thread::sleep_for(UPDATE_INTERVAL);
  a.CalculateBitrate();
  EXPECT_GT(a.GetBitrate(), 0.0);
}
