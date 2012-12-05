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

#include "utils/RingBuffer.h"

#include "gtest/gtest.h"

TEST(TestRingBuffer, General)
{
  CRingBuffer a;
  char data[20];
  unsigned int i;

  EXPECT_TRUE(a.Create(20));
  EXPECT_EQ((unsigned int)20, a.getSize());
  memset(data, 0, sizeof(data));
  for (i = 0; i < a.getSize(); i++)
    EXPECT_TRUE(a.WriteData(data, 1));
  a.Clear();

  memcpy(data, "0123456789", sizeof("0123456789"));
  EXPECT_TRUE(a.WriteData(data, sizeof("0123456789")));
  EXPECT_STREQ("0123456789", a.getBuffer());

  memset(data, 0, sizeof(data));
  EXPECT_TRUE(a.ReadData(data, 5));
  EXPECT_STREQ("01234", data);
}
