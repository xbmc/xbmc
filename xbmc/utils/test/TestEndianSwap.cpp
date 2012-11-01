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

#include "utils/EndianSwap.h"

#include "gtest/gtest.h"

TEST(TestEndianSwap, Endian_Swap16)
{
  uint16_t ref, var;
  ref = 0x00FF;
  var = Endian_Swap16(0xFF00);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_Swap32)
{
  uint32_t ref, var;
  ref = 0x00FF00FF;
  var = Endian_Swap32(0xFF00FF00);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_Swap64)
{
  uint64_t ref, var;
  ref = UINT64_C(0x00FF00FF00FF00FF);
  var = Endian_Swap64(UINT64_C(0xFF00FF00FF00FF00));
  EXPECT_EQ(ref, var);
}

#ifndef WORDS_BIGENDIAN
TEST(TestEndianSwap, Endian_SwapLE16)
{
  uint16_t ref, var;
  ref = 0x00FF;
  var = Endian_SwapLE16(0x00FF);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapLE32)
{
  uint32_t ref, var;
  ref = 0x00FF00FF;
  var = Endian_SwapLE32(0x00FF00FF);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapLE64)
{
  uint64_t ref, var;
  ref = UINT64_C(0x00FF00FF00FF00FF);
  var = Endian_SwapLE64(UINT64_C(0x00FF00FF00FF00FF));
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapBE16)
{
  uint16_t ref, var;
  ref = 0x00FF;
  var = Endian_SwapBE16(0xFF00);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapBE32)
{
  uint32_t ref, var;
  ref = 0x00FF00FF;
  var = Endian_SwapBE32(0xFF00FF00);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapBE64)
{
  uint64_t ref, var;
  ref = UINT64_C(0x00FF00FF00FF00FF);
  var = Endian_SwapBE64(UINT64_C(0xFF00FF00FF00FF00));
  EXPECT_EQ(ref, var);
}
#else
TEST(TestEndianSwap, Endian_SwapLE16)
{
  uint16_t ref, var;
  ref = 0x00FF;
  var = Endian_SwapLE16(0xFF00);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapLE32)
{
  uint32_t ref, var;
  ref = 0x00FF00FF;
  var = Endian_SwapLE32(0xFF00FF00);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapLE64)
{
  uint64_t ref, var;
  ref = UINT64_C(0x00FF00FF00FF00FF);
  var = Endian_SwapLE64(UINT64_C(0xFF00FF00FF00FF00));
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapBE16)
{
  uint16_t ref, var;
  ref = 0x00FF;
  var = Endian_SwapBE16(0x00FF);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapBE32)
{
  uint32_t ref, var;
  ref = 0x00FF00FF;
  var = Endian_SwapBE32(0x00FF00FF);
  EXPECT_EQ(ref, var);
}

TEST(TestEndianSwap, Endian_SwapBE64)
{
  uint64_t ref, var;
  ref = UINT64_C(0x00FF00FF00FF00FF);
  var = Endian_SwapBE64(UINT64_C(0x00FF00FF00FF00FF));
  EXPECT_EQ(ref, var);
}
#endif
