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

#include "utils/StdString.h"

#include "gtest/gtest.h"

TEST(TestStdString, CStdString)
{
  CStdString ref, var;

  ref = "CStdString test";
  var = ref;
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStdString, CStdStringA)
{
  CStdStringA ref, var;

  ref = "CStdStringA test";
  var = ref;
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStdString, CStdStringW)
{
  CStdStringW ref, var;

  ref = L"CStdStringW test";
  var = ref;
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStdString, CStdString16)
{
  CStdString16 ref, var;

  ref = (uint16_t*)"CStdString16 test";
  var = ref;
  EXPECT_TRUE(!memcmp(ref.c_str(), var.c_str(),
                      ref.length() * sizeof(uint16_t)));
}

TEST(TestStdString, CStdString32)
{
  CStdString32 ref, var;

  ref = (uint32_t*)"CStdString32 test";
  var = ref;
  EXPECT_TRUE(!memcmp(ref.c_str(), var.c_str(),
                      ref.length() * sizeof(uint32_t)));
}

TEST(TestStdString, CStdStringO)
{
  CStdStringO ref, var;

  ref = "CStdString32 test";
  var = ref;
  EXPECT_TRUE(!memcmp(ref.c_str(), var.c_str(),
                      ref.length() * sizeof(OLECHAR)));
}
