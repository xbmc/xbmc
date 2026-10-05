/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/BlurayDiscCache.h"

#include <string>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
const std::string DISC{"D:\\Movies\\Movie\\"};
} // namespace

TEST(TestBlurayDiscCache, CheckDisc_KeepsTheSameDisc)
{
  CBlurayDiscCache cache;
  cache.CheckDisc(DISC, "1:100");
  cache.SetDiscTitle(DISC, "Movie");

  cache.CheckDisc(DISC, "1:100");

  std::string title;
  EXPECT_TRUE(cache.GetDiscTitle(DISC, title));
  EXPECT_EQ(title, "Movie");
}

TEST(TestBlurayDiscCache, CheckDisc_DropsAReplacedDisc)
{
  CBlurayDiscCache cache;
  cache.CheckDisc(DISC, "1:100");
  cache.SetDiscTitle(DISC, "Movie");

  cache.CheckDisc(DISC, "2:100");

  std::string title;
  EXPECT_FALSE(cache.GetDiscTitle(DISC, title));

  // The replacement is now the disc held
  cache.SetDiscTitle(DISC, "Other");
  cache.CheckDisc(DISC, "2:100");
  EXPECT_TRUE(cache.GetDiscTitle(DISC, title));
  EXPECT_EQ(title, "Other");
}
