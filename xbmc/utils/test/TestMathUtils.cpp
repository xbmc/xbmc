/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MathUtils.h"

#include "gtest/gtest.h"

TEST(TestMathUtils, round_int)
{
  int refval, varval, i;

  for (i = -8; i < 8; ++i)
  {
    double d = 0.25*i;
    refval = (i < 0) ? (i - 1) / 4 : (i + 2) / 4;
    varval = MathUtils::round_int(d);
    EXPECT_EQ(refval, varval);
  }
}

TEST(TestMathUtils, truncate_int)
{
  int refval, varval, i;

  for (i = -8; i < 8; ++i)
  {
    double d = 0.25*i;
    refval = i / 4;
    varval = MathUtils::truncate_int(d);
    EXPECT_EQ(refval, varval);
  }
}

TEST(TestMathUtils, abs)
{
  int64_t refval, varval;

  refval = 5;
  varval = MathUtils::abs(-5);
  EXPECT_EQ(refval, varval);
}

TEST(TestMathUtils, bitcount)
{
  unsigned refval, varval;

  refval = 10;
  varval = MathUtils::bitcount(0x03FF);
  EXPECT_EQ(refval, varval);

  refval = 8;
  varval = MathUtils::bitcount(0x2AD5);
  EXPECT_EQ(refval, varval);
}
