/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
