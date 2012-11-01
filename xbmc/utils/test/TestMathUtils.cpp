/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
